// =============================================================================
// IronvaleBehaviorTasks.cpp — Custom behavior tree tasks implementation
// Project Ironvale
// =============================================================================

#include "AI/IronvaleBehaviorTasks.h"
#include "Ironvale.h"
#include "AI/IronvaleAIController.h"
#include "AI/IronvaleScheduleComponent.h"
#include "Characters/IronvaleNPCCharacter.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h"

// =============================================================================
// BTTask_GoToScheduleLocation
// =============================================================================

UBTTask_GoToScheduleLocation::UBTTask_GoToScheduleLocation()
{
	NodeName = TEXT("Go To Schedule Location");
	bNotifyTick = false;
	bNotifyTaskFinished = false;
}

EBTNodeResult::Type UBTTask_GoToScheduleLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		UE_LOG(LogIronvale, Warning, TEXT("[BTTask_GoToScheduleLocation] No AI controller or pawn"));
		return EBTNodeResult::Failed;
	}

	// Read target location from blackboard
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Failed;
	}

	const FName LocationName = BB->GetValueAsName(AIronvaleAIController::BB_TargetLocation);
	if (LocationName.IsNone())
	{
		UE_LOG(LogIronvale, Verbose, TEXT("[BTTask_GoToScheduleLocation] %s has no target location — skipping"),
			*AIController->GetPawn()->GetName());
		return EBTNodeResult::Failed;
	}

	// Resolve location name to world position
	FVector TargetPosition;
	if (!ResolveLocationToWorldPosition(OwnerComp, LocationName, TargetPosition))
	{
		UE_LOG(LogIronvale, Warning, TEXT("[BTTask_GoToScheduleLocation] Could not resolve location '%s' for %s"),
			*LocationName.ToString(), *AIController->GetPawn()->GetName());
		return EBTNodeResult::Failed;
	}

	// Check if we're already close enough
	const float DistSq = FVector::DistSquared(AIController->GetPawn()->GetActorLocation(), TargetPosition);
	if (DistSq <= FMath::Square(AcceptanceRadius))
	{
		return EBTNodeResult::Succeeded;
	}

	// Issue move request
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(TargetPosition);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetAllowPartialPath(bAllowPartialPath);
	MoveRequest.SetUsePathfinding(true);

	const FPathFollowingRequestResult MoveResult = AIController->MoveTo(MoveRequest);

	switch (MoveResult.Code)
	{
	case EPathFollowingRequestResult::AlreadyAtGoal:
		return EBTNodeResult::Succeeded;

	case EPathFollowingRequestResult::RequestSuccessful:
		{
			// Bind completion callback
			AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
				this, &UBTTask_GoToScheduleLocation::HandleMoveCompleted, &OwnerComp);
			return EBTNodeResult::InProgress;
		}

	default:
		UE_LOG(LogIronvale, Warning, TEXT("[BTTask_GoToScheduleLocation] MoveTo failed for %s → '%s'"),
			*AIController->GetPawn()->GetName(), *LocationName.ToString());
		return EBTNodeResult::Failed;
	}
}

EBTNodeResult::Type UBTTask_GoToScheduleLocation::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->StopMovement();

		if (AIController->GetPathFollowingComponent())
		{
			AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
		}
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_GoToScheduleLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Move to schedule location (Radius=%.0f)"), AcceptanceRadius);
}

bool UBTTask_GoToScheduleLocation::ResolveLocationToWorldPosition(
	UBehaviorTreeComponent& OwnerComp, FName LocationName, FVector& OutPosition) const
{
	const UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		return false;
	}

	// Strategy 1: Find an actor with a matching tag
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->Tags.Contains(LocationName))
		{
			OutPosition = Actor->GetActorLocation();
			return true;
		}
	}

	// Strategy 2: Check if the NPC character has a named location mapping
	// (e.g. HomeLocationID, WorkLocationID on AIronvaleNPCCharacter)
	if (const AAIController* AIC = OwnerComp.GetAIOwner())
	{
		if (const AIronvaleNPCCharacter* NPC = Cast<AIronvaleNPCCharacter>(AIC->GetPawn()))
		{
			// Match against well-known location IDs on the NPC
			if (LocationName == NPC->HomeLocationID || LocationName == NPC->WorkLocationID)
			{
				// Fall through to tagged actor search (already done above)
				// This branch is for future extension (e.g. a location registry)
			}
		}
	}

	UE_LOG(LogIronvale, Warning, TEXT("[BTTask_GoToScheduleLocation] No actor found with tag '%s'"),
		*LocationName.ToString());
	return false;
}

void UBTTask_GoToScheduleLocation::HandleMoveCompleted(
	FAIRequestID RequestID, const FPathFollowingResult& Result, UBehaviorTreeComponent* OwnerComp)
{
	if (!OwnerComp)
	{
		return;
	}

	// Unbind
	if (AAIController* AIC = OwnerComp->GetAIOwner())
	{
		if (AIC->GetPathFollowingComponent())
		{
			AIC->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
		}
	}

	const bool bSuccess = Result.Code == EPathFollowingResult::Success;
	FinishLatentTask(*OwnerComp, bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}

// =============================================================================
// BTTask_PerformActivity
// =============================================================================

UBTTask_PerformActivity::UBTTask_PerformActivity()
{
	NodeName = TEXT("Perform Activity");
	bNotifyTick = true;
	bNotifyTaskFinished = false;
}

EBTNodeResult::Type UBTTask_PerformActivity::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTPerformActivityMemory* Memory = reinterpret_cast<FBTPerformActivityMemory*>(NodeMemory);
	Memory->ElapsedTime = 0.0f;
	Memory->bMontageStarted = false;

	const AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		return EBTNodeResult::Failed;
	}

	// Read the current activity from the blackboard
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Failed;
	}

	const EIronvaleActivity Activity = static_cast<EIronvaleActivity>(
		BB->GetValueAsInt(AIronvaleAIController::BB_CurrentActivity));

	// Try to play a matching montage
	if (UAnimMontage** MontagePtr = ActivityMontages.Find(Activity))
	{
		if (UAnimMontage* Montage = *MontagePtr)
		{
			if (ACharacter* Character = Cast<ACharacter>(AIController->GetPawn()))
			{
				if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
				{
					const float MontageLength = AnimInstance->Montage_Play(Montage);
					if (MontageLength > 0.0f)
					{
						Memory->bMontageStarted = true;
						UE_LOG(LogIronvale, Verbose,
							TEXT("[BTTask_PerformActivity] %s playing montage for activity %d (%.1fs)"),
							*AIController->GetPawn()->GetName(),
							static_cast<int32>(Activity), MontageLength);
					}
				}
			}
		}
	}

	// If duration is <= 0, run indefinitely (until aborted)
	if (Duration <= 0.0f)
	{
		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_PerformActivity::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTPerformActivityMemory* Memory = reinterpret_cast<FBTPerformActivityMemory*>(NodeMemory);

	if (Duration <= 0.0f)
	{
		// Indefinite duration — never self-complete
		return;
	}

	Memory->ElapsedTime += DeltaSeconds;

	if (Memory->ElapsedTime >= Duration)
	{
		// Stop montage if we started one
		if (Memory->bMontageStarted)
		{
			if (const AAIController* AIC = OwnerComp.GetAIOwner())
			{
				if (ACharacter* Character = Cast<ACharacter>(AIC->GetPawn()))
				{
					if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
					{
						AnimInstance->Montage_Stop(0.25f);
					}
				}
			}
		}

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_PerformActivity::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTPerformActivityMemory* Memory = reinterpret_cast<FBTPerformActivityMemory*>(NodeMemory);

	if (Memory->bMontageStarted)
	{
		if (const AAIController* AIC = OwnerComp.GetAIOwner())
		{
			if (ACharacter* Character = Cast<ACharacter>(AIC->GetPawn()))
			{
				if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
				{
					AnimInstance->Montage_Stop(0.25f);
				}
			}
		}
	}

	return EBTNodeResult::Aborted;
}

uint16 UBTTask_PerformActivity::GetInstanceMemorySize() const
{
	return sizeof(FBTPerformActivityMemory);
}

FString UBTTask_PerformActivity::GetStaticDescription() const
{
	return FString::Printf(TEXT("Perform activity for %.1f sec"), Duration);
}

// =============================================================================
// BTTask_InvestigateDisturbance
// =============================================================================

UBTTask_InvestigateDisturbance::UBTTask_InvestigateDisturbance()
{
	NodeName = TEXT("Investigate Disturbance");
	bNotifyTick = true;
	bNotifyTaskFinished = false;
}

EBTNodeResult::Type UBTTask_InvestigateDisturbance::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTInvestigateMemory* Memory = reinterpret_cast<FBTInvestigateMemory*>(NodeMemory);
	Memory->InvestigateTimer = 0.0f;
	Memory->bArrived = false;
	Memory->bMoving = false;

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		return EBTNodeResult::Failed;
	}

	// Get the last heard stimulus location from perception
	UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComponent();
	if (!PerceptionComp)
	{
		return EBTNodeResult::Failed;
	}

	// Find the most recent hearing stimulus
	FVector DisturbanceLocation = FVector::ZeroVector;
	bool bFoundHearing = false;
	float MostRecentAge = TNumericLimits<float>::Max();

	TArray<AActor*> PerceivedActors;
	PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), PerceivedActors);

	for (AActor* PerceivedActor : PerceivedActors)
	{
		if (!PerceivedActor)
		{
			continue;
		}

		FActorPerceptionBlueprintInfo Info;
		PerceptionComp->GetActorsPerception(PerceivedActor, Info);

		for (const FAIStimulus& Stimulus : Info.LastSensedStimuli)
		{
			if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
			{
				if (Stimulus.GetAge() < MostRecentAge)
				{
					MostRecentAge = Stimulus.GetAge();
					DisturbanceLocation = Stimulus.StimulusLocation;
					bFoundHearing = true;
				}
			}
		}
	}

	if (!bFoundHearing)
	{
		UE_LOG(LogIronvale, Verbose, TEXT("[BTTask_InvestigateDisturbance] %s has no hearing stimulus to investigate"),
			*AIController->GetPawn()->GetName());
		return EBTNodeResult::Failed;
	}

	// Move to the disturbance
	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(DisturbanceLocation);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);

	const FPathFollowingRequestResult MoveResult = AIController->MoveTo(MoveRequest);

	switch (MoveResult.Code)
	{
	case EPathFollowingRequestResult::AlreadyAtGoal:
		Memory->bArrived = true;
		return EBTNodeResult::InProgress; // Will tick through investigation phase

	case EPathFollowingRequestResult::RequestSuccessful:
		Memory->bMoving = true;
		AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
			this, &UBTTask_InvestigateDisturbance::HandleMoveCompleted, &OwnerComp);
		return EBTNodeResult::InProgress;

	default:
		return EBTNodeResult::Failed;
	}
}

void UBTTask_InvestigateDisturbance::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTInvestigateMemory* Memory = reinterpret_cast<FBTInvestigateMemory*>(NodeMemory);

	if (!Memory->bArrived)
	{
		return; // Still moving
	}

	// Investigation phase — look around for a while
	Memory->InvestigateTimer += DeltaSeconds;

	// Rotate the NPC slowly to look around
	if (const AAIController* AIC = OwnerComp.GetAIOwner())
	{
		if (APawn* Pawn = AIC->GetPawn())
		{
			const float RotationRate = 45.0f; // Degrees per second
			FRotator CurrentRot = Pawn->GetActorRotation();
			CurrentRot.Yaw += RotationRate * DeltaSeconds;
			Pawn->SetActorRotation(CurrentRot);
		}
	}

	if (Memory->InvestigateTimer >= InvestigateDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_InvestigateDisturbance::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTInvestigateMemory* Memory = reinterpret_cast<FBTInvestigateMemory*>(NodeMemory);

	if (Memory->bMoving)
	{
		if (AAIController* AIC = OwnerComp.GetAIOwner())
		{
			AIC->StopMovement();
			if (AIC->GetPathFollowingComponent())
			{
				AIC->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
			}
		}
	}

	return EBTNodeResult::Aborted;
}

uint16 UBTTask_InvestigateDisturbance::GetInstanceMemorySize() const
{
	return sizeof(FBTInvestigateMemory);
}

FString UBTTask_InvestigateDisturbance::GetStaticDescription() const
{
	return FString::Printf(TEXT("Investigate disturbance (%.1fs)"), InvestigateDuration);
}

void UBTTask_InvestigateDisturbance::HandleMoveCompleted(
	FAIRequestID RequestID, const FPathFollowingResult& Result, UBehaviorTreeComponent* OwnerComp)
{
	if (!OwnerComp)
	{
		return;
	}

	if (AAIController* AIC = OwnerComp->GetAIOwner())
	{
		if (AIC->GetPathFollowingComponent())
		{
			AIC->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
		}
	}

	// Access node memory through the behavior tree component
	// We need to find the active task memory. Since we're the active task,
	// we can mark arrival and let TickTask handle the investigation phase.
	uint8* RawMemory = OwnerComp->GetNodeMemory(this, OwnerComp->FindInstanceContainingNode(this));
	if (RawMemory)
	{
		FBTInvestigateMemory* Memory = reinterpret_cast<FBTInvestigateMemory*>(RawMemory);
		Memory->bMoving = false;

		if (Result.Code == EPathFollowingResult::Success)
		{
			Memory->bArrived = true;
		}
		else
		{
			FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
		}
	}
}

// =============================================================================
// BTTask_PatrolRoute
// =============================================================================

UBTTask_PatrolRoute::UBTTask_PatrolRoute()
{
	NodeName = TEXT("Patrol Route");
	bNotifyTick = true;
	bNotifyTaskFinished = false;
}

EBTNodeResult::Type UBTTask_PatrolRoute::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTPatrolMemory* Memory = reinterpret_cast<FBTPatrolMemory*>(NodeMemory);
	Memory->WaitTimer = 0.0f;
	Memory->bWaiting = false;
	Memory->bMoving = false;

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController || !AIController->GetPawn())
	{
		return EBTNodeResult::Failed;
	}

	// Cache waypoints on first execution
	if (!Memory->bWaypointsCached)
	{
		Memory->WaypointLocations.Empty();
		Memory->CurrentWaypointIndex = 0;

		const UWorld* World = OwnerComp.GetWorld();
		if (!World)
		{
			return EBTNodeResult::Failed;
		}

		// Collect waypoint actors matching the patrol tag
		TArray<AActor*> WaypointActors;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->Tags.Contains(PatrolRouteTag))
			{
				WaypointActors.Add(Actor);
			}
		}

		if (WaypointActors.Num() == 0)
		{
			UE_LOG(LogIronvale, Warning, TEXT("[BTTask_PatrolRoute] No waypoints found with tag '%s' for %s"),
				*PatrolRouteTag.ToString(), *AIController->GetPawn()->GetName());
			return EBTNodeResult::Failed;
		}

		// Sort by name for deterministic ordering
		WaypointActors.Sort([](const AActor& A, const AActor& B)
		{
			return A.GetName() < B.GetName();
		});

		for (const AActor* WP : WaypointActors)
		{
			Memory->WaypointLocations.Add(WP->GetActorLocation());
		}

		Memory->bWaypointsCached = true;

		UE_LOG(LogIronvale, Log, TEXT("[BTTask_PatrolRoute] %s cached %d patrol waypoints (tag='%s')"),
			*AIController->GetPawn()->GetName(),
			Memory->WaypointLocations.Num(),
			*PatrolRouteTag.ToString());
	}

	if (Memory->WaypointLocations.Num() == 0)
	{
		return EBTNodeResult::Failed;
	}

	// Start moving to current waypoint
	const FVector& Target = Memory->WaypointLocations[Memory->CurrentWaypointIndex];

	FAIMoveRequest MoveRequest;
	MoveRequest.SetGoalLocation(Target);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
	MoveRequest.SetUsePathfinding(true);

	const FPathFollowingRequestResult MoveResult = AIController->MoveTo(MoveRequest);

	switch (MoveResult.Code)
	{
	case EPathFollowingRequestResult::AlreadyAtGoal:
		Memory->bWaiting = true;
		return EBTNodeResult::InProgress;

	case EPathFollowingRequestResult::RequestSuccessful:
		Memory->bMoving = true;
		AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
			this, &UBTTask_PatrolRoute::HandleMoveCompleted, &OwnerComp);
		return EBTNodeResult::InProgress;

	default:
		return EBTNodeResult::Failed;
	}
}

void UBTTask_PatrolRoute::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTPatrolMemory* Memory = reinterpret_cast<FBTPatrolMemory*>(NodeMemory);

	if (!Memory->bWaiting)
	{
		return; // Still moving to current waypoint
	}

	Memory->WaitTimer += DeltaSeconds;

	if (Memory->WaitTimer >= WaitTimeAtWaypoint)
	{
		// Advance to next waypoint
		Memory->CurrentWaypointIndex =
			(Memory->CurrentWaypointIndex + 1) % Memory->WaypointLocations.Num();
		Memory->WaitTimer = 0.0f;
		Memory->bWaiting = false;

		// Issue new move request
		AAIController* AIController = OwnerComp.GetAIOwner();
		if (!AIController)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		const FVector& Target = Memory->WaypointLocations[Memory->CurrentWaypointIndex];

		FAIMoveRequest MoveRequest;
		MoveRequest.SetGoalLocation(Target);
		MoveRequest.SetAcceptanceRadius(AcceptanceRadius);
		MoveRequest.SetUsePathfinding(true);

		const FPathFollowingRequestResult MoveResult = AIController->MoveTo(MoveRequest);

		switch (MoveResult.Code)
		{
		case EPathFollowingRequestResult::AlreadyAtGoal:
			Memory->bWaiting = true;
			break;

		case EPathFollowingRequestResult::RequestSuccessful:
			Memory->bMoving = true;
			AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(
				this, &UBTTask_PatrolRoute::HandleMoveCompleted, &OwnerComp);
			break;

		default:
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			break;
		}
	}
}

EBTNodeResult::Type UBTTask_PatrolRoute::AbortTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTPatrolMemory* Memory = reinterpret_cast<FBTPatrolMemory*>(NodeMemory);

	if (Memory->bMoving)
	{
		if (AAIController* AIC = OwnerComp.GetAIOwner())
		{
			AIC->StopMovement();
			if (AIC->GetPathFollowingComponent())
			{
				AIC->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
			}
		}
	}

	return EBTNodeResult::Aborted;
}

uint16 UBTTask_PatrolRoute::GetInstanceMemorySize() const
{
	return sizeof(FBTPatrolMemory);
}

FString UBTTask_PatrolRoute::GetStaticDescription() const
{
	return FString::Printf(TEXT("Patrol route (tag='%s', wait=%.1fs)"),
		*PatrolRouteTag.ToString(), WaitTimeAtWaypoint);
}

void UBTTask_PatrolRoute::HandleMoveCompleted(
	FAIRequestID RequestID, const FPathFollowingResult& Result, UBehaviorTreeComponent* OwnerComp)
{
	if (!OwnerComp)
	{
		return;
	}

	if (AAIController* AIC = OwnerComp->GetAIOwner())
	{
		if (AIC->GetPathFollowingComponent())
		{
			AIC->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
		}
	}

	uint8* RawMemory = OwnerComp->GetNodeMemory(this, OwnerComp->FindInstanceContainingNode(this));
	if (RawMemory)
	{
		FBTPatrolMemory* Memory = reinterpret_cast<FBTPatrolMemory*>(RawMemory);
		Memory->bMoving = false;

		if (Result.Code == EPathFollowingResult::Success)
		{
			Memory->bWaiting = true;
			Memory->WaitTimer = 0.0f;
		}
		else
		{
			FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
		}
	}
}
