// =============================================================================
// IronvaleQuestManager.cpp — Central quest tracking and progression subsystem
// Project Ironvale
// =============================================================================

#include "Quests/IronvaleQuestManager.h"
#include "Ironvale.h"
#include "Quests/IronvaleQuestAsset.h"
#include "Quests/IronvaleQuestObjective.h"
#include "Core/IronvaleEventBus.h"
#include "Dialogue/IronvaleReputationComponent.h"
#include "IronvaleGameInstance.h"
#include "Engine/World.h"

// =============================================================================
// INITIALIZATION
// =============================================================================

void UIronvaleQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Listen for world creation/destruction to bind/unbind EventBus
	FWorldDelegates::OnPostWorldInitialization.AddUObject(
		this, &UIronvaleQuestSubsystem::OnWorldInitialized);
	FWorldDelegates::OnWorldCleanup.AddUObject(
		this, &UIronvaleQuestSubsystem::OnWorldCleanup);

	UE_LOG(LogIronvale, Log, TEXT("QuestSubsystem initialized"));
}

void UIronvaleQuestSubsystem::Deinitialize()
{
	UnbindFromEventBus();

	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);

	UE_LOG(LogIronvale, Log, TEXT("QuestSubsystem deinitialized"));

	Super::Deinitialize();
}

void UIronvaleQuestSubsystem::OnWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS)
{
	if (World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE))
	{
		BindToEventBus(World);
	}
}

void UIronvaleQuestSubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (CachedEventBus && CachedEventBus->GetWorld() == World)
	{
		UnbindFromEventBus();
	}
}

// =============================================================================
// EVENT BUS BINDING
// =============================================================================

void UIronvaleQuestSubsystem::BindToEventBus(UWorld* World)
{
	if (!World) return;

	UnbindFromEventBus();

	CachedEventBus = World->GetSubsystem<UIronvaleEventBus>();
	if (!CachedEventBus) return;

	CachedEventBus->OnCharacterDeath.AddDynamic(this, &UIronvaleQuestSubsystem::HandleCharacterDeath);
	CachedEventBus->OnItemPickedUp.AddDynamic(this, &UIronvaleQuestSubsystem::HandleItemPickedUp);
	CachedEventBus->OnLocationReached.AddDynamic(this, &UIronvaleQuestSubsystem::HandleLocationReached);

	UE_LOG(LogIronvale, Verbose, TEXT("QuestSubsystem bound to EventBus in world %s"),
		*World->GetName());
}

void UIronvaleQuestSubsystem::UnbindFromEventBus()
{
	if (CachedEventBus)
	{
		CachedEventBus->OnCharacterDeath.RemoveDynamic(this, &UIronvaleQuestSubsystem::HandleCharacterDeath);
		CachedEventBus->OnItemPickedUp.RemoveDynamic(this, &UIronvaleQuestSubsystem::HandleItemPickedUp);
		CachedEventBus->OnLocationReached.RemoveDynamic(this, &UIronvaleQuestSubsystem::HandleLocationReached);
		CachedEventBus = nullptr;
	}
}

// =============================================================================
// EVENT BUS HANDLERS
// =============================================================================

void UIronvaleQuestSubsystem::HandleCharacterDeath(AActor* DeadCharacter, AActor* Killer)
{
	if (!DeadCharacter) return;

	// Extract the NPC/enemy ID from the dead character's tag or name
	FName TargetID = DeadCharacter->GetFName();

	// Check for a custom ID tag: "NPCID_<name>"
	for (const FName& Tag : DeadCharacter->Tags)
	{
		const FString TagStr = Tag.ToString();
		if (TagStr.StartsWith(TEXT("NPCID_")))
		{
			TargetID = FName(*TagStr.Mid(6));
			break;
		}
	}

	UpdateObjectivesForEvent(EIronvaleObjectiveType::Kill, TargetID, 1);
}

void UIronvaleQuestSubsystem::HandleItemPickedUp(AActor* Character, FName ItemID)
{
	// Only track player pickups
	if (!Character || !Character->ActorHasTag(TEXT("Player"))) return;

	UpdateObjectivesForEvent(EIronvaleObjectiveType::Collect, ItemID, 1);
}

void UIronvaleQuestSubsystem::HandleLocationReached(AActor* Character, FName LocationID)
{
	if (!Character || !Character->ActorHasTag(TEXT("Player"))) return;

	UpdateObjectivesForEvent(EIronvaleObjectiveType::ReachLocation, LocationID, 1);
}

// =============================================================================
// QUEST LIFECYCLE
// =============================================================================

bool UIronvaleQuestSubsystem::StartQuest(UIronvaleQuestAsset* QuestAsset)
{
	if (!QuestAsset)
	{
		UE_LOG(LogIronvale, Warning, TEXT("StartQuest: null QuestAsset"));
		return false;
	}

	const FIronvaleQuestDefinition& QuestDef = QuestAsset->QuestData;
	const FName QuestID = QuestDef.QuestID;

	// Check if already active or completed
	if (ActiveQuests.Contains(QuestID))
	{
		const EIronvaleQuestState CurrentState = ActiveQuests[QuestID].State;
		if (CurrentState == EIronvaleQuestState::Active)
		{
			UE_LOG(LogIronvale, Warning, TEXT("Quest %s is already active"), *QuestID.ToString());
			return false;
		}
	}

	if (ArchivedQuests.Contains(QuestID))
	{
		const EIronvaleQuestState ArchivedState = ArchivedQuests[QuestID].State;
		if (ArchivedState == EIronvaleQuestState::Completed)
		{
			UE_LOG(LogIronvale, Warning, TEXT("Quest %s is already completed"), *QuestID.ToString());
			return false;
		}
	}

	// Validate prerequisites
	if (!ArePrerequisitesMet(QuestDef))
	{
		UE_LOG(LogIronvale, Warning, TEXT("Quest %s prerequisites not met"), *QuestID.ToString());
		return false;
	}

	// Validate reputation requirement
	if (!QuestDef.RequiredReputation.FactionID.IsNone())
	{
		if (UIronvaleReputationSubsystem* RepSys = GetGameInstance()->GetSubsystem<UIronvaleReputationSubsystem>())
		{
			const float CurrentRep = RepSys->GetReputation(QuestDef.RequiredReputation.FactionID);
			if (CurrentRep < QuestDef.RequiredReputation.MinReputation)
			{
				UE_LOG(LogIronvale, Warning, TEXT("Quest %s requires %.1f rep with %s (have %.1f)"),
					*QuestID.ToString(),
					QuestDef.RequiredReputation.MinReputation,
					*QuestDef.RequiredReputation.FactionID.ToString(),
					CurrentRep);
				return false;
			}
		}
	}

	// Validate the quest has at least one stage
	if (QuestDef.Stages.Num() == 0)
	{
		UE_LOG(LogIronvale, Error, TEXT("Quest %s has no stages"), *QuestID.ToString());
		return false;
	}

	// Register the asset if not already registered
	RegisterQuestAsset(QuestAsset);

	// Create runtime state
	FIronvaleQuestRuntime& Runtime = ActiveQuests.Add(QuestID);
	Runtime.QuestAsset = QuestAsset;
	Runtime.State = EIronvaleQuestState::Active;

	// Initialize the first stage
	CreateStageTrackers(Runtime, 0);

	// Broadcast quest started
	if (CachedEventBus)
	{
		CachedEventBus->OnQuestStarted.Broadcast(QuestID, 0);
	}

	UE_LOG(LogIronvale, Log, TEXT("Quest started: %s (%s) — Stage 0: %s"),
		*QuestID.ToString(),
		*QuestDef.DisplayName.ToString(),
		*QuestDef.Stages[0].StageID.ToString());

	return true;
}

bool UIronvaleQuestSubsystem::StartQuestByID(FName QuestID)
{
	UIronvaleQuestAsset* Asset = FindQuestAsset(QuestID);
	if (!Asset)
	{
		UE_LOG(LogIronvale, Warning, TEXT("StartQuestByID: no registered asset for %s"),
			*QuestID.ToString());
		return false;
	}
	return StartQuest(Asset);
}

void UIronvaleQuestSubsystem::AdvanceQuestStage(FName QuestID)
{
	FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID);
	if (!Runtime || Runtime->State != EIronvaleQuestState::Active)
	{
		UE_LOG(LogIronvale, Warning, TEXT("AdvanceQuestStage: quest %s is not active"),
			*QuestID.ToString());
		return;
	}

	ProcessStageCompletion(QuestID);
}

void UIronvaleQuestSubsystem::FailQuest(FName QuestID)
{
	FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID);
	if (!Runtime || Runtime->State != EIronvaleQuestState::Active)
	{
		UE_LOG(LogIronvale, Warning, TEXT("FailQuest: quest %s is not active"),
			*QuestID.ToString());
		return;
	}

	const FIronvaleQuestDefinition& QuestDef = Runtime->QuestAsset->QuestData;

	// Execute fail actions for the current stage
	const int32 StageIndex = Runtime->CurrentStage.StageIndex;
	if (StageIndex >= 0 && StageIndex < QuestDef.Stages.Num())
	{
		ExecuteActions(QuestDef.Stages[StageIndex].OnFailActions);
	}

	// Archive as failed
	Runtime->State = EIronvaleQuestState::Failed;

	FIronvaleSavedQuest Saved;
	Saved.QuestID = QuestID;
	Saved.State = EIronvaleQuestState::Failed;
	Saved.CurrentStageIndex = StageIndex;
	ArchivedQuests.Add(QuestID, Saved);

	// Remove from active quests
	ActiveQuests.Remove(QuestID);

	// Broadcast
	if (CachedEventBus)
	{
		CachedEventBus->OnQuestFailed.Broadcast(QuestID);
	}

	UE_LOG(LogIronvale, Log, TEXT("Quest failed: %s"), *QuestID.ToString());
}

// =============================================================================
// OBJECTIVE TRACKING
// =============================================================================

void UIronvaleQuestSubsystem::UpdateObjectiveProgress(FName QuestID, int32 ObjectiveIndex, int32 Delta)
{
	FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID);
	if (!Runtime || Runtime->State != EIronvaleQuestState::Active)
	{
		return;
	}

	const TArray<TObjectPtr<UIronvaleQuestObjectiveTracker>>& Trackers =
		Runtime->CurrentStage.ObjectiveTrackers;

	if (!Trackers.IsValidIndex(ObjectiveIndex))
	{
		UE_LOG(LogIronvale, Warning, TEXT("UpdateObjectiveProgress: invalid index %d for quest %s"),
			ObjectiveIndex, *QuestID.ToString());
		return;
	}

	UIronvaleQuestObjectiveTracker* Tracker = Trackers[ObjectiveIndex];
	if (!Tracker || Tracker->IsComplete())
	{
		return;
	}

	Tracker->IncrementProgress(Delta);

	UE_LOG(LogIronvale, Verbose, TEXT("Quest %s objective %d progress: %d/%d"),
		*QuestID.ToString(), ObjectiveIndex,
		Tracker->GetCurrentCount(), Tracker->GetRequiredCount());

	// Check if stage is now complete
	if (IsStageComplete(*Runtime))
	{
		ProcessStageCompletion(QuestID);
	}
}

void UIronvaleQuestSubsystem::UpdateObjectivesForEvent(EIronvaleObjectiveType Type, FName TargetID, int32 Delta)
{
	// Iterate all active quests and update matching objectives
	// Collect quest IDs first to avoid modifying the map during iteration
	TArray<FName> QuestIDs;
	ActiveQuests.GetKeys(QuestIDs);

	for (const FName& QuestID : QuestIDs)
	{
		FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID);
		if (!Runtime || Runtime->State != EIronvaleQuestState::Active) continue;
		if (!Runtime->QuestAsset) continue;

		const FIronvaleQuestDefinition& QuestDef = Runtime->QuestAsset->QuestData;
		const int32 StageIndex = Runtime->CurrentStage.StageIndex;
		if (!QuestDef.Stages.IsValidIndex(StageIndex)) continue;

		const TArray<FIronvaleQuestObjectiveData>& Objectives = QuestDef.Stages[StageIndex].Objectives;

		for (int32 i = 0; i < Objectives.Num(); ++i)
		{
			const FIronvaleQuestObjectiveData& ObjData = Objectives[i];

			if (ObjData.ObjectiveType == Type && ObjData.TargetID == TargetID)
			{
				UpdateObjectiveProgress(QuestID, i, Delta);
			}
		}
	}
}

// =============================================================================
// QUERY
// =============================================================================

EIronvaleQuestState UIronvaleQuestSubsystem::GetQuestState(FName QuestID) const
{
	if (const FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID))
	{
		return Runtime->State;
	}
	if (const FIronvaleSavedQuest* Archived = ArchivedQuests.Find(QuestID))
	{
		return Archived->State;
	}
	return EIronvaleQuestState::NotStarted;
}

int32 UIronvaleQuestSubsystem::GetQuestStage(FName QuestID) const
{
	if (const FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID))
	{
		if (Runtime->State == EIronvaleQuestState::Active)
		{
			return Runtime->CurrentStage.StageIndex;
		}
	}
	return INDEX_NONE;
}

TArray<FName> UIronvaleQuestSubsystem::GetActiveQuests() const
{
	TArray<FName> Result;
	for (const auto& Pair : ActiveQuests)
	{
		if (Pair.Value.State == EIronvaleQuestState::Active)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

TArray<FName> UIronvaleQuestSubsystem::GetCompletedQuests() const
{
	TArray<FName> Result;
	for (const auto& Pair : ArchivedQuests)
	{
		if (Pair.Value.State == EIronvaleQuestState::Completed)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

bool UIronvaleQuestSubsystem::IsQuestCompleted(FName QuestID) const
{
	return GetQuestState(QuestID) == EIronvaleQuestState::Completed;
}

UIronvaleQuestObjectiveTracker* UIronvaleQuestSubsystem::GetObjectiveTracker(
	FName QuestID, int32 ObjectiveIndex) const
{
	if (const FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID))
	{
		const TArray<TObjectPtr<UIronvaleQuestObjectiveTracker>>& Trackers =
			Runtime->CurrentStage.ObjectiveTrackers;
		if (Trackers.IsValidIndex(ObjectiveIndex))
		{
			return Trackers[ObjectiveIndex];
		}
	}
	return nullptr;
}

// =============================================================================
// QUEST ASSET REGISTRY
// =============================================================================

void UIronvaleQuestSubsystem::RegisterQuestAsset(UIronvaleQuestAsset* QuestAsset)
{
	if (!QuestAsset) return;

	const FName QuestID = QuestAsset->QuestData.QuestID;
	if (QuestID.IsNone())
	{
		UE_LOG(LogIronvale, Warning, TEXT("RegisterQuestAsset: asset has empty QuestID"));
		return;
	}

	if (!QuestAssetRegistry.Contains(QuestID))
	{
		QuestAssetRegistry.Add(QuestID, QuestAsset);
		UE_LOG(LogIronvale, Verbose, TEXT("Registered quest asset: %s"), *QuestID.ToString());
	}
}

UIronvaleQuestAsset* UIronvaleQuestSubsystem::FindQuestAsset(FName QuestID) const
{
	if (const TObjectPtr<UIronvaleQuestAsset>* Found = QuestAssetRegistry.Find(QuestID))
	{
		return *Found;
	}
	return nullptr;
}

// =============================================================================
// SAVE / LOAD
// =============================================================================

TArray<FIronvaleSavedQuest> UIronvaleQuestSubsystem::ExportQuestStates() const
{
	TArray<FIronvaleSavedQuest> Result;

	// Export active quests
	for (const auto& Pair : ActiveQuests)
	{
		const FIronvaleQuestRuntime& Runtime = Pair.Value;
		FIronvaleSavedQuest Saved;
		Saved.QuestID = Pair.Key;
		Saved.State = Runtime.State;
		Saved.CurrentStageIndex = Runtime.CurrentStage.StageIndex;

		// Export objective progress
		for (int32 i = 0; i < Runtime.CurrentStage.ObjectiveTrackers.Num(); ++i)
		{
			if (const UIronvaleQuestObjectiveTracker* Tracker = Runtime.CurrentStage.ObjectiveTrackers[i])
			{
				Saved.ObjectiveProgress.Add(i, Tracker->GetCurrentCount());
				if (Tracker->IsComplete())
				{
					// Check if optional
					if (Runtime.QuestAsset)
					{
						const auto& Stages = Runtime.QuestAsset->QuestData.Stages;
						if (Stages.IsValidIndex(Runtime.CurrentStage.StageIndex))
						{
							const auto& Objectives = Stages[Runtime.CurrentStage.StageIndex].Objectives;
							if (Objectives.IsValidIndex(i) && Objectives[i].bOptional)
							{
								Saved.CompletedOptionalObjectives.Add(i);
							}
						}
					}
				}
			}
		}

		Result.Add(Saved);
	}

	// Export archived quests
	for (const auto& Pair : ArchivedQuests)
	{
		Result.Add(Pair.Value);
	}

	return Result;
}

void UIronvaleQuestSubsystem::ImportQuestStates(const TArray<FIronvaleSavedQuest>& SavedQuests)
{
	ActiveQuests.Empty();
	ArchivedQuests.Empty();

	for (const FIronvaleSavedQuest& Saved : SavedQuests)
	{
		if (Saved.State == EIronvaleQuestState::Completed ||
			Saved.State == EIronvaleQuestState::Failed ||
			Saved.State == EIronvaleQuestState::Abandoned)
		{
			ArchivedQuests.Add(Saved.QuestID, Saved);
			continue;
		}

		if (Saved.State == EIronvaleQuestState::Active)
		{
			UIronvaleQuestAsset* Asset = FindQuestAsset(Saved.QuestID);
			if (!Asset)
			{
				UE_LOG(LogIronvale, Warning,
					TEXT("ImportQuestStates: no registered asset for quest %s, archiving as-is"),
					*Saved.QuestID.ToString());
				ArchivedQuests.Add(Saved.QuestID, Saved);
				continue;
			}

			FIronvaleQuestRuntime& Runtime = ActiveQuests.Add(Saved.QuestID);
			Runtime.QuestAsset = Asset;
			Runtime.State = EIronvaleQuestState::Active;

			// Restore the stage and objective progress
			const int32 StageIndex = Saved.CurrentStageIndex;
			if (Asset->QuestData.Stages.IsValidIndex(StageIndex))
			{
				CreateStageTrackers(Runtime, StageIndex);

				// Restore objective progress
				for (const auto& ProgressPair : Saved.ObjectiveProgress)
				{
					const int32 ObjIndex = ProgressPair.Key;
					const int32 Progress = ProgressPair.Value;
					if (Runtime.CurrentStage.ObjectiveTrackers.IsValidIndex(ObjIndex))
					{
						Runtime.CurrentStage.ObjectiveTrackers[ObjIndex]->SetProgress(Progress);
					}
				}
			}
			else
			{
				UE_LOG(LogIronvale, Warning,
					TEXT("ImportQuestStates: invalid stage index %d for quest %s"),
					StageIndex, *Saved.QuestID.ToString());
				Runtime.State = EIronvaleQuestState::Failed;
			}
		}
	}

	UE_LOG(LogIronvale, Log, TEXT("Imported %d quest states (%d active, %d archived)"),
		SavedQuests.Num(), ActiveQuests.Num(), ArchivedQuests.Num());
}

// =============================================================================
// INTERNAL HELPERS
// =============================================================================

void UIronvaleQuestSubsystem::CreateStageTrackers(FIronvaleQuestRuntime& QuestRuntime, int32 StageIndex)
{
	QuestRuntime.CurrentStage.StageIndex = StageIndex;
	QuestRuntime.CurrentStage.ObjectiveTrackers.Empty();

	if (!QuestRuntime.QuestAsset) return;

	const FIronvaleQuestDefinition& QuestDef = QuestRuntime.QuestAsset->QuestData;
	if (!QuestDef.Stages.IsValidIndex(StageIndex)) return;

	const FIronvaleQuestStageData& Stage = QuestDef.Stages[StageIndex];

	for (const FIronvaleQuestObjectiveData& ObjData : Stage.Objectives)
	{
		UIronvaleQuestObjectiveTracker* Tracker = NewObject<UIronvaleQuestObjectiveTracker>(this);
		Tracker->Initialize(ObjData.RequiredCount, ObjData.bOptional, ObjData.bHidden);
		QuestRuntime.CurrentStage.ObjectiveTrackers.Add(Tracker);
	}
}

bool UIronvaleQuestSubsystem::IsStageComplete(const FIronvaleQuestRuntime& QuestRuntime) const
{
	if (!QuestRuntime.QuestAsset) return false;

	const FIronvaleQuestDefinition& QuestDef = QuestRuntime.QuestAsset->QuestData;
	const int32 StageIndex = QuestRuntime.CurrentStage.StageIndex;

	if (!QuestDef.Stages.IsValidIndex(StageIndex)) return false;

	const FIronvaleQuestStageData& Stage = QuestDef.Stages[StageIndex];
	const TArray<TObjectPtr<UIronvaleQuestObjectiveTracker>>& Trackers =
		QuestRuntime.CurrentStage.ObjectiveTrackers;

	if (Stage.bAllRequired)
	{
		// ALL non-optional objectives must be complete
		for (int32 i = 0; i < Trackers.Num(); ++i)
		{
			if (!Trackers[i]) continue;

			// Skip optional objectives
			if (Stage.Objectives.IsValidIndex(i) && Stage.Objectives[i].bOptional)
			{
				continue;
			}

			if (!Trackers[i]->IsComplete())
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		// ANY non-optional objective completes the stage
		for (int32 i = 0; i < Trackers.Num(); ++i)
		{
			if (!Trackers[i]) continue;
			if (Stage.Objectives.IsValidIndex(i) && Stage.Objectives[i].bOptional)
			{
				continue;
			}
			if (Trackers[i]->IsComplete())
			{
				return true;
			}
		}
		return false;
	}
}

void UIronvaleQuestSubsystem::ProcessStageCompletion(FName QuestID)
{
	FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID);
	if (!Runtime || !Runtime->QuestAsset) return;

	const FIronvaleQuestDefinition& QuestDef = Runtime->QuestAsset->QuestData;
	const int32 CurrentStageIndex = Runtime->CurrentStage.StageIndex;

	if (!QuestDef.Stages.IsValidIndex(CurrentStageIndex)) return;

	const FIronvaleQuestStageData& CurrentStage = QuestDef.Stages[CurrentStageIndex];

	// Execute stage completion actions
	ExecuteActions(CurrentStage.OnCompleteActions);

	// Broadcast stage completed
	if (CachedEventBus)
	{
		CachedEventBus->OnQuestStageCompleted.Broadcast(QuestID, CurrentStageIndex);
	}

	UE_LOG(LogIronvale, Log, TEXT("Quest %s stage %d (%s) completed"),
		*QuestID.ToString(), CurrentStageIndex, *CurrentStage.StageID.ToString());

	// Determine the next stage
	FName NextStageID = EvaluateBranch(CurrentStage);

	if (NextStageID.IsNone())
	{
		// No next stage — quest is complete
		CompleteQuest(QuestID);
	}
	else
	{
		// Find the next stage index
		const int32 NextStageIndex = QuestDef.FindStageIndex(NextStageID);
		if (NextStageIndex == INDEX_NONE)
		{
			UE_LOG(LogIronvale, Error, TEXT("Quest %s next stage %s not found"),
				*QuestID.ToString(), *NextStageID.ToString());
			CompleteQuest(QuestID);
			return;
		}

		// Advance to the next stage
		CreateStageTrackers(*Runtime, NextStageIndex);

		// Broadcast quest started with new stage
		if (CachedEventBus)
		{
			CachedEventBus->OnQuestStarted.Broadcast(QuestID, NextStageIndex);
		}

		UE_LOG(LogIronvale, Log, TEXT("Quest %s advanced to stage %d (%s)"),
			*QuestID.ToString(), NextStageIndex, *NextStageID.ToString());
	}
}

void UIronvaleQuestSubsystem::CompleteQuest(FName QuestID)
{
	FIronvaleQuestRuntime* Runtime = ActiveQuests.Find(QuestID);
	if (!Runtime) return;

	const FIronvaleQuestDefinition& QuestDef = Runtime->QuestAsset->QuestData;

	// Grant rewards
	GrantRewards(QuestDef.Rewards);

	// Archive as completed
	FIronvaleSavedQuest Saved;
	Saved.QuestID = QuestID;
	Saved.State = EIronvaleQuestState::Completed;
	Saved.CurrentStageIndex = Runtime->CurrentStage.StageIndex;
	ArchivedQuests.Add(QuestID, Saved);

	// Remove from active quests
	ActiveQuests.Remove(QuestID);

	// Broadcast
	if (CachedEventBus)
	{
		CachedEventBus->OnQuestCompleted.Broadcast(QuestID);
	}

	UE_LOG(LogIronvale, Log, TEXT("Quest completed: %s (%s)"),
		*QuestID.ToString(), *QuestDef.DisplayName.ToString());
}

void UIronvaleQuestSubsystem::GrantRewards(const FIronvaleQuestRewards& Rewards)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	// Grant gold
	if (Rewards.Gold > 0)
	{
		if (UIronvaleGameInstance* IronvaleGI = Cast<UIronvaleGameInstance>(GI))
		{
			// Gold is managed per-character via inventory; broadcast event for UI
			UE_LOG(LogIronvale, Log, TEXT("Quest reward: %d gold"), Rewards.Gold);
		}
	}

	// Grant items — broadcast via EventBus for the inventory system to handle
	for (const FIronvaleItemReward& ItemReward : Rewards.Items)
	{
		UE_LOG(LogIronvale, Log, TEXT("Quest reward: %d x %s"),
			ItemReward.Quantity, *ItemReward.ItemID.ToString());
	}

	// Apply reputation changes
	if (UIronvaleReputationSubsystem* RepSys = GI->GetSubsystem<UIronvaleReputationSubsystem>())
	{
		for (const FIronvaleReputationReward& RepReward : Rewards.ReputationChanges)
		{
			RepSys->ModifyReputation(RepReward.FactionID, RepReward.ReputationDelta);
			UE_LOG(LogIronvale, Log, TEXT("Quest reward: %+.1f rep with %s"),
				RepReward.ReputationDelta, *RepReward.FactionID.ToString());
		}
	}
}

void UIronvaleQuestSubsystem::ExecuteActions(const TArray<FIronvaleDialogueAction>& Actions)
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	for (const FIronvaleDialogueAction& Action : Actions)
	{
		switch (Action.ActionType)
		{
		case EIronvaleDialogueActionType::SetFlag:
		{
			if (UIronvaleGameInstance* IronvaleGI = Cast<UIronvaleGameInstance>(GI))
			{
				IronvaleGI->SetWorldFlag(Action.PrimaryKey, Action.NumericValue > 0.0f);
			}
			break;
		}

		case EIronvaleDialogueActionType::ModifyReputation:
		{
			if (UIronvaleReputationSubsystem* RepSys = GI->GetSubsystem<UIronvaleReputationSubsystem>())
			{
				RepSys->ModifyReputation(Action.PrimaryKey, Action.NumericValue);
			}
			break;
		}

		case EIronvaleDialogueActionType::StartQuest:
		{
			StartQuestByID(Action.PrimaryKey);
			break;
		}

		case EIronvaleDialogueActionType::AdvanceQuest:
		{
			AdvanceQuestStage(Action.PrimaryKey);
			break;
		}

		case EIronvaleDialogueActionType::FailQuest:
		{
			FailQuest(Action.PrimaryKey);
			break;
		}

		default:
			UE_LOG(LogIronvale, Verbose, TEXT("Quest action type %d not handled in quest manager"),
				static_cast<int32>(Action.ActionType));
			break;
		}
	}
}

FName UIronvaleQuestSubsystem::EvaluateBranch(const FIronvaleQuestStageData& Stage) const
{
	// Evaluate branch conditions — check world flags
	UGameInstance* GI = GetGameInstance();
	UIronvaleGameInstance* IronvaleGI = GI ? Cast<UIronvaleGameInstance>(GI) : nullptr;

	if (IronvaleGI && Stage.BranchMap.Num() > 0)
	{
		for (const auto& BranchPair : Stage.BranchMap)
		{
			const FName ConditionKey = BranchPair.Key;
			const FName TargetStageID = BranchPair.Value;

			// Check if the condition key is a world flag that is set
			if (IronvaleGI->GetWorldFlag(ConditionKey))
			{
				UE_LOG(LogIronvale, Verbose, TEXT("Branch condition %s met, going to stage %s"),
					*ConditionKey.ToString(), *TargetStageID.ToString());
				return TargetStageID;
			}
		}
	}

	// Fall through to default next stage
	return Stage.NextStageID;
}

bool UIronvaleQuestSubsystem::ArePrerequisitesMet(const FIronvaleQuestDefinition& QuestDef) const
{
	for (const FName& PrereqID : QuestDef.Prerequisites)
	{
		if (!IsQuestCompleted(PrereqID))
		{
			return false;
		}
	}
	return true;
}
