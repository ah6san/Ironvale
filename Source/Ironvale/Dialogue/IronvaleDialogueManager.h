// =============================================================================
// IronvaleDialogueManager.h — Dialogue flow controller
// Project Ironvale
//
// World subsystem that manages active dialogue sessions: loading dialogue
// assets, evaluating conditions, advancing nodes, and executing actions.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IronvaleDialogueTypes.h"
#include "IronvaleDialogueManager.generated.h"

class UIronvaleDialogueAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueNodeDisplaySignature,
	const FIronvaleDialogueNode&, Node, const TArray<int32>&, AvailableChoiceIndices);

UCLASS()
class IRONVALE_API UIronvaleDialogueSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Start a dialogue session with an NPC using their dialogue asset */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Dialogue")
	bool StartDialogue(UIronvaleDialogueAsset* DialogueAsset, AActor* NPC, AActor* Player);

	/** Select a choice by index in the current node */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Dialogue")
	void SelectChoice(int32 ChoiceIndex);

	/** Advance to the next node (for nodes with no choices) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Dialogue")
	void AdvanceDialogue();

	/** End the current dialogue session */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Dialogue")
	void EndDialogue();

	/** Is a dialogue session currently active? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Dialogue")
	bool IsInDialogue() const { return bIsInDialogue; }

	/** Get the current dialogue node */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Dialogue")
	const FIronvaleDialogueNode& GetCurrentNode() const { return CurrentNode; }

	/** Get indices of choices available to the player (conditions met) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Dialogue")
	TArray<int32> GetAvailableChoiceIndices() const;

	/** Fired when a new node should be displayed by the UI */
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Dialogue")
	FOnDialogueNodeDisplaySignature OnDialogueNodeDisplay;

protected:
	bool bIsInDialogue = false;

	UPROPERTY()
	UIronvaleDialogueAsset* ActiveDialogueAsset = nullptr;

	UPROPERTY()
	AActor* ActiveNPC = nullptr;

	UPROPERTY()
	AActor* ActivePlayer = nullptr;

	FIronvaleDialogueNode CurrentNode;

	/** Navigate to a specific node by ID */
	void GoToNode(FName NodeID);

	/** Execute dialogue actions (set flags, modify rep, start quests, etc.) */
	void ExecuteActions(const TArray<FIronvaleDialogueAction>& Actions);

	/** Execute a single dialogue action */
	void ExecuteAction(const FIronvaleDialogueAction& Action);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}
};
