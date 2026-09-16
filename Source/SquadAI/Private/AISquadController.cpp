// AISquadController.cpp

#include "AISquadController.h"
#include "SquadCoordinatorComponent.h"
#include "SquadUtilityAction.h"
#include "SquadUtilityActions_Common.h"
#include "CoverPointSubsystem.h"
#include "DefaultSquadSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

AAISquadController::AAISquadController()
{
	PrimaryActorTick.bCanEverTick = true;

	// Assigning through a local first, then handing it to
	// SetPerceptionComponent(), which is what actually populates the
	// inherited PerceptionComponent member - avoids re-declaring that member
	// ourselves (AAIController already owns it).
	UAIPerceptionComponent* NewPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
	SetPerceptionComponent(*NewPerceptionComponent);

	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2500.f;
	SightConfig->LoseSightRadius = 3000.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	UAISenseConfig_Hearing* HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 1500.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;

	NewPerceptionComponent->ConfigureSense(*SightConfig);
	NewPerceptionComponent->ConfigureSense(*HearingConfig);
	NewPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	// Pre-populate a working default action roster so a freshly-dragged-in
	// bot has something to do immediately, with no Blueprint setup required.
	// These are plain instanced UObjects, editable per-instance afterward
	// (or removable) in the Details panel like any other Instanced array.
	AvailableActions.Add(CreateDefaultSubobject<USquadAction_TakeCover>(TEXT("DefaultTakeCover")));
	AvailableActions.Add(CreateDefaultSubobject<USquadAction_Suppress>(TEXT("DefaultSuppress")));
	AvailableActions.Add(CreateDefaultSubobject<USquadAction_Flank>(TEXT("DefaultFlank")));
	AvailableActions.Add(CreateDefaultSubobject<USquadAction_Reload>(TEXT("DefaultReload")));
}

void AAISquadController::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoRegisterPlayerAsPerceptionSource)
	{
		AutoRegisterPlayerPawnsAsPerceptionSources();
	}
	if (bAutoJoinDefaultSquad)
	{
		AutoJoinDefaultSquadIfUnassigned();
	}
	if (bAutoGenerateCoverOnBeginPlay)
	{
		AutoGenerateCoverIfNoneExists();
	}
}

void AAISquadController::EnsureActorIsPerceptionSource(AActor* Actor)
{
	if (!Actor || Actor->FindComponentByClass<UAIPerceptionStimuliSourceComponent>())
	{
		return; // already registered, or nothing to register
	}

	UAIPerceptionStimuliSourceComponent* StimuliSource = NewObject<UAIPerceptionStimuliSourceComponent>(Actor);
	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());
	StimuliSource->RegisterComponent();
	StimuliSource->RegisterWithPerceptionSystem();
}

void AAISquadController::AutoRegisterPlayerPawnsAsPerceptionSources()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Covers local multiplayer / split-screen too, not just player index 0.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APlayerController* PC = It->Get())
		{
			EnsureActorIsPerceptionSource(PC->GetPawn());
		}
	}
}

void AAISquadController::AutoJoinDefaultSquadIfUnassigned()
{
	if (SquadCoordinator.IsValid())
	{
		return; // something (e.g. a spawner) already explicitly assigned a squad
	}

	if (UDefaultSquadSubsystem* DefaultSquadSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UDefaultSquadSubsystem>() : nullptr)
	{
		AssignToSquad(DefaultSquadSubsystem->GetOrCreateDefaultCoordinator());
	}
}

void AAISquadController::AutoGenerateCoverIfNoneExists()
{
	UCoverPointSubsystem* CoverSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UCoverPointSubsystem>() : nullptr;
	const APawn* ControlledPawn = GetPawn();
	if (!CoverSubsystem || !ControlledPawn)
	{
		return;
	}

	// Only the first bot to reach this (across the whole level) actually
	// generates anything - everyone after sees a non-empty list and skips it,
	// so N bots dropped in the same area don't redundantly regenerate.
	if (CoverSubsystem->GetAllCoverPoints().Num() == 0)
	{
		CoverSubsystem->GenerateCoverPointsAround(ControlledPawn->GetActorLocation(), AutoCoverGenerationRadius);
	}
}

void AAISquadController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (PerceptionComponent)
	{
		// NB: this is the TArray<AActor*> "batch" delegate, not
		// OnTargetPerceptionUpdated (which passes a single AActor*+FAIStimulus
		// and has a different signature) - binding to the wrong one is a
		// signature mismatch that won't compile.
		PerceptionComponent->OnPerceptionUpdated.AddUniqueDynamic(this, &AAISquadController::OnPerceptionUpdated);
	}
}

void AAISquadController::AssignToSquad(USquadCoordinatorComponent* Coordinator)
{
	if (SquadCoordinator.IsValid())
	{
		SquadCoordinator->UnregisterMember(this);
	}

	SquadCoordinator = Coordinator;

	if (Coordinator)
	{
		Coordinator->RegisterMember(this);
	}
}

float AAISquadController::GetHealthFraction() const
{
	// Deliberately not assuming a specific health component/attribute set -
	// projects vary too much here (UGameplayAbilities health attribute vs. a
	// plain float on a custom Character base class). Override this in a
	// game-specific subclass to read your actual health source. Defaulting to
	// 1.0 (full health) rather than 0.0 so an un-overridden bot doesn't
	// immediately look mortally wounded to every utility action.
	return 1.f;
}

bool AAISquadController::TryGetPriorityEnemy(FSquadEnemyKnowledge& OutEnemy) const
{
	if (SquadCoordinator.IsValid())
	{
		return SquadCoordinator->GetPriorityEnemy(OutEnemy);
	}
	return false;
}

void AAISquadController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	if (!SquadCoordinator.IsValid() || !PerceptionComponent)
	{
		return;
	}

	for (AActor* Actor : UpdatedActors)
	{
		ReportActorPerceptionToSquad(Actor);
	}
}

void AAISquadController::ReportActorPerceptionToSquad(AActor* Actor)
{
	if (!Actor || !PerceptionComponent || !SquadCoordinator.IsValid())
	{
		return;
	}

	FActorPerceptionBlueprintInfo Info;
	PerceptionComponent->GetActorsPerception(Actor, Info);

	for (const FAIStimulus& Stimulus : Info.LastSensedStimuli)
	{
		// Only sight stimuli carry a meaningful "currently visible" flag for
		// our purposes; a heard-but-not-seen actor is still reported, just as
		// not-currently-visible.
		const bool bVisible = Stimulus.WasSuccessfullySensed() && Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
		SquadCoordinator->ReportEnemySighting(this, Actor, Stimulus.StimulusLocation, bVisible);
	}
}

void AAISquadController::PollHearingAsFallback()
{
	// Workaround for a UE 5.8 engine bug (open on Epic's forums as of writing)
	// where AISense_Hearing does not reliably fire OnPerceptionUpdated /
	// OnTargetPerceptionUpdated. This polls hearing-sensed actors directly
	// each decision tick instead of relying on the delegate for that sense.
	// Cheap to leave in even after Epic fixes it - it's a no-op duplicate
	// report in that case, and ReportEnemySighting already collapses repeat
	// reports for the same actor. Remove once you've confirmed the delegate
	// fires correctly on your target engine version.
	if (!PerceptionComponent)
	{
		return;
	}

	TArray<AActor*> HeardActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), HeardActors);
	for (AActor* Actor : HeardActors)
	{
		ReportActorPerceptionToSquad(Actor);
	}
}

void AAISquadController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeUntilNextDecision -= DeltaTime;
	if (TimeUntilNextDecision <= 0.f)
	{
		RunDecisionTick(DeltaTime);
		TimeUntilNextDecision = DecisionIntervalSeconds;
	}

	if (CurrentAction)
	{
		CurrentAction->TickAction(this, DeltaTime);
	}
}

void AAISquadController::RunDecisionTick(float DeltaTime)
{
	PushStateToSquad();
	PollHearingAsFallback();

	USquadUtilityAction* BestAction = nullptr;
	float BestScore = -1.f;

	for (USquadUtilityAction* Action : AvailableActions)
	{
		if (!Action)
		{
			continue;
		}
		const float Score = Action->ScoreAction(this);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestAction = Action;
		}
	}

	// A score of 0 or below means "not viable" - if nothing scored above
	// that, deliberately fall back to no action rather than picking an
	// arbitrary non-viable one.
	if (BestScore <= 0.f)
	{
		BestAction = nullptr;
	}

	if (BestAction != CurrentAction)
	{
		if (CurrentAction)
		{
			CurrentAction->ExitAction(this);
		}
		CurrentAction = BestAction;
		if (CurrentAction)
		{
			CurrentAction->EnterAction(this);
		}
	}
}

void AAISquadController::PushStateToSquad() const
{
	if (!SquadCoordinator.IsValid())
	{
		return;
	}

	FSquadMemberState State;
	State.Controller = const_cast<AAISquadController*>(this);
	State.HealthFraction = GetHealthFraction();
	State.AmmoCount = CurrentAmmo;
	if (const APawn* ControlledPawn = GetPawn())
	{
		State.Location = ControlledPawn->GetActorLocation();
	}

	SquadCoordinator->UpdateMemberState(State);
}

bool AAISquadController::RequestCoverFromCurrentThreat()
{
	UCoverPointSubsystem* CoverSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UCoverPointSubsystem>() : nullptr;
	APawn* ControlledPawn = GetPawn();
	if (!CoverSubsystem || !ControlledPawn)
	{
		return false;
	}

	FSquadEnemyKnowledge Threat;
	if (!TryGetPriorityEnemy(Threat))
	{
		return false;
	}

	FCoverPointData CoverPoint;
	if (!CoverSubsystem->FindCoverFromThreat(ControlledPawn->GetActorLocation(), Threat.LastKnownLocation, CoverSearchRadius, CoverPoint))
	{
		return false;
	}

	// Try to claim it through the squad coordinator so two bots can't
	// converge on the same spot.
	if (SquadCoordinator.IsValid() && !SquadCoordinator->ClaimCoverPoint(this, CoverPoint.Location))
	{
		return false;
	}

	ReleaseClaimedCover();
	ClaimedCoverLocation = CoverPoint.Location;
	bHasClaimedCover = true;

	MoveToLocation(CoverPoint.Location, /*AcceptanceRadius=*/50.f);
	return true;
}

void AAISquadController::ReleaseClaimedCover()
{
	if (bHasClaimedCover && SquadCoordinator.IsValid())
	{
		SquadCoordinator->ReleaseCoverPoint(this);
	}
	bHasClaimedCover = false;
}
