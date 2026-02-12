// =============================================================================
// IronvaleEncounterManager.h — Random encounter spawning subsystem
// Project Ironvale
//
// World subsystem that periodically checks if the player is traveling and
// spawns encounters from weighted tables filtered by biome, time, player
// level, and quest flags. Tracks daily encounters to enforce uniqueness.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/IronvaleEncounterTypes.h"
#include "IronvaleEncounterManager.generated.h"

class UIronvaleEventBus;

/**
 * Encounter manager — controls random encounter spawning in the open world.
 *
 * Every CheckIntervalSeconds, the manager evaluates whether the player is
 * traveling (moving above a speed threshold outside of towns). If so, it
 * rolls against the encounter chance and selects an encounter from the
 * weighted table for the current region.
 */
UCLASS()
class IRONVALE_API UIronvaleEncounterManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** How often (in seconds) the manager checks for encounter eligibility */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ironvale|Encounters")
	float CheckIntervalSeconds = 30.0f;

	/** Base probability (0-1) of an encounter on each check when traveling */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ironvale|Encounters",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseEncounterChance = 0.25f;

	/** Minimum player speed (cm/s) to be considered "traveling" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ironvale|Encounters",
		meta = (ClampMin = "0.0"))
	float TravelSpeedThreshold = 200.0f;

	/** Minimum time between encounters (seconds) to avoid overwhelming the player */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Ironvale|Encounters",
		meta = (ClampMin = "0.0"))
	float EncounterCooldownSeconds = 120.0f;

	// =========================================================================
	// ENCOUNTER TABLE
	// =========================================================================

	/** Register encounter templates for selection */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Encounters")
	void RegisterEncounterTemplate(const FIronvaleEncounterTemplate& Template);

	/** Register multiple templates at once */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Encounters")
	void RegisterEncounterTemplates(const TArray<FIronvaleEncounterTemplate>& Templates);

	/** Load encounter templates from a DataTable */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Encounters")
	void LoadEncounterTable(UDataTable* InTable);

	// =========================================================================
	// ENCOUNTER CONTROL
	// =========================================================================

	/**
	 * Manually trigger an encounter check. Ignores the timer but respects
	 * all other filters (biome, time, level, cooldown, town check).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Encounters")
	bool TriggerEncounterCheck();

	/**
	 * Force a specific encounter to spawn, bypassing all filters.
	 * Used by quests and scripted events.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Encounters")
	bool ForceEncounter(FName EncounterID);

	/** Enable or disable the encounter system */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Encounters")
	void SetEncountersEnabled(bool bEnabled);

	/** Is the encounter system currently enabled? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Encounters")
	bool AreEncountersEnabled() const { return bEncountersEnabled; }

	/** Reset the daily encounter log (called on new game day) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Encounters")
	void ResetDailyEncounters();

	/** Get the number of encounters spawned today */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Encounters")
	int32 GetDailyEncounterCount() const { return DailyEncounterLog.Num(); }

	// =========================================================================
	// ENCOUNTER SELECTION (exposed for testing and scripting)
	// =========================================================================

	/**
	 * Select an encounter from the weighted table, filtered by current conditions.
	 * @param OutTemplate  The selected encounter template
	 * @return true if a valid encounter was found
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Encounters")
	bool SelectEncounter(FIronvaleEncounterTemplate& OutTemplate) const;

protected:
	// =========================================================================
	// INTERNAL STATE
	// =========================================================================

	/** All registered encounter templates, keyed by EncounterID */
	TMap<FName, FIronvaleEncounterTemplate> EncounterTemplates;

	/** Encounter IDs triggered today (for bUniquePerDay enforcement) */
	TArray<FName> DailyEncounterLog;

	/** World time when the last encounter was spawned */
	float LastEncounterTime = -999.0f;

	/** Whether the encounter system is enabled */
	bool bEncountersEnabled = true;

	/** Timer handle for periodic checks */
	FTimerHandle CheckTimerHandle;

	/** Cached event bus reference */
	UPROPERTY()
	TObjectPtr<UIronvaleEventBus> CachedEventBus = nullptr;

	// =========================================================================
	// INTERNAL METHODS
	// =========================================================================

	/** Timer callback for periodic encounter checks */
	void PerformEncounterCheck();

	/** Check if the player is currently traveling (not in town, moving fast enough) */
	bool IsPlayerTraveling() const;

	/** Get the current player pawn */
	APawn* GetPlayerPawn() const;

	/** Check if an encounter template is valid for current conditions */
	bool IsEncounterEligible(const FIronvaleEncounterTemplate& Template,
		EIronvaleBiome CurrentBiome, EIronvaleTimeOfDay CurrentTime,
		int32 PlayerLevel) const;

	/**
	 * Spawn the encounter — place enemies near the player.
	 * Returns true if at least one enemy was spawned.
	 */
	bool SpawnEncounter(const FIronvaleEncounterTemplate& Template);

	/** Calculate the spawn location offset from the player */
	FVector CalculateSpawnLocation(const FIronvaleEncounterTemplate& Template) const;
};
