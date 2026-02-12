// =============================================================================
// IronvaleDialogueManager.cpp — Dialogue flow controller implementation
// Project Ironvale
// =============================================================================

#include "Dialogue/IronvaleDialogueManager.h"
#include "Ironvale.h"
#include "Dialogue/IronvaleDialogueAsset.h"
#include "Dialogue/IronvaleDialogueCondition.h"
#include "Dialogue/IronvaleReputationComponent.h"
#include "IronvaleGameInstance.h"
#include "IronvaleGameState.h"
#include "Core/IronvaleEventBus.h"

bool UIronvaleDialogueSubsystem::StartDialogue(UIronvaleDialogueAsset* DialogueAsset, AActor* NPC, AActor* Player)
{
	if (!DialogueAsset || !NPC || !Player)
	{
		UE_LOG(LogIronvale, Warning, TEXT("StartDialogue: invalid parameters"));
		return false;
	}

	if (bIsInDialogue)
	{
		UE_LOG(LogIronvale, Warning, TEXT("Already in dialogue, ending current before starting new"));
		EndDialogue();
	}

	ActiveDialogueAsset = DialogueAsset;
	ActiveNPC = NPC;
	ActivePlayer = Player;
	bIsInDialogue = true;

	// Pause game time during dialogue
	if (AIronvaleGameState* GS = GetWorld()->GetGameState<AIronvaleGameState>())
	{
		GS->SetTimePaused(true);
	}

	// Broadcast dialogue started
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnDialogueStarted.Broadcast(NPC, Player);
	}

	// Navigate to entry node
	GoToNode(DialogueAsset->EntryNodeID);

	UE_LOG(LogIronvale, Log, TEXT("Dialogue started: %s with %s"),
		*DialogueAsset->DialogueID.ToString(), *NPC->GetName());

	return true;
}

void UIronvaleDialogueSubsystem::SelectChoice(int32 ChoiceIndex)
{
	if (!bIsInDialogue) return;

	if (ChoiceIndex < 0 || ChoiceIndex >= CurrentNode.Choices.Num())
	{
		UE_LOG(LogIronvale, Warning, TEXT("Invalid choice index: %d"), ChoiceIndex);
		return;
	}

	const FIronvaleDialogueChoice& Choice = CurrentNode.Choices[ChoiceIndex];

	// Execute choice actions
	ExecuteActions(Choice.Actions);

	// Broadcast choice made
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnDialogueChoiceMade.Broadcast(CurrentNode.NodeID, ChoiceIndex);
	}

	// Navigate to next node
	if (!Choice.NextNodeID.IsNone())
	{
		GoToNode(Choice.NextNodeID);
	}
	else
	{
		EndDialogue();
	}
}

void UIronvaleDialogueSubsystem::AdvanceDialogue()
{
	if (!bIsInDialogue) return;

	if (CurrentNode.bIsEndNode || CurrentNode.NextNodeID.IsNone())
	{
		EndDialogue();
	}
	else
	{
		GoToNode(CurrentNode.NextNodeID);
	}
}

void UIronvaleDialogueSubsystem::EndDialogue()
{
	if (!bIsInDialogue) return;

	bIsInDialogue = false;

	// Resume game time
	if (AIronvaleGameState* GS = GetWorld()->GetGameState<AIronvaleGameState>())
	{
		GS->SetTimePaused(false);
	}

	// Broadcast dialogue ended
	if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
	{
		EventBus->OnDialogueEnded.Broadcast(ActiveNPC, ActivePlayer);
	}

	UE_LOG(LogIronvale, Log, TEXT("Dialogue ended"));

	ActiveDialogueAsset = nullptr;
	ActiveNPC = nullptr;
	ActivePlayer = nullptr;
}

void UIronvaleDialogueSubsystem::GoToNode(FName NodeID)
{
	if (!ActiveDialogueAsset) return;

	const FIronvaleDialogueNode* Node = ActiveDialogueAsset->FindNode(NodeID);
	if (!Node)
	{
		UE_LOG(LogIronvale, Error, TEXT("Dialogue node not found: %s"), *NodeID.ToString());
		EndDialogue();
		return;
	}

	// Check node conditions
	if (!UIronvaleDialogueConditionEvaluator::EvaluateAllConditions(this, Node->Conditions, ActivePlayer))
	{
		UE_LOG(LogIronvale, Verbose, TEXT("Node %s conditions not met, skipping"), *NodeID.ToString());
		// If node has a next node, try that instead
		if (!Node->NextNodeID.IsNone())
		{
			GoToNode(Node->NextNodeID);
		}
		else
		{
			EndDialogue();
		}
		return;
	}

	CurrentNode = *Node;

	// Execute on-enter actions
	ExecuteActions(CurrentNode.OnEnterActions);

	// Get available choices for UI
	TArray<int32> AvailableChoices = GetAvailableChoiceIndices();

	// Notify UI to display this node
	OnDialogueNodeDisplay.Broadcast(CurrentNode, AvailableChoices);
}

TArray<int32> UIronvaleDialogueSubsystem::GetAvailableChoiceIndices() const
{
	TArray<int32> Available;

	for (int32 i = 0; i < CurrentNode.Choices.Num(); ++i)
	{
		const FIronvaleDialogueChoice& Choice = CurrentNode.Choices[i];

		bool bAvailable = UIronvaleDialogueConditionEvaluator::EvaluateAllConditions(
			const_cast<UIronvaleDialogueSubsystem*>(this), Choice.Conditions, ActivePlayer);

		if (bAvailable && Choice.RequiredTags.IsValid() && Choice.RequiredTags.Num() > 0)
		{
			bAvailable = UIronvaleDialogueConditionEvaluator::PlayerHasRequiredTags(
				ActivePlayer, Choice.RequiredTags);
		}

		if (bAvailable || Choice.bShowWhenUnavailable)
		{
			Available.Add(i);
		}
	}

	return Available;
}

void UIronvaleDialogueSubsystem::ExecuteActions(const TArray<FIronvaleDialogueAction>& Actions)
{
	for (const FIronvaleDialogueAction& Action : Actions)
	{
		ExecuteAction(Action);
	}
}

void UIronvaleDialogueSubsystem::ExecuteAction(const FIronvaleDialogueAction& Action)
{
	switch (Action.ActionType)
	{
	case EIronvaleDialogueActionType::SetFlag:
	{
		if (UIronvaleGameInstance* GI = Cast<UIronvaleGameInstance>(GetWorld()->GetGameInstance()))
		{
			GI->SetWorldFlag(Action.PrimaryKey, Action.NumericValue > 0.0f);
		}
		break;
	}

	case EIronvaleDialogueActionType::ModifyReputation:
	{
		if (UIronvaleReputationSubsystem* RepSys = GetWorld()->GetGameInstance()->GetSubsystem<UIronvaleReputationSubsystem>())
		{
			RepSys->ModifyReputation(Action.PrimaryKey, Action.NumericValue);
		}
		break;
	}

	case EIronvaleDialogueActionType::StartQuest:
	{
		if (UIronvaleEventBus* EventBus = GetWorld()->GetSubsystem<UIronvaleEventBus>())
		{
			EventBus->OnQuestStarted.Broadcast(Action.PrimaryKey, 0);
		}
		break;
	}

	case EIronvaleDialogueActionType::GiveGold:
	{
		if (ActivePlayer)
		{
			if (UIronvaleInventoryComponent* Inv = ActivePlayer->FindComponentByClass<UIronvaleInventoryComponent>())
			{
				Inv->AddGold(FMath::RoundToInt32(Action.NumericValue));
			}
		}
		break;
	}

	default:
		UE_LOG(LogIronvale, Verbose, TEXT("Dialogue action type %d not fully implemented yet"),
			static_cast<int32>(Action.ActionType));
		break;
	}
}
