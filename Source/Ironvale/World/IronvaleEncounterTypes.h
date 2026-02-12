// =============================================================================
// IronvaleEncounterTypes.h — Encounter template data structures
// Project Ironvale
//
// Defines the data format for random encounters. Encounter templates specify
// which enemies can appear, under what conditions (biome, time, player level),
// and how likely they are to be selected.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleEncounterTypes.generated.h"

/**
 * A single encounter template — defines one possible random encounter.
 *
 * The encounter manager selects from available templates using weighted
 * random selection, filtered by the current biome, time of day, player
 * level, and uniqueness constraints.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleEncounterTemplate : public FTableRowBase
{
	GENERATED_BODY()

	/** Unique encounter identifier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName EncounterID;

	/**
	 * Enemy IDs that compose this encounter.
	 * Each entry refers to an enemy archetype or specific NPC.
	 * The encounter manager spawns all of them when this encounter triggers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TArray<FName> EnemyIDs;

	/** Minimum player level for this encounter to be eligible */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "1"))
	int32 MinPlayerLevel = 1;

	/** Maximum player level (0 = no upper limit) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter", meta = (ClampMin = "0"))
	int32 MaxPlayerLevel = 0;

	/**
	 * Biome filter: encounter only spawns in these biomes.
	 * Empty array = valid in all biomes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TArray<EIronvaleBiome> BiomeFilter;

	/**
	 * Time-of-day filter: encounter only spawns during these times.
	 * Empty array = valid at all times.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	TArray<EIronvaleTimeOfDay> TimeFilter;

	/**
	 * Selection weight for weighted random picking.
	 * Higher weight = more likely to be chosen relative to other eligible encounters.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter",
		meta = (ClampMin = "0.01"))
	float Weight = 1.0f;

	/**
	 * If true, this encounter can only occur once per game day.
	 * The encounter manager tracks daily encounter history.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	bool bUniquePerDay = false;

	/** Optional quest flag required for this encounter to appear */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName RequiredQuestFlag;

	/** Optional quest flag that suppresses this encounter */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName SuppressedByFlag;

	/** Minimum distance from the player at which enemies spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter",
		meta = (ClampMin = "500.0"))
	float MinSpawnDistance = 2000.0f;

	/** Maximum distance from the player at which enemies spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter",
		meta = (ClampMin = "1000.0"))
	float MaxSpawnDistance = 5000.0f;

	/** Whether enemies in this encounter should be aware of the player immediately */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	bool bAlertOnSpawn = false;

	/** Optional encounter dialogue/bark ID (e.g., bandits shouting before attacking) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter")
	FName EncounterBarkID;
};
