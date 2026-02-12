// =============================================================================
// IronvaleReputationTypes.h — Faction and reputation data structures
// Project Ironvale
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "IronvaleReputationTypes.generated.h"

/**
 * Faction definition — one row per faction in the DataTable.
 */
USTRUCT(BlueprintType)
struct FIronvaleFactionData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation")
	FName FactionID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation")
	FText DisplayName;

	/** Starting reputation with this faction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation",
		meta = (ClampMin = "-100.0", ClampMax = "100.0"))
	float DefaultReputation = 0.0f;

	/** Reputation threshold below which NPCs of this faction become hostile */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation")
	float HostileThreshold = -50.0f;

	/** Reputation threshold above which NPCs offer better prices and quests */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation")
	float FriendlyThreshold = 30.0f;

	/** Factions that are allied (rep changes propagate partially) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation")
	TArray<FName> AlliedFactions;

	/** Factions that are enemies (rep changes propagate inversely) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation")
	TArray<FName> EnemyFactions;

	/** How much allied factions gain when this faction's rep increases (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AlliancePropagation = 0.25f;

	/** How much enemy factions lose when this faction's rep increases (0-1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reputation",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EnemyPropagation = 0.15f;
};
