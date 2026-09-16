// SquadUtilityAction.h
// Base class for a single scoreable action a bot can take (take cover, flank,
// suppress, advance, reload, retreat...). Each frame (well - each decision
// tick, see AAISquadController::DecisionIntervalSeconds), the controller asks
// every registered action to score itself against the current world state and
// executes whichever scores highest. This is deliberately NOT a Behavior Tree:
// there's no fixed priority order baked into a tree structure, so adding a new
// action can't accidentally break the ordering of existing ones - it just
// competes on score.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SquadUtilityAction.generated.h"

class AAISquadController;

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class SQUADAI_API USquadUtilityAction : public UObject
{
	GENERATED_BODY()

public:
	// Returns a desirability score, conventionally in [0, 1], for this action
	// given the controller's current state. Implementations should be cheap -
	// this runs once per candidate action per decision tick, for every bot.
	// Return a negative value (or 0) to indicate "not currently viable" (e.g.
	// Reload scores 0 if the bot already has full ammo).
	UFUNCTION(BlueprintNativeEvent, Category = "SquadAI")
	float ScoreAction(AAISquadController* Controller) const;
	virtual float ScoreAction_Implementation(AAISquadController* Controller) const { return 0.f; }

	// Called once, the frame this action is selected as the new highest-scoring
	// action (i.e. on a transition into this action, not every tick it remains
	// active). Good place to request a squad role, claim a cover point, etc.
	UFUNCTION(BlueprintNativeEvent, Category = "SquadAI")
	void EnterAction(AAISquadController* Controller);
	virtual void EnterAction_Implementation(AAISquadController* Controller) {}

	// Called every decision tick while this remains the highest-scoring
	// action. Drive movement/aiming/firing here.
	UFUNCTION(BlueprintNativeEvent, Category = "SquadAI")
	void TickAction(AAISquadController* Controller, float DeltaTime);
	virtual void TickAction_Implementation(AAISquadController* Controller, float DeltaTime) {}

	// Called once when a different action outscores this one and takes over.
	// Release any claimed role/cover point here.
	UFUNCTION(BlueprintNativeEvent, Category = "SquadAI")
	void ExitAction(AAISquadController* Controller);
	virtual void ExitAction_Implementation(AAISquadController* Controller) {}

	// Human-readable name for debug display (UE_VLOG / on-screen debug), not
	// used for any logic decisions.
	UPROPERTY(EditAnywhere, Category = "SquadAI")
	FName DebugName;
};
