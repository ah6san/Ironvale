// =============================================================================
// IronvaleAIController.cpp — NPC AI controller implementation
// Project Ironvale
// =============================================================================

#include "AI/IronvaleAIController.h"
#include "Ironvale.h"
#include "AI/IronvaleScheduleComponent.h"
#include "AI/IronvaleCrimeResponseComponent.h"
#include "Characters/IronvaleNPCCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"

// =============================================================================
// Static blackboard key names
// =============================================================================

const FName AIronvaleAIController::BB_CurrentActivity     = TEXT("CurrentActivity");
const FName AIronvaleAIController::BB_TargetLocation      = TEXT("TargetLocation");
const FName AIronvaleAIController::BB_ThreatActor         = TEXT("ThreatActor");
const FName AIronvaleAIController::BB_CrimeWitnessed      = TEXT("CrimeWitnessed");
const FName AIronvaleAIController::BB_QuestOverrideActive = TEXT("QuestOverrideActive");

// =============================================================================
// Constructor
// =============================================================================

AIronvaleAIController::AIronvaleAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f; // 4 Hz is plenty for NPC AI

	// Create perception component
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerceptionComp);

	// Sense configs are created in ConfigurePerception (called in constructor body)
	ConfigurePerception();
}

// =============================================================================
// Possession lifecycle
// =============================================================================

void AIronvaleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn)
	{
		return;
	}

	UE_LOG(LogIronvale, Log, TEXT("[AIController] Possessed %s"), *InPawn->GetName());

	// Cache the schedule component
	CachedScheduleComp = InPawn->FindComponentByClass<UIronvaleScheduleComponent>();

	// Subscribe to schedule changes
	if (CachedScheduleComp)
	{
		CachedScheduleComp->OnScheduleEntryChanged.AddDynamic(
			this, &AIronvaleAIController::HandleScheduleEntryChanged);
	}

	// Bind perception callback
	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
			this, &AIronvaleAIController::HandlePerceptionUpdated);
	}

	// Start behavior tree
	StartBehaviorTreeForPawn(InPawn);
}

void AIronvaleAIController::OnUnPossess()
{
	// Unbind schedule delegate
	if (CachedScheduleComp)
	{
		CachedScheduleComp->OnScheduleEntryChanged.RemoveDynamic(
			this, &AIronvaleAIController::HandleScheduleEntryChanged);
		CachedScheduleComp = nullptr;
	}

	// Unbind perception delegate
	if (AIPerceptionComp)
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(
			this, &AIronvaleAIController::HandlePerceptionUpdated);
	}

	// Stop the behavior tree
	if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(GetBrainComponent()))
	{
		BTComp->StopTree(EBTStopMode::Safe);
	}

	Super::OnUnPossess();
}

// =============================================================================
// Tick
// =============================================================================

void AIronvaleAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Periodic blackboard sync — catches edge cases where perception or
	// schedule state drifts without an explicit event.
	UpdateBlackboardFromSchedule();
	UpdateBlackboardFromPerception();
}

// =============================================================================
// Blackboard helpers
// =============================================================================

void AIronvaleAIController::UpdateBlackboardFromSchedule()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB || !CachedScheduleComp)
	{
		return;
	}

	BB->SetValueAsInt(BB_CurrentActivity, static_cast<int32>(CachedScheduleComp->GetCurrentActivity()));
	BB->SetValueAsName(BB_TargetLocation, CachedScheduleComp->GetCurrentTargetLocation());
	BB->SetValueAsBool(BB_QuestOverrideActive, CachedScheduleComp->HasActiveOverride());
}

void AIronvaleAIController::UpdateBlackboardFromPerception()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB || !AIPerceptionComp)
	{
		return;
	}

	// Find the nearest currently-perceived hostile actor
	AActor* NearestThreat = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	TArray<AActor*> PerceivedActors;
	AIPerceptionComp->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	const APawn* ControlledPawn = GetPawn();
	const AIronvaleNPCCharacter* NPC = ControlledPawn ? Cast<AIronvaleNPCCharacter>(ControlledPawn) : nullptr;

	for (AActor* PerceivedActor : PerceivedActors)
	{
		if (!PerceivedActor || PerceivedActor == ControlledPawn)
		{
			continue;
		}

		// Check if this NPC considers the perceived actor hostile
		const bool bIsHostile = NPC ? NPC->IsHostileToward(PerceivedActor) : false;
		if (!bIsHostile)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(ControlledPawn->GetActorLocation(),
			PerceivedActor->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestThreat = PerceivedActor;
		}
	}

	BB->SetValueAsObject(BB_ThreatActor, NearestThreat);

	// Update crime witnessed flag from the crime response component
	if (NPC)
	{
		if (const UIronvaleCrimeResponseComponent* CrimeComp = NPC->GetCrimeResponseComponent())
		{
			BB->SetValueAsBool(BB_CrimeWitnessed, CrimeComp->GetHasWitnessedCrime());
		}
	}
}

// =============================================================================
// Perception callback
// =============================================================================

void AIronvaleAIController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
	{
		return;
	}

	// Immediate blackboard update on perception change so the BT can react
	// within the same frame
	UpdateBlackboardFromPerception();

	UE_LOG(LogIronvale, Verbose, TEXT("[AIController] %s perceived %s (Sense=%s, Active=%s)"),
		*GetPawn()->GetName(),
		*Actor->GetName(),
		*Stimulus.Type.Name.ToString(),
		Stimulus.WasSuccessfullySensed() ? TEXT("Yes") : TEXT("No"));
}

// =============================================================================
// Schedule callback
// =============================================================================

void AIronvaleAIController::HandleScheduleEntryChanged(EIronvaleActivity NewActivity, FName NewLocation)
{
	UpdateBlackboardFromSchedule();

	UE_LOG(LogIronvale, Verbose, TEXT("[AIController] %s schedule changed: Activity=%d, Location=%s"),
		GetPawn() ? *GetPawn()->GetName() : TEXT("(null)"),
		static_cast<int32>(NewActivity),
		*NewLocation.ToString());
}

// =============================================================================
// Internal setup
// =============================================================================

void AIronvaleAIController::ConfigurePerception()
{
	if (!AIPerceptionComp)
	{
		return;
	}

	// Sight
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRange;
	SightConfig->LoseSightRadius = SightRange * 1.2f;
	SightConfig->PeripheralVisionAngleDegrees = SightPeripheralAngle;
	SightConfig->SetMaxAge(SightMaxAge);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 500.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	AIPerceptionComp->ConfigureSense(*SightConfig);

	// Hearing
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.0f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
	AIPerceptionComp->ConfigureSense(*HearingConfig);

	// Set sight as the dominant sense
	AIPerceptionComp->SetDominantSense(UAISenseConfig_Sight::StaticClass());
}

void AIronvaleAIController::StartBehaviorTreeForPawn(APawn* InPawn)
{
	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogIronvale, Warning, TEXT("[AIController] No BehaviorTree assigned for %s — AI will idle"),
			*InPawn->GetName());
		return;
	}

	// Use the blackboard embedded in the behavior tree if no explicit asset is set
	UBlackboardData* BBAsset = BlackboardAsset;
	if (!BBAsset && BehaviorTreeAsset->BlackboardAsset)
	{
		BBAsset = BehaviorTreeAsset->BlackboardAsset;
	}

	if (!BBAsset)
	{
		UE_LOG(LogIronvale, Warning, TEXT("[AIController] No Blackboard assigned for %s — cannot start BT"),
			*InPawn->GetName());
		return;
	}

	// Initialize blackboard
	UseBlackboard(BBAsset, Blackboard);

	if (!Blackboard)
	{
		UE_LOG(LogIronvale, Error, TEXT("[AIController] Failed to initialize blackboard for %s"),
			*InPawn->GetName());
		return;
	}

	// Seed initial values
	UpdateBlackboardFromSchedule();

	// Run the tree
	const bool bStarted = RunBehaviorTree(BehaviorTreeAsset);

	UE_LOG(LogIronvale, Log, TEXT("[AIController] BehaviorTree %s for %s: %s"),
		*BehaviorTreeAsset->GetName(),
		*InPawn->GetName(),
		bStarted ? TEXT("STARTED") : TEXT("FAILED"));
}
