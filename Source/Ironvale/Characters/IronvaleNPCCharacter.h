// =============================================================================
// IronvaleNPCCharacter.h — NPC character with AI hooks
// Project Ironvale
//
// Extends AIronvaleCharacterBase with NPC-specific data: archetype, faction,
// schedule component reference, dialogue asset, and persistence ID.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Characters/IronvaleCharacterBase.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleNPCCharacter.generated.h"

class UIronvaleScheduleComponent;
class UIronvaleCrimeResponseComponent;
class UIronvaleDialogueAsset;

UCLASS()
class IRONVALE_API AIronvaleNPCCharacter : public AIronvaleCharacterBase
{
	GENERATED_BODY()

public:
	AIronvaleNPCCharacter();

	virtual void BeginPlay() override;

	// =========================================================================
	// NPC IDENTITY
	// =========================================================================

	/** NPC archetype — determines default behavior, schedule, and reactions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|NPC")
	EIronvaleNPCArchetype Archetype = EIronvaleNPCArchetype::Villager;

	/** Faction this NPC belongs to (for reputation lookups) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|NPC")
	FName FactionID;

	/** Whether this NPC is essential (cannot be permanently killed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|NPC")
	bool bIsEssential = false;

	/** Whether this NPC's state should be saved/loaded */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|NPC")
	bool bIsPersistent = true;

	/** Home location — where this NPC defaults to when not on a schedule task */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|NPC")
	FName HomeLocationID;

	/** Workplace — where this NPC goes during work hours */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|NPC")
	FName WorkLocationID;

	// =========================================================================
	// DIALOGUE
	// =========================================================================

	/** Dialogue tree asset for this NPC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|NPC|Dialogue")
	TSoftObjectPtr<UIronvaleDialogueAsset> DialogueAsset;

	/** Generic barks (greeting, warning, fear, etc.) — selected by tag at runtime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|NPC|Dialogue")
	TMap<FName, FText> Barks;

	// =========================================================================
	// COMPONENT ACCESSORS
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|NPC")
	UIronvaleScheduleComponent* GetScheduleComponent() const { return ScheduleComponent; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|NPC")
	UIronvaleCrimeResponseComponent* GetCrimeResponseComponent() const { return CrimeResponseComponent; }

	// =========================================================================
	// AI QUERIES
	// =========================================================================

	/** Get this NPC's disposition toward a specific actor (based on faction reputation) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|NPC")
	float GetDispositionToward(AActor* OtherActor) const;

	/** Is this NPC hostile toward the given actor? */
	UFUNCTION(BlueprintPure, Category = "Ironvale|NPC")
	bool IsHostileToward(AActor* OtherActor) const;

	// =========================================================================
	// DEATH OVERRIDE
	// =========================================================================

	virtual void Die(AActor* Killer) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleScheduleComponent* ScheduleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleCrimeResponseComponent* CrimeResponseComponent;
};
