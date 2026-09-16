// SquadUtilityActions_Common.h
// A starter roster of concrete actions. These are intentionally simple -
// they're meant as a working baseline and a template for writing your own,
// not a finished tuning pass. Expect to rewrite the scoring curves once you
// see them play out against your actual level geometry and weapon feel.

#pragma once

#include "CoreMinimal.h"
#include "SquadUtilityAction.h"
#include "SquadUtilityActions_Common.generated.h"

// Retreat to cover from the current priority threat. Scores higher when
// health is low or the bot has no cover claimed yet while under fire.
UCLASS(EditInlineNew)
class SQUADAI_API USquadAction_TakeCover : public USquadUtilityAction
{
	GENERATED_BODY()
public:
	virtual float ScoreAction_Implementation(AAISquadController* Controller) const override;
	virtual void EnterAction_Implementation(AAISquadController* Controller) override;
	virtual void ExitAction_Implementation(AAISquadController* Controller) override;

	// Below this health fraction, taking cover becomes strongly preferred
	// over anything except reloading with zero ammo.
	UPROPERTY(EditAnywhere)
	float LowHealthThreshold = 0.4f;
};

// Hold current position (assumed to already be cover) and fire on the
// priority enemy. Scores well when a target is visible and the bot already
// has decent health/ammo - this is the "default" combat state.
UCLASS(EditInlineNew)
class SQUADAI_API USquadAction_Suppress : public USquadUtilityAction
{
	GENERATED_BODY()
public:
	virtual float ScoreAction_Implementation(AAISquadController* Controller) const override;
	virtual void EnterAction_Implementation(AAISquadController* Controller) override;
	virtual void ExitAction_Implementation(AAISquadController* Controller) override;
};

// Move to a flanking position relative to the priority enemy's last known
// location. Requires the squad to grant the Flanking role (limited slots), so
// this naturally only fires for one (or MaxConcurrentFlankers) bot(s) at a
// time even though every bot scores it every tick.
UCLASS(EditInlineNew)
class SQUADAI_API USquadAction_Flank : public USquadUtilityAction
{
	GENERATED_BODY()
public:
	virtual float ScoreAction_Implementation(AAISquadController* Controller) const override;
	virtual void EnterAction_Implementation(AAISquadController* Controller) override;
	virtual void TickAction_Implementation(AAISquadController* Controller, float DeltaTime) override;
	virtual void ExitAction_Implementation(AAISquadController* Controller) override;

	// How far to the side of the enemy's last known location to path toward,
	// picked perpendicular to the vector from the bot to the enemy.
	UPROPERTY(EditAnywhere)
	float FlankOffsetDistance = 800.f;

private:
	mutable FVector CachedFlankDestination = FVector::ZeroVector;
};

// Duck into claimed cover and reload. Scores at 0 (not viable) whenever ammo
// is already full, so it never competes for the top slot unless actually needed.
UCLASS(EditInlineNew)
class SQUADAI_API USquadAction_Reload : public USquadUtilityAction
{
	GENERATED_BODY()
public:
	virtual float ScoreAction_Implementation(AAISquadController* Controller) const override;
	virtual void EnterAction_Implementation(AAISquadController* Controller) override;
	virtual void TickAction_Implementation(AAISquadController* Controller, float DeltaTime) override;
	virtual void ExitAction_Implementation(AAISquadController* Controller) override;

	UPROPERTY(EditAnywhere)
	float ReloadDurationSeconds = 2.f;

private:
	mutable float TimeRemaining = 0.f;
};
