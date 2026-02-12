// =============================================================================
// IronvaleSaveGame.cpp — USaveGame implementation
// Project Ironvale
// =============================================================================

#include "World/IronvaleSaveGame.h"
#include "Ironvale.h"

UIronvaleSaveGame::UIronvaleSaveGame()
{
	// Set default values
	SaveVersion = 1;
	PlayTimeSeconds = 0.0f;
}

void UIronvaleSaveGame::UpdateSlotInfo(const FString& SlotName, const FString& PlayerLocation,
	float InPlayTime)
{
	SlotInfo.SlotName = SlotName;
	SlotInfo.SaveTimestamp = FDateTime::Now();
	SlotInfo.PlayerLocation = PlayerLocation;
	SlotInfo.DayCount = WorldState.DayCount;
	SlotInfo.PlayTimeSeconds = InPlayTime;
	PlayTimeSeconds = InPlayTime;
}

bool UIronvaleSaveGame::ValidateSaveData() const
{
	// Validate save version
	if (SaveVersion < 1)
	{
		UE_LOG(LogIronvale, Warning, TEXT("Invalid save version: %d"), SaveVersion);
		return false;
	}

	// Validate player health is reasonable
	if (PlayerData.Health < 0.0f)
	{
		UE_LOG(LogIronvale, Warning, TEXT("Invalid player health: %.1f"), PlayerData.Health);
		return false;
	}

	if (PlayerData.MaxHealth <= 0.0f)
	{
		UE_LOG(LogIronvale, Warning, TEXT("Invalid max health: %.1f"), PlayerData.MaxHealth);
		return false;
	}

	// Validate game time is in range
	if (WorldState.GameTimeHours < 0.0f || WorldState.GameTimeHours >= 24.0f)
	{
		UE_LOG(LogIronvale, Warning, TEXT("Invalid game time: %.2f hours"),
			WorldState.GameTimeHours);
		return false;
	}

	// Validate day count
	if (WorldState.DayCount < 1)
	{
		UE_LOG(LogIronvale, Warning, TEXT("Invalid day count: %d"), WorldState.DayCount);
		return false;
	}

	// Validate play time is non-negative
	if (PlayTimeSeconds < 0.0f)
	{
		UE_LOG(LogIronvale, Warning, TEXT("Invalid play time: %.1f seconds"), PlayTimeSeconds);
		return false;
	}

	// Validate inventory items have valid IDs
	for (const FIronvaleSavedItem& Item : PlayerData.InventoryItems)
	{
		if (Item.ItemID.IsNone())
		{
			UE_LOG(LogIronvale, Warning, TEXT("Inventory item with empty ItemID found"));
			return false;
		}
	}

	// Validate quest states
	for (const FIronvaleSavedQuest& Quest : WorldState.Quests)
	{
		if (Quest.QuestID.IsNone())
		{
			UE_LOG(LogIronvale, Warning, TEXT("Quest with empty QuestID found in save"));
			return false;
		}
	}

	UE_LOG(LogIronvale, Verbose, TEXT("Save data validation passed (v%d, day %d, %.1fs playtime)"),
		SaveVersion, WorldState.DayCount, PlayTimeSeconds);

	return true;
}
