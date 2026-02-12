// =============================================================================
// IronvaleAmbientSoundManager.cpp — Layered ambient audio implementation
// Project Ironvale
// =============================================================================

#include "Immersion/IronvaleAmbientSoundManager.h"
#include "Ironvale.h"
#include "Core/IronvaleEventBus.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundCue.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

// =============================================================================
// SUBSYSTEM LIFECYCLE
// =============================================================================

void UIronvaleAmbientSoundSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Ensure EventBus is initialized before us
	Collection.InitializeDependency<UIronvaleEventBus>();

	// Register tick
	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaSeconds) -> bool
		{
			Tick(DeltaSeconds);
			return true;
		}),
		0.0f
	);

	// Bind to event bus after a short delay to ensure the world is ready
	// (actual binding happens on first tick or via deferred call)
	if (UWorld* World = GetWorld())
	{
		World->OnWorldBeginPlay.AddLambda([this]()
		{
			BindToEventBus();
		});
	}

	UE_LOG(LogIronvale, Log, TEXT("AmbientSoundSubsystem initialized"));
}

void UIronvaleAmbientSoundSubsystem::Deinitialize()
{
	UnbindFromEventBus();

	if (TickDelegateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
		TickDelegateHandle.Reset();
	}

	// Clean up audio components
	StopAndClear(BiomeAudioComponent);
	StopAndClear(BiomeAudioFadeOut);
	StopAndClear(WeatherAudioComponent);
	StopAndClear(WeatherAudioFadeOut);
	StopAndClear(TimeOfDayAudioComponent);
	StopAndClear(TimeOfDayAudioFadeOut);
	StopAndClear(InteriorAudioComponent);

	Super::Deinitialize();
}

bool UIronvaleAmbientSoundSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// =============================================================================
// EVENT BUS BINDING
// =============================================================================

void UIronvaleAmbientSoundSubsystem::BindToEventBus()
{
	UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>();
	if (!EventBus)
	{
		UE_LOG(LogIronvale, Warning, TEXT("AmbientSound: EventBus not available for binding"));
		return;
	}

	EventBus->OnWeatherChanged.AddDynamic(this, &UIronvaleAmbientSoundSubsystem::HandleWeatherChanged);
	EventBus->OnTimeOfDayChanged.AddDynamic(this, &UIronvaleAmbientSoundSubsystem::HandleTimeOfDayChanged);

	UE_LOG(LogIronvale, Log, TEXT("AmbientSound: Bound to EventBus weather/time delegates"));
}

void UIronvaleAmbientSoundSubsystem::UnbindFromEventBus()
{
	if (!GetWorld())
	{
		return;
	}

	UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>();
	if (!EventBus)
	{
		return;
	}

	EventBus->OnWeatherChanged.RemoveDynamic(this, &UIronvaleAmbientSoundSubsystem::HandleWeatherChanged);
	EventBus->OnTimeOfDayChanged.RemoveDynamic(this, &UIronvaleAmbientSoundSubsystem::HandleTimeOfDayChanged);
}

// =============================================================================
// EVENT HANDLERS
// =============================================================================

void UIronvaleAmbientSoundSubsystem::HandleWeatherChanged(EIronvaleWeatherState NewWeather)
{
	if (NewWeather == CurrentWeather)
	{
		return;
	}

	UE_LOG(LogIronvale, Log, TEXT("AmbientSound: Weather changed to %d — updating weather audio layer"),
		static_cast<int32>(NewWeather));

	CurrentWeather = NewWeather;

	// Find the sound cue for this weather state
	USoundCue* NewCue = nullptr;
	if (const TSoftObjectPtr<USoundCue>* CuePtr = WeatherSoundCues.Find(NewWeather))
	{
		NewCue = LoadSoundCue(*CuePtr);
	}

	if (NewCue)
	{
		CrossfadeLayer(WeatherAudioComponent, WeatherAudioFadeOut,
			WeatherFadeAlpha, WeatherFadeTarget, NewCue, WeatherVolume);
	}
	else
	{
		// No sound for this weather (e.g., Clear) — fade out current
		WeatherFadeTarget = 0.0f;
	}
}

void UIronvaleAmbientSoundSubsystem::HandleTimeOfDayChanged(EIronvaleTimeOfDay NewTimeOfDay)
{
	if (NewTimeOfDay == CurrentTimeOfDay)
	{
		return;
	}

	UE_LOG(LogIronvale, Log, TEXT("AmbientSound: Time of day changed to %d — updating TOD audio layer"),
		static_cast<int32>(NewTimeOfDay));

	CurrentTimeOfDay = NewTimeOfDay;

	// Find the sound cue for this time of day
	USoundCue* NewCue = nullptr;
	if (const TSoftObjectPtr<USoundCue>* CuePtr = TimeOfDaySoundCues.Find(NewTimeOfDay))
	{
		NewCue = LoadSoundCue(*CuePtr);
	}

	if (NewCue)
	{
		CrossfadeLayer(TimeOfDayAudioComponent, TimeOfDayAudioFadeOut,
			TimeOfDayFadeAlpha, TimeOfDayFadeTarget, NewCue, TimeOfDayVolume);
	}
	else
	{
		// No sound for this period — fade out
		TimeOfDayFadeTarget = 0.0f;
	}
}

// =============================================================================
// PUBLIC CONTROL
// =============================================================================

void UIronvaleAmbientSoundSubsystem::SetBiome(EIronvaleBiome NewBiome)
{
	if (NewBiome == CurrentBiome && BiomeAudioComponent)
	{
		return;
	}

	UE_LOG(LogIronvale, Log, TEXT("AmbientSound: Biome changed to %d"), static_cast<int32>(NewBiome));
	CurrentBiome = NewBiome;

	USoundCue* NewCue = nullptr;
	if (const TSoftObjectPtr<USoundCue>* CuePtr = BiomeSoundCues.Find(NewBiome))
	{
		NewCue = LoadSoundCue(*CuePtr);
	}

	if (NewCue)
	{
		const float EffectiveVolume = bIsInterior ? (BiomeVolume * InteriorExteriorAttenuation) : BiomeVolume;
		CrossfadeLayer(BiomeAudioComponent, BiomeAudioFadeOut,
			BiomeFadeAlpha, BiomeFadeTarget, NewCue, EffectiveVolume);
	}
	else
	{
		BiomeFadeTarget = 0.0f;
		UE_LOG(LogIronvale, Warning, TEXT("AmbientSound: No sound cue mapped for biome %d"),
			static_cast<int32>(NewBiome));
	}
}

void UIronvaleAmbientSoundSubsystem::SetInterior(bool bNewInterior)
{
	if (bNewInterior == bIsInterior)
	{
		return;
	}

	UE_LOG(LogIronvale, Log, TEXT("AmbientSound: Interior state changed to %s"),
		bNewInterior ? TEXT("INTERIOR") : TEXT("EXTERIOR"));

	bIsInterior = bNewInterior;

	if (bIsInterior)
	{
		// Fade in interior ambience
		InteriorFadeTarget = 1.0f;

		USoundCue* IntCue = LoadSoundCue(InteriorSoundCue);
		if (IntCue && !InteriorAudioComponent)
		{
			InteriorAudioComponent = CreateAudioLayer(IntCue, 0.0f); // Start silent, fade in
			InteriorFadeAlpha = 0.0f;
		}

		// Attenuate exterior layers — handled in tick via volume targets
	}
	else
	{
		// Fade out interior ambience
		InteriorFadeTarget = 0.0f;
	}
}

// =============================================================================
// TICK — VOLUME FADING
// =============================================================================

void UIronvaleAmbientSoundSubsystem::Tick(float DeltaSeconds)
{
	// Calculate effective volumes based on interior/exterior state
	const float EffectiveBiomeVolume = bIsInterior ? (BiomeVolume * InteriorExteriorAttenuation) : BiomeVolume;
	const float EffectiveWeatherVolume = bIsInterior ? (WeatherVolume * InteriorExteriorAttenuation) : WeatherVolume;
	const float EffectiveTimeOfDayVolume = bIsInterior ? (TimeOfDayVolume * InteriorExteriorAttenuation) : TimeOfDayVolume;

	// Update each layer's fade
	UpdateLayerFade(BiomeAudioComponent, BiomeAudioFadeOut,
		BiomeFadeAlpha, BiomeFadeTarget, EffectiveBiomeVolume, DeltaSeconds);

	UpdateLayerFade(WeatherAudioComponent, WeatherAudioFadeOut,
		WeatherFadeAlpha, WeatherFadeTarget, EffectiveWeatherVolume, DeltaSeconds);

	UpdateLayerFade(TimeOfDayAudioComponent, TimeOfDayAudioFadeOut,
		TimeOfDayFadeAlpha, TimeOfDayFadeTarget, EffectiveTimeOfDayVolume, DeltaSeconds);

	// Interior layer — only fades based on interior state
	if (InteriorAudioComponent)
	{
		const float FadeSpeed = (CrossfadeDuration > 0.0f) ? (1.0f / CrossfadeDuration) : 10.0f;

		if (InteriorFadeAlpha < InteriorFadeTarget)
		{
			InteriorFadeAlpha = FMath::Min(InteriorFadeAlpha + FadeSpeed * DeltaSeconds, InteriorFadeTarget);
		}
		else if (InteriorFadeAlpha > InteriorFadeTarget)
		{
			InteriorFadeAlpha = FMath::Max(InteriorFadeAlpha - FadeSpeed * DeltaSeconds, InteriorFadeTarget);
		}

		InteriorAudioComponent->SetVolumeMultiplier(InteriorFadeAlpha * InteriorVolume);

		// Clean up if fully faded out
		if (InteriorFadeAlpha <= 0.0f && InteriorFadeTarget <= 0.0f)
		{
			StopAndClear(InteriorAudioComponent);
		}
	}
}

void UIronvaleAmbientSoundSubsystem::UpdateLayerFade(
	UAudioComponent* ActiveComp, UAudioComponent*& FadeOutComp,
	float& FadeAlpha, float FadeTarget, float TargetVolume, float DeltaSeconds)
{
	const float FadeSpeed = (CrossfadeDuration > 0.0f) ? (1.0f / CrossfadeDuration) : 10.0f;

	// Fade in the active component
	if (ActiveComp)
	{
		if (FadeAlpha < FadeTarget)
		{
			FadeAlpha = FMath::Min(FadeAlpha + FadeSpeed * DeltaSeconds, FadeTarget);
		}
		else if (FadeAlpha > FadeTarget)
		{
			FadeAlpha = FMath::Max(FadeAlpha - FadeSpeed * DeltaSeconds, FadeTarget);
		}

		ActiveComp->SetVolumeMultiplier(FadeAlpha * TargetVolume);

		// If faded out completely, stop and clean up
		if (FadeAlpha <= 0.0f && FadeTarget <= 0.0f)
		{
			StopAndClear(ActiveComp);
		}
	}

	// Fade out the old component
	if (FadeOutComp)
	{
		const float CurrentVol = FadeOutComp->VolumeMultiplier;
		const float NewVol = FMath::Max(CurrentVol - FadeSpeed * TargetVolume * DeltaSeconds, 0.0f);
		FadeOutComp->SetVolumeMultiplier(NewVol);

		if (NewVol <= 0.0f)
		{
			StopAndClear(FadeOutComp);
		}
	}
}

// =============================================================================
// AUDIO COMPONENT MANAGEMENT
// =============================================================================

UAudioComponent* UIronvaleAmbientSoundSubsystem::CreateAudioLayer(USoundCue* Cue, float Volume)
{
	if (!Cue)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Create the audio component on the world settings actor (persistent, always present)
	AWorldSettings* WorldSettings = World->GetWorldSettings();
	if (!WorldSettings)
	{
		return nullptr;
	}

	UAudioComponent* AudioComp = NewObject<UAudioComponent>(WorldSettings);
	if (!AudioComp)
	{
		return nullptr;
	}

	AudioComp->SetSound(Cue);
	AudioComp->SetVolumeMultiplier(Volume);
	AudioComp->bAutoActivate = false;
	AudioComp->bIsUISound = false;
	AudioComp->bAllowSpatialization = false; // Ambient is non-spatialized (2D)
	AudioComp->bAutoDestroy = false;
	AudioComp->RegisterComponent();
	AudioComp->Play();

	return AudioComp;
}

void UIronvaleAmbientSoundSubsystem::CrossfadeLayer(
	UAudioComponent*& ActiveComp, UAudioComponent*& FadeOutComp,
	float& FadeAlpha, float& FadeTarget, USoundCue* NewCue, float TargetVolume)
{
	// Move current active to fade-out slot
	if (FadeOutComp)
	{
		// Already fading something out — stop it immediately
		StopAndClear(FadeOutComp);
	}

	if (ActiveComp)
	{
		FadeOutComp = ActiveComp;
		ActiveComp = nullptr;
	}

	// Create new active component starting at zero volume
	if (NewCue)
	{
		ActiveComp = CreateAudioLayer(NewCue, 0.0f);
		FadeAlpha = 0.0f;
		FadeTarget = 1.0f;
	}
}

USoundCue* UIronvaleAmbientSoundSubsystem::LoadSoundCue(const TSoftObjectPtr<USoundCue>& SoftPtr) const
{
	if (SoftPtr.IsNull())
	{
		return nullptr;
	}

	// Synchronous load — ambient sounds are always needed and should be preloaded
	return SoftPtr.LoadSynchronous();
}

void UIronvaleAmbientSoundSubsystem::StopAndClear(UAudioComponent*& Comp)
{
	if (Comp)
	{
		Comp->Stop();
		Comp->DestroyComponent();
		Comp = nullptr;
	}
}
