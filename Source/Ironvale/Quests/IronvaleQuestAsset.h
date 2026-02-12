// =============================================================================
// IronvaleQuestAsset.h — Data asset containing a complete quest definition
// Project Ironvale
//
// Designers create these in the editor to define quests. The quest manager
// loads them via the asset manager or direct references and uses the
// FIronvaleQuestDefinition to drive runtime quest state.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "IronvaleQuestTypes.h"
#include "IronvaleQuestAsset.generated.h"

/**
 * Primary data asset containing a single quest definition.
 *
 * Usage:
 *   1. Create a new UIronvaleQuestAsset in the Content Browser
 *   2. Fill out QuestData with stages, objectives, rewards, and prerequisites
 *   3. The quest manager references these assets to drive quest logic
 *
 * The asset ID uses "IronvaleQuest" as the type, with QuestData.QuestID as the name,
 * enabling async loading via the UE Asset Manager.
 */
UCLASS(BlueprintType)
class IRONVALE_API UIronvaleQuestAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Complete quest definition — stages, objectives, rewards, prerequisites */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest",
		meta = (ShowOnlyInnerProperties))
	FIronvaleQuestDefinition QuestData;

	/** Convenience accessor for the quest ID */
	UFUNCTION(BlueprintPure, Category = "Quest")
	FName GetQuestID() const { return QuestData.QuestID; }

	/** Convenience accessor for the display name */
	UFUNCTION(BlueprintPure, Category = "Quest")
	FText GetDisplayName() const { return QuestData.DisplayName; }

	/** Get the number of stages in this quest */
	UFUNCTION(BlueprintPure, Category = "Quest")
	int32 GetStageCount() const { return QuestData.Stages.Num(); }

	/** Find a stage by ID within this quest */
	const FIronvaleQuestStageData* FindStage(FName StageID) const
	{
		return QuestData.FindStage(StageID);
	}

	// =========================================================================
	// UPrimaryDataAsset interface
	// =========================================================================

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("IronvaleQuest"), QuestData.QuestID);
	}

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		// Validate that all stages have unique IDs
		TSet<FName> SeenIDs;
		for (const FIronvaleQuestStageData& Stage : QuestData.Stages)
		{
			if (Stage.StageID.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("Quest %s has a stage with empty StageID"),
					*QuestData.QuestID.ToString());
			}
			else if (SeenIDs.Contains(Stage.StageID))
			{
				UE_LOG(LogTemp, Warning, TEXT("Quest %s has duplicate StageID: %s"),
					*QuestData.QuestID.ToString(), *Stage.StageID.ToString());
			}
			SeenIDs.Add(Stage.StageID);
		}
	}
#endif
};
