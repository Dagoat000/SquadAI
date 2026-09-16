// SquadCoordinatorComponent.h
// One instance of this lives on a "squad owner" actor (could be a dedicated
// empty AActor placed in the level, or the first bot spawned in a squad - your
// call). Every AAISquadController in the squad registers with it. It is the
// only place squad-wide state is written, so there's a single source of truth
// for "who is doing what" and "what have we collectively seen."
//
// Deliberately a UActorComponent rather than a subsystem: a subsystem would be
// one-per-world, which forces every squad in a level to share state. A
// component per squad-owner actor lets you run several independent squads.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SquadAITypes.h"
#include "SquadCoordinatorComponent.generated.h"

UCLASS(ClassGroup = (SquadAI), meta = (BlueprintSpawnableComponent))
class SQUADAI_API USquadCoordinatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USquadCoordinatorComponent();

	// --- Membership ---

	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	void RegisterMember(AAISquadController* Controller);

	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	void UnregisterMember(AAISquadController* Controller);

	UFUNCTION(BlueprintPure, Category = "SquadAI")
	const TArray<FSquadMemberState>& GetMemberStates() const { return MemberStates; }

	// Called every tick (or on a throttled timer, see TickIntervalSeconds) by
	// each controller to push its current state into the squad snapshot.
	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	void UpdateMemberState(const FSquadMemberState& NewState);

	// --- Role arbitration ---

	// Ask permission to take on a role. Returns true and reserves the role if
	// the squad's role budget allows it (e.g. MaxConcurrentFlankers), false
	// otherwise. A bot that is refused should fall back to a different action
	// this frame rather than retrying immediately.
	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	bool RequestRole(AAISquadController* Controller, ESquadRole RequestedRole);

	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	void ReleaseRole(AAISquadController* Controller);

	UFUNCTION(BlueprintPure, Category = "SquadAI")
	int32 CountMembersInRole(ESquadRole Role) const;

	// --- Shared enemy knowledge ---

	// Merges a sighting into the squad's shared knowledge. If the enemy is
	// already known, updates location/timestamp/visibility rather than
	// duplicating the entry.
	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	void ReportEnemySighting(AAISquadController* Reporter, AActor* Enemy, FVector Location, bool bVisible);

	// Sightings older than this are pruned from GetKnownEnemies() so bots
	// don't chase ghosts indefinitely.
	UPROPERTY(EditAnywhere, Category = "SquadAI|Tuning")
	float EnemyKnowledgeStaleAfterSeconds = 12.f;

	UFUNCTION(BlueprintPure, Category = "SquadAI")
	TArray<FSquadEnemyKnowledge> GetKnownEnemies() const;

	// The single most relevant enemy to react to right now: currently visible
	// to the most members if any are visible, else most recently seen.
	UFUNCTION(BlueprintPure, Category = "SquadAI")
	bool GetPriorityEnemy(FSquadEnemyKnowledge& OutEnemy) const;

	// --- Cover claiming (thin pass-through so bots don't need a separate
	// reference to the cover subsystem for the "claim" half of the workflow) ---

	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	bool ClaimCoverPoint(AAISquadController* Controller, const FVector& CoverLocation);

	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	void ReleaseCoverPoint(AAISquadController* Controller);

	// Max bots allowed to simultaneously hold the Flanking role. Kept small -
	// two bots is usually already enough to look "coordinated" without every
	// member abandoning suppression at once.
	UPROPERTY(EditAnywhere, Category = "SquadAI|Tuning")
	int32 MaxConcurrentFlankers = 1;

	UPROPERTY(EditAnywhere, Category = "SquadAI|Tuning")
	int32 MaxConcurrentAdvancers = 2;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	TArray<FSquadMemberState> MemberStates;

	UPROPERTY()
	TArray<FSquadEnemyKnowledge> KnownEnemies;

	// Location -> claiming controller. Kept separate from FCoverPointData
	// because cover points themselves are regenerated/queried from
	// UCoverPointSubsystem and we don't want to own their storage here.
	TMap<FVector, TWeakObjectPtr<AAISquadController>> ClaimedCoverPoints;

	void PruneStaleEnemyKnowledge();

	int32 FindMemberStateIndex(const AAISquadController* Controller) const;
};
