// SquadCoordinatorComponent.cpp

#include "SquadCoordinatorComponent.h"
#include "AISquadController.h"

USquadCoordinatorComponent::USquadCoordinatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.25f; // squad-level bookkeeping doesn't need per-frame ticking
}

void USquadCoordinatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PruneStaleEnemyKnowledge();
}

void USquadCoordinatorComponent::RegisterMember(AAISquadController* Controller)
{
	if (!Controller || FindMemberStateIndex(Controller) != INDEX_NONE)
	{
		return;
	}

	FSquadMemberState NewState;
	NewState.Controller = Controller;
	MemberStates.Add(NewState);
}

void USquadCoordinatorComponent::UnregisterMember(AAISquadController* Controller)
{
	ReleaseRole(Controller);
	ReleaseCoverPoint(Controller);

	const int32 Index = FindMemberStateIndex(Controller);
	if (Index != INDEX_NONE)
	{
		MemberStates.RemoveAtSwap(Index);
	}
}

void USquadCoordinatorComponent::UpdateMemberState(const FSquadMemberState& NewState)
{
	const int32 Index = FindMemberStateIndex(NewState.Controller.Get());
	if (Index != INDEX_NONE)
	{
		// Preserve the role the coordinator assigned - a controller pushing its
		// own state shouldn't be able to silently overwrite arbitration state.
		const ESquadRole PreservedRole = MemberStates[Index].CurrentRole;
		MemberStates[Index] = NewState;
		MemberStates[Index].CurrentRole = PreservedRole;
	}
}

int32 USquadCoordinatorComponent::FindMemberStateIndex(const AAISquadController* Controller) const
{
	if (!Controller)
	{
		return INDEX_NONE;
	}

	for (int32 i = 0; i < MemberStates.Num(); ++i)
	{
		if (MemberStates[i].Controller.Get() == Controller)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 USquadCoordinatorComponent::CountMembersInRole(ESquadRole Role) const
{
	int32 Count = 0;
	for (const FSquadMemberState& State : MemberStates)
	{
		if (State.CurrentRole == Role)
		{
			++Count;
		}
	}
	return Count;
}

bool USquadCoordinatorComponent::RequestRole(AAISquadController* Controller, ESquadRole RequestedRole)
{
	const int32 Index = FindMemberStateIndex(Controller);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	// Already holding it - idempotent success.
	if (MemberStates[Index].CurrentRole == RequestedRole)
	{
		return true;
	}

	// Roles with a budget: enforce the cap. Roles without an explicit cap
	// (Suppressing, Retreating, Reloading, None) are unlimited - every bot is
	// always allowed to duck out and reload, for instance.
	int32 Budget = INT32_MAX;
	switch (RequestedRole)
	{
		case ESquadRole::Flanking:
			Budget = MaxConcurrentFlankers;
			break;
		case ESquadRole::Advancing:
			Budget = MaxConcurrentAdvancers;
			break;
		default:
			break;
	}

	if (CountMembersInRole(RequestedRole) >= Budget)
	{
		return false;
	}

	MemberStates[Index].CurrentRole = RequestedRole;
	return true;
}

void USquadCoordinatorComponent::ReleaseRole(AAISquadController* Controller)
{
	const int32 Index = FindMemberStateIndex(Controller);
	if (Index != INDEX_NONE)
	{
		MemberStates[Index].CurrentRole = ESquadRole::None;
	}
}

void USquadCoordinatorComponent::ReportEnemySighting(AAISquadController* Reporter, AActor* Enemy, FVector Location, bool bVisible)
{
	if (!Enemy)
	{
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const int32 ReporterId = Reporter ? Reporter->GetUniqueID() : INDEX_NONE;

	for (FSquadEnemyKnowledge& Existing : KnownEnemies)
	{
		if (Existing.Enemy.Get() == Enemy)
		{
			Existing.LastKnownLocation = Location;
			Existing.LastSeenTimestamp = Now;
			Existing.bCurrentlyVisible = bVisible;
			Existing.ReportedByControllerId = ReporterId;
			return;
		}
	}

	FSquadEnemyKnowledge NewKnowledge;
	NewKnowledge.Enemy = Enemy;
	NewKnowledge.LastKnownLocation = Location;
	NewKnowledge.LastSeenTimestamp = Now;
	NewKnowledge.bCurrentlyVisible = bVisible;
	NewKnowledge.ReportedByControllerId = ReporterId;
	KnownEnemies.Add(NewKnowledge);
}

void USquadCoordinatorComponent::PruneStaleEnemyKnowledge()
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	KnownEnemies.RemoveAll([this, Now](const FSquadEnemyKnowledge& Knowledge)
	{
		return !Knowledge.Enemy.IsValid() || (Now - Knowledge.LastSeenTimestamp) > EnemyKnowledgeStaleAfterSeconds;
	});
}

TArray<FSquadEnemyKnowledge> USquadCoordinatorComponent::GetKnownEnemies() const
{
	return KnownEnemies;
}

bool USquadCoordinatorComponent::GetPriorityEnemy(FSquadEnemyKnowledge& OutEnemy) const
{
	if (KnownEnemies.Num() == 0)
	{
		return false;
	}

	// Prefer visible enemies (most recently seen among them), fall back to
	// most recently known overall.
	const FSquadEnemyKnowledge* Best = nullptr;
	for (const FSquadEnemyKnowledge& Knowledge : KnownEnemies)
	{
		if (!Knowledge.bCurrentlyVisible)
		{
			continue;
		}
		if (!Best || Knowledge.LastSeenTimestamp > Best->LastSeenTimestamp)
		{
			Best = &Knowledge;
		}
	}

	if (!Best)
	{
		for (const FSquadEnemyKnowledge& Knowledge : KnownEnemies)
		{
			if (!Best || Knowledge.LastSeenTimestamp > Best->LastSeenTimestamp)
			{
				Best = &Knowledge;
			}
		}
	}

	if (Best)
	{
		OutEnemy = *Best;
		return true;
	}
	return false;
}

bool USquadCoordinatorComponent::ClaimCoverPoint(AAISquadController* Controller, const FVector& CoverLocation)
{
	if (const TWeakObjectPtr<AAISquadController>* Existing = ClaimedCoverPoints.Find(CoverLocation))
	{
		if (Existing->IsValid() && Existing->Get() != Controller)
		{
			return false; // someone else already has it
		}
	}
	ClaimedCoverPoints.Add(CoverLocation, Controller);
	return true;
}

void USquadCoordinatorComponent::ReleaseCoverPoint(AAISquadController* Controller)
{
	for (auto It = ClaimedCoverPoints.CreateIterator(); It; ++It)
	{
		if (It->Value.Get() == Controller)
		{
			It.RemoveCurrent();
		}
	}
}
