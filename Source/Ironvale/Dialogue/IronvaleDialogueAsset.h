// =============================================================================
// IronvaleDialogueAsset.h — UDataAsset containing a complete dialogue tree
// Project Ironvale
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IronvaleDialogueTypes.h"
#include "IronvaleDialogueAsset.generated.h"

UCLASS(BlueprintType)
class IRONVALE_API UIronvaleDialogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique dialogue tree ID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueID;

	/** ID of the entry node (first node to evaluate) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName EntryNodeID;

	/** All nodes in this dialogue tree */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FIronvaleDialogueNode> Nodes;

	/** Find a node by ID */
	const FIronvaleDialogueNode* FindNode(FName NodeID) const
	{
		for (const FIronvaleDialogueNode& Node : Nodes)
		{
			if (Node.NodeID == NodeID) return &Node;
		}
		return nullptr;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("IronvaleDialogue", DialogueID);
	}
};
