// =============================================================================
// IronvaleBehaviorDecorators.h — Custom behavior tree decorator nodes
// Project Ironvale
//
// Decorators:
//   BTDecorator_IsTimeInRange   — Gate: current game hour within [Min, Max]
//   BTDecorator_HasScheduleTask — Gate: schedule component has a valid task
//   BTDecorator_IsWeatherState  — Gate: current weather matches specified state
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleBehaviorDecorators.generated.h"

// =============================================================================
// BTDecorator_IsTimeInRange
// =============================================================================

/**
 * Decorator that checks whether the current game hour falls within a
 * specified range. Supports overnight ranges (e.g. 22:00 – 06:00).
 *
 * Uses the bInverseCondition property (from base class) to negate the check.
 */
UCLASS()
class IRONVALE_API UBTDecorator_IsTimeInRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IsTimeInRange();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

	/** Tick mode so the decorator re-evaluates as time passes */
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/** Start of the valid hour range (0.0 – 24.0) */
	UPROPERTY(EditAnywhere, Category = "Ironvale|Time",
		meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float MinHour = 6.0f;

	/** End of the valid hour range (0.0 – 24.0) */
	UPROPERTY(EditAnywhere, Category = "Ironvale|Time",
		meta = (ClampMin = "0.0", ClampMax = "24.0"))
	float MaxHour = 22.0f;

	/** How often (seconds) to re-check the time condition */
	UPROPERTY(EditAnywhere, Category = "Ironvale|Time",
		meta = (ClampMin = "0.1"))
	float RecheckInterval = 5.0f;

	/** Accumulator for recheck timing */
	mutable float TimeSinceLastCheck = 0.0f;
};

// =============================================================================
// BTDecorator_HasScheduleTask
// =============================================================================

/**
 * Decorator that passes if the NPC's ScheduleComponent currently resolves
 * to a valid schedule entry (i.e. has something to do right now).
 */
UCLASS()
class IRONVALE_API UBTDecorator_HasScheduleTask : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HasScheduleTask();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
};

// =============================================================================
// BTDecorator_IsWeatherState
// =============================================================================

/**
 * Decorator that passes if the current weather matches the specified state.
 * Useful for conditional branches (e.g. guards seek shelter in storms).
 */
UCLASS()
class IRONVALE_API UBTDecorator_IsWeatherState : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IsWeatherState();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

protected:
	/** The weather state(s) that satisfy this condition */
	UPROPERTY(EditAnywhere, Category = "Ironvale|Weather")
	TArray<EIronvaleWeatherState> AcceptedWeatherStates;
};
