// =============================================================================
// IronvaleRadiantQuestGenerator.h — Procedural quest generation system
// Project Ironvale
//
// Generates quests from templates by filling in slots (targets, locations,
// items, rewards) from region-appropriate pools. Inspired by Skyrim's radiant
// quest system but with more authored control over template quality.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Quests/IronvaleQuestTypes.h"
#include "IronvaleRadiantQuestGenerator.generated.h"

class UIronvaleQuestAsset;

// =============================================================================
// RADIANT QUEST ENUMS
// =============================================================================

/**
 * Types of radiant quest templates.
 */
UENUM(BlueprintType)
enum class EIronvaleRadiantQuestType : uint8
{
	Bounty    UMETA(DisplayName = "Bounty"),     // Kill specific enemies
	Delivery  UMETA(DisplayName = "Delivery"),   // Bring item from A to B
	Patrol    UMETA(DisplayName = "Patrol"),      // Visit a series of locations
	Escort    UMETA(DisplayName = "Escort")       // Escort an NPC safely
};

// =============================================================================
// RADIANT QUEST DATA STRUCTURES
// =============================================================================

/**
 * A pool of valid targets for a specific slot in a radiant template.
 * Example: valid enemies for a bounty, valid items for a delivery.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleRadiantTargetPool
{
	GENERATED_BODY()

	/** Display name for this pool (for editor readability) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FText PoolName;

	/** Valid target IDs (NPC IDs, Item IDs, Location IDs, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	TArray<FName> TargetIDs;

	/** Optional region filter — if set, only these regions can use this pool */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	TArray<FName> ValidRegions;

	/** Minimum player level for targets in this pool */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant", meta = (ClampMin = "1"))
	int32 MinPlayerLevel = 1;

	/** Maximum player level for targets in this pool (0 = no cap) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant", meta = (ClampMin = "0"))
	int32 MaxPlayerLevel = 0;
};

/**
 * Reward range for radiant quest scaling.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleRadiantRewardRange
{
	GENERATED_BODY()

	/** Gold reward range */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FVector2D GoldRange = FVector2D(10.0, 50.0);

	/** Reputation reward range */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FVector2D ReputationRange = FVector2D(1.0, 5.0);

	/** Faction to grant reputation with (if any) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FName RewardFactionID;

	/** Optional item reward pool — one item randomly selected */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	TArray<FName> ItemRewardPool;
};

/**
 * A radiant quest template — defines the structure and slot pools for
 * procedural quest generation.
 *
 * Templates are authored by designers, and the generator fills in the
 * blanks based on the current region, player level, and available targets.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleRadiantTemplate
{
	GENERATED_BODY()

	/** Unique template identifier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FName TemplateID;

	/** Category of quest this template generates */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	EIronvaleRadiantQuestType QuestType = EIronvaleRadiantQuestType::Bounty;

	/** Display name template — supports {Target}, {Location}, {Item} placeholders */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FText NameTemplate;

	/** Description template — supports the same placeholders */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FText DescriptionTemplate;

	/** Target pools for primary objective (enemies, items, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FIronvaleRadiantTargetPool PrimaryTargetPool;

	/** Location pools (destinations, patrol points, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FIronvaleRadiantTargetPool LocationPool;

	/** Secondary target pool (e.g., NPC to escort, NPC to deliver to) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FIronvaleRadiantTargetPool SecondaryTargetPool;

	/** Reward scaling ranges */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FIronvaleRadiantRewardRange RewardRange;

	/** Number of primary targets required (e.g., kill 3-7 enemies) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FVector2D TargetCountRange = FVector2D(1.0, 5.0);

	/** Number of patrol/waypoint locations (for Patrol quests) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant")
	FVector2D PatrolPointCountRange = FVector2D(2.0, 4.0);

	/**
	 * Selection weight for this template relative to others.
	 * Higher = more likely to be chosen by the generator.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant", meta = (ClampMin = "0.01"))
	float SelectionWeight = 1.0f;

	/** Cooldown in game-days before this template can be reused */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Radiant", meta = (ClampMin = "0"))
	int32 CooldownDays = 1;
};

// =============================================================================
// RADIANT QUEST GENERATOR
// =============================================================================

/**
 * Generates procedural quests from authored templates.
 *
 * Usage:
 *   1. Register templates (from DataTable or hardcoded for testing)
 *   2. Call GenerateQuest(TemplateID, RegionID) to create a quest definition
 *   3. Create a UIronvaleQuestAsset from the generated definition
 *   4. Pass to the quest manager to start
 *
 * The generator respects cooldowns, region filters, and player level gates.
 * It produces FIronvaleQuestDefinition structs that are identical in format
 * to hand-authored quests — the quest manager cannot distinguish them.
 */
UCLASS(BlueprintType)
class IRONVALE_API UIronvaleRadiantQuestGenerator : public UObject
{
	GENERATED_BODY()

public:
	// =========================================================================
	// TEMPLATE MANAGEMENT
	// =========================================================================

	/** Register a radiant quest template */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Radiant")
	void RegisterTemplate(const FIronvaleRadiantTemplate& Template);

	/** Register multiple templates at once */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Radiant")
	void RegisterTemplates(const TArray<FIronvaleRadiantTemplate>& Templates);

	/** Get all registered template IDs */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Radiant")
	TArray<FName> GetRegisteredTemplateIDs() const;

	// =========================================================================
	// QUEST GENERATION
	// =========================================================================

	/**
	 * Generate a quest from a specific template for a given region.
	 * @param TemplateID  The template to use
	 * @param RegionID    The region context (filters target/location pools)
	 * @param PlayerLevel Current player level for scaling
	 * @param OutDefinition  The generated quest definition
	 * @return true if generation succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Radiant")
	bool GenerateQuest(FName TemplateID, FName RegionID, int32 PlayerLevel,
		FIronvaleQuestDefinition& OutDefinition);

	/**
	 * Generate a quest using a randomly selected valid template for the region.
	 * Uses weighted random selection among templates that pass region/level filters.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Radiant")
	bool GenerateRandomQuest(FName RegionID, int32 PlayerLevel,
		FIronvaleQuestDefinition& OutDefinition);

	/**
	 * Check if a template is currently available (not on cooldown, valid for region).
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Radiant")
	bool IsTemplateAvailable(FName TemplateID, FName RegionID, int32 PlayerLevel) const;

	// =========================================================================
	// COOLDOWN MANAGEMENT
	// =========================================================================

	/** Advance the cooldown clock by one game day. Call from the day-change event. */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Radiant")
	void AdvanceCooldowns();

	/** Reset all cooldowns */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Radiant")
	void ResetAllCooldowns();

protected:
	/** Registered templates, keyed by TemplateID */
	TMap<FName, FIronvaleRadiantTemplate> Templates;

	/** Cooldown timers: TemplateID → remaining days */
	TMap<FName, int32> TemplateCooldowns;

	/** Counter for generating unique quest IDs */
	int32 GeneratedQuestCounter = 0;

	// =========================================================================
	// INTERNAL GENERATION HELPERS
	// =========================================================================

	/** Generate a bounty quest (kill targets) */
	bool GenerateBountyQuest(const FIronvaleRadiantTemplate& Template, FName RegionID,
		int32 PlayerLevel, FIronvaleQuestDefinition& OutDef);

	/** Generate a delivery quest (bring item to NPC) */
	bool GenerateDeliveryQuest(const FIronvaleRadiantTemplate& Template, FName RegionID,
		int32 PlayerLevel, FIronvaleQuestDefinition& OutDef);

	/** Generate a patrol quest (visit locations) */
	bool GeneratePatrolQuest(const FIronvaleRadiantTemplate& Template, FName RegionID,
		int32 PlayerLevel, FIronvaleQuestDefinition& OutDef);

	/** Generate an escort quest (escort NPC to location) */
	bool GenerateEscortQuest(const FIronvaleRadiantTemplate& Template, FName RegionID,
		int32 PlayerLevel, FIronvaleQuestDefinition& OutDef);

	/** Pick a random target from a pool, respecting region and level filters */
	FName SelectTarget(const FIronvaleRadiantTargetPool& Pool, FName RegionID,
		int32 PlayerLevel) const;

	/** Pick N random non-duplicate targets from a pool */
	TArray<FName> SelectMultipleTargets(const FIronvaleRadiantTargetPool& Pool,
		FName RegionID, int32 PlayerLevel, int32 Count) const;

	/** Generate scaled rewards based on the template's reward range and player level */
	FIronvaleQuestRewards GenerateRewards(const FIronvaleRadiantRewardRange& Range,
		int32 PlayerLevel) const;

	/** Fill text template placeholders with actual names */
	FText FillTextTemplate(const FText& Template, const TMap<FString, FString>& Replacements) const;

	/** Generate a unique quest ID for a radiant quest */
	FName GenerateUniqueQuestID(FName TemplateID);

	/** Apply cooldown after generating from a template */
	void ApplyCooldown(FName TemplateID, int32 CooldownDays);

	/** Check if a target pool has valid entries for the region/level */
	bool HasValidTargets(const FIronvaleRadiantTargetPool& Pool, FName RegionID,
		int32 PlayerLevel) const;
};
