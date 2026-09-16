// CoverPointSubsystem.h
// Generates candidate cover points by sampling the navmesh and testing for
// nearby vertical obstacles via line traces, then answers "best cover point
// near X that blocks line of sight to Y" queries for individual bots.
//
// A world subsystem rather than per-squad, because cover geometry is a
// property of the level, not of any particular squad - two rival squads in
// the same room should be able to see (and steal) the same cover points.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SquadAITypes.h"
#include "CoverPointSubsystem.generated.h"

UCLASS()
class SQUADAI_API UCoverPointSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Populates the internal cover point cache by sampling a grid of navmesh
	// points around Origin within Radius, and line-tracing outward in
	// CandidateDirections.Num() directions from each to find obstacle-adjacent
	// spots. Call once per relevant area (e.g. on level load, or the first time
	// combat starts in a region) rather than every tick - this is not cheap.
	UFUNCTION(BlueprintCallable, Category = "SquadAI|Cover")
	void GenerateCoverPointsAround(FVector Origin, float Radius, float GridSpacing = 200.f);

	// Finds the best unclaimed cover point within SearchRadius of SeekerLocation
	// that has line-of-sight blocked to ThreatLocation. Returns false if none found.
	// "Best" = closest to SeekerLocation among those that qualify.
	UFUNCTION(BlueprintCallable, Category = "SquadAI|Cover")
	bool FindCoverFromThreat(FVector SeekerLocation, FVector ThreatLocation, float SearchRadius, FCoverPointData& OutCover) const;

	// Distance from an obstacle a cover point sample sits at. Should roughly
	// match your character's capsule radius plus a small buffer, so the bot's
	// pawn actually fits at the point without clipping.
	UPROPERTY(EditAnywhere, Category = "SquadAI|Cover|Tuning")
	float StandoffFromObstacle = 60.f;

	// Minimum obstacle height (from trace-down at the sample point) to count
	// as usable cover - filters out curbs and low clutter that wouldn't
	// actually block a shot.
	UPROPERTY(EditAnywhere, Category = "SquadAI|Cover|Tuning")
	float MinObstacleHeight = 90.f;

	UPROPERTY(EditAnywhere, Category = "SquadAI|Cover|Tuning")
	int32 TraceDirectionCount = 8;

	UFUNCTION(BlueprintCallable, Category = "SquadAI|Cover")
	void ClearCoverPoints() { CoverPoints.Empty(); }

	UFUNCTION(BlueprintPure, Category = "SquadAI|Cover")
	const TArray<FCoverPointData>& GetAllCoverPoints() const { return CoverPoints; }

private:
	UPROPERTY()
	TArray<FCoverPointData> CoverPoints;

	// Tests whether a candidate point actually has an obstacle behind it tall
	// enough to matter, and if so fills in FacingDirection. Returns false if
	// the point isn't valid cover (e.g. mid-air, or obstacle too short).
	bool EvaluateCandidatePoint(const FVector& NavPoint, const FVector& DirectionToObstacle, FCoverPointData& OutCoverPoint) const;

	bool HasLineOfSight(const FVector& From, const FVector& To) const;
};
