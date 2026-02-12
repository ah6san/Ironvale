// =============================================================================
// IronvaleDialogueCondition.h — Condition evaluation for dialogue system
// Project Ironvale
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "IronvaleDialogueTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IronvaleDialogueCondition.generated.h"

UCLASS()
class IRONVALE_API UIronvaleDialogueConditionEvaluator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Evaluate a single condition against current game state */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Dialogue", meta = (WorldContext = "WorldContext"))
	static bool EvaluateCondition(UObject* WorldContext, const FIronvaleDialogueCondition& Condition, AActor* Player);

	/** Evaluate all conditions (AND logic — all must pass) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Dialogue", meta = (WorldContext = "WorldContext"))
	static bool EvaluateAllConditions(UObject* WorldContext, const TArray<FIronvaleDialogueCondition>& Conditions, AActor* Player);

	/** Check if a player has the required gameplay tags for a choice */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Dialogue")
	static bool PlayerHasRequiredTags(AActor* Player, const FGameplayTagContainer& RequiredTags);

private:
	static bool CompareFloat(float A, EIronvaleComparisonOp Op, float B);
};
