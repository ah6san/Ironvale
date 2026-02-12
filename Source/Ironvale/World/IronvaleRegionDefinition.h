// =============================================================================
// IronvaleRegionDefinition.h — Region data for the open-world system
// Project Ironvale
//
// Defines region data structures used by the world manager to track
// where the player is, what encounters can spawn, and which faction
// controls the area.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DataAsset.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleRegionDefinition.generated.h"

/**
 * Region definition row for the region DataTable.
 *
 * Each row defines a named world region with its biome, difficulty range,
 * encounter table, and faction association. Used by the world manager and
 * encounter manager for context-sensitive gameplay.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleRegionData : public FTableRowBase
{
	GENERATED_BODY()

	/** Unique region identifier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FName RegionID;

	/** Player-facing region name (localized) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FText DisplayName;

	/** Biome type — drives encounter selection, ambient audio, and VFX */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	EIronvaleBiome Biome = EIronvaleBiome::Forest;

	/**
	 * Difficulty range for this region.
	 * X = minimum difficulty, Y = maximum difficulty.
	 * Used to filter encounters and scale enemy stats.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FVector2D DifficultyRange = FVector2D(1.0, 5.0);

	/** Reference to the encounter table used in this region */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FName EncounterTableID;

	/** Whether this region is a town/settlement (disables random hostile encounters) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	bool bIsTown = false;

	/** Faction that controls this region (guards, laws, prices) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FName AssociatedFaction;

	/** Sub-regions within this region (for finer-grained location tracking) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	TArray<FName> SubRegions;

	/** Music/ambience cue ID for this region */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FName AmbienceCueID;

	/** Loading screen tip or description shown when entering */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region")
	FText EntryDescription;
};

/**
 * Data asset containing a complete region definition for editor authoring.
 *
 * While regions can be defined via DataTable rows, this data asset variant
 * provides richer editor support and can be referenced directly by
 * level blueprints and triggers.
 */
UCLASS(BlueprintType)
class IRONVALE_API UIronvaleRegionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The region data contained in this asset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region",
		meta = (ShowOnlyInnerProperties))
	FIronvaleRegionData RegionData;

	/** Convenience accessor for the region ID */
	UFUNCTION(BlueprintPure, Category = "Region")
	FName GetRegionID() const { return RegionData.RegionID; }

	/** Convenience accessor for the display name */
	UFUNCTION(BlueprintPure, Category = "Region")
	FText GetDisplayName() const { return RegionData.DisplayName; }

	/** Check if this region is a town */
	UFUNCTION(BlueprintPure, Category = "Region")
	bool IsTown() const { return RegionData.bIsTown; }

	/** Get the biome type */
	UFUNCTION(BlueprintPure, Category = "Region")
	EIronvaleBiome GetBiome() const { return RegionData.Biome; }

	/** Get the controlling faction */
	UFUNCTION(BlueprintPure, Category = "Region")
	FName GetAssociatedFaction() const { return RegionData.AssociatedFaction; }

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("IronvaleRegion"), RegionData.RegionID);
	}
};
