// =============================================================================
// IronvaleWorldManager.h — Open world region tracking and streaming
// Project Ironvale
//
// World subsystem that tracks the player's current region, provides region
// queries for other systems, and manages NPC AI LOD based on distance.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/IronvaleRegionDefinition.h"
#include "IronvaleWorldManager.generated.h"

class UIronvaleEventBus;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleRegionChangedSignature,
	FName, OldRegionID, FName, NewRegionID);

/**
 * World manager subsystem — central authority for open world state.
 *
 * Tracks which region the player occupies, manages region data assets,
 * broadcasts region change events, and provides query API for biome,
 * difficulty, faction, and POI data.
 */
UCLASS()
class IRONVALE_API UIronvaleWorldManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	void Tick(float DeltaSeconds);

	// =========================================================================
	// REGION REGISTRATION
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category = "Ironvale|World")
	void RegisterRegion(UIronvaleRegionDataAsset* RegionAsset);

	UFUNCTION(BlueprintCallable, Category = "Ironvale|World")
	void RegisterRegions(const TArray<UIronvaleRegionDataAsset*>& RegionAssets);

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	TArray<FName> GetRegisteredRegionIDs() const;

	// =========================================================================
	// REGION QUERIES
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	FName GetCurrentRegionID() const { return CurrentRegionID; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	bool GetCurrentRegionData(FIronvaleRegionData& OutData) const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	bool GetRegionData(FName RegionID, FIronvaleRegionData& OutData) const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	EIronvaleBiome GetCurrentBiome() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	bool IsInTown() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	FName GetCurrentFaction() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	FName GetCurrentEncounterTableID() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	FVector2D GetCurrentDifficultyRange() const;

	// =========================================================================
	// POINT OF INTEREST
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category = "Ironvale|World")
	void DiscoverLocation(FName LocationID);

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	bool IsLocationDiscovered(FName LocationID) const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	TArray<FName> GetDiscoveredLocations() const;

	// =========================================================================
	// DELEGATES
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|World")
	FOnIronvaleRegionChangedSignature OnRegionChanged;

	// =========================================================================
	// NPC AI LOD CONFIG
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|World|AILOD")
	float AILODUpdateInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|World|AILOD",
		meta = (ClampMin = "1000.0"))
	float SimplifiedAIDistance = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|World|AILOD",
		meta = (ClampMin = "5000.0"))
	float FrozenAIDistance = 30000.0f;

protected:
	UPROPERTY()
	TMap<FName, TObjectPtr<UIronvaleRegionDataAsset>> RegionRegistry;

	FName CurrentRegionID;

	TSet<FName> DiscoveredLocations;

	UPROPERTY()
	TObjectPtr<UIronvaleEventBus> CachedEventBus = nullptr;

	FDelegateHandle TickDelegateHandle;
	FTimerHandle AILODTimerHandle;

	void UpdatePlayerRegion();
	void HandleRegionChange(FName OldRegionID, FName NewRegionID);
	void UpdateNPCAILOD();
	FVector GetPlayerPosition() const;
};
