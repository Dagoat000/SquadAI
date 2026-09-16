// DefaultSquadSubsystem.cpp

#include "DefaultSquadSubsystem.h"
#include "SquadCoordinatorComponent.h"
#include "Engine/World.h"

USquadCoordinatorComponent* UDefaultSquadSubsystem::GetOrCreateDefaultCoordinator()
{
	if (DefaultCoordinator)
	{
		return DefaultCoordinator;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient; // never saved into the level - purely a runtime convenience
	CoordinatorOwner = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!CoordinatorOwner)
	{
		return nullptr;
	}

#if WITH_EDITOR
	CoordinatorOwner->SetActorLabel(TEXT("SquadAI_DefaultCoordinator (auto-generated)"));
#endif

	DefaultCoordinator = NewObject<USquadCoordinatorComponent>(CoordinatorOwner);
	DefaultCoordinator->RegisterComponent();
	CoordinatorOwner->AddInstanceComponent(DefaultCoordinator);

	return DefaultCoordinator;
}
