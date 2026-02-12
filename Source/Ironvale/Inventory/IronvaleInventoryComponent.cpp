// =============================================================================
// IronvaleInventoryComponent.cpp — Inventory container implementation
// Project Ironvale
// =============================================================================

#include "Inventory/IronvaleInventoryComponent.h"
#include "Ironvale.h"
#include "Core/IronvaleEventBus.h"

UIronvaleInventoryComponent::UIronvaleInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIronvaleInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	RecalculateWeight();
}

// =============================================================================
// ITEM OPERATIONS
// =============================================================================

UIronvaleItemInstance* UIronvaleInventoryComponent::AddItem(const FIronvaleItemData& ItemData, int32 Count)
{
	if (Count <= 0) return nullptr;

	// Check weight limit
	const float AddedWeight = ItemData.Weight * Count;
	if (WouldExceedWeight(AddedWeight))
	{
		UE_LOG(LogIronvale, Warning, TEXT("Cannot add %s x%d: would exceed carry weight"),
			*ItemData.ItemID.ToString(), Count);
		return nullptr;
	}

	// Try to merge into existing stacks first
	int32 Remaining = Count;
	if (ItemData.IsStackable())
	{
		Remaining = TryMergeIntoExistingStack(ItemData, Remaining);
	}

	UIronvaleItemInstance* LastCreated = nullptr;

	// Create new instances for any remaining
	while (Remaining > 0)
	{
		UIronvaleItemInstance* NewInstance = NewObject<UIronvaleItemInstance>(this);
		NewInstance->InitFromData(ItemData);

		const int32 ToAdd = FMath::Min(Remaining, ItemData.MaxStack);
		if (ToAdd > 1)
		{
			NewInstance->AddToStack(ToAdd - 1); // InitFromData sets count to 1
		}
		Remaining -= ToAdd;

		Items.Add(NewInstance);
		LastCreated = NewInstance;
	}

	RecalculateWeight();
	NotifyInventoryChanged();

	// Broadcast to event bus
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnItemPickedUp.Broadcast(GetOwner(), ItemData.ItemID);
	}

	return LastCreated;
}

bool UIronvaleInventoryComponent::AddItemInstance(UIronvaleItemInstance* Instance)
{
	if (!Instance) return false;

	if (WouldExceedWeight(Instance->GetTotalWeight()))
	{
		return false;
	}

	// Try to stack if possible
	if (Instance->GetItemData().IsStackable())
	{
		int32 Remaining = TryMergeIntoExistingStack(Instance->GetItemData(), Instance->GetStackCount());
		if (Remaining <= 0)
		{
			RecalculateWeight();
			NotifyInventoryChanged();
			return true;
		}
		// Adjust the instance's stack count to the remainder
		Instance->RemoveFromStack(Instance->GetStackCount() - Remaining);
	}

	Items.Add(Instance);
	RecalculateWeight();
	NotifyInventoryChanged();

	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnItemPickedUp.Broadcast(GetOwner(), Instance->GetItemID());
	}

	return true;
}

int32 UIronvaleInventoryComponent::RemoveItemByID(FName ItemID, int32 Count)
{
	if (Count <= 0) return 0;

	int32 Removed = 0;

	for (int32 i = Items.Num() - 1; i >= 0 && Removed < Count; --i)
	{
		UIronvaleItemInstance* Item = Items[i];
		if (!Item || Item->GetItemID() != ItemID) continue;

		const int32 ToRemove = FMath::Min(Count - Removed, Item->GetStackCount());
		Item->RemoveFromStack(ToRemove);
		Removed += ToRemove;

		// Remove empty stacks
		if (Item->GetStackCount() <= 0)
		{
			Items.RemoveAt(i);
		}
	}

	if (Removed > 0)
	{
		RecalculateWeight();
		NotifyInventoryChanged();

		if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
		{
			EventBus->OnItemDropped.Broadcast(GetOwner(), ItemID);
		}
	}

	return Removed;
}

bool UIronvaleInventoryComponent::RemoveItemInstance(UIronvaleItemInstance* Instance)
{
	if (!Instance) return false;

	const int32 Index = Items.IndexOfByKey(Instance);
	if (Index == INDEX_NONE) return false;

	const FName ItemID = Instance->GetItemID();
	Items.RemoveAt(Index);

	RecalculateWeight();
	NotifyInventoryChanged();

	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnItemDropped.Broadcast(GetOwner(), ItemID);
	}

	return true;
}

bool UIronvaleInventoryComponent::HasItem(FName ItemID, int32 Count) const
{
	return GetItemCount(ItemID) >= Count;
}

int32 UIronvaleInventoryComponent::GetItemCount(FName ItemID) const
{
	int32 Total = 0;
	for (const UIronvaleItemInstance* Item : Items)
	{
		if (Item && Item->GetItemID() == ItemID)
		{
			Total += Item->GetStackCount();
		}
	}
	return Total;
}

UIronvaleItemInstance* UIronvaleInventoryComponent::FindItemByID(FName ItemID) const
{
	for (UIronvaleItemInstance* Item : Items)
	{
		if (Item && Item->GetItemID() == ItemID)
		{
			return Item;
		}
	}
	return nullptr;
}

UIronvaleItemInstance* UIronvaleInventoryComponent::FindItemByGUID(const FGuid& GUID) const
{
	for (UIronvaleItemInstance* Item : Items)
	{
		if (Item && Item->GetInstanceID() == GUID)
		{
			return Item;
		}
	}
	return nullptr;
}

TArray<UIronvaleItemInstance*> UIronvaleInventoryComponent::GetItemsByCategory(EIronvaleItemCategory Category) const
{
	TArray<UIronvaleItemInstance*> Result;
	for (UIronvaleItemInstance* Item : Items)
	{
		if (Item && Item->GetItemData().Category == Category)
		{
			Result.Add(Item);
		}
	}
	return Result;
}

// =============================================================================
// WEIGHT
// =============================================================================

bool UIronvaleInventoryComponent::WouldExceedWeight(float AdditionalWeight) const
{
	return (CurrentWeight + AdditionalWeight) > MaxCarryWeight;
}

float UIronvaleInventoryComponent::GetWeightRatio() const
{
	if (MaxCarryWeight <= 0.0f) return 0.0f;
	return CurrentWeight / MaxCarryWeight;
}

void UIronvaleInventoryComponent::RecalculateWeight()
{
	CurrentWeight = 0.0f;
	for (const UIronvaleItemInstance* Item : Items)
	{
		if (Item)
		{
			CurrentWeight += Item->GetTotalWeight();
		}
	}
}

// =============================================================================
// GOLD
// =============================================================================

void UIronvaleInventoryComponent::AddGold(int32 Amount)
{
	if (Amount > 0)
	{
		Gold += Amount;
		NotifyInventoryChanged();
	}
}

bool UIronvaleInventoryComponent::RemoveGold(int32 Amount)
{
	if (Amount <= 0) return true;
	if (Gold < Amount) return false;

	Gold -= Amount;
	NotifyInventoryChanged();
	return true;
}

// =============================================================================
// INTERNAL
// =============================================================================

void UIronvaleInventoryComponent::NotifyInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}

int32 UIronvaleInventoryComponent::TryMergeIntoExistingStack(const FIronvaleItemData& ItemData, int32 Count)
{
	int32 Remaining = Count;

	for (UIronvaleItemInstance* Existing : Items)
	{
		if (Remaining <= 0) break;
		if (!Existing || Existing->GetItemID() != ItemData.ItemID) continue;
		if (Existing->GetRemainingStackSpace() <= 0) continue;

		const int32 Added = Existing->GetRemainingStackSpace();
		const int32 ToAdd = FMath::Min(Remaining, Added);
		Existing->AddToStack(ToAdd);
		Remaining -= ToAdd;
	}

	return Remaining;
}

// =============================================================================
// SAVE/LOAD
// =============================================================================

TArray<FIronvaleSavedItem> UIronvaleInventoryComponent::ToSaveData() const
{
	TArray<FIronvaleSavedItem> Result;
	for (const UIronvaleItemInstance* Item : Items)
	{
		if (Item)
		{
			Result.Add(Item->ToSaveData());
		}
	}
	return Result;
}

void UIronvaleInventoryComponent::LoadFromSaveData(const TArray<FIronvaleSavedItem>& SavedItems, UDataTable* ItemDataTable)
{
	Items.Empty();

	if (!ItemDataTable)
	{
		UE_LOG(LogIronvale, Error, TEXT("Cannot load inventory: no ItemDataTable provided"));
		return;
	}

	for (const FIronvaleSavedItem& Saved : SavedItems)
	{
		// Look up static data from DataTable
		const FIronvaleItemData* Data = ItemDataTable->FindRow<FIronvaleItemData>(Saved.ItemID, TEXT("LoadInventory"));
		if (!Data)
		{
			UE_LOG(LogIronvale, Warning, TEXT("Item ID '%s' not found in DataTable, skipping"),
				*Saved.ItemID.ToString());
			continue;
		}

		UIronvaleItemInstance* Instance = NewObject<UIronvaleItemInstance>(this);
		Instance->LoadFromSaveData(Saved, *Data);
		Items.Add(Instance);
	}

	RecalculateWeight();
	NotifyInventoryChanged();

	UE_LOG(LogIronvale, Log, TEXT("Loaded %d items from save data"), Items.Num());
}
