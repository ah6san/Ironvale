// =============================================================================
// IronvaleSaveGame.h — USaveGame subclass for persistent game state
// Project Ironvale
//
// The canonical save format for Ironvale. Contains all data needed to fully
// restore a game session: player state, world state, quest progress,
// reputation, and metadata.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Core/IronvaleSaveTypes.h"
#include "IronvaleSaveGame.generated.h"

/**
 * Unreal save game object for Project Ironvale.
 *
 * Serialized to disk via UGameplayStatics::SaveGameToSlot / LoadGameFromSlot.
 * All data uses the SaveGame UPROPERTY specifier for automatic serialization.
 *
 * The save system collects data from all subsystems (quest manager, reputation,
 * player state, world state) into this object before writing, and distributes
 * it back on load.
 */
UCLASS()
class IRONVALE_API UIronvaleSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UIronvaleSaveGame();

	// =========================================================================
	// SAVE DATA
	// =========================================================================

	/** Player character data: location, health, inventory, equipment, needs */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FIronvaleSavedPlayerData PlayerData;

	/** World state: time, weather, flags, NPCs, quests, reputations */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FIronvaleSavedWorldState WorldState;

	// =========================================================================
	// METADATA
	// =========================================================================

	/** Save slot metadata (displayed in load screen) */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FIronvaleSaveSlotInfo SlotInfo;

	/** Total play time in seconds at the time of save */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float PlayTimeSeconds = 0.0f;

	/** Save format version — used for migration when save format changes */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Save")
	int32 SaveVersion = 1;

	/** The current region the player was in at save time */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FName CurrentRegionID;

	// =========================================================================
	// HELPERS
	// =========================================================================

	/** Update the slot info metadata before saving */
	void UpdateSlotInfo(const FString& SlotName, const FString& PlayerLocation, float InPlayTime);

	/** Validate save data integrity after loading */
	bool ValidateSaveData() const;
};
