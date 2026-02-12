// =============================================================================
// IronvaleInventoryComponent.h — Inventory container component
// Project Ironvale
//
// Attached to any actor that holds items (player, NPCs, chests, corpses).
// Manages item instances, weight tracking, and broadcasts changes to EventBus.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IronvaleItemInstance.h"
#include "IronvaleInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedSignature);

UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleInventoryComponent();

	virtual void BeginPlay() override;

	// =========================================================================
	// ITEM OPERATIONS
	// =========================================================================

	/**
	 * Add an item to inventory by item data. Creates a new instance.
	 * Returns the created instance, or nullptr if inventory is full.
	 * Handles stacking automatically for stackable items.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Inventory")
	UIronvaleItemInstance* AddItem(const FIronvaleItemData& ItemData, int32 Count = 1);

	/**
	 * Add an existing item instance (e.g., picked up from world).
	 * Returns true if the item was added successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Inventory")
	bool AddItemInstance(UIronvaleItemInstance* Instance);

	/**
	 * Remove a specific number of items by ID.
	 * Returns the actual number removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Inventory")
	int32 RemoveItemByID(FName ItemID, int32 Count = 1);

	/**
	 * Remove a specific item instance from inventory.
	 * Returns true if found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Inventory")
	bool RemoveItemInstance(UIronvaleItemInstance* Instance);

	/** Check if inventory contains at least Count of the given item ID */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	bool HasItem(FName ItemID, int32 Count = 1) const;

	/** Get total count of a specific item ID across all stacks */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	int32 GetItemCount(FName ItemID) const;

	/** Find the first instance of an item by ID (or nullptr) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	UIronvaleItemInstance* FindItemByID(FName ItemID) const;

	/** Find an item instance by its unique GUID */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	UIronvaleItemInstance* FindItemByGUID(const FGuid& GUID) const;

	/** Get all items in inventory */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	const TArray<UIronvaleItemInstance*>& GetAllItems() const { return Items; }

	/** Get items filtered by category */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Inventory")
	TArray<UIronvaleItemInstance*> GetItemsByCategory(EIronvaleItemCategory Category) const;

	// =========================================================================
	// WEIGHT / ENCUMBRANCE
	// =========================================================================

	/** Current total weight of all items */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	float GetCurrentWeight() const { return CurrentWeight; }

	/** Maximum carry weight (can be modified by stats, buffs, etc.) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	float GetMaxWeight() const { return MaxCarryWeight; }

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Inventory")
	void SetMaxWeight(float NewMax) { MaxCarryWeight = FMath::Max(0.0f, NewMax); }

	/** Would adding this weight exceed the hard carry limit? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	bool WouldExceedWeight(float AdditionalWeight) const;

	/** Current weight as ratio of max (0.0 - 1.0+) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	float GetWeightRatio() const;

	// =========================================================================
	// GOLD
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Inventory")
	int32 GetGold() const { return Gold; }

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Inventory")
	void AddGold(int32 Amount);

	/** Remove gold. Returns false if insufficient funds. */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Inventory")
	bool RemoveGold(int32 Amount);

	// =========================================================================
	// SAVE/LOAD
	// =========================================================================

	TArray<FIronvaleSavedItem> ToSaveData() const;
	void LoadFromSaveData(const TArray<FIronvaleSavedItem>& SavedItems, UDataTable* ItemDataTable);

	// =========================================================================
	// DELEGATES
	// =========================================================================

	/** Fired whenever inventory contents change (add, remove, stack change) */
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Inventory")
	FOnInventoryChangedSignature OnInventoryChanged;

protected:
	UPROPERTY()
	TArray<UIronvaleItemInstance*> Items;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Inventory")
	float MaxCarryWeight = IronvaleConstants::MAX_CARRY_WEIGHT;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Inventory")
	float CurrentWeight = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Inventory")
	int32 Gold = 0;

	void RecalculateWeight();
	void NotifyInventoryChanged();

	/** Try to merge item into an existing stack. Returns remaining count. */
	int32 TryMergeIntoExistingStack(const FIronvaleItemData& ItemData, int32 Count);
};
