// =============================================================================
// IronvaleEquipmentComponent.cpp — Equipment slot manager implementation
// Project Ironvale
// =============================================================================

#include "Inventory/IronvaleEquipmentComponent.h"
#include "Inventory/IronvaleInventoryComponent.h"
#include "Ironvale.h"
#include "Core/IronvaleEventBus.h"
#include "Core/IronvaleGameplayTags.h"

UIronvaleEquipmentComponent::UIronvaleEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIronvaleEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	SlotInfos = IronvaleEquipmentSlotHelper::GetDefaultSlotInfos();
}

// =============================================================================
// EQUIP / UNEQUIP
// =============================================================================

bool UIronvaleEquipmentComponent::EquipItem(UIronvaleItemInstance* Item)
{
	if (!Item) return false;

	const EIronvaleEquipmentSlot Slot = DetermineSlotForItem(Item);
	if (Slot == EIronvaleEquipmentSlot::MAX) return false;

	return EquipItemToSlot(Item, Slot);
}

bool UIronvaleEquipmentComponent::EquipItemToSlot(UIronvaleItemInstance* Item, EIronvaleEquipmentSlot Slot)
{
	if (!Item || Slot == EIronvaleEquipmentSlot::MAX) return false;

	if (!IsItemValidForSlot(Item, Slot))
	{
		UE_LOG(LogIronvale, Warning, TEXT("Item %s cannot be equipped in slot %d"),
			*Item->GetItemID().ToString(), static_cast<int32>(Slot));
		return false;
	}

	// Unequip existing item in slot (if any)
	if (IsSlotOccupied(Slot))
	{
		UnequipSlot(Slot);
	}

	EquippedItems.Add(Slot, Item);
	BroadcastSlotChanged(Slot, Item);

	UE_LOG(LogIronvale, Verbose, TEXT("Equipped %s in slot %d"),
		*Item->GetItemID().ToString(), static_cast<int32>(Slot));

	return true;
}

UIronvaleItemInstance* UIronvaleEquipmentComponent::UnequipSlot(EIronvaleEquipmentSlot Slot)
{
	UIronvaleItemInstance** Found = EquippedItems.Find(Slot);
	if (!Found || !*Found) return nullptr;

	UIronvaleItemInstance* Item = *Found;
	EquippedItems.Remove(Slot);
	BroadcastSlotChanged(Slot, nullptr);

	UE_LOG(LogIronvale, Verbose, TEXT("Unequipped %s from slot %d"),
		*Item->GetItemID().ToString(), static_cast<int32>(Slot));

	return Item;
}

UIronvaleItemInstance* UIronvaleEquipmentComponent::GetItemInSlot(EIronvaleEquipmentSlot Slot) const
{
	const UIronvaleItemInstance* const* Found = EquippedItems.Find(Slot);
	return Found ? const_cast<UIronvaleItemInstance*>(*Found) : nullptr;
}

bool UIronvaleEquipmentComponent::IsSlotOccupied(EIronvaleEquipmentSlot Slot) const
{
	return GetItemInSlot(Slot) != nullptr;
}

// =============================================================================
// ARMOR QUERIES
// =============================================================================

TArray<FIronvaleArmorLayer> UIronvaleEquipmentComponent::GetArmorLayersForZone(EIronvaleArmorZone Zone) const
{
	TArray<FIronvaleArmorLayer> Layers;

	for (const auto& Pair : EquippedItems)
	{
		const UIronvaleItemInstance* Item = Pair.Value;
		if (!Item) continue;

		const FIronvaleItemData& Data = Item->GetItemData();
		if (Data.Category != EIronvaleItemCategory::Armor) continue;
		if (Data.ArmorZone != Zone) continue;

		FIronvaleArmorLayer Layer;
		Layer.ArmorType = Data.ArmorMaterial;
		Layer.ArmorRating = Data.ArmorRating;
		Layer.CurrentDurability = Item->GetCurrentDurability();
		Layer.MaxDurability = Data.MaxDurability;
		Layers.Add(Layer);
	}

	// Sort by armor type (cloth first, plate last) for layered damage calculation
	Layers.Sort([](const FIronvaleArmorLayer& A, const FIronvaleArmorLayer& B)
	{
		return static_cast<int32>(A.ArmorType) < static_cast<int32>(B.ArmorType);
	});

	return Layers;
}

float UIronvaleEquipmentComponent::GetTotalArmorRatingForZone(EIronvaleArmorZone Zone) const
{
	float Total = 0.0f;
	for (const FIronvaleArmorLayer& Layer : GetArmorLayersForZone(Zone))
	{
		Total += Layer.GetEffectiveRating();
	}
	return Total;
}

float UIronvaleEquipmentComponent::GetTotalEquippedWeight() const
{
	float Total = 0.0f;
	for (const auto& Pair : EquippedItems)
	{
		if (Pair.Value)
		{
			Total += Pair.Value->GetItemData().Weight;
		}
	}
	return Total;
}

float UIronvaleEquipmentComponent::GetTotalStaminaRegenPenalty() const
{
	float Total = 0.0f;
	for (const auto& Pair : EquippedItems)
	{
		if (Pair.Value && Pair.Value->GetItemData().Category == EIronvaleItemCategory::Armor)
		{
			Total += Pair.Value->GetItemData().StaminaRegenPenalty;
		}
	}
	return FMath::Clamp(Total, 0.0f, 0.9f); // Cap at 90% reduction
}

float UIronvaleEquipmentComponent::GetTotalNoiseLevel() const
{
	float Total = 0.0f;
	for (const auto& Pair : EquippedItems)
	{
		if (Pair.Value && Pair.Value->GetItemData().Category == EIronvaleItemCategory::Armor)
		{
			Total += Pair.Value->GetItemData().NoiseLevel;
		}
	}
	return Total;
}

// =============================================================================
// WEAPON QUERIES
// =============================================================================

UIronvaleItemInstance* UIronvaleEquipmentComponent::GetMainHandWeapon() const
{
	return GetItemInSlot(EIronvaleEquipmentSlot::MainHand);
}

UIronvaleItemInstance* UIronvaleEquipmentComponent::GetOffHandItem() const
{
	return GetItemInSlot(EIronvaleEquipmentSlot::OffHand);
}

bool UIronvaleEquipmentComponent::IsArmed() const
{
	return GetMainHandWeapon() != nullptr;
}

// =============================================================================
// APPEARANCE TAGS
// =============================================================================

FGameplayTagContainer UIronvaleEquipmentComponent::GetAppearanceTags() const
{
	FGameplayTagContainer Tags;

	if (IsArmed())
	{
		Tags.AddTag(IronvaleTags::Appearance_Armed);
	}

	// Check for full plate (head + torso + legs all plate)
	bool bHasPlateHead = false;
	bool bHasPlateTorso = false;
	bool bHasPlateLegs = false;

	for (const auto& Pair : EquippedItems)
	{
		const UIronvaleItemInstance* Item = Pair.Value;
		if (!Item) continue;
		const FIronvaleItemData& Data = Item->GetItemData();

		if (Data.Category == EIronvaleItemCategory::Armor && Data.ArmorMaterial == EIronvaleArmorType::Plate)
		{
			if (Pair.Key == EIronvaleEquipmentSlot::Head) bHasPlateHead = true;
			if (Pair.Key == EIronvaleEquipmentSlot::Torso) bHasPlateTorso = true;
			if (Pair.Key == EIronvaleEquipmentSlot::Legs) bHasPlateLegs = true;
		}

		// Check for noble or peasant clothing via item tags
		if (Data.Tags.HasTagExact(IronvaleTags::Appearance_NobleClothes))
		{
			Tags.AddTag(IronvaleTags::Appearance_NobleClothes);
		}
		if (Data.Tags.HasTagExact(IronvaleTags::Appearance_PeasantClothes))
		{
			Tags.AddTag(IronvaleTags::Appearance_PeasantClothes);
		}
		if (Data.Tags.HasTagExact(IronvaleTags::Appearance_Hooded))
		{
			Tags.AddTag(IronvaleTags::Appearance_Hooded);
		}
	}

	if (bHasPlateHead && bHasPlateTorso && bHasPlateLegs)
	{
		Tags.AddTag(IronvaleTags::Appearance_FullPlate);
	}

	return Tags;
}

// =============================================================================
// DURABILITY
// =============================================================================

void UIronvaleEquipmentComponent::DamageArmorInZone(EIronvaleArmorZone Zone, float DurabilityDamage)
{
	if (DurabilityDamage <= 0.0f) return;

	for (auto& Pair : EquippedItems)
	{
		UIronvaleItemInstance* Item = Pair.Value;
		if (!Item) continue;
		if (Item->GetItemData().Category != EIronvaleItemCategory::Armor) continue;
		if (Item->GetItemData().ArmorZone != Zone) continue;

		// Distribute damage across layers (outermost takes most)
		Item->DamageDurability(DurabilityDamage);
	}
}

void UIronvaleEquipmentComponent::DamageMainHandWeapon(float DurabilityDamage)
{
	if (UIronvaleItemInstance* Weapon = GetMainHandWeapon())
	{
		Weapon->DamageDurability(DurabilityDamage);
	}
}

// =============================================================================
// INTERNAL
// =============================================================================

EIronvaleEquipmentSlot UIronvaleEquipmentComponent::DetermineSlotForItem(const UIronvaleItemInstance* Item) const
{
	if (!Item) return EIronvaleEquipmentSlot::MAX;

	const FIronvaleItemData& Data = Item->GetItemData();

	if (Data.Category == EIronvaleItemCategory::Weapon)
	{
		return EIronvaleEquipmentSlot::MainHand;
	}
	else if (Data.Category == EIronvaleItemCategory::Armor)
	{
		return Data.EquipSlot;
	}

	return EIronvaleEquipmentSlot::MAX;
}

bool UIronvaleEquipmentComponent::IsItemValidForSlot(const UIronvaleItemInstance* Item, EIronvaleEquipmentSlot Slot) const
{
	if (!Item || Slot == EIronvaleEquipmentSlot::MAX) return false;

	const FIronvaleItemData& Data = Item->GetItemData();

	// Find slot info and check allowed categories
	for (const FIronvaleEquipmentSlotInfo& Info : SlotInfos)
	{
		if (Info.Slot == Slot)
		{
			return Info.AllowedCategories.Contains(Data.Category);
		}
	}

	return false;
}

void UIronvaleEquipmentComponent::BroadcastSlotChanged(EIronvaleEquipmentSlot Slot, UIronvaleItemInstance* NewItem)
{
	OnEquipmentSlotChanged.Broadcast(Slot, NewItem);

	// Also broadcast to global event bus
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		if (NewItem)
		{
			EventBus->OnItemEquipped.Broadcast(GetOwner(), NewItem->GetItemID(), static_cast<int32>(Slot));
		}
		else
		{
			EventBus->OnItemUnequipped.Broadcast(GetOwner(), NAME_None, static_cast<int32>(Slot));
		}
	}
}

// =============================================================================
// SAVE/LOAD
// =============================================================================

FIronvaleSavedEquipment UIronvaleEquipmentComponent::ToSaveData() const
{
	FIronvaleSavedEquipment Data;
	for (const auto& Pair : EquippedItems)
	{
		if (Pair.Value)
		{
			Data.SlotToItemID.Add(static_cast<int32>(Pair.Key), Pair.Value->GetInstanceID());
		}
	}
	return Data;
}

void UIronvaleEquipmentComponent::LoadFromSaveData(const FIronvaleSavedEquipment& SaveData, UIronvaleInventoryComponent* Inventory)
{
	EquippedItems.Empty();

	if (!Inventory)
	{
		UE_LOG(LogIronvale, Error, TEXT("Cannot load equipment: no inventory component"));
		return;
	}

	for (const auto& Pair : SaveData.SlotToItemID)
	{
		UIronvaleItemInstance* Item = Inventory->FindItemByGUID(Pair.Value);
		if (Item)
		{
			const EIronvaleEquipmentSlot Slot = static_cast<EIronvaleEquipmentSlot>(Pair.Key);
			EquippedItems.Add(Slot, Item);
		}
	}

	UE_LOG(LogIronvale, Log, TEXT("Loaded %d equipment slots from save"), EquippedItems.Num());
}
