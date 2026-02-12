// =============================================================================
// IronvaleTimeOfDayManager.h — Sun/moon position, lighting, and fog from game time
// Project Ironvale: First-person grounded medieval RPG
//
// UWorldSubsystem that reads game time from AIronvaleGameState and interpolates
// sun direction, sky light intensity, light color temperature, and fog density.
//
// Uses soft pointers to UCurveFloat for designer-tunable curves. Falls back to
// sensible procedural defaults if curves are not assigned.
//
// Cross-platform: references engine-native UDirectionalLightComponent and
// USkyLightComponent — no platform-specific rendering code.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleTimeOfDayManager.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class UCurveFloat;
class UCurveLinearColor;
class AIronvaleGameState;

/**
 * Time-of-day subsystem — manages environmental lighting based on game clock.
 *
 * Each tick:
 *   1. Reads GameTimeHours from AIronvaleGameState
 *   2. Evaluates curves (or procedural fallbacks) for sun angle, intensity, color, fog
 *   3. Applies values to the world's directional light (sun/moon) and sky light
 *
 * Designers configure behavior by assigning curve assets in the editor or via
 * Blueprint. If no curves are assigned, the system uses built-in sinusoidal
 * approximations that produce reasonable day/night cycles.
 */
UCLASS()
class IRONVALE_API UIronvaleTimeOfDaySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// =========================================================================
	// SUBSYSTEM LIFECYCLE
	// =========================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Called each frame to update lighting from game time */
	void Tick(float DeltaSeconds);

	// =========================================================================
	// CURVE ASSETS (designer-tunable)
	// =========================================================================

	/**
	 * Sun pitch angle over 24 hours (X = hour 0-24, Y = pitch degrees).
	 * Positive = above horizon, negative = below.
	 * If null, uses procedural sinusoidal curve.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay")
	TSoftObjectPtr<UCurveFloat> SunRotationCurve;

	/**
	 * Directional light intensity over 24 hours (X = hour 0-24, Y = intensity).
	 * If null, uses procedural curve keyed to sun elevation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay")
	TSoftObjectPtr<UCurveFloat> LightIntensityCurve;

	/**
	 * Light color temperature curve over 24 hours (X = hour 0-24, Y = color temperature in Kelvin).
	 * Dawn/dusk = warm (3000K), midday = cool (6500K), night = blue (8000K).
	 * If null, uses procedural approximation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay")
	TSoftObjectPtr<UCurveFloat> ColorTemperatureCurve;

	/**
	 * Fog density multiplier over 24 hours (X = hour 0-24, Y = density 0-1).
	 * If null, uses procedural curve (denser at dawn/dusk, thinner at midday).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay")
	TSoftObjectPtr<UCurveFloat> FogDensityCurve;

	// =========================================================================
	// SCENE REFERENCES
	// =========================================================================

	/**
	 * Set the directional light to control (call from level Blueprint or BeginPlay).
	 * The subsystem will rotate and adjust this light each tick.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|TimeOfDay")
	void SetDirectionalLight(UDirectionalLightComponent* InLight);

	/** Set the sky light to control */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|TimeOfDay")
	void SetSkyLight(USkyLightComponent* InSkyLight);

	// =========================================================================
	// QUERIES
	// =========================================================================

	/** Get the current sun direction vector (world space, normalized) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|TimeOfDay")
	FVector GetSunDirection() const { return CachedSunDirection; }

	/** Get the current sun pitch angle in degrees (positive = above horizon) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|TimeOfDay")
	float GetSunPitchAngle() const { return CachedSunPitch; }

	/** Get the current sky light intensity multiplier (0-1) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|TimeOfDay")
	float GetSkyLightIntensity() const { return CachedSkyLightIntensity; }

	/** Get the current fog density multiplier (0-1) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|TimeOfDay")
	float GetFogDensity() const { return CachedFogDensity; }

	/** Get the current light color temperature in Kelvin */
	UFUNCTION(BlueprintPure, Category = "Ironvale|TimeOfDay")
	float GetColorTemperature() const { return CachedColorTemperature; }

	/** Is it currently nighttime? (sun below horizon) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|TimeOfDay")
	bool IsNight() const { return CachedSunPitch < 0.0f; }

	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** Yaw angle of the sun's arc (compass direction). 0 = East-West arc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay")
	float SunArcYaw = 0.0f;

	/** Base directional light intensity at solar noon (lux-like value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay", meta = (ClampMin = "0.0"))
	float BaseSunIntensity = 10.0f;

	/** Base sky light intensity at solar noon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay", meta = (ClampMin = "0.0"))
	float BaseSkyIntensity = 1.0f;

	/** Moonlight intensity multiplier (fraction of sun intensity applied at night) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MoonlightFraction = 0.05f;

	/** Base fog density at the densest time of day */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|TimeOfDay", meta = (ClampMin = "0.0"))
	float BaseFogDensity = 0.02f;

protected:
	// =========================================================================
	// CACHED VALUES (updated each tick)
	// =========================================================================

	FVector CachedSunDirection = FVector(0.0f, 0.0f, 1.0f);
	float CachedSunPitch = 45.0f;
	float CachedSkyLightIntensity = 1.0f;
	float CachedFogDensity = 0.01f;
	float CachedColorTemperature = 6500.0f;

	// =========================================================================
	// SCENE REFERENCES
	// =========================================================================

	UPROPERTY()
	TWeakObjectPtr<UDirectionalLightComponent> DirectionalLight;

	UPROPERTY()
	TWeakObjectPtr<USkyLightComponent> SkyLight;

	/** Delegate handle for the tick function */
	FDelegateHandle TickDelegateHandle;

	// =========================================================================
	// INTERNAL
	// =========================================================================

	/** Calculate sun pitch angle from game hour (procedural fallback) */
	float CalculateProceduralSunPitch(float GameHour) const;

	/** Calculate light intensity from sun pitch (procedural fallback) */
	float CalculateProceduralLightIntensity(float SunPitch) const;

	/** Calculate color temperature from game hour (procedural fallback) */
	float CalculateProceduralColorTemperature(float GameHour) const;

	/** Calculate fog density from game hour (procedural fallback) */
	float CalculateProceduralFogDensity(float GameHour) const;

	/** Apply calculated values to the scene lights */
	void ApplyToSceneLights(float GameHour);

	/** Evaluate a curve or use the procedural fallback */
	float EvaluateCurveOrDefault(const TSoftObjectPtr<UCurveFloat>& Curve, float Time, float DefaultValue) const;
};
