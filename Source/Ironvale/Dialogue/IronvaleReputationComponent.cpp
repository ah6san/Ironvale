// =============================================================================
// IronvaleReputationComponent.cpp — Reputation subsystem implementation
// Project Ironvale
// =============================================================================

#include "Dialogue/IronvaleReputationComponent.h"
#include "Ironvale.h"
#include "Core/IronvaleEventBus.h"
#include "Core/IronvaleStatics.h"
#include "Engine/World.h"

void UIronvaleReputationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FactionDataTable)
	{
		LoadFactionData(FactionDataTable);
	}

	UE_LOG(LogIronvale, Log, TEXT("Reputation subsystem initialized with %d factions"), FactionData.Num());
}

void UIronvaleReputationSubsystem::LoadFactionData(UDataTable* InTable)
{
	FactionDataTable = InTable;
	FactionData.Empty();
	ReputationValues.Empty();

	if (!InTable) return;

	TArray<FIronvaleFactionData*> Rows;
	InTable->GetAllRows<FIronvaleFactionData>(TEXT("FactionData"), Rows);

	for (const FIronvaleFactionData* Row : Rows)
	{
		if (Row)
		{
			FactionData.Add(Row->FactionID, *Row);
			ReputationValues.Add(Row->FactionID, Row->DefaultReputation);
		}
	}
}

float UIronvaleReputationSubsystem::GetReputation(FName FactionID) const
{
	const float* Value = ReputationValues.Find(FactionID);
	return Value ? *Value : 0.0f;
}

void UIronvaleReputationSubsystem::ModifyReputation(FName FactionID, float Delta, bool bPropagate)
{
	if (FMath::IsNearlyZero(Delta)) return;

	float* Value = ReputationValues.Find(FactionID);
	if (!Value)
	{
		ReputationValues.Add(FactionID, 0.0f);
		Value = ReputationValues.Find(FactionID);
	}

	const float OldValue = *Value;
	*Value = FMath::Clamp(*Value + Delta, IronvaleConstants::MIN_REPUTATION, IronvaleConstants::MAX_REPUTATION);

	UE_LOG(LogIronvale, Log, TEXT("Reputation with %s: %.0f -> %.0f (delta %.0f)"),
		*FactionID.ToString(), OldValue, *Value, Delta);

	// Broadcast change through EventBus
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UWorld* World = GI->GetWorld())
		{
			if (UIronvaleEventBus* EventBus = World->GetSubsystem<UIronvaleEventBus>())
			{
				EventBus->OnReputationChanged.Broadcast(FactionID, OldValue, *Value);
			}
		}
	}

	// Propagate to allied/enemy factions
	if (bPropagate)
	{
		PropagateReputationChange(FactionID, Delta);
	}
}

void UIronvaleReputationSubsystem::SetReputation(FName FactionID, float Value)
{
	const float ClampedValue = FMath::Clamp(Value, IronvaleConstants::MIN_REPUTATION, IronvaleConstants::MAX_REPUTATION);
	ReputationValues.Add(FactionID, ClampedValue);
}

bool UIronvaleReputationSubsystem::IsFactionHostile(FName FactionID) const
{
	const FIronvaleFactionData* Data = FactionData.Find(FactionID);
	const float Rep = GetReputation(FactionID);
	const float Threshold = Data ? Data->HostileThreshold : -50.0f;
	return Rep < Threshold;
}

bool UIronvaleReputationSubsystem::IsFactionFriendly(FName FactionID) const
{
	const FIronvaleFactionData* Data = FactionData.Find(FactionID);
	const float Rep = GetReputation(FactionID);
	const float Threshold = Data ? Data->FriendlyThreshold : 30.0f;
	return Rep >= Threshold;
}

TArray<FIronvaleReputationEntry> UIronvaleReputationSubsystem::GetAllReputations() const
{
	TArray<FIronvaleReputationEntry> Result;
	for (const auto& Pair : ReputationValues)
	{
		FIronvaleReputationEntry Entry;
		Entry.FactionID = Pair.Key;
		Entry.Value = Pair.Value;
		Result.Add(Entry);
	}
	return Result;
}

void UIronvaleReputationSubsystem::LoadReputations(const TArray<FIronvaleReputationEntry>& SavedReps)
{
	for (const FIronvaleReputationEntry& Entry : SavedReps)
	{
		ReputationValues.Add(Entry.FactionID, Entry.Value);
	}
	UE_LOG(LogIronvale, Log, TEXT("Loaded %d reputation entries from save"), SavedReps.Num());
}

float UIronvaleReputationSubsystem::GetPriceMultiplier(FName FactionID) const
{
	return UIronvaleStatics::GetReputationPriceMultiplier(GetReputation(FactionID));
}

void UIronvaleReputationSubsystem::PropagateReputationChange(FName SourceFaction, float Delta)
{
	const FIronvaleFactionData* Data = FactionData.Find(SourceFaction);
	if (!Data) return;

	// Allied factions gain a fraction of the rep change
	for (const FName& AllyID : Data->AlliedFactions)
	{
		const float PropDelta = Delta * Data->AlliancePropagation;
		if (!FMath::IsNearlyZero(PropDelta))
		{
			ModifyReputation(AllyID, PropDelta, false); // Don't recurse
		}
	}

	// Enemy factions lose a fraction (inverted)
	for (const FName& EnemyID : Data->EnemyFactions)
	{
		const float PropDelta = -Delta * Data->EnemyPropagation;
		if (!FMath::IsNearlyZero(PropDelta))
		{
			ModifyReputation(EnemyID, PropDelta, false);
		}
	}
}
