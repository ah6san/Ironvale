// =============================================================================
// IronvaleItemInstance.cpp — Runtime item instance implementation
// Project Ironvale
// =============================================================================

#include "Inventory/IronvaleItemInstance.h"
#include "Ironvale.h"
#include "Core/IronvaleSaveTypes.h"

UIronvaleItemInstance::UIronvaleItemInstance()
{
	InstanceID = FGuid::NewGuid();
}

void UIronvaleItemInstance::InitFromData(const FIronvaleItemData& InData)
{
	ItemData = InData;
	CurrentDurability = InData.MaxDurability;
	StackCount = 1;
	Quality = EIronvaleItemQuality::Common;
	AccumulatedSpoilageHours = 0.0f;
	bIsSpoiled = false;
}

// =============================================================================
// VALUE CALCULATIONS
// =============================================================================

float UIronvaleItemInstance::GetDurabilityPercent() const
{
	if (!ItemData.HasDurability()) return 1.0f;
	if (ItemData.MaxDurability <= 0.0f) return 1.0f;
	return FMath::Clamp(CurrentDurability / ItemData.MaxDurability, 0.0f, 1.0f);
}

int32 UIronvaleItemInstance::GetCurrentValue() const
{
	float Value = static_cast<float>(ItemData.BaseValue);

	// Quality multiplier
	static const float QualityMultipliers[] = {
		0.1f,  // Broken
		0.5f,  // Poor
		1.0f,  // Common
		1.5f,  // Fine
		2.0f,  // Superior
		3.0f   // Masterwork
	};
	const int32 QualIdx = static_cast<int32>(Quality);
	if (QualIdx >= 0 && QualIdx < UE_ARRAY_COUNT(QualityMultipliers))
	{
		Value *= QualityMultipliers[QualIdx];
	}

	// Durability factor: broken items are worth much less
	if (ItemData.HasDurability())
	{
		Value *= FMath::Max(0.1f, GetDurabilityPercent());
	}

	// Spoiled consumables are nearly worthless
	if (bIsSpoiled)
	{
		Value *= 0.05f;
	}

	return FMath::Max(1, FMath::RoundToInt32(Value));
}

// =============================================================================
// STACK OPERATIONS
// =============================================================================

int32 UIronvaleItemInstance::AddToStack(int32 Amount)
{
	if (Amount <= 0) return 0;

	const int32 Space = GetRemainingStackSpace();
	const int32 ToAdd = FMath::Min(Amount, Space);
	StackCount += ToAdd;

	// Return overflow
	return Amount - ToAdd;
}

int32 UIronvaleItemInstance::RemoveFromStack(int32 Amount)
{
	if (Amount <= 0) return 0;

	const int32 ToRemove = FMath::Min(Amount, StackCount);
	StackCount -= ToRemove;

	return ToRemove;
}

int32 UIronvaleItemInstance::GetRemainingStackSpace() const
{
	return FMath::Max(0, ItemData.MaxStack - StackCount);
}

bool UIronvaleItemInstance::CanStackWith(const UIronvaleItemInstance* Other) const
{
	if (!Other) return false;
	if (!ItemData.IsStackable()) return false;
	if (ItemData.ItemID != Other->ItemData.ItemID) return false;
	if (GetRemainingStackSpace() <= 0) return false;
	// Don't stack items with different spoilage states
	if (bIsSpoiled != Other->bIsSpoiled) return false;
	return true;
}

// =============================================================================
// DURABILITY
// =============================================================================

bool UIronvaleItemInstance::DamageDurability(float Amount)
{
	if (!ItemData.HasDurability()) return false;
	if (Amount <= 0.0f) return false;

	// Unbreakable items tagged via gameplay tags
	if (ItemData.Tags.HasTagExact(FGameplayTag::RequestGameplayTag(FName("Item.Special.Unbreakable"))))
	{
		return false;
	}

	CurrentDurability = FMath::Max(0.0f, CurrentDurability - Amount);

	if (CurrentDurability <= 0.0f)
	{
		UE_LOG(LogIronvale, Log, TEXT("Item %s broke!"), *ItemData.ItemID.ToString());
		return true;
	}

	return false;
}

void UIronvaleItemInstance::RepairDurability(float Amount)
{
	if (!ItemData.HasDurability()) return;
	CurrentDurability = FMath::Min(ItemData.MaxDurability, CurrentDurability + Amount);
}

void UIronvaleItemInstance::FullRepair()
{
	if (!ItemData.HasDurability()) return;
	CurrentDurability = ItemData.MaxDurability;
}

// =============================================================================
// SPOILAGE
// =============================================================================

bool UIronvaleItemInstance::UpdateSpoilage(float ElapsedGameHours)
{
	if (!ItemData.CanSpoil() || bIsSpoiled) return false;

	AccumulatedSpoilageHours += ElapsedGameHours;

	if (AccumulatedSpoilageHours >= ItemData.SpoilageRateHours)
	{
		bIsSpoiled = true;
		UE_LOG(LogIronvale, Verbose, TEXT("Item %s has spoiled"), *ItemData.ItemID.ToString());
		return true;
	}

	return false;
}

float UIronvaleItemInstance::GetFreshnessPercent() const
{
	if (!ItemData.CanSpoil()) return 1.0f;
	if (bIsSpoiled) return 0.0f;
	return FMath::Clamp(1.0f - (AccumulatedSpoilageHours / ItemData.SpoilageRateHours), 0.0f, 1.0f);
}

// =============================================================================
// SAVE/LOAD
// =============================================================================

FIronvaleSavedItem UIronvaleItemInstance::ToSaveData() const
{
	FIronvaleSavedItem Data;
	Data.ItemID = ItemData.ItemID;
	Data.StackCount = StackCount;
	Data.CurrentDurability = CurrentDurability;
	Data.Quality = Quality;
	Data.InstanceID = InstanceID;
	return Data;
}

void UIronvaleItemInstance::LoadFromSaveData(const FIronvaleSavedItem& SaveData, const FIronvaleItemData& InData)
{
	ItemData = InData;
	InstanceID = SaveData.InstanceID;
	StackCount = SaveData.StackCount;
	CurrentDurability = SaveData.CurrentDurability;
	Quality = SaveData.Quality;
}
