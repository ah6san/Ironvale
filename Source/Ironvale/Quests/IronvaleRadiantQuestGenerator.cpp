// =============================================================================
// IronvaleRadiantQuestGenerator.cpp — Procedural quest generation implementation
// Project Ironvale
// =============================================================================

#include "Quests/IronvaleRadiantQuestGenerator.h"
#include "Ironvale.h"

// =============================================================================
// TEMPLATE MANAGEMENT
// =============================================================================

void UIronvaleRadiantQuestGenerator::RegisterTemplate(const FIronvaleRadiantTemplate& Template)
{
	if (Template.TemplateID.IsNone())
	{
		UE_LOG(LogIronvale, Warning, TEXT("RegisterTemplate: template has empty TemplateID"));
		return;
	}

	Templates.Add(Template.TemplateID, Template);
	UE_LOG(LogIronvale, Verbose, TEXT("Registered radiant template: %s"), *Template.TemplateID.ToString());
}

void UIronvaleRadiantQuestGenerator::RegisterTemplates(const TArray<FIronvaleRadiantTemplate>& InTemplates)
{
	for (const FIronvaleRadiantTemplate& Template : InTemplates)
	{
		RegisterTemplate(Template);
	}
}

TArray<FName> UIronvaleRadiantQuestGenerator::GetRegisteredTemplateIDs() const
{
	TArray<FName> Result;
	Templates.GetKeys(Result);
	return Result;
}

// =============================================================================
// QUEST GENERATION
// =============================================================================

bool UIronvaleRadiantQuestGenerator::GenerateQuest(FName TemplateID, FName RegionID,
	int32 PlayerLevel, FIronvaleQuestDefinition& OutDefinition)
{
	const FIronvaleRadiantTemplate* Template = Templates.Find(TemplateID);
	if (!Template)
	{
		UE_LOG(LogIronvale, Warning, TEXT("GenerateQuest: template %s not found"),
			*TemplateID.ToString());
		return false;
	}

	if (!IsTemplateAvailable(TemplateID, RegionID, PlayerLevel))
	{
		UE_LOG(LogIronvale, Warning, TEXT("GenerateQuest: template %s not available for region %s, level %d"),
			*TemplateID.ToString(), *RegionID.ToString(), PlayerLevel);
		return false;
	}

	bool bSuccess = false;

	switch (Template->QuestType)
	{
	case EIronvaleRadiantQuestType::Bounty:
		bSuccess = GenerateBountyQuest(*Template, RegionID, PlayerLevel, OutDefinition);
		break;
	case EIronvaleRadiantQuestType::Delivery:
		bSuccess = GenerateDeliveryQuest(*Template, RegionID, PlayerLevel, OutDefinition);
		break;
	case EIronvaleRadiantQuestType::Patrol:
		bSuccess = GeneratePatrolQuest(*Template, RegionID, PlayerLevel, OutDefinition);
		break;
	case EIronvaleRadiantQuestType::Escort:
		bSuccess = GenerateEscortQuest(*Template, RegionID, PlayerLevel, OutDefinition);
		break;
	}

	if (bSuccess)
	{
		ApplyCooldown(TemplateID, Template->CooldownDays);
		UE_LOG(LogIronvale, Log, TEXT("Generated radiant quest: %s from template %s in region %s"),
			*OutDefinition.QuestID.ToString(), *TemplateID.ToString(), *RegionID.ToString());
	}

	return bSuccess;
}

bool UIronvaleRadiantQuestGenerator::GenerateRandomQuest(FName RegionID, int32 PlayerLevel,
	FIronvaleQuestDefinition& OutDefinition)
{
	// Build a weighted list of available templates
	TArray<TPair<FName, float>> WeightedCandidates;
	float TotalWeight = 0.0f;

	for (const auto& Pair : Templates)
	{
		if (IsTemplateAvailable(Pair.Key, RegionID, PlayerLevel))
		{
			WeightedCandidates.Add(TPair<FName, float>(Pair.Key, Pair.Value.SelectionWeight));
			TotalWeight += Pair.Value.SelectionWeight;
		}
	}

	if (WeightedCandidates.Num() == 0 || TotalWeight <= 0.0f)
	{
		UE_LOG(LogIronvale, Warning,
			TEXT("GenerateRandomQuest: no available templates for region %s, level %d"),
			*RegionID.ToString(), PlayerLevel);
		return false;
	}

	// Weighted random selection
	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	FName SelectedTemplateID;

	for (const auto& Candidate : WeightedCandidates)
	{
		Roll -= Candidate.Value;
		if (Roll <= 0.0f)
		{
			SelectedTemplateID = Candidate.Key;
			break;
		}
	}

	// Fallback to last candidate if floating point drift
	if (SelectedTemplateID.IsNone() && WeightedCandidates.Num() > 0)
	{
		SelectedTemplateID = WeightedCandidates.Last().Key;
	}

	return GenerateQuest(SelectedTemplateID, RegionID, PlayerLevel, OutDefinition);
}

bool UIronvaleRadiantQuestGenerator::IsTemplateAvailable(FName TemplateID, FName RegionID,
	int32 PlayerLevel) const
{
	const FIronvaleRadiantTemplate* Template = Templates.Find(TemplateID);
	if (!Template)
	{
		return false;
	}

	// Check cooldown
	if (const int32* Remaining = TemplateCooldowns.Find(TemplateID))
	{
		if (*Remaining > 0)
		{
			return false;
		}
	}

	// Check if primary target pool has valid targets for this region/level
	if (!HasValidTargets(Template->PrimaryTargetPool, RegionID, PlayerLevel))
	{
		return false;
	}

	return true;
}

// =============================================================================
// COOLDOWN MANAGEMENT
// =============================================================================

void UIronvaleRadiantQuestGenerator::AdvanceCooldowns()
{
	TArray<FName> ToRemove;

	for (auto& Pair : TemplateCooldowns)
	{
		Pair.Value = FMath::Max(0, Pair.Value - 1);
		if (Pair.Value <= 0)
		{
			ToRemove.Add(Pair.Key);
		}
	}

	for (const FName& Key : ToRemove)
	{
		TemplateCooldowns.Remove(Key);
	}
}

void UIronvaleRadiantQuestGenerator::ResetAllCooldowns()
{
	TemplateCooldowns.Empty();
}

// =============================================================================
// BOUNTY QUEST GENERATION
// =============================================================================

bool UIronvaleRadiantQuestGenerator::GenerateBountyQuest(
	const FIronvaleRadiantTemplate& Template, FName RegionID,
	int32 PlayerLevel, FIronvaleQuestDefinition& OutDef)
{
	// Select a primary target (enemy type)
	const FName TargetID = SelectTarget(Template.PrimaryTargetPool, RegionID, PlayerLevel);
	if (TargetID.IsNone())
	{
		UE_LOG(LogIronvale, Warning, TEXT("GenerateBountyQuest: no valid target for template %s"),
			*Template.TemplateID.ToString());
		return false;
	}

	// Determine kill count
	const int32 KillCount = FMath::RandRange(
		FMath::RoundToInt32(Template.TargetCountRange.X),
		FMath::RoundToInt32(Template.TargetCountRange.Y));

	// Fill text placeholders
	TMap<FString, FString> Replacements;
	Replacements.Add(TEXT("{Target}"), TargetID.ToString());
	Replacements.Add(TEXT("{Count}"), FString::FromInt(KillCount));
	Replacements.Add(TEXT("{Location}"), RegionID.ToString());

	// Build quest definition
	OutDef.QuestID = GenerateUniqueQuestID(Template.TemplateID);
	OutDef.DisplayName = FillTextTemplate(Template.NameTemplate, Replacements);
	OutDef.Description = FillTextTemplate(Template.DescriptionTemplate, Replacements);
	OutDef.bCanFail = false;

	// Single stage: kill N enemies
	FIronvaleQuestStageData Stage;
	Stage.StageID = FName(*(OutDef.QuestID.ToString() + TEXT("_Stage0")));
	Stage.Description = OutDef.Description;
	Stage.bAllRequired = true;

	FIronvaleQuestObjectiveData Objective;
	Objective.ObjectiveType = EIronvaleObjectiveType::Kill;
	Objective.TargetID = TargetID;
	Objective.RequiredCount = KillCount;
	Objective.Description = FText::Format(
		NSLOCTEXT("IronvaleRadiant", "BountyObj", "Kill {0} {1}"),
		FText::AsNumber(KillCount),
		FText::FromName(TargetID));
	Objective.bOptional = false;
	Objective.bHidden = false;

	Stage.Objectives.Add(Objective);
	OutDef.Stages.Add(Stage);

	// Generate scaled rewards
	OutDef.Rewards = GenerateRewards(Template.RewardRange, PlayerLevel);

	return true;
}

// =============================================================================
// DELIVERY QUEST GENERATION
// =============================================================================

bool UIronvaleRadiantQuestGenerator::GenerateDeliveryQuest(
	const FIronvaleRadiantTemplate& Template, FName RegionID,
	int32 PlayerLevel, FIronvaleQuestDefinition& OutDef)
{
	// Select an item to deliver
	const FName ItemID = SelectTarget(Template.PrimaryTargetPool, RegionID, PlayerLevel);
	if (ItemID.IsNone())
	{
		return false;
	}

	// Select a delivery destination NPC
	const FName RecipientID = SelectTarget(Template.SecondaryTargetPool, RegionID, PlayerLevel);
	if (RecipientID.IsNone())
	{
		return false;
	}

	// Fill text placeholders
	TMap<FString, FString> Replacements;
	Replacements.Add(TEXT("{Item}"), ItemID.ToString());
	Replacements.Add(TEXT("{Target}"), RecipientID.ToString());
	Replacements.Add(TEXT("{Location}"), RegionID.ToString());

	// Build quest definition
	OutDef.QuestID = GenerateUniqueQuestID(Template.TemplateID);
	OutDef.DisplayName = FillTextTemplate(Template.NameTemplate, Replacements);
	OutDef.Description = FillTextTemplate(Template.DescriptionTemplate, Replacements);
	OutDef.bCanFail = false;

	// Stage 1: Collect the item
	FIronvaleQuestStageData CollectStage;
	CollectStage.StageID = FName(*(OutDef.QuestID.ToString() + TEXT("_Collect")));
	CollectStage.Description = FText::Format(
		NSLOCTEXT("IronvaleRadiant", "DeliveryCollect", "Obtain the {0}"),
		FText::FromName(ItemID));
	CollectStage.bAllRequired = true;

	FIronvaleQuestObjectiveData CollectObj;
	CollectObj.ObjectiveType = EIronvaleObjectiveType::Collect;
	CollectObj.TargetID = ItemID;
	CollectObj.RequiredCount = 1;
	CollectObj.Description = CollectStage.Description;
	CollectStage.Objectives.Add(CollectObj);

	// Stage 2: Deliver to the NPC
	FIronvaleQuestStageData DeliverStage;
	DeliverStage.StageID = FName(*(OutDef.QuestID.ToString() + TEXT("_Deliver")));
	DeliverStage.Description = FText::Format(
		NSLOCTEXT("IronvaleRadiant", "DeliveryDeliver", "Deliver the {0} to {1}"),
		FText::FromName(ItemID),
		FText::FromName(RecipientID));
	DeliverStage.bAllRequired = true;

	FIronvaleQuestObjectiveData DeliverObj;
	DeliverObj.ObjectiveType = EIronvaleObjectiveType::TalkTo;
	DeliverObj.TargetID = RecipientID;
	DeliverObj.RequiredCount = 1;
	DeliverObj.Description = DeliverStage.Description;
	DeliverStage.Objectives.Add(DeliverObj);

	// Link stages
	CollectStage.NextStageID = DeliverStage.StageID;

	OutDef.Stages.Add(CollectStage);
	OutDef.Stages.Add(DeliverStage);

	// Generate scaled rewards
	OutDef.Rewards = GenerateRewards(Template.RewardRange, PlayerLevel);

	return true;
}

// =============================================================================
// PATROL QUEST GENERATION
// =============================================================================

bool UIronvaleRadiantQuestGenerator::GeneratePatrolQuest(
	const FIronvaleRadiantTemplate& Template, FName RegionID,
	int32 PlayerLevel, FIronvaleQuestDefinition& OutDef)
{
	const int32 PatrolPointCount = FMath::RandRange(
		FMath::RoundToInt32(Template.PatrolPointCountRange.X),
		FMath::RoundToInt32(Template.PatrolPointCountRange.Y));

	// Select patrol locations
	TArray<FName> PatrolPoints = SelectMultipleTargets(
		Template.LocationPool, RegionID, PlayerLevel, PatrolPointCount);

	if (PatrolPoints.Num() < 2)
	{
		UE_LOG(LogIronvale, Warning,
			TEXT("GeneratePatrolQuest: not enough valid locations (need 2, got %d)"),
			PatrolPoints.Num());
		return false;
	}

	// Fill text placeholders
	TMap<FString, FString> Replacements;
	Replacements.Add(TEXT("{Location}"), RegionID.ToString());
	Replacements.Add(TEXT("{Count}"), FString::FromInt(PatrolPoints.Num()));

	// Build quest definition
	OutDef.QuestID = GenerateUniqueQuestID(Template.TemplateID);
	OutDef.DisplayName = FillTextTemplate(Template.NameTemplate, Replacements);
	OutDef.Description = FillTextTemplate(Template.DescriptionTemplate, Replacements);
	OutDef.bCanFail = false;

	// Single stage with multiple reach-location objectives
	FIronvaleQuestStageData PatrolStage;
	PatrolStage.StageID = FName(*(OutDef.QuestID.ToString() + TEXT("_Patrol")));
	PatrolStage.Description = OutDef.Description;
	PatrolStage.bAllRequired = true;

	for (int32 i = 0; i < PatrolPoints.Num(); ++i)
	{
		FIronvaleQuestObjectiveData Obj;
		Obj.ObjectiveType = EIronvaleObjectiveType::ReachLocation;
		Obj.TargetID = PatrolPoints[i];
		Obj.RequiredCount = 1;
		Obj.Description = FText::Format(
			NSLOCTEXT("IronvaleRadiant", "PatrolObj", "Patrol checkpoint: {0}"),
			FText::FromName(PatrolPoints[i]));
		Obj.bOptional = false;
		Obj.bHidden = false;
		PatrolStage.Objectives.Add(Obj);
	}

	OutDef.Stages.Add(PatrolStage);

	// Generate scaled rewards
	OutDef.Rewards = GenerateRewards(Template.RewardRange, PlayerLevel);

	return true;
}

// =============================================================================
// ESCORT QUEST GENERATION
// =============================================================================

bool UIronvaleRadiantQuestGenerator::GenerateEscortQuest(
	const FIronvaleRadiantTemplate& Template, FName RegionID,
	int32 PlayerLevel, FIronvaleQuestDefinition& OutDef)
{
	// Select the NPC to escort
	const FName EscorteeID = SelectTarget(Template.PrimaryTargetPool, RegionID, PlayerLevel);
	if (EscorteeID.IsNone())
	{
		return false;
	}

	// Select the destination
	const FName DestinationID = SelectTarget(Template.LocationPool, RegionID, PlayerLevel);
	if (DestinationID.IsNone())
	{
		return false;
	}

	// Fill text placeholders
	TMap<FString, FString> Replacements;
	Replacements.Add(TEXT("{Target}"), EscorteeID.ToString());
	Replacements.Add(TEXT("{Location}"), DestinationID.ToString());

	// Build quest definition
	OutDef.QuestID = GenerateUniqueQuestID(Template.TemplateID);
	OutDef.DisplayName = FillTextTemplate(Template.NameTemplate, Replacements);
	OutDef.Description = FillTextTemplate(Template.DescriptionTemplate, Replacements);
	OutDef.bCanFail = true;

	// Fail condition: the escortee dies
	FIronvaleDialogueCondition FailCond;
	FailCond.ConditionType = EIronvaleDialogueConditionType::Flag;
	FailCond.Key = FName(*(EscorteeID.ToString() + TEXT("_Dead")));
	FailCond.Operator = EIronvaleComparisonOp::Equal;
	FailCond.Value = 1.0f;
	OutDef.FailConditions.Add(FailCond);

	// Stage 1: Meet the NPC
	FIronvaleQuestStageData MeetStage;
	MeetStage.StageID = FName(*(OutDef.QuestID.ToString() + TEXT("_Meet")));
	MeetStage.Description = FText::Format(
		NSLOCTEXT("IronvaleRadiant", "EscortMeet", "Meet {0}"),
		FText::FromName(EscorteeID));
	MeetStage.bAllRequired = true;

	FIronvaleQuestObjectiveData MeetObj;
	MeetObj.ObjectiveType = EIronvaleObjectiveType::TalkTo;
	MeetObj.TargetID = EscorteeID;
	MeetObj.RequiredCount = 1;
	MeetObj.Description = MeetStage.Description;
	MeetStage.Objectives.Add(MeetObj);

	// Stage 2: Escort to destination
	FIronvaleQuestStageData EscortStage;
	EscortStage.StageID = FName(*(OutDef.QuestID.ToString() + TEXT("_Escort")));
	EscortStage.Description = FText::Format(
		NSLOCTEXT("IronvaleRadiant", "EscortTravel", "Escort {0} to {1}"),
		FText::FromName(EscorteeID),
		FText::FromName(DestinationID));
	EscortStage.bAllRequired = true;

	FIronvaleQuestObjectiveData EscortObj;
	EscortObj.ObjectiveType = EIronvaleObjectiveType::Escort;
	EscortObj.TargetID = EscorteeID;
	EscortObj.RequiredCount = 1;
	EscortObj.Description = EscortStage.Description;
	EscortStage.Objectives.Add(EscortObj);

	// Link stages
	MeetStage.NextStageID = EscortStage.StageID;

	OutDef.Stages.Add(MeetStage);
	OutDef.Stages.Add(EscortStage);

	// Generate scaled rewards (escort quests are harder, so boost slightly)
	FIronvaleRadiantRewardRange BoostedRange = Template.RewardRange;
	BoostedRange.GoldRange *= 1.25;
	BoostedRange.ReputationRange *= 1.25;
	OutDef.Rewards = GenerateRewards(BoostedRange, PlayerLevel);

	return true;
}

// =============================================================================
// INTERNAL HELPERS
// =============================================================================

FName UIronvaleRadiantQuestGenerator::SelectTarget(
	const FIronvaleRadiantTargetPool& Pool, FName RegionID, int32 PlayerLevel) const
{
	TArray<FName> ValidTargets;

	// Check region filter
	if (Pool.ValidRegions.Num() > 0 && !Pool.ValidRegions.Contains(RegionID))
	{
		return NAME_None;
	}

	// Check level range
	if (PlayerLevel < Pool.MinPlayerLevel)
	{
		return NAME_None;
	}
	if (Pool.MaxPlayerLevel > 0 && PlayerLevel > Pool.MaxPlayerLevel)
	{
		return NAME_None;
	}

	if (Pool.TargetIDs.Num() == 0)
	{
		return NAME_None;
	}

	// All targets in the pool are valid if region/level checks pass
	const int32 Index = FMath::RandRange(0, Pool.TargetIDs.Num() - 1);
	return Pool.TargetIDs[Index];
}

TArray<FName> UIronvaleRadiantQuestGenerator::SelectMultipleTargets(
	const FIronvaleRadiantTargetPool& Pool, FName RegionID,
	int32 PlayerLevel, int32 Count) const
{
	TArray<FName> Result;

	// Check region filter
	if (Pool.ValidRegions.Num() > 0 && !Pool.ValidRegions.Contains(RegionID))
	{
		return Result;
	}

	// Check level range
	if (PlayerLevel < Pool.MinPlayerLevel)
	{
		return Result;
	}
	if (Pool.MaxPlayerLevel > 0 && PlayerLevel > Pool.MaxPlayerLevel)
	{
		return Result;
	}

	// Shuffle a copy of the pool and take up to Count
	TArray<FName> Available = Pool.TargetIDs;
	const int32 NumToSelect = FMath::Min(Count, Available.Num());

	// Fisher-Yates shuffle
	for (int32 i = Available.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Available.Swap(i, j);
	}

	for (int32 i = 0; i < NumToSelect; ++i)
	{
		Result.Add(Available[i]);
	}

	return Result;
}

FIronvaleQuestRewards UIronvaleRadiantQuestGenerator::GenerateRewards(
	const FIronvaleRadiantRewardRange& Range, int32 PlayerLevel) const
{
	FIronvaleQuestRewards Rewards;

	// Scale factor based on player level (linear scaling, level 1 = min, level 20 = max)
	const float LevelFactor = FMath::Clamp(
		static_cast<float>(PlayerLevel - 1) / 19.0f, 0.0f, 1.0f);

	// Gold — lerp between min and max, with some random variance
	const float BaseGold = FMath::Lerp(Range.GoldRange.X, Range.GoldRange.Y, LevelFactor);
	const float GoldVariance = BaseGold * 0.2f; // +/- 20%
	Rewards.Gold = FMath::RoundToInt32(
		FMath::FRandRange(BaseGold - GoldVariance, BaseGold + GoldVariance));
	Rewards.Gold = FMath::Max(1, Rewards.Gold);

	// Reputation
	if (!Range.RewardFactionID.IsNone())
	{
		FIronvaleReputationReward RepReward;
		RepReward.FactionID = Range.RewardFactionID;
		RepReward.ReputationDelta = FMath::Lerp(
			Range.ReputationRange.X, Range.ReputationRange.Y, LevelFactor);
		Rewards.ReputationChanges.Add(RepReward);
	}

	// Optional item reward — roll a random item from the pool
	if (Range.ItemRewardPool.Num() > 0)
	{
		const int32 ItemIndex = FMath::RandRange(0, Range.ItemRewardPool.Num() - 1);
		FIronvaleItemReward ItemReward;
		ItemReward.ItemID = Range.ItemRewardPool[ItemIndex];
		ItemReward.Quantity = 1;
		Rewards.Items.Add(ItemReward);
	}

	return Rewards;
}

FText UIronvaleRadiantQuestGenerator::FillTextTemplate(
	const FText& Template, const TMap<FString, FString>& Replacements) const
{
	FString Result = Template.ToString();

	for (const auto& Pair : Replacements)
	{
		Result = Result.Replace(*Pair.Key, *Pair.Value);
	}

	return FText::FromString(Result);
}

FName UIronvaleRadiantQuestGenerator::GenerateUniqueQuestID(FName TemplateID)
{
	++GeneratedQuestCounter;
	const FString UniqueID = FString::Printf(TEXT("Radiant_%s_%04d"),
		*TemplateID.ToString(), GeneratedQuestCounter);
	return FName(*UniqueID);
}

void UIronvaleRadiantQuestGenerator::ApplyCooldown(FName TemplateID, int32 CooldownDays)
{
	if (CooldownDays > 0)
	{
		TemplateCooldowns.Add(TemplateID, CooldownDays);
	}
}

bool UIronvaleRadiantQuestGenerator::HasValidTargets(
	const FIronvaleRadiantTargetPool& Pool, FName RegionID, int32 PlayerLevel) const
{
	if (Pool.TargetIDs.Num() == 0)
	{
		return false;
	}

	// Check region filter
	if (Pool.ValidRegions.Num() > 0 && !Pool.ValidRegions.Contains(RegionID))
	{
		return false;
	}

	// Check level range
	if (PlayerLevel < Pool.MinPlayerLevel)
	{
		return false;
	}
	if (Pool.MaxPlayerLevel > 0 && PlayerLevel > Pool.MaxPlayerLevel)
	{
		return false;
	}

	return true;
}
