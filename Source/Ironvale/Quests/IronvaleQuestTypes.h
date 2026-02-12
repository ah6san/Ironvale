// =============================================================================
// IronvaleQuestTypes.h — Quest data structures for the quest system
// Project Ironvale
//
// Defines the data-driven quest format: objectives, stages, definitions,
// and reward structures. Authored via DataAssets or imported from JSON.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Core/IronvaleTypes.h"
#include "Dialogue/IronvaleDialogueTypes.h"
#include "IronvaleQuestTypes.generated.h"

// =============================================================================
// QUEST OBJECTIVE DATA
// =============================================================================

/**
 * A single quest objective — the atomic unit of quest tracking.
 * Examples: "Kill 5 bandits", "Collect 3 herbs", "Reach the cave entrance".
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleQuestObjectiveData
{
	GENERATED_BODY()

	/** What type of action the player must perform */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	EIronvaleObjectiveType ObjectiveType = EIronvaleObjectiveType::Kill;

	/**
	 * Target identifier — meaning varies by ObjectiveType:
	 *   Kill:          NPC/enemy ID to kill
	 *   Collect:       Item ID to collect
	 *   TalkTo:        NPC ID to speak with
	 *   ReachLocation: Location trigger ID
	 *   Escort:        NPC ID to escort
	 *   UseItem:       Item ID to use
	 *   Custom:        Custom event key
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	FName TargetID;

	/** Number of times this objective must be fulfilled (e.g., kill 5, collect 3) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;

	/** Player-facing description (localized) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	FText Description;

	/** If true, this objective is optional and does not block stage completion */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	bool bOptional = false;

	/** If true, this objective is hidden from the player until revealed by script */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	bool bHidden = false;
};

// =============================================================================
// QUEST STAGE DATA
// =============================================================================

/**
 * A quest stage groups related objectives. A quest progresses through stages
 * linearly, unless branching is defined via BranchMap. Stages can trigger
 * dialogue actions on completion or failure.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleQuestStageData
{
	GENERATED_BODY()

	/** Unique stage identifier within this quest */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Stage")
	FName StageID;

	/** Player-facing description of the current stage (localized) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Stage")
	FText Description;

	/** All objectives in this stage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Stage")
	TArray<FIronvaleQuestObjectiveData> Objectives;

	/**
	 * If true, ALL non-optional objectives must be complete to advance.
	 * If false, ANY single non-optional objective completes the stage.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Stage")
	bool bAllRequired = true;

	/** Actions executed when this stage completes successfully */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Stage")
	TArray<FIronvaleDialogueAction> OnCompleteActions;

	/** Actions executed if this stage fails */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Stage")
	TArray<FIronvaleDialogueAction> OnFailActions;

	/**
	 * Default next stage when this stage completes.
	 * If NAME_None and BranchMap is empty, stage completion means quest completion.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Stage")
	FName NextStageID;

	/**
	 * Conditional branching: maps a condition key (e.g., world flag, completed
	 * objective ID) to a target StageID. Evaluated in order; first match wins.
	 * If no match, falls through to NextStageID.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Stage")
	TMap<FName, FName> BranchMap;
};

// =============================================================================
// QUEST REWARD DATA
// =============================================================================

/**
 * Reputation change as part of a quest reward.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleReputationReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward")
	FName FactionID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward")
	float ReputationDelta = 0.0f;
};

/**
 * Item reward entry — an item and its quantity.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleItemReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};

/**
 * Complete reward package for a quest.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleQuestRewards
{
	GENERATED_BODY()

	/** Gold awarded on completion */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward", meta = (ClampMin = "0"))
	int32 Gold = 0;

	/** Items awarded on completion */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward")
	TArray<FIronvaleItemReward> Items;

	/** Reputation changes applied on completion */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward")
	TArray<FIronvaleReputationReward> ReputationChanges;
};

// =============================================================================
// QUEST PREREQUISITE — REPUTATION REQUIREMENT
// =============================================================================

/**
 * Minimum reputation needed with a specific faction to accept this quest.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleQuestReputationRequirement
{
	GENERATED_BODY()

	/** Faction whose reputation is checked */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Prerequisite")
	FName FactionID;

	/** Minimum reputation value required */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Prerequisite")
	float MinReputation = 0.0f;
};

// =============================================================================
// QUEST DEFINITION
// =============================================================================

/**
 * Complete quest definition — the static, authored data for a single quest.
 * At runtime, the quest manager creates a FIronvaleSavedQuest to track progress.
 */
USTRUCT(BlueprintType)
struct IRONVALE_API FIronvaleQuestDefinition
{
	GENERATED_BODY()

	/** Unique quest identifier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestID;

	/** Player-facing quest name (localized) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText DisplayName;

	/** Full quest description for the journal (localized) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText Description;

	/** Ordered stages of this quest */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FIronvaleQuestStageData> Stages;

	/** Whether this quest can be failed (vs. just being blocked until conditions are met) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	bool bCanFail = true;

	/** Conditions that, if met, immediately fail this quest */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest",
		meta = (EditCondition = "bCanFail"))
	TArray<FIronvaleDialogueCondition> FailConditions;

	/**
	 * Quest IDs that must be completed before this quest can be started.
	 * The quest manager checks these when StartQuest is called.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Prerequisite")
	TArray<FName> Prerequisites;

	/** Required faction reputation to accept this quest */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Prerequisite")
	FIronvaleQuestReputationRequirement RequiredReputation;

	/** Rewards granted on successful completion */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Reward")
	FIronvaleQuestRewards Rewards;

	/** Find a stage by ID; returns nullptr if not found */
	const FIronvaleQuestStageData* FindStage(FName StageID) const
	{
		for (const FIronvaleQuestStageData& Stage : Stages)
		{
			if (Stage.StageID == StageID)
			{
				return &Stage;
			}
		}
		return nullptr;
	}

	/** Get stage index by ID; returns INDEX_NONE if not found */
	int32 FindStageIndex(FName StageID) const
	{
		for (int32 i = 0; i < Stages.Num(); ++i)
		{
			if (Stages[i].StageID == StageID)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}
};
