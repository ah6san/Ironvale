// =============================================================================
// IronvaleAmbientSoundManager.h — Layered ambient audio system
// Project Ironvale: First-person grounded medieval RPG
//
// UWorldSubsystem that manages ambient audio layers:
//   1. Base biome loop (forest, town, dungeon, etc.)
//   2. Weather overlay (rain, storm, wind)
//   3. Time-of-day layer (crickets at night, birds at dawn, wind at midday)
//   4. Interior/exterior switch (muffled exterior sounds when indoors)
//
// Subscribes to EventBus for weather/time changes. Uses Unreal's native
// UAudioComponent for cross-platform compatibility — no middleware required.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleAmbientSoundManager.generated.h"

class UAudioComponent;
class USoundCue;
class UIronvaleEventBus;

/**
 * Ambient sound subsystem — manages layered environmental audio.
 *
 * Audio layers (each is a separate UAudioComponent for independent volume control):
 *   - Biome:   Base ambience loop for the current biome (forest birds, town bustle, etc.)
 *   - Weather: Overlay loop that matches current weather (rain patter, thunder rumbles)
 *   - TimeOfDay: Layer that changes with time (crickets at night, dawn chorus, etc.)
 *   - Interior: Optional interior ambience when the player is indoors
 *
 * Volume crossfading ensures smooth transitions between layers. All sounds use
 * Unreal's native audio system (UAudioComponent + USoundCue) for full cross-platform
 * support without middleware dependencies.
 */
UCLASS()
class IRONVALE_API UIronvaleAmbientSoundSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// =========================================================================
	// SUBSYSTEM LIFECYCLE
	// =========================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** Called each frame to update volume fades and layer transitions */
	void Tick(float DeltaSeconds);

	// =========================================================================
	// SOUND CUE MAPPINGS
	// =========================================================================

	/** Base ambience loops per biome — the foundational ambient layer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient|Biome")
	TMap<EIronvaleBiome, TSoftObjectPtr<USoundCue>> BiomeSoundCues;

	/** Weather overlay loops — layered on top of biome ambience */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient|Weather")
	TMap<EIronvaleWeatherState, TSoftObjectPtr<USoundCue>> WeatherSoundCues;

	/** Time-of-day ambient loops — crickets at night, birds at dawn, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient|TimeOfDay")
	TMap<EIronvaleTimeOfDay, TSoftObjectPtr<USoundCue>> TimeOfDaySoundCues;

	/** Interior ambience cue — replaces biome when indoors */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient|Interior")
	TSoftObjectPtr<USoundCue> InteriorSoundCue;

	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** Duration in seconds for volume crossfades between layers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient", meta = (ClampMin = "0.1"))
	float CrossfadeDuration = 2.0f;

	/** Master volume for biome layer (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BiomeVolume = 1.0f;

	/** Master volume for weather overlay (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WeatherVolume = 0.8f;

	/** Master volume for time-of-day layer (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TimeOfDayVolume = 0.6f;

	/** Master volume for interior ambience (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InteriorVolume = 0.7f;

	/** Volume attenuation for exterior layers when indoors (0 = silent, 1 = full) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Ambient", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InteriorExteriorAttenuation = 0.2f;

	// =========================================================================
	// CONTROL METHODS
	// =========================================================================

	/**
	 * Set the current biome — cross-fades the biome ambient layer.
	 * Called when the player moves between biome zones.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Ambient")
	void SetBiome(EIronvaleBiome NewBiome);

	/**
	 * Set interior/exterior state — adjusts layering and applies attenuation.
	 * Called by trigger volumes or room detection systems.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Ambient")
	void SetInterior(bool bIsInterior);

	/** Get the current biome */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Ambient")
	EIronvaleBiome GetCurrentBiome() const { return CurrentBiome; }

	/** Is the player currently indoors? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Ambient")
	bool IsInterior() const { return bIsInterior; }

protected:
	// =========================================================================
	// STATE
	// =========================================================================

	EIronvaleBiome CurrentBiome = EIronvaleBiome::Forest;
	EIronvaleWeatherState CurrentWeather = EIronvaleWeatherState::Clear;
	EIronvaleTimeOfDay CurrentTimeOfDay = EIronvaleTimeOfDay::Morning;
	bool bIsInterior = false;

	// =========================================================================
	// AUDIO COMPONENTS (one per layer)
	// =========================================================================

	/** Active biome ambience audio component */
	UPROPERTY()
	UAudioComponent* BiomeAudioComponent = nullptr;

	/** Outgoing biome audio (fading out during crossfade) */
	UPROPERTY()
	UAudioComponent* BiomeAudioFadeOut = nullptr;

	/** Active weather overlay audio */
	UPROPERTY()
	UAudioComponent* WeatherAudioComponent = nullptr;

	/** Outgoing weather audio (fading out) */
	UPROPERTY()
	UAudioComponent* WeatherAudioFadeOut = nullptr;

	/** Active time-of-day audio */
	UPROPERTY()
	UAudioComponent* TimeOfDayAudioComponent = nullptr;

	/** Outgoing time-of-day audio (fading out) */
	UPROPERTY()
	UAudioComponent* TimeOfDayAudioFadeOut = nullptr;

	/** Interior ambience audio */
	UPROPERTY()
	UAudioComponent* InteriorAudioComponent = nullptr;

	/** Delegate handle for tick */
	FDelegateHandle TickDelegateHandle;

	// =========================================================================
	// CROSSFADE STATE
	// =========================================================================

	/** Tracks fade-in progress per layer (0 = silent, 1 = full volume) */
	float BiomeFadeAlpha = 0.0f;
	float WeatherFadeAlpha = 0.0f;
	float TimeOfDayFadeAlpha = 0.0f;
	float InteriorFadeAlpha = 0.0f;

	/** Target fade alpha (1.0 if active, 0.0 if fading out) */
	float BiomeFadeTarget = 1.0f;
	float WeatherFadeTarget = 0.0f;
	float TimeOfDayFadeTarget = 1.0f;
	float InteriorFadeTarget = 0.0f;

	// =========================================================================
	// INTERNAL
	// =========================================================================

	/** Subscribe to EventBus weather/time delegates */
	void BindToEventBus();

	/** Unsubscribe from EventBus */
	void UnbindFromEventBus();

	/** EventBus handler: weather changed */
	UFUNCTION()
	void HandleWeatherChanged(EIronvaleWeatherState NewWeather);

	/** EventBus handler: time of day changed */
	UFUNCTION()
	void HandleTimeOfDayChanged(EIronvaleTimeOfDay NewTimeOfDay);

	/**
	 * Create or reuse a UAudioComponent for a specific layer.
	 * Uses the owning actor (world settings or a persistent actor) as outer.
	 */
	UAudioComponent* CreateAudioLayer(USoundCue* Cue, float Volume);

	/** Crossfade a layer: updates volume based on alpha progress */
	void UpdateLayerFade(UAudioComponent* ActiveComp, UAudioComponent*& FadeOutComp,
		float& FadeAlpha, float FadeTarget, float TargetVolume, float DeltaSeconds);

	/** Load a sound cue from a soft pointer (synchronous for ambient — always needed) */
	USoundCue* LoadSoundCue(const TSoftObjectPtr<USoundCue>& SoftPtr) const;

	/** Stop and null out an audio component */
	void StopAndClear(UAudioComponent*& Comp);

	/** Transition a layer to a new sound cue with crossfade */
	void CrossfadeLayer(UAudioComponent*& ActiveComp, UAudioComponent*& FadeOutComp,
		float& FadeAlpha, float& FadeTarget, USoundCue* NewCue, float TargetVolume);
};
