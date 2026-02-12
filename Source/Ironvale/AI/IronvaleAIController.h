// =============================================================================
// IronvaleAIController.h — NPC AI controller with perception and schedule
// Project Ironvale
//
// Manages the behavior tree lifecycle, configures AI perception (sight +
// hearing), and keeps blackboard keys synchronized with the NPC's schedule
// component and crime response state.
//
// Blackboard keys:
//   CurrentActivity     (Enum / int)   — EIronvaleActivity from schedule
//   TargetLocation      (Name)         — FName of schedule location
//   ThreatActor         (Object)       — Perceived hostile actor
//   CrimeWitnessed      (Bool)         — True if NPC has witnessed a crime
//   QuestOverrideActive (Bool)         — True if a quest override is in effect
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "IronvaleAIController.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAIPerceptionComponent;
class UBehaviorTree;
class UBlackboardData;
class UIronvaleScheduleComponent;

/**
 * AIronvaleAIController
 *
 * Possesses NPC characters and runs their behavior tree. Reads schedule data
 * from UIronvaleScheduleComponent and perception data from
 * UAIPerceptionComponent, feeding both into blackboard keys consumed by
 * behavior tree tasks and decorators.
 */
UCLASS()
class IRONVALE_API AIronvaleAIController : public AAIController
{
	GENERATED_BODY()

public:
	AIronvaleAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

	// =========================================================================
	// BEHAVIOR TREE
	// =========================================================================

	/** Behavior tree asset to run. Assign per-archetype in the NPC blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|AI")
	UBehaviorTree* BehaviorTreeAsset;

	/** Blackboard data asset (matched to the behavior tree) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|AI")
	UBlackboardData* BlackboardAsset;

	// =========================================================================
	// PERCEPTION CONFIG
	// =========================================================================

	/** Sight range in centimeters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|AI|Perception",
		meta = (ClampMin = "0.0"))
	float SightRange = 2000.0f;

	/** Peripheral vision half-angle in degrees */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|AI|Perception",
		meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SightPeripheralAngle = 90.0f;

	/** How long a sight stimulus remains valid after loss of LOS (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|AI|Perception",
		meta = (ClampMin = "0.0"))
	float SightMaxAge = 5.0f;

	/** Hearing range in centimeters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|AI|Perception",
		meta = (ClampMin = "0.0"))
	float HearingRange = 3000.0f;

	// =========================================================================
	// BLACKBOARD KEY NAMES
	// =========================================================================

	static const FName BB_CurrentActivity;
	static const FName BB_TargetLocation;
	static const FName BB_ThreatActor;
	static const FName BB_CrimeWitnessed;
	static const FName BB_QuestOverrideActive;

	// =========================================================================
	// BLACKBOARD HELPERS
	// =========================================================================

	/** Push current schedule state into the blackboard */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|AI")
	void UpdateBlackboardFromSchedule();

	/** Push perception state (nearest threat) into the blackboard */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|AI")
	void UpdateBlackboardFromPerception();

protected:
	// =========================================================================
	// PERCEPTION
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|AI|Perception")
	UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY()
	UAISenseConfig_Hearing* HearingConfig;

	/** Called by the perception component when a stimulus is updated */
	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// =========================================================================
	// HELPERS
	// =========================================================================

	/** Cached reference to the possessed NPC's schedule component */
	UPROPERTY()
	UIronvaleScheduleComponent* CachedScheduleComp;

	/** Set up sight and hearing sense configs on the perception component */
	void ConfigurePerception();

	/** Initialize the blackboard and start the behavior tree */
	void StartBehaviorTreeForPawn(APawn* InPawn);

	/** Schedule entry changed callback */
	UFUNCTION()
	void HandleScheduleEntryChanged(EIronvaleActivity NewActivity, FName NewLocation);
};
