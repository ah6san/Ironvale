// =============================================================================
// IronvaleNeedsComponent.h — Player needs tracking (hunger, thirst, fatigue, cleanliness)
// Project Ironvale
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleNeedsTypes.h"
#include "IronvaleNeedsComponent.generated.h"

UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleNeedsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleNeedsComponent();

	virtual void BeginPlay() override;

	// =========================================================================
	// NEED QUERIES
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Needs")
	float GetNeedValue(EIronvaleNeedType NeedType) const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Needs")
	FIronvaleNeedStatus GetNeedStatus(EIronvaleNeedType NeedType) const;

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Needs")
	TArray<FIronvaleNeedStatus> GetAllNeedStatuses() const;

	/** Get the worst (most penalizing) active debuff multipliers */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Needs")
	float GetStaminaRegenMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Needs")
	float GetMovementSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Needs")
	float GetVisionBlurIntensity() const;

	// =========================================================================
	// NEED MODIFICATION
	// =========================================================================

	/** Restore a need by amount (eating, drinking, sleeping, bathing) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Needs")
	void RestoreNeed(EIronvaleNeedType NeedType, float Amount);

	/** Apply an item's nutrition/hydration effects */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Needs")
	void ApplyConsumableEffects(float NutritionValue, float HydrationValue, float HealthRestore);

	/** Sleep for a number of game-hours (restores fatigue based on bed quality) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Needs")
	void Sleep(float GameHours, float BedQuality = 1.0f);

	/** Bathe (restores cleanliness to max) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Needs")
	void Bathe();

	/** Force set a need value (for save/load) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Needs")
	void SetNeedValue(EIronvaleNeedType NeedType, float Value);

	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** DataTable containing need configurations (thresholds, decay rates) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Needs")
	UDataTable* NeedsConfigTable;

protected:
	/** Current values for each need type */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Needs")
	TMap<EIronvaleNeedType, float> NeedValues;

	/** Cached configurations from DataTable */
	TMap<EIronvaleNeedType, FIronvaleNeedConfig> NeedConfigs;

	/** Timer handle for periodic need updates */
	FTimerHandle NeedsUpdateTimerHandle;

	/** Called periodically to decay all needs */
	void UpdateNeeds();

	/** Check thresholds and apply/remove debuffs */
	void EvaluateThresholds(EIronvaleNeedType NeedType, float OldValue, float NewValue);

	/** Load configurations from DataTable */
	void LoadNeedConfigs();

	/** How often to update needs (real seconds). Maps to game-time via time scale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Needs")
	float UpdateIntervalSeconds = 5.0f;
};
