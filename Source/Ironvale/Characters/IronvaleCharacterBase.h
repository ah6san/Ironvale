// =============================================================================
// IronvaleCharacterBase.h — Base character class shared by Player and NPCs
// Project Ironvale
//
// Contains components common to all characters: health, stamina, combat,
// inventory, equipment. Player and NPC subclasses add their specific logic.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleCharacterBase.generated.h"

class UIronvaleCombatComponent;
class UIronvaleStaminaComponent;
class UIronvaleHealthComponent;
class UIronvaleInventoryComponent;
class UIronvaleEquipmentComponent;

UCLASS(Abstract)
class IRONVALE_API AIronvaleCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AIronvaleCharacterBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// COMPONENT ACCESSORS
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Character")
	UIronvaleCombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Character")
	UIronvaleStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Character")
	UIronvaleHealthComponent* GetHealthComponent() const { return HealthComponent; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Character")
	UIronvaleInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Character")
	UIronvaleEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	// =========================================================================
	// CHARACTER IDENTITY
	// =========================================================================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Character")
	FName CharacterID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Character")
	FText DisplayName;

	// =========================================================================
	// DEATH / KNOCKOUT
	// =========================================================================

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Character")
	virtual void Die(AActor* Killer);

	UFUNCTION(BlueprintPure, Category = "Ironvale|Character")
	bool IsDead() const { return bIsDead; }

	/** Enable ragdoll physics on death/knockout */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Character")
	void EnableRagdoll();

	/** Disable ragdoll and return to animated state */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Character")
	void DisableRagdoll();

protected:
	// --- Components (created in constructor) ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleStaminaComponent* StaminaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Components")
	UIronvaleEquipmentComponent* EquipmentComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Character")
	bool bIsDead = false;

	/** Called when health reaches zero */
	UFUNCTION()
	void HandleHealthDepleted(AActor* DamageCauser);
};
