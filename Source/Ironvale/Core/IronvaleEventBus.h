// =============================================================================
// IronvaleEventBus.h — Central event/delegate hub for cross-system communication
// Project Ironvale
//
// Systems broadcast events here instead of directly coupling to each other.
// Example: Combat fires OnEnemyKilled → Quest manager listens for kill objectives.
//
// Usage: Get the subsystem via GetWorld()->GetSubsystem<UIronvaleEventBus>()
//        or UGameplayStatics::GetGameInstance()->GetSubsystem<UIronvaleEventBus>().
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "IronvaleTypes.h"
#include "IronvaleEventBus.generated.h"

// Forward declarations
class AIronvaleCharacterBase;
class UIronvaleItemInstance;

// =============================================================================
// DELEGATE DECLARATIONS
// Naming convention: FOnIronvale[Event]Signature
// =============================================================================

// --- Combat Events ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnIronvaleDamageDealtSignature,
	AActor*, Attacker, AActor*, Victim, FIronvaleDamageResult, DamageResult);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleCombatStateChangedSignature,
	AActor*, Character, EIronvaleCombatState, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleCharacterDeathSignature,
	AActor*, DeadCharacter, AActor*, Killer);

// --- Inventory / Equipment Events ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleItemPickedUpSignature,
	AActor*, Character, FName, ItemID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleItemDroppedSignature,
	AActor*, Character, FName, ItemID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnIronvaleItemEquippedSignature,
	AActor*, Character, FName, ItemID, int32, SlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnIronvaleItemUnequippedSignature,
	AActor*, Character, FName, ItemID, int32, SlotIndex);

// --- Needs / Survival Events ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnIronvaleNeedThresholdCrossedSignature,
	AActor*, Character, EIronvaleNeedType, NeedType, float, NewValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleNeedCriticalSignature,
	AActor*, Character, EIronvaleNeedType, NeedType);

// --- Reputation Events ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnIronvaleReputationChangedSignature,
	FName, FactionID, float, OldValue, float, NewValue);

// --- Dialogue Events ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleDialogueStartedSignature,
	AActor*, NPC, AActor*, Player);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleDialogueEndedSignature,
	AActor*, NPC, AActor*, Player);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleDialogueChoiceMadeSignature,
	FName, DialogueNodeID, int32, ChoiceIndex);

// --- Quest Events ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleQuestStartedSignature,
	FName, QuestID, int32, StageIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleQuestStageCompletedSignature,
	FName, QuestID, int32, StageIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleQuestCompletedSignature,
	FName, QuestID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleQuestFailedSignature,
	FName, QuestID);

// --- World Events ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIronvaleLocationReachedSignature,
	AActor*, Character, FName, LocationID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleWeatherChangedSignature,
	EIronvaleWeatherState, NewWeather);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleTimeOfDayChangedSignature,
	EIronvaleTimeOfDay, NewTimeOfDay);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleHourChangedSignature,
	int32, NewHour);

// --- Crime Events ---
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnIronvaleCrimeCommittedSignature,
	AActor*, Criminal, FName, CrimeType, AActor*, Witness);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIronvaleWorldFlagSetSignature,
	FName, FlagName);

// =============================================================================
// EVENT BUS SUBSYSTEM
// =============================================================================

/**
 * Central event bus for Project Ironvale.
 *
 * World subsystem — one instance per UWorld. All game systems broadcast and
 * subscribe to events here, avoiding direct coupling between systems.
 *
 * Usage example (broadcasting):
 *   GetWorld()->GetSubsystem<UIronvaleEventBus>()->OnCharacterDeath.Broadcast(DeadActor, KillerActor);
 *
 * Usage example (subscribing):
 *   GetWorld()->GetSubsystem<UIronvaleEventBus>()->OnCharacterDeath.AddDynamic(this, &UMyComponent::HandleDeath);
 */
UCLASS()
class IRONVALE_API UIronvaleEventBus : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Combat ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Combat")
	FOnIronvaleDamageDealtSignature OnDamageDealt;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Combat")
	FOnIronvaleCombatStateChangedSignature OnCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Combat")
	FOnIronvaleCharacterDeathSignature OnCharacterDeath;

	// --- Inventory ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Inventory")
	FOnIronvaleItemPickedUpSignature OnItemPickedUp;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Inventory")
	FOnIronvaleItemDroppedSignature OnItemDropped;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Inventory")
	FOnIronvaleItemEquippedSignature OnItemEquipped;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Inventory")
	FOnIronvaleItemUnequippedSignature OnItemUnequipped;

	// --- Needs ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Needs")
	FOnIronvaleNeedThresholdCrossedSignature OnNeedThresholdCrossed;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Needs")
	FOnIronvaleNeedCriticalSignature OnNeedCritical;

	// --- Reputation ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Reputation")
	FOnIronvaleReputationChangedSignature OnReputationChanged;

	// --- Dialogue ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Dialogue")
	FOnIronvaleDialogueStartedSignature OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Dialogue")
	FOnIronvaleDialogueEndedSignature OnDialogueEnded;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Dialogue")
	FOnIronvaleDialogueChoiceMadeSignature OnDialogueChoiceMade;

	// --- Quests ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Quests")
	FOnIronvaleQuestStartedSignature OnQuestStarted;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Quests")
	FOnIronvaleQuestStageCompletedSignature OnQuestStageCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Quests")
	FOnIronvaleQuestCompletedSignature OnQuestCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Quests")
	FOnIronvaleQuestFailedSignature OnQuestFailed;

	// --- World ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|World")
	FOnIronvaleLocationReachedSignature OnLocationReached;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|World")
	FOnIronvaleWeatherChangedSignature OnWeatherChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|World")
	FOnIronvaleTimeOfDayChangedSignature OnTimeOfDayChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|World")
	FOnIronvaleHourChangedSignature OnHourChanged;

	// --- Crime ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|Crime")
	FOnIronvaleCrimeCommittedSignature OnCrimeCommitted;

	// --- World Flags ---
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Events|World")
	FOnIronvaleWorldFlagSetSignature OnWorldFlagSet;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
	{
		// Only active in game worlds and PIE, not in editor preview
		return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
	}
};
