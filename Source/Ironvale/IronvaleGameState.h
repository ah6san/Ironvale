// =============================================================================
// IronvaleGameState.h — Game state accessible by all actors in the world
// Project Ironvale
//
// Responsibilities:
//   - Exposes game-time clock (hours, day count) to all systems
//   - Tracks current weather state
//   - Provides helper queries for AI, dialogue, and gameplay systems
//   - Multiplayer-safe: replicated state lives here
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleGameState.generated.h"

UCLASS()
class IRONVALE_API AIronvaleGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AIronvaleGameState();

	virtual void Tick(float DeltaSeconds) override;

	// =========================================================================
	// GAME TIME
	// =========================================================================

	/** Current game time as hours (0.0 - 24.0, wraps daily) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Time")
	float GetGameTimeHours() const { return GameTimeHours; }

	/** Current day count (starts at 1) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Time")
	int32 GetDayCount() const { return DayCount; }

	/** Current time-of-day period */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Time")
	EIronvaleTimeOfDay GetCurrentTimeOfDay() const { return CurrentTimeOfDay; }

	/** Advance game time by a number of hours (e.g., sleeping, waiting) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Time")
	void AdvanceTime(float Hours);

	/** Set the time scale (real seconds per game hour) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Time")
	void SetTimeScale(float NewScale) { TimeScale = FMath::Max(0.0f, NewScale); }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Time")
	float GetTimeScale() const { return TimeScale; }

	/** Pause/resume the game clock (for menus, dialogue, cutscenes) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Time")
	void SetTimePaused(bool bPaused) { bTimePaused = bPaused; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Time")
	bool IsTimePaused() const { return bTimePaused; }

	// =========================================================================
	// WEATHER
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Weather")
	EIronvaleWeatherState GetCurrentWeather() const { return CurrentWeather; }

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Weather")
	void SetWeather(EIronvaleWeatherState NewWeather);

	// =========================================================================
	// SAVE/LOAD HELPERS
	// =========================================================================

	void SetGameTimeFromSave(float Hours, int32 Day);

protected:
	/** Current game clock in hours (0-24) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Time")
	float GameTimeHours = 8.0f;

	/** Day counter */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Time")
	int32 DayCount = 1;

	/** Cached time-of-day enum, updated when hour changes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Time")
	EIronvaleTimeOfDay CurrentTimeOfDay = EIronvaleTimeOfDay::Morning;

	/** Previous hour (integer), for detecting hour boundaries */
	int32 PreviousHour = 8;

	/** Game-hours per real-second. Default: 1 real hour = 1 game day → 24/3600 ≈ 0.00667 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Time")
	float TimeScale = IronvaleConstants::DEFAULT_TIME_SCALE / 3600.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Time")
	bool bTimePaused = false;

	/** Current weather */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Weather")
	EIronvaleWeatherState CurrentWeather = EIronvaleWeatherState::Clear;

	void BroadcastHourChanged(int32 NewHour);
	void BroadcastTimeOfDayChanged(EIronvaleTimeOfDay NewTOD);
};
