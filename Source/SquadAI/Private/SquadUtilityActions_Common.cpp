// SquadUtilityActions_Common.cpp

#include "SquadUtilityActions_Common.h"
#include "AISquadController.h"
#include "SquadCoordinatorComponent.h"
#include "GameFramework/Pawn.h"

// --- TakeCover ---

float USquadAction_TakeCover::ScoreAction_Implementation(AAISquadController* Controller) const
{
	if (!Controller)
	{
		return 0.f;
	}

	FSquadEnemyKnowledge Threat;
	const bool bUnderThreat = Controller->TryGetPriorityEnemy(Threat);
	if (!bUnderThreat)
	{
		return 0.f; // nothing to take cover from
	}

	const float Health = Controller->GetHealthFraction();
	// Rises sharply as health drops below the threshold, otherwise a modest
	// baseline whenever a threat is visible (cover is generally always
	// somewhat desirable in a firefight).
	float Score = 0.3f;
	if (Health < LowHealthThreshold)
	{
		Score += (LowHealthThreshold - Health) * 1.5f;
	}
	return FMath::Clamp(Score, 0.f, 1.f);
}

void USquadAction_TakeCover::EnterAction_Implementation(AAISquadController* Controller)
{
	if (Controller)
	{
		Controller->RequestCoverFromCurrentThreat();
	}
}

void USquadAction_TakeCover::ExitAction_Implementation(AAISquadController* Controller)
{
	if (Controller)
	{
		Controller->ReleaseClaimedCover();
	}
}

// --- Suppress ---

float USquadAction_Suppress::ScoreAction_Implementation(AAISquadController* Controller) const
{
	if (!Controller)
	{
		return 0.f;
	}

	FSquadEnemyKnowledge Threat;
	if (!Controller->TryGetPriorityEnemy(Threat) || !Threat.bCurrentlyVisible)
	{
		return 0.f; // nothing to suppress
	}

	if (Controller->CurrentAmmo <= 0)
	{
		return 0.f; // can't suppress with an empty gun
	}

	// Baseline "keep fighting from here" score - deliberately not the highest
	// possible so TakeCover/Flank can outscore it under the right conditions.
	return 0.6f;
}

void USquadAction_Suppress::EnterAction_Implementation(AAISquadController* Controller)
{
	if (Controller && Controller->GetSquadCoordinator())
	{
		Controller->GetSquadCoordinator()->RequestRole(Controller, ESquadRole::Suppressing);
	}
	// Actual weapon-firing hookup is project-specific (whatever your
	// AWeapon/UCombatComponent equivalent looks like) - call it here.
}

void USquadAction_Suppress::ExitAction_Implementation(AAISquadController* Controller)
{
	if (Controller && Controller->GetSquadCoordinator())
	{
		Controller->GetSquadCoordinator()->ReleaseRole(Controller);
	}
}

// --- Flank ---

float USquadAction_Flank::ScoreAction_Implementation(AAISquadController* Controller) const
{
	if (!Controller || !Controller->GetSquadCoordinator())
	{
		return 0.f; // flanking needs squad coordination to make sense
	}

	FSquadEnemyKnowledge Threat;
	if (!Controller->TryGetPriorityEnemy(Threat))
	{
		return 0.f;
	}

	// Only worth attempting if the enemy has been known long enough that a
	// flank has a chance of paying off, and the bot isn't critically low on
	// health (that's TakeCover/Retreat's job).
	if (Controller->GetHealthFraction() < 0.5f)
	{
		return 0.f;
	}

	// Precompute a candidate destination so EnterAction doesn't redo this
	// work; a modest fixed score keeps this competitive with Suppress without
	// dominating it, so a squad doesn't have everyone flank simultaneously
	// even before role arbitration kicks in.
	const APawn* ControlledPawn = Controller->GetPawn();
	if (!ControlledPawn)
	{
		return 0.f;
	}

	const FVector ToEnemy = (Threat.LastKnownLocation - ControlledPawn->GetActorLocation()).GetSafeNormal2D();
	const FVector Perpendicular = FVector(-ToEnemy.Y, ToEnemy.X, 0.f);
	CachedFlankDestination = Threat.LastKnownLocation + Perpendicular * FlankOffsetDistance;

	return 0.55f;
}

void USquadAction_Flank::EnterAction_Implementation(AAISquadController* Controller)
{
	if (!Controller || !Controller->GetSquadCoordinator())
	{
		return;
	}

	// Ask permission - if the squad's flanker budget is already spent, this
	// bot backs off and will re-score next decision tick (likely landing on
	// Suppress instead).
	if (!Controller->GetSquadCoordinator()->RequestRole(Controller, ESquadRole::Flanking))
	{
		return;
	}

	Controller->MoveToLocation(CachedFlankDestination, /*AcceptanceRadius=*/100.f);
}

void USquadAction_Flank::TickAction_Implementation(AAISquadController* Controller, float DeltaTime)
{
	// Re-issuing MoveToLocation every tick would fight the nav movement
	// component; nothing to do here in the baseline version beyond what
	// EnterAction already kicked off. A more advanced version could re-path
	// periodically if the enemy's last known location moves significantly.
}

void USquadAction_Flank::ExitAction_Implementation(AAISquadController* Controller)
{
	if (Controller && Controller->GetSquadCoordinator())
	{
		Controller->GetSquadCoordinator()->ReleaseRole(Controller);
	}
}

// --- Reload ---

float USquadAction_Reload::ScoreAction_Implementation(AAISquadController* Controller) const
{
	if (!Controller || Controller->CurrentAmmo >= Controller->MaxAmmo)
	{
		return 0.f; // never viable at full ammo
	}

	if (Controller->CurrentAmmo <= 0)
	{
		return 0.95f; // empty gun beats almost everything except critical retreat
	}

	// Otherwise scale up as ammo gets low, but stay below Suppress/TakeCover
	// so a bot with partial ammo keeps fighting rather than ducking out early.
	const float AmmoFraction = static_cast<float>(Controller->CurrentAmmo) / FMath::Max(1, Controller->MaxAmmo);
	return FMath::Clamp((0.3f - AmmoFraction), 0.f, 0.5f);
}

void USquadAction_Reload::EnterAction_Implementation(AAISquadController* Controller)
{
	if (Controller && Controller->GetSquadCoordinator())
	{
		Controller->GetSquadCoordinator()->RequestRole(Controller, ESquadRole::Reloading);
	}
	if (Controller)
	{
		Controller->RequestCoverFromCurrentThreat(); // reload behind cover if any is available
	}
	TimeRemaining = ReloadDurationSeconds;
}

void USquadAction_Reload::TickAction_Implementation(AAISquadController* Controller, float DeltaTime)
{
	TimeRemaining -= DeltaTime;
	if (TimeRemaining <= 0.f && Controller)
	{
		Controller->CurrentAmmo = Controller->MaxAmmo;
	}
}

void USquadAction_Reload::ExitAction_Implementation(AAISquadController* Controller)
{
	if (Controller)
	{
		if (Controller->GetSquadCoordinator())
		{
			Controller->GetSquadCoordinator()->ReleaseRole(Controller);
		}
		Controller->ReleaseClaimedCover();
	}
}
