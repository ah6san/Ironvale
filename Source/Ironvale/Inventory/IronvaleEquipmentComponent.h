// =============================================================================
// IronvaleEquipmentComponent.h — Equipment slot manager
// Project Ironvale
//
// Manages equipped items (weapons, armor, rings, etc.) in specific slots.
// Works alongside InventoryComponent — equipping moves an item from inventory
// to an equipment slot; unequipping moves it back.
//
// Provides aggregated armor data per zone for the combat damage system.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IronvaleEquipmentSlots.h"
#include "IronvaleItemInstance.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleEquipmentComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentSlotChangedSignature,
	EIronvaleEquipmentSlot, Slot, UIronvaleItemInstance*, NewItem);

UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleEquipmentComponent();

	virtual void BeginPlay() override;

	// =========================================================================
	// EQUIP / UNEQUIP
	// =========================================================================

	/**
	 * Equip an item to the appropriate slot.
	 * The slot is determined by the item's EquipSlot (armor) or MainHand (weapon).
	 * Returns true if equipped successfully. If the slot is occupied, the existing
	 * item is unequipped first (returned to inventory).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Equipment")
	bool EquipItem(UIronvaleItemInstance* Item);

	/**
	 * Equip an item to a specific slot (override auto-detection).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Equipment")
	bool EquipItemToSlot(UIronvaleItemInstance* Item, EIronvaleEquipmentSlot Slot);

	/**
	 * Unequip the item in the given slot. Returns the item, or nullptr if slot was empty.
	 * The item is NOT automatically returned to inventory — caller must do that.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Equipment")
	UIronvaleItemInstance* UnequipSlot(EIronvaleEquipmentSlot Slot);

	/** Get the item currently in a slot (or nullptr) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	UIronvaleItemInstance* GetItemInSlot(EIronvaleEquipmentSlot Slot) const;

	/** Check if a slot has an item equipped */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	bool IsSlotOccupied(EIronvaleEquipmentSlot Slot) const;

	// =========================================================================
	// ARMOR QUERIES (for Combat system)
	// =========================================================================

	/**
	 * Get all armor layers protecting a specific body zone.
	 * Used by DamageCalculator to determine damage reduction.
	 * A zone can be protected by multiple items (e.g., mail torso + plate torso + cloak).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Equipment")
	TArray<FIronvaleArmorLayer> GetArmorLayersForZone(EIronvaleArmorZone Zone) const;

	/**
	 * Get total armor rating for a zone (sum of all layer effective ratings).
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	float GetTotalArmorRatingForZone(EIronvaleArmorZone Zone) const;

	/** Get total weight of all equipped items */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	float GetTotalEquippedWeight() const;

	/** Get total stamina regen penalty from all equipped armor */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	float GetTotalStaminaRegenPenalty() const;

	/** Get total noise level from all equipped armor (for stealth) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	float GetTotalNoiseLevel() const;

	// =========================================================================
	// WEAPON QUERIES (for Combat system)
	// =========================================================================

	/** Get the currently equipped main-hand weapon (or nullptr if unarmed) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	UIronvaleItemInstance* GetMainHandWeapon() const;

	/** Get the off-hand item (shield, second weapon, or nullptr) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	UIronvaleItemInstance* GetOffHandItem() const;

	/** Is the character currently armed? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Equipment")
	bool IsArmed() const;

	// =========================================================================
	// APPEARANCE TAGS (for Dialogue/NPC reaction checks)
	// =========================================================================

	/**
	 * Get gameplay tags describing the character's current appearance
	 * based on equipped items (e.g., "Appearance.FullPlate", "Appearance.Armed").
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Equipment")
	FGameplayTagContainer GetAppearanceTags() const;

	// =========================================================================
	// DURABILITY
	// =========================================================================

	/**
	 * Apply durability damage to armor in a specific zone (called after taking a hit).
	 * Distributes damage across armor layers in the zone.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Equipment")
	void DamageArmorInZone(EIronvaleArmorZone Zone, float DurabilityDamage);

	/**
	 * Apply durability damage to the main-hand weapon (called after attacking).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Equipment")
	void DamageMainHandWeapon(float DurabilityDamage);

	// =========================================================================
	// DELEGATES
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Equipment")
	FOnEquipmentSlotChangedSignature OnEquipmentSlotChanged;

	// =========================================================================
	// SAVE/LOAD
	// =========================================================================

	FIronvaleSavedEquipment ToSaveData() const;
	void LoadFromSaveData(const FIronvaleSavedEquipment& SaveData, UIronvaleInventoryComponent* Inventory);

protected:
	/** Map from slot enum to equipped item instance */
	UPROPERTY()
	TMap<EIronvaleEquipmentSlot, UIronvaleItemInstance*> EquippedItems;

	/** Cached slot info for validation */
	TArray<FIronvaleEquipmentSlotInfo> SlotInfos;

	/** Get the best slot for an item based on its data */
	EIronvaleEquipmentSlot DetermineSlotForItem(const UIronvaleItemInstance* Item) const;

	/** Check if an item is valid for a given slot */
	bool IsItemValidForSlot(const UIronvaleItemInstance* Item, EIronvaleEquipmentSlot Slot) const;

	void BroadcastSlotChanged(EIronvaleEquipmentSlot Slot, UIronvaleItemInstance* NewItem);
};
