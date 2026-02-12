// =============================================================================
// IronvaleQuestManager.h — Central quest tracking and progression subsystem
// Project Ironvale
//
// Game instance subsystem — persists across level loads. Manages all active,
// completed, and failed quests. Subscribes to EventBus events to automatically
// update objective progress (kills, item pickups, location arrivals).
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/IronvaleSaveTypes.h"
#include "Quests/IronvaleQuestTypes.h"
#include "IronvaleQuestManager.generated.h"

class UIronvaleQuestAsset;
class UIronvaleQuestObjectiveTracker;
class UIronvaleEventBus;

/**
 * Runtime state for a single quest stage, including per-objective trackers.
 */
USTRUCT()
struct FIronvaleQuestStageRuntime
{
	GENERATED_BODY()

	/** Index of the stage in the quest definition's Stages array */
	UPROPERTY()
	int32 StageIndex = 0;

	/** Objective trackers — one per objective in this stage */
	UPROPERTY()
	TArray<TObjectPtr<UIronvaleQuestObjectiveTracker>> ObjectiveTrackers;
};

/**
 * Complete runtime state for a single quest.
 */
USTRUCT()
struct FIronvaleQuestRuntime
{
	GENERATED_BODY()

	/** Reference to the quest definition asset */
	UPROPERTY()
	TObjectPtr<UIronvaleQuestAsset> QuestAsset = nullptr;

	/** Current quest state */
	UPROPERTY()
	EIronvaleQuestState State = EIronvaleQuestState::NotStarted;

	/** Current stage runtime data */
	UPROPERTY()
	FIronvaleQuestStageRuntime CurrentStage;
};

/**
 * Quest manager subsystem — the central authority for all quest operations.
 *
 * Subscribes to EventBus delegates (character death, item pickup, location reached)
 * and automatically increments matching objective trackers. Systems should not
 * modify quest state directly; use this subsystem's public API.
 */
UCLASS()
class IRONVALE_API UIronvaleQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// =========================================================================
	// QUEST LIFECYCLE
	// =========================================================================

	/**
	 * Start a quest from a quest asset. Validates prerequisites and reputation.
	 * @return true if the quest was successfully started
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Quest")
	bool StartQuest(UIronvaleQuestAsset* QuestAsset);

	/**
	 * Start a quest by ID. Searches registered quest assets.
	 * @return true if the quest was found and started
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Quest")
	bool StartQuestByID(FName QuestID);

	/**
	 * Advance a quest to its next stage. Evaluates branch conditions.
	 * Called internally when all objectives are met, or externally by scripts.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Quest")
	void AdvanceQuestStage(FName QuestID);

	/**
	 * Fail a quest. Executes stage fail actions and marks as failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Quest")
	void FailQuest(FName QuestID);

	// =========================================================================
	// OBJECTIVE TRACKING
	// =========================================================================

	/**
	 * Manually update objective progress for a quest.
	 * @param QuestID     Quest to update
	 * @param ObjectiveIndex  Index into the current stage's objectives array
	 * @param Delta       Amount to add (usually 1)
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Quest")
	void UpdateObjectiveProgress(FName QuestID, int32 ObjectiveIndex, int32 Delta = 1);

	// =========================================================================
	// QUERY
	// =========================================================================

	/** Get the current state of a quest */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest")
	EIronvaleQuestState GetQuestState(FName QuestID) const;

	/** Get the current stage index of a quest (INDEX_NONE if not active) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest")
	int32 GetQuestStage(FName QuestID) const;

	/** Get all currently active quest IDs */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest")
	TArray<FName> GetActiveQuests() const;

	/** Get all completed quest IDs */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest")
	TArray<FName> GetCompletedQuests() const;

	/** Check if a specific quest is completed */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest")
	bool IsQuestCompleted(FName QuestID) const;

	/** Get the objective tracker for a specific objective in a quest */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest")
	UIronvaleQuestObjectiveTracker* GetObjectiveTracker(FName QuestID, int32 ObjectiveIndex) const;

	// =========================================================================
	// QUEST ASSET REGISTRY
	// =========================================================================

	/** Register a quest asset so it can be started by ID */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Quest")
	void RegisterQuestAsset(UIronvaleQuestAsset* QuestAsset);

	/** Find a registered quest asset by ID */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Quest")
	UIronvaleQuestAsset* FindQuestAsset(FName QuestID) const;

	// =========================================================================
	// SAVE / LOAD
	// =========================================================================

	/** Export all quest states for serialization */
	TArray<FIronvaleSavedQuest> ExportQuestStates() const;

	/** Import quest states from save data */
	void ImportQuestStates(const TArray<FIronvaleSavedQuest>& SavedQuests);

protected:
	/** All runtime quest data, keyed by QuestID */
	UPROPERTY()
	TMap<FName, FIronvaleQuestRuntime> ActiveQuests;

	/** Saved state for quests that are completed/failed (no longer need full runtime) */
	TMap<FName, FIronvaleSavedQuest> ArchivedQuests;

	/** Registry of known quest assets, keyed by QuestID */
	UPROPERTY()
	TMap<FName, TObjectPtr<UIronvaleQuestAsset>> QuestAssetRegistry;

	/** Cached pointer to the world's event bus (re-acquired when world changes) */
	UPROPERTY()
	TObjectPtr<UIronvaleEventBus> CachedEventBus = nullptr;

	// =========================================================================
	// EVENT BUS HANDLERS
	// =========================================================================

	/** Bind to EventBus delegates in the current world */
	void BindToEventBus(UWorld* World);

	/** Unbind from EventBus delegates */
	void UnbindFromEventBus();

	UFUNCTION()
	void HandleCharacterDeath(AActor* DeadCharacter, AActor* Killer);

	UFUNCTION()
	void HandleItemPickedUp(AActor* Character, FName ItemID);

	UFUNCTION()
	void HandleLocationReached(AActor* Character, FName LocationID);

	// =========================================================================
	// INTERNAL HELPERS
	// =========================================================================

	/** Create objective trackers for the given stage */
	void CreateStageTrackers(FIronvaleQuestRuntime& QuestRuntime, int32 StageIndex);

	/** Check if the current stage's completion conditions are met */
	bool IsStageComplete(const FIronvaleQuestRuntime& QuestRuntime) const;

	/** Process the completion of a quest stage */
	void ProcessStageCompletion(FName QuestID);

	/** Complete a quest — grant rewards, archive, and broadcast */
	void CompleteQuest(FName QuestID);

	/** Grant rewards defined in the quest definition */
	void GrantRewards(const FIronvaleQuestRewards& Rewards);

	/** Execute dialogue actions (delegates to dialogue subsystem pattern) */
	void ExecuteActions(const TArray<FIronvaleDialogueAction>& Actions);

	/** Evaluate branch conditions to determine next stage */
	FName EvaluateBranch(const FIronvaleQuestStageData& Stage) const;

	/** Check if quest prerequisites are met */
	bool ArePrerequisitesMet(const FIronvaleQuestDefinition& QuestDef) const;

	/** Iterate all active quests and update matching objectives */
	void UpdateObjectivesForEvent(EIronvaleObjectiveType Type, FName TargetID, int32 Delta);

	/** Handle world initialization to bind events */
	void OnWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS);

	/** Handle world cleanup to unbind events */
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
};
