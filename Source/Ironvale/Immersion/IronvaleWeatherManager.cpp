// =============================================================================
// IronvaleWeatherManager.cpp — Weather state machine implementation
// Project Ironvale
// =============================================================================

#include "Immersion/IronvaleWeatherManager.h"
#include "Ironvale.h"
#include "IronvaleGameState.h"
#include "Core/IronvaleEventBus.h"
#include "Engine/World.h"

// =============================================================================
// SUBSYSTEM LIFECYCLE
// =============================================================================

void UIronvaleWeatherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Set initial state
	CurrentWeather = EIronvaleWeatherState::Clear;
	PreviousWeather = EIronvaleWeatherState::Clear;
	CurrentIntensity = 1.0f;
	bIsTransitioning = false;
	RemainingDuration = RollWeatherDuration();

	// Register tick function via FTickerDelegate on the world's timer
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaSeconds) -> bool
		{
			Tick(DeltaSeconds);
			return true; // Keep ticking
		}),
		0.0f // Tick every frame
	);

	UE_LOG(LogIronvale, Log, TEXT("WeatherSubsystem initialized. Starting weather: Clear, Duration: %.1f game-hours"),
		RemainingDuration);
}

void UIronvaleWeatherSubsystem::Deinitialize()
{
	// Unregister tick
	if (TickDelegateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
		TickDelegateHandle.Reset();
	}

	Super::Deinitialize();
}

bool UIronvaleWeatherSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// =============================================================================
// TICK
// =============================================================================

void UIronvaleWeatherSubsystem::Tick(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const AIronvaleGameState* GameState = World->GetGameState<AIronvaleGameState>();
	if (!GameState || GameState->IsTimePaused())
	{
		return;
	}

	// Convert real-time delta to game-hours
	const float DeltaGameHours = GameState->GetTimeScale() * DeltaSeconds;

	if (bIsTransitioning)
	{
		// Advance transition
		TransitionElapsed += DeltaGameHours;
		const float TransitionAlpha = FMath::Clamp(TransitionElapsed / TransitionDuration, 0.0f, 1.0f);

		// Smooth ease-in-out for natural-feeling transitions
		CurrentIntensity = FMath::InterpEaseInOut(0.0f, 1.0f, TransitionAlpha, 2.0f);

		if (TransitionAlpha >= 1.0f)
		{
			CompleteTransition();
		}
	}
	else
	{
		// Count down current weather duration
		RemainingDuration -= DeltaGameHours;

		if (RemainingDuration <= 0.0f)
		{
			// Time to change weather
			const EIronvaleWeatherState NextWeather = RollNextWeather();
			const float NextDuration = RollWeatherDuration();

			UE_LOG(LogIronvale, Log, TEXT("Weather duration expired. Transitioning from %d to %d (duration: %.1f hours)"),
				static_cast<int32>(CurrentWeather), static_cast<int32>(NextWeather), NextDuration);

			BeginTransition(NextWeather, NextDuration);
		}
	}

	// Update fog independently
	UpdateFog(DeltaGameHours);
}

// =============================================================================
// STATE MACHINE
// =============================================================================

void UIronvaleWeatherSubsystem::BeginTransition(EIronvaleWeatherState NewWeather, float NewDuration)
{
	PreviousWeather = CurrentWeather;
	CurrentWeather = NewWeather;
	RemainingDuration = NewDuration;
	bIsTransitioning = true;
	TransitionElapsed = 0.0f;
	CurrentIntensity = 0.0f;

	// Roll fog for the new weather state
	RollFog();
}

void UIronvaleWeatherSubsystem::CompleteTransition()
{
	bIsTransitioning = false;
	CurrentIntensity = 1.0f;
	TransitionElapsed = 0.0f;

	// Update GameState and broadcast
	BroadcastWeatherChanged(CurrentWeather);

	UE_LOG(LogIronvale, Log, TEXT("Weather transition complete. Now: %d, Remaining: %.1f game-hours"),
		static_cast<int32>(CurrentWeather), RemainingDuration);
}

float UIronvaleWeatherSubsystem::RollWeatherDuration() const
{
	return FMath::FRandRange(MinWeatherDuration, MaxWeatherDuration);
}

EIronvaleWeatherState UIronvaleWeatherSubsystem::RollNextWeather() const
{
	const float Roll = FMath::FRand(); // 0.0 - 1.0

	switch (CurrentWeather)
	{
	case EIronvaleWeatherState::Clear:
		// Clear -> Cloudy (70%), stay Clear (30%)
		return (Roll < 0.70f) ? EIronvaleWeatherState::Cloudy : EIronvaleWeatherState::Clear;

	case EIronvaleWeatherState::Cloudy:
		// Cloudy -> Clear (30%), Rain (40%), stay Cloudy (30%)
		if (Roll < 0.30f) return EIronvaleWeatherState::Clear;
		if (Roll < 0.70f) return EIronvaleWeatherState::Rain;
		return EIronvaleWeatherState::Cloudy;

	case EIronvaleWeatherState::Rain:
		// Rain -> Cloudy (35%), Storm (25%), stay Rain (40%)
		if (Roll < 0.35f) return EIronvaleWeatherState::Cloudy;
		if (Roll < 0.60f) return EIronvaleWeatherState::Storm;
		return EIronvaleWeatherState::Rain;

	case EIronvaleWeatherState::Storm:
		// Storm -> Cloudy (50%), Rain (30%), Clear (20%)
		if (Roll < 0.50f) return EIronvaleWeatherState::Cloudy;
		if (Roll < 0.80f) return EIronvaleWeatherState::Rain;
		return EIronvaleWeatherState::Clear;

	case EIronvaleWeatherState::Fog:
		// Fog is handled separately; if somehow in this state, go to Clear
		return EIronvaleWeatherState::Clear;

	case EIronvaleWeatherState::Snow:
		// Snow -> Cloudy (60%), Clear (40%)
		return (Roll < 0.60f) ? EIronvaleWeatherState::Cloudy : EIronvaleWeatherState::Clear;

	default:
		return EIronvaleWeatherState::Clear;
	}
}

void UIronvaleWeatherSubsystem::RollFog()
{
	// Fog is more likely during rain/storm, less likely during clear skies
	float EffectiveFogChance = FogChance;

	switch (CurrentWeather)
	{
	case EIronvaleWeatherState::Rain:
		EffectiveFogChance *= 1.5f;
		break;
	case EIronvaleWeatherState::Storm:
		EffectiveFogChance *= 0.5f; // Storm winds disperse fog
		break;
	case EIronvaleWeatherState::Clear:
		EffectiveFogChance *= 0.7f;
		break;
	default:
		break;
	}

	EffectiveFogChance = FMath::Clamp(EffectiveFogChance, 0.0f, 1.0f);
	const bool bNewFogActive = FMath::FRand() < EffectiveFogChance;
	SetFogActive(bNewFogActive);
}

void UIronvaleWeatherSubsystem::UpdateFog(float DeltaGameHours)
{
	if (FMath::IsNearlyEqual(CurrentFogIntensity, TargetFogIntensity, 0.001f))
	{
		CurrentFogIntensity = TargetFogIntensity;
		return;
	}

	// Fog ramps at the same rate as weather transitions
	const float FogTransitionSpeed = (TransitionDuration > 0.0f) ? (1.0f / TransitionDuration) : 2.0f;
	const float FogDelta = FogTransitionSpeed * DeltaGameHours;

	if (CurrentFogIntensity < TargetFogIntensity)
	{
		CurrentFogIntensity = FMath::Min(CurrentFogIntensity + FogDelta, TargetFogIntensity);
	}
	else
	{
		CurrentFogIntensity = FMath::Max(CurrentFogIntensity - FogDelta, TargetFogIntensity);
	}
}

// =============================================================================
// BROADCAST
// =============================================================================

void UIronvaleWeatherSubsystem::BroadcastWeatherChanged(EIronvaleWeatherState NewWeather)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Update GameState
	if (AIronvaleGameState* GameState = World->GetGameState<AIronvaleGameState>())
	{
		GameState->SetWeather(NewWeather);
	}

	// Broadcast through EventBus (GameState::SetWeather already broadcasts, but we ensure it here)
	if (UIronvaleEventBus* EventBus = World->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnWeatherChanged.Broadcast(NewWeather);
	}
}

// =============================================================================
// PUBLIC CONTROL
// =============================================================================

void UIronvaleWeatherSubsystem::ForceWeather(EIronvaleWeatherState NewWeather, float Duration, bool bInstant)
{
	const float FinalDuration = (Duration > 0.0f) ? Duration : RollWeatherDuration();

	if (bInstant)
	{
		PreviousWeather = CurrentWeather;
		CurrentWeather = NewWeather;
		CurrentIntensity = 1.0f;
		bIsTransitioning = false;
		TransitionElapsed = 0.0f;
		RemainingDuration = FinalDuration;

		BroadcastWeatherChanged(NewWeather);

		UE_LOG(LogIronvale, Log, TEXT("Weather forced (instant) to %d for %.1f game-hours"),
			static_cast<int32>(NewWeather), FinalDuration);
	}
	else
	{
		BeginTransition(NewWeather, FinalDuration);

		UE_LOG(LogIronvale, Log, TEXT("Weather forced (transition) to %d for %.1f game-hours"),
			static_cast<int32>(NewWeather), FinalDuration);
	}
}

void UIronvaleWeatherSubsystem::SetFogActive(bool bActive)
{
	bFogActive = bActive;
	TargetFogIntensity = bActive ? 1.0f : 0.0f;

	UE_LOG(LogIronvale, Verbose, TEXT("Fog %s"), bActive ? TEXT("activated") : TEXT("deactivated"));
}
