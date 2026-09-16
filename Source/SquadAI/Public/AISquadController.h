// AISquadController.h
// One instance per bot. Owns the bot's utility action set, talks to
// AIPerceptionComponent for local senses, and reports/reads shared state via
// the squad's USquadCoordinatorComponent. Movement itself is delegated to
// UNavigationSystemV1 (via AAIController::MoveToLocation) - this class decides
// *where* and *why*, not how to path there.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SquadAITypes.h"
#include "AISquadController.generated.h"

class USquadUtilityAction;
class USquadCoordinatorComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

UCLASS()
class SQUADAI_API AAISquadController : public AAIController
{
	GENERATED_BODY()

public:
	AAISquadController();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	// The squad this bot belongs to. Set this (e.g. from the spawner/level
	// blueprint) before or immediately after possession. A controller with no
	// coordinator assigned still functions - it just fights alone with no
	// coordination bonus, useful for solo enemies using the same AI stack.
	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	void AssignToSquad(USquadCoordinatorComponent* Coordinator);

	UFUNCTION(BlueprintPure, Category = "SquadAI")
	USquadCoordinatorComponent* GetSquadCoordinator() const { return SquadCoordinator.Get(); }

	// --- State queried by USquadUtilityAction implementations ---

	UFUNCTION(BlueprintPure, Category = "SquadAI")
	float GetHealthFraction() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SquadAI|State")
	int32 CurrentAmmo = 30;

	UPROPERTY(EditAnywhere, Category = "SquadAI|State")
	int32 MaxAmmo = 30;

	UFUNCTION(BlueprintPure, Category = "SquadAI")
	bool TryGetPriorityEnemy(FSquadEnemyKnowledge& OutEnemy) const;

	// How often (seconds) the utility action set is re-scored. Kept coarse
	// (a few times a second, not every tick) - squad tactics don't need
	// frame-perfect reaction, and this keeps N-bot cost manageable.
	UPROPERTY(EditAnywhere, Category = "SquadAI|Tuning")
	float DecisionIntervalSeconds = 0.3f;

	// Radius searched for cover when an action calls RequestCoverFromCurrentThreat.
	UPROPERTY(EditAnywhere, Category = "SquadAI|Tuning")
	float CoverSearchRadius = 2000.f;

	// Convenience used by actions: finds and claims the nearest valid cover
	// point from the current priority threat, and starts moving there.
	// Returns false if no cover was available (caller should fall back).
	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	bool RequestCoverFromCurrentThreat();

	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	void ReleaseClaimedCover();

	// The set of actions this bot considers each decision tick. Pre-populated
	// in the constructor with a sensible default roster (TakeCover, Suppress,
	// Flank, Reload) so a bot works out of the box with zero Blueprint setup -
	// edit per-instance in the Details panel, or replace entirely, to
	// customize.
	UPROPERTY(EditAnywhere, Instanced, Category = "SquadAI")
	TArray<TObjectPtr<USquadUtilityAction>> AvailableActions;

	// --- Zero-setup convenience behavior ---
	// All of the below happens automatically in BeginPlay so that dragging a
	// pawn possessed by this controller into a level, with no other setup,
	// results in a working bot: it joins a shared default squad with any
	// other un-assigned bots, generates cover points around itself if none
	// exist yet in the level, and registers the player pawn(s) as something
	// it can actually perceive. Every piece of this can be overridden or
	// disabled per-instance below if you want manual control instead.

	// If true (default), a controller with no squad explicitly assigned via
	// AssignToSquad() automatically joins a single shared coordinator for the
	// whole level, so multiple dragged-in bots squad up with zero wiring.
	// Set false if you're going to call AssignToSquad() yourself for every
	// bot and don't want the automatic fallback.
	UPROPERTY(EditAnywhere, Category = "SquadAI|AutoSetup")
	bool bAutoJoinDefaultSquad = true;

	// If true (default) and the level has no cover points generated yet,
	// this bot generates a batch centered on its own spawn location the
	// first time it begins play. Only the first bot to spawn actually
	// triggers generation; later bots see cover points already exist.
	UPROPERTY(EditAnywhere, Category = "SquadAI|AutoSetup")
	bool bAutoGenerateCoverOnBeginPlay = true;

	// Radius (from this bot's spawn point) used for automatic cover
	// generation. For a level much bigger than this, call
	// UCoverPointSubsystem::GenerateCoverPointsAround() yourself with a
	// larger radius (e.g. from a level Blueprint on BeginPlay) instead of
	// relying on the automatic per-bot default.
	UPROPERTY(EditAnywhere, Category = "SquadAI|AutoSetup")
	float AutoCoverGenerationRadius = 4000.f;

	// If true (default), registers all current PlayerController pawns as AI
	// Perception stimuli sources on BeginPlay, so this bot can actually sense
	// the player without you needing to add a UAIPerceptionStimuliSourceComponent
	// to your player Character yourself. Safe to leave on even with multiple
	// bots - registering an already-registered actor is a no-op.
	UPROPERTY(EditAnywhere, Category = "SquadAI|AutoSetup")
	bool bAutoRegisterPlayerAsPerceptionSource = true;

protected:
	// NB: do NOT declare a PerceptionComponent member here - AAIController
	// already provides one (that's what SetPerceptionComponent() populates in
	// the constructor below). Redeclaring it in a subclass is illegal member
	// shadowing and won't compile. Use the inherited PerceptionComponent
	// directly wherever it's needed.

	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

private:
	UPROPERTY()
	TWeakObjectPtr<USquadCoordinatorComponent> SquadCoordinator;

	UPROPERTY()
	TObjectPtr<USquadUtilityAction> CurrentAction;

	float TimeUntilNextDecision = 0.f;

	FVector ClaimedCoverLocation = FVector::ZeroVector;
	bool bHasClaimedCover = false;

	void RunDecisionTick(float DeltaTime);
	void PushStateToSquad() const;

	// Shared by the perception delegate and the hearing-poll fallback below.
	void ReportActorPerceptionToSquad(AActor* Actor);

	// Workaround for a UE 5.8 hearing-sense delegate bug - see .cpp for details.
	void PollHearingAsFallback();

	// --- Zero-setup helpers, called from BeginPlay ---
	void AutoJoinDefaultSquadIfUnassigned();
	void AutoGenerateCoverIfNoneExists();
	void AutoRegisterPlayerPawnsAsPerceptionSources();
	static void EnsureActorIsPerceptionSource(AActor* Actor);
};
