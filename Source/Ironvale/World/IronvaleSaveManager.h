// =============================================================================
// IronvaleSaveManager.h — Save/load orchestration subsystem
// Project Ironvale
//
// Game instance subsystem that coordinates saving and loading. Gathers state
// from all game systems, writes to UIronvaleSaveGame, and serializes via
// UGameplayStatics. Supports multiple manual slots, auto-save, and quicksave.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/IronvaleSaveTypes.h"
#include "IronvaleSaveManager.generated.h"

class UIronvaleSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleSaveCompleteSignature,
	bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleLoadCompleteSignature,
	bool, bSuccess);

/**
 * Save manager subsystem — coordinates save/load across all game systems.
 *
 * Slot layout:
 *   "Save_0" through "Save_N"  — manual save slots
 *   "AutoSave"                 — automatic save (overwritten periodically)
 *   "QuickSave"                — quicksave slot (single)
 *
 * Save flow:
 *   1. Gather player data (location, health, inventory, equipment, needs)
 *   2. Gather world data (time, weather, flags, reputation)
 *   3. Gather quest data from QuestSubsystem
 *   4. Gather NPC states from important/essential NPCs
 *   5. Write all to UIronvaleSaveGame and serialize to slot
 *
 * Load flow:
 *   1. Deserialize UIronvaleSaveGame from slot
 *   2. Validate save version and apply migrations if needed
 *   3. Restore world state (time, weather, flags)
 *   4. Restore player data (teleport, set health/stamina, rebuild inventory)
 *   5. Restore quest data via QuestSubsystem::ImportQuestStates
 *   6. Restore NPC states
 */
UCLASS()
class IRONVALE_API UIronvaleSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// =========================================================================
	// SAVE OPERATIONS
	// =========================================================================

	/**
	 * Save to a named slot. Gathers all game state and serializes.
	 * @param SlotName  Slot identifier (e.g., "Save_0", "AutoSave", "QuickSave")
	 * @return true if the save succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Save")
	bool SaveToSlot(const FString& SlotName);

	/** Quicksave shortcut */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Save")
	bool QuickSave();

	/** Auto-save shortcut */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Save")
	bool AutoSave();

	// =========================================================================
	// LOAD OPERATIONS
	// =========================================================================

	/**
	 * Load from a named slot. Deserializes and restores all game state.
	 * @param SlotName  Slot identifier
	 * @return true if the load succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Save")
	bool LoadFromSlot(const FString& SlotName);

	/** Quickload shortcut */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Save")
	bool QuickLoad();

	// =========================================================================
	// SLOT MANAGEMENT
	// =========================================================================

	/** Check if a save slot exists */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Save")
	bool DoesSaveExist(const FString& SlotName) const;

	/** Delete a save slot */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Save")
	bool DeleteSave(const FString& SlotName);

	/** Get metadata for a save slot (without loading the full save) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Save")
	bool GetSlotInfo(const FString& SlotName, FIronvaleSaveSlotInfo& OutInfo);

	/** Get metadata for all save slots that exist */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Save")
	TArray<FIronvaleSaveSlotInfo> GetAllSaveSlotInfos();

	/** Maximum number of manual save slots */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ironvale|Save")
	int32 MaxManualSlots = 10;

	// =========================================================================
	// AUTO-SAVE CONFIGURATION
	// =========================================================================

	/** Interval in game-hours between auto-saves (0 = disabled) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Save")
	float AutoSaveIntervalHours = 1.0f;

	/** Whether auto-save is enabled */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Save")
	bool bAutoSaveEnabled = true;

	// =========================================================================
	// DELEGATES
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Save")
	FOnIronvaleSaveCompleteSignature OnSaveComplete;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Save")
	FOnIronvaleLoadCompleteSignature OnLoadComplete;

protected:
	/** The current save game version we write */
	static constexpr int32 CurrentSaveVersion = 1;

	/** User index for save/load (always 0 for single-player) */
	static constexpr int32 UserIndex = 0;

	/** Prefix for manual save slot names */
	static const FString ManualSlotPrefix;
	static const FString AutoSaveSlotName;
	static const FString QuickSaveSlotName;

	// =========================================================================
	// INTERNAL — STATE GATHERING
	// =========================================================================

	/** Create a new UIronvaleSaveGame and populate it with current game state */
	UIronvaleSaveGame* GatherSaveData();

	/** Populate player data in the save game */
	void GatherPlayerData(UIronvaleSaveGame* SaveGame);

	/** Populate world state in the save game */
	void GatherWorldData(UIronvaleSaveGame* SaveGame);

	/** Populate quest data in the save game */
	void GatherQuestData(UIronvaleSaveGame* SaveGame);

	/** Populate NPC state data */
	void GatherNPCData(UIronvaleSaveGame* SaveGame);

	// =========================================================================
	// INTERNAL — STATE RESTORATION
	// =========================================================================

	/** Restore all game state from a loaded save game */
	bool RestoreFromSaveData(UIronvaleSaveGame* SaveGame);

	/** Restore player data */
	void RestorePlayerData(const UIronvaleSaveGame* SaveGame);

	/** Restore world state */
	void RestoreWorldData(const UIronvaleSaveGame* SaveGame);

	/** Restore quest data */
	void RestoreQuestData(const UIronvaleSaveGame* SaveGame);

	/** Restore NPC states */
	void RestoreNPCData(const UIronvaleSaveGame* SaveGame);

	/** Validate save version and apply migrations */
	bool ValidateAndMigrate(UIronvaleSaveGame* SaveGame);
};
