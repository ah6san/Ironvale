// =============================================================================
// IronvaleBehaviorTasks.h — Custom behavior tree task nodes for NPC AI
// Project Ironvale
//
// Task nodes:
//   BTTask_GoToScheduleLocation  — Move NPC to current schedule location
//   BTTask_PerformActivity       — Play activity animation for duration
//   BTTask_InvestigateDisturbance — Guard: move to last-heard disturbance
//   BTTask_PatrolRoute           — Guard: follow patrol waypoints in sequence
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "IronvaleBehaviorTasks.generated.h"

class AIronvaleAIController;
class UIronvaleScheduleComponent;

// =============================================================================
// BTTask_GoToScheduleLocation
// =============================================================================

/**
 * Moves the NPC to the location specified by their current schedule entry.
 *
 * Reads the TargetLocation blackboard key (FName), resolves it to a world
 * position via tagged actors, and issues a MoveTo request. Succeeds when
 * the NPC reaches the acceptance radius, fails on path failure.
 */
UCLASS()
class IRONVALE_API UBTTask_GoToScheduleLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_GoToScheduleLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

protected:
	/** Acceptance radius for reaching the destination (cm) */
	UPROPERTY(EditAnywhere, Category = "Ironvale", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 100.0f;

	/** Whether to allow partial paths when the full path is blocked */
	UPROPERTY(EditAnywhere, Category = "Ironvale")
	bool bAllowPartialPath = true;

	/**
	 * Resolve a location FName to a world position.
	 * Searches for actors with a matching tag in the current level.
	 */
	bool ResolveLocationToWorldPosition(UBehaviorTreeComponent& OwnerComp, FName LocationName, FVector& OutPosition) const;

	/** Callback when the move request completes */
	void HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result, UBehaviorTreeComponent* OwnerComp);
};

// =============================================================================
// BTTask_PerformActivity
// =============================================================================

/**
 * Has the NPC perform their current activity for a specified duration.
 *
 * Plays an optional animation montage associated with the activity type
 * and waits for the duration to elapse. If no montage is set, the NPC
 * simply idles in place.
 */
UCLASS()
class IRONVALE_API UBTTask_PerformActivity : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PerformActivity();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;

	virtual FString GetStaticDescription() const override;

protected:
	/**
	 * How long (in seconds, real time) to perform the activity before
	 * succeeding. If <= 0, the task runs until aborted.
	 */
	UPROPERTY(EditAnywhere, Category = "Ironvale", meta = (ClampMin = "0.0"))
	float Duration = 10.0f;

	/** Map from activity type to animation montage. Designers fill this in. */
	UPROPERTY(EditAnywhere, Category = "Ironvale")
	TMap<EIronvaleActivity, UAnimMontage*> ActivityMontages;
};

/** Instance memory for BTTask_PerformActivity */
struct FBTPerformActivityMemory
{
	float ElapsedTime = 0.0f;
	bool bMontageStarted = false;
};

// =============================================================================
// BTTask_InvestigateDisturbance
// =============================================================================

/**
 * Guard behavior: move to the location of a perceived disturbance (hearing
 * stimulus) and look around.
 *
 * Reads the last hearing stimulus position from the perception component
 * and issues a MoveTo. On arrival, the NPC pauses briefly then succeeds.
 */
UCLASS()
class IRONVALE_API UBTTask_InvestigateDisturbance : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_InvestigateDisturbance();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;

	virtual FString GetStaticDescription() const override;

protected:
	/** How long to investigate at the disturbance location (seconds) */
	UPROPERTY(EditAnywhere, Category = "Ironvale", meta = (ClampMin = "0.0"))
	float InvestigateDuration = 5.0f;

	/** Acceptance radius for reaching disturbance location (cm) */
	UPROPERTY(EditAnywhere, Category = "Ironvale", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 150.0f;

	void HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result, UBehaviorTreeComponent* OwnerComp);
};

/** Instance memory for BTTask_InvestigateDisturbance */
struct FBTInvestigateMemory
{
	float InvestigateTimer = 0.0f;
	bool bArrived = false;
	bool bMoving = false;
};

// =============================================================================
// BTTask_PatrolRoute
// =============================================================================

/**
 * Guard behavior: follow a patrol route defined by tagged waypoints.
 *
 * Waypoints are actors in the level tagged with the PatrolRouteTag. The
 * guard visits them in order, looping back to the first after the last.
 * Each waypoint may have an optional wait time.
 */
UCLASS()
class IRONVALE_API UBTTask_PatrolRoute : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_PatrolRoute();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;

	virtual FString GetStaticDescription() const override;

protected:
	/**
	 * Actor tag used to find patrol waypoints in the level.
	 * Waypoints are sorted by name for deterministic ordering.
	 */
	UPROPERTY(EditAnywhere, Category = "Ironvale")
	FName PatrolRouteTag = TEXT("PatrolPoint");

	/** Acceptance radius for reaching a waypoint (cm) */
	UPROPERTY(EditAnywhere, Category = "Ironvale", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 100.0f;

	/** How long to wait at each waypoint before moving to the next (seconds) */
	UPROPERTY(EditAnywhere, Category = "Ironvale", meta = (ClampMin = "0.0"))
	float WaitTimeAtWaypoint = 3.0f;

	void HandleMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result, UBehaviorTreeComponent* OwnerComp);
};

/** Instance memory for BTTask_PatrolRoute */
struct FBTPatrolMemory
{
	int32 CurrentWaypointIndex = 0;
	float WaitTimer = 0.0f;
	bool bWaiting = false;
	bool bMoving = false;
	TArray<FVector> WaypointLocations;
	bool bWaypointsCached = false;
};
