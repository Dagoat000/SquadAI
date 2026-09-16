// DefaultSquadSubsystem.h
// Exists purely so that dragging multiple AAISquadController-possessed
// pawns into a level "just works" as a squad with zero manual wiring: any
// controller that isn't explicitly given a coordinator via AssignToSquad()
// falls back to this subsystem's single shared coordinator instead. Projects
// that want several independent squads still call AssignToSquad() with their
// own USquadCoordinatorComponent per squad - that explicit assignment always
// takes priority over this default.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DefaultSquadSubsystem.generated.h"

class USquadCoordinatorComponent;

UCLASS()
class SQUADAI_API UDefaultSquadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Returns the shared default coordinator, spawning the (invisible, never
	// ticking-in-render-terms) actor that hosts it on first call.
	UFUNCTION(BlueprintCallable, Category = "SquadAI")
	USquadCoordinatorComponent* GetOrCreateDefaultCoordinator();

private:
	UPROPERTY()
	TObjectPtr<AActor> CoordinatorOwner;

	UPROPERTY()
	TObjectPtr<USquadCoordinatorComponent> DefaultCoordinator;
};
