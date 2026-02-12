// =============================================================================
// IronvaleNeedsComponent.cpp — Needs system implementation
// Project Ironvale
// =============================================================================

#include "Needs/IronvaleNeedsComponent.h"
#include "Ironvale.h"
#include "Core/IronvaleEventBus.h"
#include "IronvaleGameState.h"
#include "TimerManager.h"

UIronvaleNeedsComponent::UIronvaleNeedsComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Uses timer instead of tick
}

void UIronvaleNeedsComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize all needs to starting values
	NeedValues.Add(EIronvaleNeedType::Hunger, 100.0f);
	NeedValues.Add(EIronvaleNeedType::Thirst, 100.0f);
	NeedValues.Add(EIronvaleNeedType::Fatigue, 100.0f);
	NeedValues.Add(EIronvaleNeedType::Cleanliness, 100.0f);

	LoadNeedConfigs();

	// Start periodic update timer
	GetWorld()->GetTimerManager().SetTimer(NeedsUpdateTimerHandle, this,
		&UIronvaleNeedsComponent::UpdateNeeds, UpdateIntervalSeconds, true);
}

void UIronvaleNeedsComponent::LoadNeedConfigs()
{
	if (!NeedsConfigTable) return;

	TArray<FIronvaleNeedConfig*> Rows;
	NeedsConfigTable->GetAllRows<FIronvaleNeedConfig>(TEXT("NeedsConfig"), Rows);

	for (const FIronvaleNeedConfig* Row : Rows)
	{
		if (Row)
		{
			NeedConfigs.Add(Row->NeedType, *Row);

			// Apply starting values from config
			if (float* Value = NeedValues.Find(Row->NeedType))
			{
				*Value = Row->StartValue;
			}
		}
	}

	UE_LOG(LogIronvale, Log, TEXT("Loaded %d need configurations"), NeedConfigs.Num());
}

void UIronvaleNeedsComponent::UpdateNeeds()
{
	// Calculate how many game-hours this update interval represents
	float GameHoursPerUpdate = 0.0f;

	if (const AIronvaleGameState* GS = GetWorld()->GetGameState<AIronvaleGameState>())
	{
		if (GS->IsTimePaused()) return;
		// TimeScale is game-hours per real-second, multiply by update interval
		GameHoursPerUpdate = GS->GetTimeScale() * UpdateIntervalSeconds;
	}
	else
	{
		// Fallback: assume default time scale
		GameHoursPerUpdate = (IronvaleConstants::DEFAULT_TIME_SCALE / 3600.0f) * UpdateIntervalSeconds;
	}

	// Decay each need
	for (auto& Pair : NeedValues)
	{
		const EIronvaleNeedType NeedType = Pair.Key;
		const float OldValue = Pair.Value;

		// Get decay rate from config
		float DecayRate = 4.0f; // Default
		if (const FIronvaleNeedConfig* Config = NeedConfigs.Find(NeedType))
		{
			DecayRate = Config->DecayRatePerHour;
		}

		const float Decay = DecayRate * GameHoursPerUpdate;
		Pair.Value = FMath::Clamp(Pair.Value - Decay, IronvaleConstants::NEED_MIN, IronvaleConstants::NEED_MAX);

		// Evaluate threshold crossings
		if (Pair.Value != OldValue)
		{
			EvaluateThresholds(NeedType, OldValue, Pair.Value);
		}
	}
}

void UIronvaleNeedsComponent::EvaluateThresholds(EIronvaleNeedType NeedType, float OldValue, float NewValue)
{
	const FIronvaleNeedConfig* Config = NeedConfigs.Find(NeedType);
	if (!Config) return;

	for (const FIronvaleNeedThreshold& Threshold : Config->Thresholds)
	{
		// Crossed below threshold
		if (OldValue >= Threshold.ThresholdValue && NewValue < Threshold.ThresholdValue)
		{
			UE_LOG(LogIronvale, Log, TEXT("Need %d crossed threshold %.0f: %s"),
				static_cast<int32>(NeedType), Threshold.ThresholdValue,
				*Threshold.Description.ToString());

			if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
			{
				EventBus->OnNeedThresholdCrossed.Broadcast(GetOwner(), NeedType, NewValue);

				// Broadcast critical if very low
				if (NewValue <= 10.0f)
				{
					EventBus->OnNeedCritical.Broadcast(GetOwner(), NeedType);
				}
			}
		}
	}
}

// =============================================================================
// QUERIES
// =============================================================================

float UIronvaleNeedsComponent::GetNeedValue(EIronvaleNeedType NeedType) const
{
	const float* Value = NeedValues.Find(NeedType);
	return Value ? *Value : 100.0f;
}

FIronvaleNeedStatus UIronvaleNeedsComponent::GetNeedStatus(EIronvaleNeedType NeedType) const
{
	FIronvaleNeedStatus Status;
	Status.NeedType = NeedType;
	Status.CurrentValue = GetNeedValue(NeedType);

	if (const FIronvaleNeedConfig* Config = NeedConfigs.Find(NeedType))
	{
		Status.DecayRate = Config->DecayRatePerHour;

		// Collect active debuffs
		for (const FIronvaleNeedThreshold& Threshold : Config->Thresholds)
		{
			if (Status.CurrentValue < Threshold.ThresholdValue && Threshold.DebuffTag.IsValid())
			{
				Status.ActiveDebuffs.AddTag(Threshold.DebuffTag);
			}
		}
	}

	return Status;
}

TArray<FIronvaleNeedStatus> UIronvaleNeedsComponent::GetAllNeedStatuses() const
{
	TArray<FIronvaleNeedStatus> Statuses;
	for (const auto& Pair : NeedValues)
	{
		Statuses.Add(GetNeedStatus(Pair.Key));
	}
	return Statuses;
}

float UIronvaleNeedsComponent::GetStaminaRegenMultiplier() const
{
	float WorstMultiplier = 1.0f;

	for (const auto& ConfigPair : NeedConfigs)
	{
		const float Value = GetNeedValue(ConfigPair.Key);
		for (const FIronvaleNeedThreshold& Threshold : ConfigPair.Value.Thresholds)
		{
			if (Value < Threshold.ThresholdValue)
			{
				WorstMultiplier = FMath::Min(WorstMultiplier, Threshold.StaminaRegenMultiplier);
			}
		}
	}

	return WorstMultiplier;
}

float UIronvaleNeedsComponent::GetMovementSpeedMultiplier() const
{
	float WorstMultiplier = 1.0f;

	for (const auto& ConfigPair : NeedConfigs)
	{
		const float Value = GetNeedValue(ConfigPair.Key);
		for (const FIronvaleNeedThreshold& Threshold : ConfigPair.Value.Thresholds)
		{
			if (Value < Threshold.ThresholdValue)
			{
				WorstMultiplier = FMath::Min(WorstMultiplier, Threshold.MovementSpeedMultiplier);
			}
		}
	}

	return WorstMultiplier;
}

float UIronvaleNeedsComponent::GetVisionBlurIntensity() const
{
	float MaxBlur = 0.0f;

	for (const auto& ConfigPair : NeedConfigs)
	{
		const float Value = GetNeedValue(ConfigPair.Key);
		for (const FIronvaleNeedThreshold& Threshold : ConfigPair.Value.Thresholds)
		{
			if (Value < Threshold.ThresholdValue)
			{
				MaxBlur = FMath::Max(MaxBlur, Threshold.VisionBlurIntensity);
			}
		}
	}

	return MaxBlur;
}

// =============================================================================
// MODIFICATION
// =============================================================================

void UIronvaleNeedsComponent::RestoreNeed(EIronvaleNeedType NeedType, float Amount)
{
	if (Amount <= 0.0f) return;

	float* Value = NeedValues.Find(NeedType);
	if (!Value) return;

	const float OldValue = *Value;
	*Value = FMath::Clamp(*Value + Amount, IronvaleConstants::NEED_MIN, IronvaleConstants::NEED_MAX);
	EvaluateThresholds(NeedType, OldValue, *Value);
}

void UIronvaleNeedsComponent::ApplyConsumableEffects(float NutritionValue, float HydrationValue, float HealthRestore)
{
	if (NutritionValue > 0.0f)
	{
		RestoreNeed(EIronvaleNeedType::Hunger, NutritionValue);
	}
	if (HydrationValue > 0.0f)
	{
		RestoreNeed(EIronvaleNeedType::Thirst, HydrationValue);
	}
	// Health restore is handled by the caller via HealthComponent
}

void UIronvaleNeedsComponent::Sleep(float GameHours, float BedQuality)
{
	// Fatigue restoration: quality 1.0 = full restore per 8 hours
	const float FatigueRestore = (GameHours / 8.0f) * 100.0f * FMath::Clamp(BedQuality, 0.1f, 2.0f);
	RestoreNeed(EIronvaleNeedType::Fatigue, FatigueRestore);

	// Sleeping also slightly increases hunger and thirst
	float* Hunger = NeedValues.Find(EIronvaleNeedType::Hunger);
	float* Thirst = NeedValues.Find(EIronvaleNeedType::Thirst);

	if (const FIronvaleNeedConfig* HungerConfig = NeedConfigs.Find(EIronvaleNeedType::Hunger))
	{
		if (Hunger) *Hunger = FMath::Max(0.0f, *Hunger - HungerConfig->DecayRatePerHour * GameHours);
	}
	if (const FIronvaleNeedConfig* ThirstConfig = NeedConfigs.Find(EIronvaleNeedType::Thirst))
	{
		if (Thirst) *Thirst = FMath::Max(0.0f, *Thirst - ThirstConfig->DecayRatePerHour * GameHours);
	}

	UE_LOG(LogIronvale, Log, TEXT("Slept for %.1f hours (bed quality %.1f). Fatigue restored by %.0f"),
		GameHours, BedQuality, FatigueRestore);
}

void UIronvaleNeedsComponent::Bathe()
{
	RestoreNeed(EIronvaleNeedType::Cleanliness, 100.0f);
	UE_LOG(LogIronvale, Log, TEXT("Player bathed — cleanliness restored"));
}

void UIronvaleNeedsComponent::SetNeedValue(EIronvaleNeedType NeedType, float Value)
{
	NeedValues.Add(NeedType, FMath::Clamp(Value, IronvaleConstants::NEED_MIN, IronvaleConstants::NEED_MAX));
}
