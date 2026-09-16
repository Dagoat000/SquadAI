// CoverPointSubsystem.cpp

#include "CoverPointSubsystem.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "Engine/World.h"

void UCoverPointSubsystem::GenerateCoverPointsAround(FVector Origin, float Radius, float GridSpacing)
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
	if (!NavSys)
	{
		UE_LOG(LogTemp, Warning, TEXT("CoverPointSubsystem: no navigation system available, aborting generation."));
		return;
	}

	const int32 Steps = FMath::Max(1, FMath::CeilToInt((Radius * 2.f) / GridSpacing));
	const int32 DirCount = FMath::Max(4, TraceDirectionCount);

	for (int32 X = -Steps; X <= Steps; ++X)
	{
		for (int32 Y = -Steps; Y <= Steps; ++Y)
		{
			const FVector GridPoint = Origin + FVector(X * GridSpacing, Y * GridSpacing, 0.f);
			if (FVector::DistSquared2D(GridPoint, Origin) > FMath::Square(Radius))
			{
				continue;
			}

			FNavLocation ProjectedPoint;
			if (!NavSys->ProjectPointToNavigation(GridPoint, ProjectedPoint, FVector(GridSpacing * 0.5f)))
			{
				continue; // no walkable navmesh near this grid cell
			}

			// Sample outward in a ring of directions looking for a nearby
			// obstacle - a cover point is only useful if something is right
			// behind it to hide behind.
			for (int32 DirIndex = 0; DirIndex < DirCount; ++DirIndex)
			{
				const float Angle = (2.f * PI * DirIndex) / DirCount;
				const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);

				FCoverPointData Candidate;
				if (EvaluateCandidatePoint(ProjectedPoint.Location, Direction, Candidate))
				{
					CoverPoints.Add(Candidate);
					break; // one valid cover point per navmesh sample is enough
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CoverPointSubsystem: generated %d cover points around %s (radius %.0f)."),
		CoverPoints.Num(), *Origin.ToString(), Radius);
}

bool UCoverPointSubsystem::EvaluateCandidatePoint(const FVector& NavPoint, const FVector& DirectionToObstacle, FCoverPointData& OutCoverPoint) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector TraceStart = NavPoint + FVector(0.f, 0.f, 50.f); // roughly chest height
	const FVector TraceEnd = TraceStart + DirectionToObstacle * StandoffFromObstacle * 1.5f;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CoverPointObstacleTrace), false);
	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params);
	if (!bHit)
	{
		return false; // nothing to hide behind in this direction
	}

	// Confirm the obstacle actually has some height to it (rules out curbs,
	// small props) by tracing straight down from just above the hit point.
	FHitResult HeightHit;
	const FVector HeightTraceStart = Hit.ImpactPoint + FVector(0.f, 0.f, MinObstacleHeight);
	const FVector HeightTraceEnd = Hit.ImpactPoint - FVector(0.f, 0.f, 10.f);
	const bool bTallEnough = World->LineTraceSingleByChannel(HeightHit, HeightTraceStart, HeightTraceEnd, ECC_WorldStatic, Params);
	if (!bTallEnough)
	{
		return false;
	}

	OutCoverPoint.Location = NavPoint;
	OutCoverPoint.FacingDirection = DirectionToObstacle.GetSafeNormal();
	OutCoverPoint.bProvidesCoverFromLastKnownEnemy = false; // evaluated per-query, not at generation time
	OutCoverPoint.ClaimedBy = nullptr;
	return true;
}

bool UCoverPointSubsystem::HasLineOfSight(const FVector& From, const FVector& To) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CoverPointLineOfSightTrace), false);
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
	return !bBlocked;
}

bool UCoverPointSubsystem::FindCoverFromThreat(FVector SeekerLocation, FVector ThreatLocation, float SearchRadius, FCoverPointData& OutCover) const
{
	bool bFound = false;
	float BestDistSq = FLT_MAX;

	for (const FCoverPointData& Point : CoverPoints)
	{
		if (Point.ClaimedBy.IsValid())
		{
			continue; // already in use
		}

		const float DistSq = FVector::DistSquared(SeekerLocation, Point.Location);
		if (DistSq > FMath::Square(SearchRadius))
		{
			continue;
		}

		// A point only counts as cover from this specific threat if the
		// obstacle direction is roughly between the point and the threat -
		// otherwise you'd be "hiding" behind a wall the enemy isn't even
		// shooting through.
		const FVector ToThreat = (ThreatLocation - Point.Location).GetSafeNormal();
		const float FacingDot = FVector::DotProduct(Point.FacingDirection, ToThreat);
		if (FacingDot < 0.4f) // obstacle is roughly toward the threat, not away from it
		{
			continue;
		}

		// Chest-height eye trace from the cover point back to the threat -
		// if it's blocked, the obstacle is doing its job.
		const FVector EyeOffset(0.f, 0.f, 50.f);
		if (HasLineOfSight(Point.Location + EyeOffset, ThreatLocation + EyeOffset))
		{
			continue; // no obstruction, doesn't actually block the shot
		}

		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			OutCover = Point;
			OutCover.bProvidesCoverFromLastKnownEnemy = true;
			bFound = true;
		}
	}

	return bFound;
}
