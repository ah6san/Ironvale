// =============================================================================
// IronvaleWeatherManager.h — Weather state machine with probabilistic transitions
// Project Ironvale: First-person grounded medieval RPG
//
// UWorldSubsystem that manages weather transitions over game time.
// State machine: Clear <-> Cloudy <-> Rain -> Storm, with Fog as a separate axis.
// Broadcasts weather changes through the EventBus for audio, visual, and AI systems.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Containers/Ticker.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleWeatherManager.generated.h"

class UIronvaleEventBus;
class AIronvaleGameState;

/**
 * Weather subsystem — manages the weather state machine and transitions.
 *
 * Each weather state has a random duration (between Min/MaxWeatherDuration).
 * When a state expires, the system rolls weighted dice to pick the next state
 * based on valid transitions:
 *
 *   Clear <-> Cloudy <-> Rain -> Storm (storm always returns to Cloudy or Clear)
 *   Fog is a separate overlay that can combine with any precipitation state.
 *
 * The system counts down in game-hours. When the current weather expires, it
 * smoothly transitions over TransitionDuration before settling into the new state.
 *
 * Updates AIronvaleGameState::SetWeather() and broadcasts via EventBus.
 */
UCLASS()
class IRONVALE_API UIronvaleWeatherSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// =========================================================================
	// SUBSYSTEM LIFECYCLE
	// =========================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Called each frame to advance the weather state machine */
	void Tick(float DeltaSeconds);

	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** Minimum duration of a weather state in game-hours */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Weather", meta = (ClampMin = "0.5"))
	float MinWeatherDuration = 2.0f;

	/** Maximum duration of a weather state in game-hours */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Weather", meta = (ClampMin = "1.0"))
	float MaxWeatherDuration = 8.0f;

	/** Duration of transitions between weather states in game-hours */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Weather", meta = (ClampMin = "0.1"))
	float TransitionDuration = 0.5f;

	/** Probability (0-1) that fog will activate alongside any weather state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Weather", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FogChance = 0.15f;

	// =========================================================================
	// QUERIES
	// =========================================================================

	/** Get the current weather state (the state we are in or transitioning to) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Weather")
	EIronvaleWeatherState GetCurrentWeather() const { return CurrentWeather; }

	/** Get the previous weather state (the state we are transitioning from) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Weather")
	EIronvaleWeatherState GetPreviousWeather() const { return PreviousWeather; }

	/**
	 * Get the current weather intensity (0.0 - 1.0).
	 * During transitions: interpolates from 0 to 1.
	 * During steady state: 1.0.
	 * Use this to drive particle density, sound volume, etc.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Weather")
	float GetWeatherIntensity() const { return CurrentIntensity; }

	/**
	 * Get the current fog intensity (0.0 - 1.0).
	 * Separate axis from main weather — can be foggy during any weather.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Weather")
	float GetFogIntensity() const { return CurrentFogIntensity; }

	/** Is the weather currently transitioning between states? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Weather")
	bool IsTransitioning() const { return bIsTransitioning; }

	/** How many game-hours remain before the next weather change */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Weather")
	float GetRemainingDuration() const { return RemainingDuration; }

	// =========================================================================
	// CONTROL
	// =========================================================================

	/**
	 * Force a specific weather state immediately (for quests, cutscenes, debug).
	 * Starts a transition from the current state to the forced state.
	 *
	 * @param NewWeather      The weather to transition to
	 * @param Duration        How long the forced weather should last (game-hours). 0 = use random.
	 * @param bInstant        If true, skip the transition and snap to the new weather immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Weather")
	void ForceWeather(EIronvaleWeatherState NewWeather, float Duration = 0.0f, bool bInstant = false);

	/** Force fog on/off */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Weather")
	void SetFogActive(bool bActive);

protected:
	// =========================================================================
	// STATE
	// =========================================================================

	/** Current active weather state */
	EIronvaleWeatherState CurrentWeather = EIronvaleWeatherState::Clear;

	/** Previous weather state (for transition interpolation) */
	EIronvaleWeatherState PreviousWeather = EIronvaleWeatherState::Clear;

	/** Remaining game-hours before next weather change */
	float RemainingDuration = 4.0f;

	/** Current intensity (0-1), used during transitions */
	float CurrentIntensity = 1.0f;

	/** Is the system currently transitioning between weather states */
	bool bIsTransitioning = false;

	/** Elapsed transition time (game-hours) */
	float TransitionElapsed = 0.0f;

	/** Whether fog is currently active (separate axis) */
	bool bFogActive = false;

	/** Current fog intensity (0-1), ramps up/down smoothly */
	float CurrentFogIntensity = 0.0f;

	/** Target fog intensity (1.0 if active, 0.0 if not) */
	float TargetFogIntensity = 0.0f;

	/** Delegate handle for the tick function */
	FTSTicker::FDelegateHandle TickDelegateHandle;

	// =========================================================================
	// INTERNAL
	// =========================================================================

	/** Roll a random duration for the current weather state */
	float RollWeatherDuration() const;

	/**
	 * Pick the next weather state based on weighted transition probabilities.
	 * Valid transitions:
	 *   Clear  -> Cloudy (70%), Clear (30%)
	 *   Cloudy -> Clear (30%), Rain (40%), Cloudy (30%)
	 *   Rain   -> Cloudy (35%), Storm (25%), Rain (40%)
	 *   Storm  -> Cloudy (50%), Rain (30%), Clear (20%)
	 */
	EIronvaleWeatherState RollNextWeather() const;

	/** Begin a transition to a new weather state */
	void BeginTransition(EIronvaleWeatherState NewWeather, float NewDuration);

	/** Finalize a transition — set the weather on GameState and broadcast */
	void CompleteTransition();

	/** Roll whether fog should activate for the new weather state */
	void RollFog();

	/** Update fog intensity smoothly toward target */
	void UpdateFog(float DeltaGameHours);

	/** Broadcast weather change via EventBus and update GameState */
	void BroadcastWeatherChanged(EIronvaleWeatherState NewWeather);
};
