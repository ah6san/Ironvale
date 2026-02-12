// =============================================================================
// IronvaleDialogueTypes.h — Data-driven dialogue system structures
// Project Ironvale
//
// Defines the dialogue tree data format: nodes, choices, conditions, actions.
// Designed to be authored via DataAssets or imported from JSON/external tools.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleDialogueTypes.generated.h"

/**
 * A condition that must be met for a dialogue node/choice to be available.
 */
USTRUCT(BlueprintType)
struct FIronvaleDialogueCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	EIronvaleDialogueConditionType ConditionType = EIronvaleDialogueConditionType::Flag;

	/** Key: flag name, stat name, faction ID, quest ID, item ID, or tag string */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName Key;

	/** Comparison operator */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	EIronvaleComparisonOp Operator = EIronvaleComparisonOp::Equal;

	/** Value to compare against (numeric for stats/rep, 1/0 for flags) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	float Value = 1.0f;

	/** For tag checks: the gameplay tag to check */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FGameplayTag TagValue;

	/** If true, this condition is inverted (NOT) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bInvert = false;
};

/**
 * An action triggered when a dialogue node plays or a choice is selected.
 */
USTRUCT(BlueprintType)
struct FIronvaleDialogueAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	EIronvaleDialogueActionType ActionType = EIronvaleDialogueActionType::SetFlag;

	/** Primary parameter: flag name, quest ID, item ID, faction ID, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName PrimaryKey;

	/** Numeric value: reputation amount, gold amount, need amount, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	float NumericValue = 0.0f;

	/** String value: animation name, location ID, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FString StringValue;

	/** Additional key-value parameters for complex actions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TMap<FName, FString> ExtraParams;
};

/**
 * A single choice option within a dialogue node.
 */
USTRUCT(BlueprintType)
struct FIronvaleDialogueChoice
{
	GENERATED_BODY()

	/** Display text for the choice (localized) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText ChoiceText;

	/** ID of the next dialogue node when this choice is selected */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextNodeID;

	/** Conditions that must be met for this choice to appear */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FIronvaleDialogueCondition> Conditions;

	/** Required gameplay tags on the player (e.g., noble clothes, high charisma) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FGameplayTagContainer RequiredTags;

	/** Actions triggered when this choice is selected (before moving to next node) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FIronvaleDialogueAction> Actions;

	/** If true, this choice is shown but grayed out when conditions aren't met */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bShowWhenUnavailable = false;

	/** Tooltip explaining why this choice is unavailable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue",
		meta = (EditCondition = "bShowWhenUnavailable"))
	FText UnavailableReason;
};

/**
 * A single dialogue node — one "screen" of dialogue.
 */
USTRUCT(BlueprintType)
struct FIronvaleDialogueNode
{
	GENERATED_BODY()

	/** Unique node identifier within this dialogue tree */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NodeID;

	/** ID of the speaking NPC (for portrait/name display) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName SpeakerID;

	/** Dialogue text (localized) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText DialogueText;

	/** Audio cue ID for voiced dialogue (loaded separately, empty = no voice) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName AudioCueID;

	/** Animation to play on the speaker during this node */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName SpeakerAnimation;

	/** Player choices available at this node (empty = auto-advance or end) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FIronvaleDialogueChoice> Choices;

	/** If no choices and this is set, auto-advance to this node */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextNodeID;

	/** Conditions for this node to be reachable (all must pass) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FIronvaleDialogueCondition> Conditions;

	/** Actions executed when this node starts playing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FIronvaleDialogueAction> OnEnterActions;

	/** If true, this node ends the conversation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bIsEndNode = false;
};
