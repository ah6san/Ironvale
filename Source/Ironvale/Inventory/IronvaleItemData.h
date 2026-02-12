// =============================================================================
// IronvaleItemData.h — Item definition data (DataTable row + DataAsset)
// Project Ironvale
//
// FIronvaleItemData is the static definition of an item type — stats, visuals,
// behavior flags. It's loaded from DataTables and never mutated at runtime.
// Runtime state (durability, stack count) lives in UIronvaleItemInstance.
//
// Engine-agnostic note: FIronvaleItemData is a pure data struct. If porting
// to another engine, replace TSoftObjectPtr with equivalent asset references.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleEquipmentSlots.h"
#include "IronvaleItemData.generated.h"

/**
 * Static item definition — one row per item type in the DataTable.
 * This struct defines WHAT an item IS. Runtime instances track CURRENT STATE.
 */
USTRUCT(BlueprintType)
struct FIronvaleItemData : public FTableRowBase
{
	GENERATED_BODY()

	// --- Identity ---

	/** Unique item type identifier (e.g., "Longsword_Iron", "Bread_Wheat") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	FName ItemID;

	/** Localized display name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	FText DisplayName;

	/** Localized description for tooltip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	FText Description;

	/** Primary category for inventory filtering and slot validation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	EIronvaleItemCategory Category = EIronvaleItemCategory::Misc;

	/** Gameplay tags for flexible filtering, conditions, and special behaviors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Identity")
	FGameplayTagContainer Tags;

	// --- Visuals ---

	/** Icon texture for UI (soft ref to avoid loading all icons at once) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	/** World mesh for pickup/drop and equipped display */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visuals")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/** Skeletal mesh for first-person weapon view (weapons only) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Visuals")
	TSoftObjectPtr<USkeletalMesh> WeaponMesh;

	// --- Physical Properties ---

	/** Weight in kilograms — contributes to encumbrance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Physical", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	/** Maximum stack size (1 = non-stackable, e.g., weapons) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Physical", meta = (ClampMin = "1"))
	int32 MaxStack = 1;

	bool IsStackable() const { return MaxStack > 1; }

	/** Base gold value — modified by quality, durability, and reputation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Physical", meta = (ClampMin = "0"))
	int32 BaseValue = 10;

	// --- Durability ---

	/** Maximum durability (0 = indestructible, e.g., quest items) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Durability", meta = (ClampMin = "0.0"))
	float MaxDurability = 100.0f;

	bool HasDurability() const { return MaxDurability > 0.0f; }

	// --- Weapon Stats (if Category == Weapon) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon",
		meta = (EditCondition = "Category == EIronvaleItemCategory::Weapon"))
	EIronvaleWeaponType WeaponType = EIronvaleWeaponType::Fists;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon",
		meta = (EditCondition = "Category == EIronvaleItemCategory::Weapon"))
	EIronvaleDamageType PrimaryDamageType = EIronvaleDamageType::Slash;

	/** Base damage before modifiers */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (ClampMin = "0.0"))
	float BaseDamage = 10.0f;

	/** Attack speed multiplier (1.0 = normal, <1.0 = slower, >1.0 = faster) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (ClampMin = "0.1"))
	float AttackSpeed = 1.0f;

	/** Stamina cost per attack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (ClampMin = "0.0"))
	float StaminaCostPerAttack = 15.0f;

	/** Weapon reach in cm — affects trace length */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (ClampMin = "0.0"))
	float WeaponReach = 100.0f;

	/** Stagger power — how much stagger this weapon inflicts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Weapon", meta = (ClampMin = "0.0"))
	float StaggerPower = 10.0f;

	// --- Armor Stats (if Category == Armor) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Armor",
		meta = (EditCondition = "Category == EIronvaleItemCategory::Armor"))
	EIronvaleArmorType ArmorMaterial = EIronvaleArmorType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Armor",
		meta = (EditCondition = "Category == EIronvaleItemCategory::Armor"))
	EIronvaleArmorZone ArmorZone = EIronvaleArmorZone::Torso;

	/** Which equipment slot this armor fits into */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Armor",
		meta = (EditCondition = "Category == EIronvaleItemCategory::Armor"))
	EIronvaleEquipmentSlot EquipSlot = EIronvaleEquipmentSlot::Torso;

	/** Base armor rating — damage reduction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Armor", meta = (ClampMin = "0.0"))
	float ArmorRating = 0.0f;

	/** How much this armor slows stamina regeneration (0.0 = no effect, 1.0 = full block) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Armor",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaRegenPenalty = 0.0f;

	/** Noise level — affects stealth detection range */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Armor", meta = (ClampMin = "0.0"))
	float NoiseLevel = 0.0f;

	// --- Consumable Stats (if Category == Consumable) ---

	/** How much hunger this restores (0 = not food) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0"))
	float NutritionValue = 0.0f;

	/** How much thirst this restores (0 = not a drink) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0"))
	float HydrationValue = 0.0f;

	/** Health restored on consumption */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0"))
	float HealthRestore = 0.0f;

	/** Spoilage rate: hours until item loses freshness (0 = never spoils) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0"))
	float SpoilageRateHours = 0.0f;

	bool CanSpoil() const { return SpoilageRateHours > 0.0f; }

	// --- Quest Item ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Quest")
	bool bIsQuestItem = false;

	/** If true, item cannot be dropped or sold */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Quest")
	bool bIsEssential = false;
};

/**
 * DataAsset wrapper for item definitions that need to be referenced by
 * Blueprints or other assets directly (not via DataTable lookup).
 */
UCLASS(BlueprintType)
class IRONVALE_API UIronvaleItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FIronvaleItemData ItemData;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("IronvaleItem", ItemData.ItemID);
	}
};
