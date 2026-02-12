// =============================================================================
// IronvaleTimeOfDayManager.cpp — Time-of-day lighting implementation
// Project Ironvale
// =============================================================================

#include "Immersion/IronvaleTimeOfDayManager.h"
#include "Ironvale.h"
#include "IronvaleGameState.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"

// =============================================================================
// SUBSYSTEM LIFECYCLE
// =============================================================================

void UIronvaleTimeOfDaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Register tick via FTSTicker so we update every frame
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaSeconds) -> bool
		{
			Tick(DeltaSeconds);
			return true;
		}),
		0.0f
	);

	UE_LOG(LogIronvale, Log, TEXT("TimeOfDaySubsystem initialized"));
}

void UIronvaleTimeOfDaySubsystem::Deinitialize()
{
	if (TickDelegateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
		TickDelegateHandle.Reset();
	}

	Super::Deinitialize();
}

bool UIronvaleTimeOfDaySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// =============================================================================
// SCENE REFERENCES
// =============================================================================

void UIronvaleTimeOfDaySubsystem::SetDirectionalLight(UDirectionalLightComponent* InLight)
{
	DirectionalLight = InLight;
	UE_LOG(LogIronvale, Log, TEXT("TimeOfDay: Directional light assigned: %s"),
		InLight ? *InLight->GetOwner()->GetName() : TEXT("null"));
}

void UIronvaleTimeOfDaySubsystem::SetSkyLight(USkyLightComponent* InSkyLight)
{
	SkyLight = InSkyLight;
	UE_LOG(LogIronvale, Log, TEXT("TimeOfDay: Sky light assigned: %s"),
		InSkyLight ? *InSkyLight->GetOwner()->GetName() : TEXT("null"));
}

// =============================================================================
// TICK
// =============================================================================

void UIronvaleTimeOfDaySubsystem::Tick(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const AIronvaleGameState* GameState = World->GetGameState<AIronvaleGameState>();
	if (!GameState)
	{
		return;
	}

	const float GameHour = GameState->GetGameTimeHours();

	// --- Sun pitch ---
	CachedSunPitch = EvaluateCurveOrDefault(
		SunRotationCurve, GameHour,
		CalculateProceduralSunPitch(GameHour)
	);

	// --- Sun direction from pitch + yaw arc ---
	{
		// Pitch: 0 = horizon, +90 = zenith, -90 = nadir
		// We rotate around the Y axis for pitch, then apply arc yaw
		const FRotator SunRotation(CachedSunPitch, SunArcYaw, 0.0f);
		CachedSunDirection = SunRotation.Vector();
	}

	// --- Light intensity ---
	{
		const float ProceduralIntensity = CalculateProceduralLightIntensity(CachedSunPitch);
		const float CurveIntensity = EvaluateCurveOrDefault(
			LightIntensityCurve, GameHour,
			ProceduralIntensity
		);
		// Scale by base intensity
		CachedSkyLightIntensity = FMath::Max(CurveIntensity * BaseSkyIntensity, MoonlightFraction * BaseSkyIntensity);
	}

	// --- Color temperature ---
	CachedColorTemperature = EvaluateCurveOrDefault(
		ColorTemperatureCurve, GameHour,
		CalculateProceduralColorTemperature(GameHour)
	);

	// --- Fog density ---
	CachedFogDensity = EvaluateCurveOrDefault(
		FogDensityCurve, GameHour,
		CalculateProceduralFogDensity(GameHour)
	) * BaseFogDensity;

	// Apply to scene lights
	ApplyToSceneLights(GameHour);
}

// =============================================================================
// PROCEDURAL FALLBACKS
// =============================================================================

float UIronvaleTimeOfDaySubsystem::CalculateProceduralSunPitch(float GameHour) const
{
	// Solar noon at 12:00, sunrise at ~6:00, sunset at ~18:00
	// Sinusoidal: pitch = 90 * sin((hour - 6) / 12 * PI)
	// This gives:
	//   6:00  = 0 degrees (sunrise)
	//   12:00 = 90 degrees (zenith)
	//   18:00 = 0 degrees (sunset)
	//   0:00  = -90 degrees (nadir / midnight)

	const float NormalizedTime = (GameHour - 6.0f) / 24.0f; // 0 at 6 AM
	const float PitchRadians = NormalizedTime * 2.0f * PI;
	return 90.0f * FMath::Sin(PitchRadians);
}

float UIronvaleTimeOfDaySubsystem::CalculateProceduralLightIntensity(float SunPitch) const
{
	if (SunPitch <= -10.0f)
	{
		// Deep night — moonlight only
		return MoonlightFraction;
	}

	if (SunPitch <= 0.0f)
	{
		// Twilight zone (-10 to 0 degrees) — interpolate between moonlight and dawn
		const float TwilightAlpha = (SunPitch + 10.0f) / 10.0f; // 0 at -10, 1 at 0
		return FMath::Lerp(MoonlightFraction, 0.2f, TwilightAlpha);
	}

	if (SunPitch <= 15.0f)
	{
		// Golden hour (0 to 15 degrees) — ramp up
		const float GoldenAlpha = SunPitch / 15.0f;
		return FMath::Lerp(0.2f, 0.7f, GoldenAlpha);
	}

	// Full daylight (15+ degrees) — approach 1.0
	const float DayAlpha = FMath::Clamp((SunPitch - 15.0f) / 75.0f, 0.0f, 1.0f);
	return FMath::Lerp(0.7f, 1.0f, DayAlpha);
}

float UIronvaleTimeOfDaySubsystem::CalculateProceduralColorTemperature(float GameHour) const
{
	// Color temperature schedule (in Kelvin):
	//   Night (20-4):    8000K (cool blue moonlight)
	//   Dawn (5-7):      3000K (warm amber)
	//   Morning (7-10):  4500K (warming up)
	//   Midday (10-14):  6500K (neutral daylight)
	//   Afternoon (14-17): 5500K (slightly warm)
	//   Dusk (17-19):    2500K (warm sunset)
	//   Evening (19-20): 6000K -> 8000K (transition to night)

	if (GameHour < 5.0f)
	{
		return 8000.0f; // Night
	}
	if (GameHour < 7.0f)
	{
		// Dawn: 8000 -> 3000 -> warming
		const float Alpha = (GameHour - 5.0f) / 2.0f;
		return FMath::Lerp(8000.0f, 3000.0f, Alpha);
	}
	if (GameHour < 10.0f)
	{
		// Morning: 3000 -> 6500
		const float Alpha = (GameHour - 7.0f) / 3.0f;
		return FMath::Lerp(3000.0f, 6500.0f, Alpha);
	}
	if (GameHour < 14.0f)
	{
		return 6500.0f; // Midday
	}
	if (GameHour < 17.0f)
	{
		// Afternoon: 6500 -> 5500
		const float Alpha = (GameHour - 14.0f) / 3.0f;
		return FMath::Lerp(6500.0f, 5500.0f, Alpha);
	}
	if (GameHour < 19.0f)
	{
		// Dusk: 5500 -> 2500
		const float Alpha = (GameHour - 17.0f) / 2.0f;
		return FMath::Lerp(5500.0f, 2500.0f, Alpha);
	}
	if (GameHour < 20.0f)
	{
		// Evening transition: 2500 -> 8000
		const float Alpha = (GameHour - 19.0f) / 1.0f;
		return FMath::Lerp(2500.0f, 8000.0f, Alpha);
	}

	return 8000.0f; // Night
}

float UIronvaleTimeOfDaySubsystem::CalculateProceduralFogDensity(float GameHour) const
{
	// Fog is denser at dawn/dusk, thinner at midday and night
	// Dawn peak at 6:00, dusk peak at 18:00, midday minimum at 12:00
	// Night has moderate fog

	if (GameHour < 4.0f)
	{
		return 0.4f; // Late night — moderate
	}
	if (GameHour < 7.0f)
	{
		// Dawn buildup: 0.4 -> 1.0 -> 0.5
		const float Alpha = (GameHour - 4.0f) / 3.0f;
		// Bell curve peaking at 5.5
		const float DawnFactor = FMath::Sin(Alpha * PI);
		return FMath::Lerp(0.4f, 1.0f, DawnFactor);
	}
	if (GameHour < 12.0f)
	{
		// Morning burn-off: descend to minimum
		const float Alpha = (GameHour - 7.0f) / 5.0f;
		return FMath::Lerp(0.5f, 0.1f, Alpha);
	}
	if (GameHour < 16.0f)
	{
		return 0.1f; // Midday — minimal fog
	}
	if (GameHour < 19.0f)
	{
		// Dusk buildup
		const float Alpha = (GameHour - 16.0f) / 3.0f;
		return FMath::Lerp(0.1f, 0.8f, Alpha);
	}
	if (GameHour < 21.0f)
	{
		// Evening settling
		const float Alpha = (GameHour - 19.0f) / 2.0f;
		return FMath::Lerp(0.8f, 0.4f, Alpha);
	}

	return 0.4f; // Night
}

// =============================================================================
// CURVE EVALUATION
// =============================================================================

float UIronvaleTimeOfDaySubsystem::EvaluateCurveOrDefault(
	const TSoftObjectPtr<UCurveFloat>& Curve, float Time, float DefaultValue) const
{
	if (!Curve.IsNull())
	{
		if (UCurveFloat* LoadedCurve = Curve.LoadSynchronous())
		{
			return LoadedCurve->GetFloatValue(Time);
		}
	}

	return DefaultValue;
}

// =============================================================================
// SCENE APPLICATION
// =============================================================================

void UIronvaleTimeOfDaySubsystem::ApplyToSceneLights(float GameHour)
{
	// --- Directional Light (Sun/Moon) ---
	if (UDirectionalLightComponent* DirLight = DirectionalLight.Get())
	{
		// Set rotation: pitch from sun angle, yaw from arc configuration
		const FRotator LightRotation(-CachedSunPitch, SunArcYaw, 0.0f);
		DirLight->SetWorldRotation(LightRotation);

		// Set intensity based on sun elevation
		const float ProceduralIntensity = CalculateProceduralLightIntensity(CachedSunPitch);
		const float FinalIntensity = EvaluateCurveOrDefault(
			LightIntensityCurve, GameHour, ProceduralIntensity
		) * BaseSunIntensity;

		DirLight->SetIntensity(FinalIntensity);

		// Apply color temperature
		DirLight->bUseTemperature = true;
		DirLight->SetTemperature(CachedColorTemperature);
	}

	// --- Sky Light ---
	if (USkyLightComponent* Sky = SkyLight.Get())
	{
		Sky->SetIntensity(CachedSkyLightIntensity);

		// Recapture sky at major transitions (dawn/dusk) would be ideal,
		// but we avoid it per-frame for performance. Systems can call
		// SkyLight->RecaptureSky() manually at TOD transitions.
	}
}
