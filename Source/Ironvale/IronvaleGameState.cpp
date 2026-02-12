// =============================================================================
// IronvaleGameState.cpp — Game state implementation
// Project Ironvale
// =============================================================================

#include "IronvaleGameState.h"
#include "Ironvale.h"
#include "Core/IronvaleStatics.h"
#include "Core/IronvaleEventBus.h"

AIronvaleGameState::AIronvaleGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f; // Every frame for smooth time
}

void AIronvaleGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bTimePaused) return;

	// Advance game clock
	const float PreviousTime = GameTimeHours;
	GameTimeHours += TimeScale * DeltaSeconds;

	// Day wrap
	if (GameTimeHours >= 24.0f)
	{
		GameTimeHours -= 24.0f;
		DayCount++;
		UE_LOG(LogIronvale, Log, TEXT("New day: Day %d"), DayCount);
	}

	// Detect hour boundary crossings
	const int32 CurrentHour = FMath::FloorToInt32(GameTimeHours);
	if (CurrentHour != PreviousHour)
	{
		PreviousHour = CurrentHour;
		BroadcastHourChanged(CurrentHour);

		// Check for time-of-day transition
		const EIronvaleTimeOfDay NewTOD = UIronvaleStatics::HourToTimeOfDay(GameTimeHours);
		if (NewTOD != CurrentTimeOfDay)
		{
			CurrentTimeOfDay = NewTOD;
			BroadcastTimeOfDayChanged(NewTOD);
		}
	}
}

void AIronvaleGameState::AdvanceTime(float Hours)
{
	if (Hours <= 0.0f) return;

	const float OldTime = GameTimeHours;
	GameTimeHours += Hours;

	// Handle multi-day advances
	while (GameTimeHours >= 24.0f)
	{
		GameTimeHours -= 24.0f;
		DayCount++;
	}

	PreviousHour = FMath::FloorToInt32(GameTimeHours);
	CurrentTimeOfDay = UIronvaleStatics::HourToTimeOfDay(GameTimeHours);

	// Broadcast current state after jump
	BroadcastHourChanged(PreviousHour);
	BroadcastTimeOfDayChanged(CurrentTimeOfDay);

	UE_LOG(LogIronvale, Log, TEXT("Time advanced by %.1f hours. Now: %s (Day %d)"),
		Hours, *UIronvaleStatics::GameHourToTimeString(GameTimeHours), DayCount);
}

void AIronvaleGameState::SetWeather(EIronvaleWeatherState NewWeather)
{
	if (CurrentWeather == NewWeather) return;

	CurrentWeather = NewWeather;

	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnWeatherChanged.Broadcast(NewWeather);
	}
}

void AIronvaleGameState::SetGameTimeFromSave(float Hours, int32 Day)
{
	GameTimeHours = FMath::Fmod(Hours, 24.0f);
	DayCount = FMath::Max(1, Day);
	PreviousHour = FMath::FloorToInt32(GameTimeHours);
	CurrentTimeOfDay = UIronvaleStatics::HourToTimeOfDay(GameTimeHours);
}

void AIronvaleGameState::BroadcastHourChanged(int32 NewHour)
{
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnHourChanged.Broadcast(NewHour);
	}
}

void AIronvaleGameState::BroadcastTimeOfDayChanged(EIronvaleTimeOfDay NewTOD)
{
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnTimeOfDayChanged.Broadcast(NewTOD);
	}

	UE_LOG(LogIronvale, Log, TEXT("Time of day changed to: %d"), static_cast<int32>(NewTOD));
}
