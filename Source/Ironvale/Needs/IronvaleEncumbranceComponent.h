// =============================================================================
// IronvaleEncumbranceComponent.h — Weight-based movement penalties
// Project Ironvale
//
// Reads total weight from InventoryComponent + EquipmentComponent,
// applies soft/hard encumbrance thresholds to movement speed.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IronvaleEncumbranceComponent.generated.h"

class UIronvaleInventoryComponent;
class UIronvaleEquipmentComponent;

UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleEncumbranceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleEncumbranceComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Current speed multiplier from encumbrance (1.0 = no penalty) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Encumbrance")
	float GetSpeedMultiplier() const { return CurrentSpeedMultiplier; }

	/** Is the player over the hard cap (cannot sprint)? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Encumbrance")
	bool IsOverEncumbered() const { return bIsOverEncumbered; }

	/** Can the player sprint? (not over hard cap and has stamina) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Encumbrance")
	bool CanSprint() const { return !bIsOverEncumbered; }

protected:
	UPROPERTY()
	UIronvaleInventoryComponent* InventoryComp = nullptr;

	UPROPERTY()
	UIronvaleEquipmentComponent* EquipmentComp = nullptr;

	float CurrentSpeedMultiplier = 1.0f;
	bool bIsOverEncumbered = false;

	void RecalculateEncumbrance();
};
