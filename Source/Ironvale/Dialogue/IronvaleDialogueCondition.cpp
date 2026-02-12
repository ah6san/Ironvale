// =============================================================================
// IronvaleDialogueCondition.cpp — Condition evaluation implementation
// Project Ironvale
// =============================================================================

#include "Dialogue/IronvaleDialogueCondition.h"
#include "Ironvale.h"
#include "IronvaleGameInstance.h"
#include "IronvaleGameState.h"
#include "Dialogue/IronvaleReputationComponent.h"
#include "Inventory/IronvaleInventoryComponent.h"
#include "Inventory/IronvaleEquipmentComponent.h"
#include "Needs/IronvaleNeedsComponent.h"
#include "Quests/IronvaleQuestManager.h"

bool UIronvaleDialogueConditionEvaluator::EvaluateCondition(UObject* WorldContext,
	const FIronvaleDialogueCondition& Condition, AActor* Player)
{
	if (!WorldContext || !Player) return false;

	UWorld* World = WorldContext->GetWorld();
	if (!World) return false;

	bool bResult = false;

	switch (Condition.ConditionType)
	{
	case EIronvaleDialogueConditionType::Flag:
	{
		if (UIronvaleGameInstance* GI = Cast<UIronvaleGameInstance>(World->GetGameInstance()))
		{
			const float FlagValue = GI->GetWorldFlag(Condition.Key) ? 1.0f : 0.0f;
			bResult = CompareFloat(FlagValue, Condition.Operator, Condition.Value);
		}
		break;
	}

	case EIronvaleDialogueConditionType::Reputation:
	{
		if (UIronvaleReputationSubsystem* RepSys = World->GetGameInstance()->GetSubsystem<UIronvaleReputationSubsystem>())
		{
			const float Rep = RepSys->GetReputation(Condition.Key);
			bResult = CompareFloat(Rep, Condition.Operator, Condition.Value);
		}
		break;
	}

	case EIronvaleDialogueConditionType::HasItem:
	{
		if (UIronvaleInventoryComponent* Inv = Player->FindComponentByClass<UIronvaleInventoryComponent>())
		{
			const float Count = static_cast<float>(Inv->GetItemCount(Condition.Key));
			bResult = CompareFloat(Count, Condition.Operator, Condition.Value);
		}
		break;
	}

	case EIronvaleDialogueConditionType::HasTag:
	{
		if (UIronvaleEquipmentComponent* Equip = Player->FindComponentByClass<UIronvaleEquipmentComponent>())
		{
			FGameplayTagContainer Tags = Equip->GetAppearanceTags();
			bResult = Condition.TagValue.IsValid() && Tags.HasTag(Condition.TagValue);
			// For tag checks, the operator is typically Equal (has) or NotEqual (doesn't have)
			if (Condition.Operator == EIronvaleComparisonOp::NotEqual)
			{
				bResult = !bResult;
			}
		}
		break;
	}

	case EIronvaleDialogueConditionType::TimeOfDay:
	{
		if (const AIronvaleGameState* GS = World->GetGameState<AIronvaleGameState>())
		{
			const float CurrentHour = GS->GetGameTimeHours();
			bResult = CompareFloat(CurrentHour, Condition.Operator, Condition.Value);
		}
		break;
	}

	case EIronvaleDialogueConditionType::QuestStage:
	{
		if (UIronvaleQuestSubsystem* QuestSys = World->GetGameInstance()->GetSubsystem<UIronvaleQuestSubsystem>())
		{
			const int32 Stage = QuestSys->GetQuestStage(Condition.Key);
			bResult = CompareFloat(static_cast<float>(Stage), Condition.Operator, Condition.Value);
		}
		break;
	}

	case EIronvaleDialogueConditionType::NeedLevel:
	{
		if (UIronvaleNeedsComponent* Needs = Player->FindComponentByClass<UIronvaleNeedsComponent>())
		{
			// Key maps to need type by name
			EIronvaleNeedType NeedType = EIronvaleNeedType::Hunger;
			if (Condition.Key == FName("Thirst")) NeedType = EIronvaleNeedType::Thirst;
			else if (Condition.Key == FName("Fatigue")) NeedType = EIronvaleNeedType::Fatigue;
			else if (Condition.Key == FName("Cleanliness")) NeedType = EIronvaleNeedType::Cleanliness;

			const float NeedValue = Needs->GetNeedValue(NeedType);
			bResult = CompareFloat(NeedValue, Condition.Operator, Condition.Value);
		}
		break;
	}

	case EIronvaleDialogueConditionType::Weather:
	{
		if (const AIronvaleGameState* GS = World->GetGameState<AIronvaleGameState>())
		{
			const float WeatherVal = static_cast<float>(GS->GetCurrentWeather());
			bResult = CompareFloat(WeatherVal, Condition.Operator, Condition.Value);
		}
		break;
	}

	default:
		bResult = true;
		break;
	}

	return Condition.bInvert ? !bResult : bResult;
}

bool UIronvaleDialogueConditionEvaluator::EvaluateAllConditions(UObject* WorldContext,
	const TArray<FIronvaleDialogueCondition>& Conditions, AActor* Player)
{
	for (const FIronvaleDialogueCondition& Condition : Conditions)
	{
		if (!EvaluateCondition(WorldContext, Condition, Player))
		{
			return false;
		}
	}
	return true;
}

bool UIronvaleDialogueConditionEvaluator::PlayerHasRequiredTags(AActor* Player,
	const FGameplayTagContainer& RequiredTags)
{
	if (!RequiredTags.IsValid() || RequiredTags.Num() == 0) return true;
	if (!Player) return false;

	if (UIronvaleEquipmentComponent* Equip = Player->FindComponentByClass<UIronvaleEquipmentComponent>())
	{
		FGameplayTagContainer PlayerTags = Equip->GetAppearanceTags();
		return PlayerTags.HasAll(RequiredTags);
	}

	return false;
}

bool UIronvaleDialogueConditionEvaluator::CompareFloat(float A, EIronvaleComparisonOp Op, float B)
{
	switch (Op)
	{
	case EIronvaleComparisonOp::Equal:          return FMath::IsNearlyEqual(A, B, 0.01f);
	case EIronvaleComparisonOp::NotEqual:       return !FMath::IsNearlyEqual(A, B, 0.01f);
	case EIronvaleComparisonOp::GreaterThan:    return A > B;
	case EIronvaleComparisonOp::LessThan:       return A < B;
	case EIronvaleComparisonOp::GreaterOrEqual: return A >= B;
	case EIronvaleComparisonOp::LessOrEqual:    return A <= B;
	default:                                     return false;
	}
}
