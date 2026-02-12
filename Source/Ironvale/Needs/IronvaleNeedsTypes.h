// =============================================================================
// IronvaleNeedsTypes.h — Needs system data structures
// Project Ironvale
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleNeedsTypes.generated.h"

/**
 * A threshold that triggers a debuff when a need drops below it.
 */
USTRUCT(BlueprintType)
struct FIronvaleNeedThreshold
{
	GENERATED_BODY()

	/** Need value at which this threshold activates (e.g., 25 = "Hungry") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float ThresholdValue = 25.0f;

	/** Gameplay tag applied as debuff when below threshold */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs")
	FGameplayTag DebuffTag;

	/** Human-readable description (for UI) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs")
	FText Description;

	/** Stamina regen multiplier when this debuff is active (1.0 = no change, 0.5 = halved) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float StaminaRegenMultiplier = 1.0f;

	/** Movement speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MovementSpeedMultiplier = 1.0f;

	/** Health drain per game-hour when this debuff is active (0 = none) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs", meta = (ClampMin = "0.0"))
	float HealthDrainPerHour = 0.0f;

	/** Vision blur intensity (0 = none, 1 = heavy blur) — for post-process effect */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VisionBlurIntensity = 0.0f;
};

/**
 * Configuration for a single need type — loaded from DataTable.
 */
USTRUCT(BlueprintType)
struct FIronvaleNeedConfig : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs")
	EIronvaleNeedType NeedType = EIronvaleNeedType::Hunger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs")
	FText DisplayName;

	/** Rate at which this need decays per game-hour */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs", meta = (ClampMin = "0.0"))
	float DecayRatePerHour = 4.0f;

	/** Thresholds (sorted high to low) that trigger debuffs */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs")
	TArray<FIronvaleNeedThreshold> Thresholds;

	/** Starting value (100 = fully satisfied) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Needs", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float StartValue = 100.0f;
};
