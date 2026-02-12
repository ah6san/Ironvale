// =============================================================================
// IronvaleQuestObjective.cpp — Single objective tracker implementation
// Project Ironvale
// =============================================================================

#include "Quests/IronvaleQuestObjective.h"
#include "Ironvale.h"

void UIronvaleQuestObjectiveTracker::Initialize(int32 InRequiredCount, bool bInOptional, bool bInHidden)
{
	RequiredCount = FMath::Max(1, InRequiredCount);
	CurrentCount = 0;
	bCompleted = false;
	bOptional = bInOptional;
	bHidden = bInHidden;
}

void UIronvaleQuestObjectiveTracker::IncrementProgress(int32 Delta)
{
	if (bCompleted)
	{
		return;
	}

	if (Delta <= 0)
	{
		return;
	}

	const int32 PreviousCount = CurrentCount;
	CurrentCount = FMath::Clamp(CurrentCount + Delta, 0, RequiredCount);

	if (CurrentCount >= RequiredCount)
	{
		bCompleted = true;
		UE_LOG(LogIronvale, Verbose, TEXT("Objective completed (%d/%d)"),
			CurrentCount, RequiredCount);
	}
	else if (CurrentCount != PreviousCount)
	{
		UE_LOG(LogIronvale, Verbose, TEXT("Objective progress: %d/%d (+%d)"),
			CurrentCount, RequiredCount, CurrentCount - PreviousCount);
	}
}

void UIronvaleQuestObjectiveTracker::SetProgress(int32 NewCount)
{
	CurrentCount = FMath::Clamp(NewCount, 0, RequiredCount);
	bCompleted = (CurrentCount >= RequiredCount);
}

float UIronvaleQuestObjectiveTracker::GetProgressPercent() const
{
	if (RequiredCount <= 0)
	{
		return 1.0f;
	}

	return FMath::Clamp(
		static_cast<float>(CurrentCount) / static_cast<float>(RequiredCount),
		0.0f,
		1.0f
	);
}
