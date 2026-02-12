// =============================================================================
// IronvaleSaveTypes.h — Data structures for save/load serialization
// Project Ironvale
//
// These structs are the canonical save format. The SaveManager serializes
// them to/from Unreal's USaveGame system. They are also JSON-serializable
// for engine-agnostic portability.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "IronvaleTypes.h"
#include "IronvaleSaveTypes.generated.h"

/**
 * Saved state of a single inventory item.
 */
USTRUCT(BlueprintType)
struct FIronvaleSavedItem
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FName ItemID;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	int32 StackCount = 1;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float CurrentDurability = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	EIronvaleItemQuality Quality = EIronvaleItemQuality::Common;

	/** Unique instance ID for tracking equipped items and quest references */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FGuid InstanceID;
};

/**
 * Saved equipment loadout — maps slot index to item instance ID.
 */
USTRUCT(BlueprintType)
struct FIronvaleSavedEquipment
{
	GENERATED_BODY()

	/** Map from equipment slot index to the item's InstanceID */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TMap<int32, FGuid> SlotToItemID;
};

/**
 * Saved player needs (hunger, thirst, etc.).
 */
USTRUCT(BlueprintType)
struct FIronvaleSavedNeeds
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float Hunger = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float Thirst = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float Fatigue = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float Cleanliness = 100.0f;
};

/**
 * Saved quest state.
 */
USTRUCT(BlueprintType)
struct FIronvaleSavedQuest
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FName QuestID;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	EIronvaleQuestState State = EIronvaleQuestState::NotStarted;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	int32 CurrentStageIndex = 0;

	/** Per-objective progress: objective index → current count */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TMap<int32, int32> ObjectiveProgress;

	/** Completed optional objectives */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<int32> CompletedOptionalObjectives;
};

/**
 * Saved NPC state — for persistent/important NPCs.
 */
USTRUCT(BlueprintType)
struct FIronvaleSavedNPC
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FName NPCID;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	bool bIsAlive = true;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float Health = 100.0f;

	/** Current schedule override (if any, from quest injection) */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FName CurrentOverrideID;
};

/**
 * Saved player data — everything about the player character.
 */
USTRUCT(BlueprintType)
struct FIronvaleSavedPlayerData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float Health = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float MaxHealth = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float Stamina = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float MaxStamina = 100.0f;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	int32 Gold = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<FIronvaleSavedItem> InventoryItems;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FIronvaleSavedEquipment Equipment;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	FIronvaleSavedNeeds Needs;
};

/**
 * Saved world state — global flags, time, weather.
 */
USTRUCT(BlueprintType)
struct FIronvaleSavedWorldState
{
	GENERATED_BODY()

	/** Current game time in hours (0-24, wrapping) */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	float GameTimeHours = 8.0f;

	/** Total in-game days elapsed */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	int32 DayCount = 1;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	EIronvaleWeatherState CurrentWeather = EIronvaleWeatherState::Clear;

	/** World flags: key-value pairs set by quests, dialogue, and scripts */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TMap<FName, bool> WorldFlags;

	/** Faction reputation values */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<FIronvaleReputationEntry> Reputations;

	/** Quest states */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<FIronvaleSavedQuest> Quests;

	/** Important NPC states */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Save")
	TArray<FIronvaleSavedNPC> NPCStates;
};

/**
 * Metadata about a save slot — displayed in the load menu.
 */
USTRUCT(BlueprintType)
struct FIronvaleSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString SlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FDateTime SaveTimestamp;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString PlayerLocation;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 DayCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	float PlayTimeSeconds = 0.0f;

	/** Optional screenshot thumbnail (stored as soft reference to avoid memory bloat) */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	TSoftObjectPtr<UTexture2D> Thumbnail;
};
