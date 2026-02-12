// =============================================================================
// IronvaleBehaviorDecorators.cpp — Custom behavior tree decorators implementation
// Project Ironvale
// =============================================================================

#include "AI/IronvaleBehaviorDecorators.h"
#include "Ironvale.h"
#include "IronvaleGameState.h"
#include "AI/IronvaleScheduleComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIController.h"

// =============================================================================
// BTDecorator_IsTimeInRange
// =============================================================================

UBTDecorator_IsTimeInRange::UBTDecorator_IsTimeInRange()
{
	NodeName = TEXT("Is Time In Range");
	bNotifyTick = true;

	// Observer-based abort so the tree reacts when the condition changes
	FlowAbortMode = EBTFlowAbortMode::Self;
}

bool UBTDecorator_IsTimeInRange::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		return false;
	}

	const AIronvaleGameState* GS = Cast<AIronvaleGameState>(World->GetGameState());
	if (!GS)
	{
		return false;
	}

	const float CurrentHour = GS->GetGameTimeHours();

	if (MinHour <= MaxHour)
	{
		// Normal range: e.g. 6:00 – 22:00
		return CurrentHour >= MinHour && CurrentHour < MaxHour;
	}
	else
	{
		// Overnight range: e.g. 22:00 – 6:00
		return CurrentHour >= MinHour || CurrentHour < MaxHour;
	}
}

void UBTDecorator_IsTimeInRange::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// Periodically re-check the condition. This avoids checking every frame
	// while still catching hour-boundary transitions in a timely manner.
	TimeSinceLastCheck += DeltaSeconds;
	if (TimeSinceLastCheck >= RecheckInterval)
	{
		TimeSinceLastCheck = 0.0f;
		// Request the decorator to re-evaluate its condition
		ConditionalFlowAbort(OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
	}
}

FString UBTDecorator_IsTimeInRange::GetStaticDescription() const
{
	const int32 MinH = FMath::FloorToInt32(MinHour);
	const int32 MinM = FMath::FloorToInt32((MinHour - MinH) * 60.0f);
	const int32 MaxH = FMath::FloorToInt32(MaxHour);
	const int32 MaxM = FMath::FloorToInt32((MaxHour - MaxH) * 60.0f);

	return FString::Printf(TEXT("Time in range [%02d:%02d – %02d:%02d]"),
		MinH, MinM, MaxH, MaxM);
}

// =============================================================================
// BTDecorator_HasScheduleTask
// =============================================================================

UBTDecorator_HasScheduleTask::UBTDecorator_HasScheduleTask()
{
	NodeName = TEXT("Has Schedule Task");
	FlowAbortMode = EBTFlowAbortMode::None;
}

bool UBTDecorator_HasScheduleTask::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC || !AIC->GetPawn())
	{
		return false;
	}

	const UIronvaleScheduleComponent* ScheduleComp =
		AIC->GetPawn()->FindComponentByClass<UIronvaleScheduleComponent>();
	if (!ScheduleComp)
	{
		return false;
	}

	FIronvaleScheduleEntry Entry;
	return ScheduleComp->GetCurrentScheduleEntry(Entry);
}

FString UBTDecorator_HasScheduleTask::GetStaticDescription() const
{
	return TEXT("Has active schedule task");
}

// =============================================================================
// BTDecorator_IsWeatherState
// =============================================================================

UBTDecorator_IsWeatherState::UBTDecorator_IsWeatherState()
{
	NodeName = TEXT("Is Weather State");
	FlowAbortMode = EBTFlowAbortMode::None;
}

bool UBTDecorator_IsWeatherState::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		return false;
	}

	const AIronvaleGameState* GS = Cast<AIronvaleGameState>(World->GetGameState());
	if (!GS)
	{
		return false;
	}

	const EIronvaleWeatherState CurrentWeather = GS->GetCurrentWeather();

	// Match against any of the accepted states
	return AcceptedWeatherStates.Contains(CurrentWeather);
}

FString UBTDecorator_IsWeatherState::GetStaticDescription() const
{
	if (AcceptedWeatherStates.Num() == 0)
	{
		return TEXT("Weather: (none configured)");
	}

	FString StateList;
	for (int32 i = 0; i < AcceptedWeatherStates.Num(); ++i)
	{
		if (i > 0)
		{
			StateList += TEXT(", ");
		}

		switch (AcceptedWeatherStates[i])
		{
		case EIronvaleWeatherState::Clear:  StateList += TEXT("Clear");  break;
		case EIronvaleWeatherState::Cloudy: StateList += TEXT("Cloudy"); break;
		case EIronvaleWeatherState::Rain:   StateList += TEXT("Rain");   break;
		case EIronvaleWeatherState::Storm:  StateList += TEXT("Storm");  break;
		case EIronvaleWeatherState::Fog:    StateList += TEXT("Fog");    break;
		case EIronvaleWeatherState::Snow:   StateList += TEXT("Snow");   break;
		default:                            StateList += TEXT("Unknown"); break;
		}
	}

	return FString::Printf(TEXT("Weather is [%s]"), *StateList);
}
