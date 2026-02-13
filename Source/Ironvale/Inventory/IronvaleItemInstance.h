// =============================================================================
// IronvaleItemInstance.h — Runtime item instance with mutable state
// Project Ironvale
//
// While FIronvaleItemData defines WHAT an item type IS (static data),
// UIronvaleItemInstance tracks the CURRENT STATE of a specific item
// (durability, stack count, spoilage, unique ID for save references).
//
// Engine-agnostic note: This is a UObject for Unreal GC/serialization.
// In another engine, this would be a plain class with manual lifetime.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IronvaleItemData.h"
#include "Core/IronvaleSaveTypes.h"
#include "IronvaleItemInstance.generated.h"

UCLASS(BlueprintType)
class IRONVALE_API UIronvaleItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UIronvaleItemInstance();

	/** Initialize this instance from static item data */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Item")
	void InitFromData(const FIronvaleItemData& InData);

	// =========================================================================
	// ACCESSORS
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	const FIronvaleItemData& GetItemData() const { return ItemData; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	FName GetItemID() const { return ItemData.ItemID; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	FGuid GetInstanceID() const { return InstanceID; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	int32 GetStackCount() const { return StackCount; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	float GetCurrentDurability() const { return CurrentDurability; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	float GetDurabilityPercent() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	bool IsBroken() const { return ItemData.HasDurability() && CurrentDurability <= 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	EIronvaleItemQuality GetQuality() const { return Quality; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	float GetTotalWeight() const { return ItemData.Weight * StackCount; }

	/** Get the effective gold value (base * quality modifier * durability factor) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	int32 GetCurrentValue() const;

	// =========================================================================
	// STACK OPERATIONS
	// =========================================================================

	/** Add to stack. Returns the amount that couldn't fit (overflow). */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Item")
	int32 AddToStack(int32 Amount);

	/** Remove from stack. Returns actual amount removed. */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Item")
	int32 RemoveFromStack(int32 Amount);

	/** How many more can fit in this stack */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	int32 GetRemainingStackSpace() const;

	/** Can this instance merge with another (same item ID, stackable, space available) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	bool CanStackWith(const UIronvaleItemInstance* Other) const;

	// =========================================================================
	// DURABILITY
	// =========================================================================

	/** Apply durability damage. Returns true if item broke (reached 0). */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Item")
	bool DamageDurability(float Amount);

	/** Repair durability by amount (clamped to max) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Item")
	void RepairDurability(float Amount);

	/** Fully repair to max durability */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Item")
	void FullRepair();

	// =========================================================================
	// SPOILAGE (for consumables)
	// =========================================================================

	/** Update spoilage based on elapsed game hours. Returns true if item spoiled. */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Item")
	bool UpdateSpoilage(float ElapsedGameHours);

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	float GetFreshnessPercent() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Item")
	bool IsSpoiled() const { return bIsSpoiled; }

	// =========================================================================
	// SAVE/LOAD
	// =========================================================================

	/** Serialize to save-compatible struct */
	FIronvaleSavedItem ToSaveData() const;

	/** Restore from save data (requires DataTable lookup for ItemData) */
	void LoadFromSaveData(const FIronvaleSavedItem& SaveData, const FIronvaleItemData& InData);

protected:
	/** Static data — copied from DataTable on creation */
	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Item")
	FIronvaleItemData ItemData;

	/** Unique instance ID — persists across save/load */
	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Item")
	FGuid InstanceID;

	/** Current stack count */
	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Item")
	int32 StackCount = 1;

	/** Current durability */
	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Item")
	float CurrentDurability = 100.0f;

	/** Item quality tier */
	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Item")
	EIronvaleItemQuality Quality = EIronvaleItemQuality::Common;

	/** Accumulated spoilage time in game-hours */
	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Item")
	float AccumulatedSpoilageHours = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Item")
	bool bIsSpoiled = false;
};
