// =============================================================================
// IronvaleStaminaComponent.h — Stamina pool with drain, regen, and armor penalties
// Project Ironvale
//
// Stamina is the core combat resource:
//   - Attacks, blocks, sprints, and dodges drain stamina
//   - Regen is reduced by armor weight
//   - At zero stamina: character is staggered (vulnerability window)
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IronvaleStaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStaminaChangedSignature,
	float, CurrentStamina, float, MaxStamina, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaminaDepletedSignature);

UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleStaminaComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =========================================================================
	// STAMINA OPERATIONS
	// =========================================================================

	/**
	 * Consume stamina. Returns false if insufficient stamina (action should be prevented).
	 * If bForceConsume is true, stamina goes negative (triggers stagger).
	 */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Stamina")
	bool ConsumeStamina(float Amount, bool bForceConsume = false);

	/** Instantly restore stamina (e.g., from potion) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Stamina")
	void RestoreStamina(float Amount);

	/** Pause regen for a duration (e.g., after attacking) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Stamina")
	void PauseRegenFor(float Seconds);

	// =========================================================================
	// QUERIES
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Stamina")
	float GetMaxStamina() const { return MaxStamina; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Stamina")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Stamina")
	bool HasStamina(float Amount) const { return CurrentStamina >= Amount; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Stamina")
	bool IsDepleted() const { return CurrentStamina <= 0.0f; }

	// =========================================================================
	// MODIFIERS
	// =========================================================================

	/** Set the armor weight penalty (0.0 = no penalty, 0.9 = 90% slower regen) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Stamina")
	void SetArmorRegenPenalty(float Penalty);

	/** Set additional regen modifier from buffs/debuffs (multiplier, 1.0 = normal) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Stamina")
	void SetRegenModifier(float Modifier) { RegenModifier = FMath::Max(0.0f, Modifier); }

	/** Set max stamina (from leveling, buffs, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Stamina")
	void SetMaxStamina(float NewMax);

	void SetStamina(float NewStamina) { CurrentStamina = FMath::Clamp(NewStamina, 0.0f, MaxStamina); }

	// =========================================================================
	// DELEGATES
	// =========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Stamina")
	FOnStaminaChangedSignature OnStaminaChanged;

	/** Fired when stamina hits zero — triggers stagger in combat system */
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Stamina")
	FOnStaminaDepletedSignature OnStaminaDepleted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Stamina", meta = (ClampMin = "1.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Stamina")
	float CurrentStamina = 100.0f;

	/** Base regen rate per second (before penalties) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Stamina", meta = (ClampMin = "0.0"))
	float BaseRegenRate = 15.0f;

	/** Delay after stamina use before regen starts (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Stamina", meta = (ClampMin = "0.0"))
	float RegenDelay = 1.0f;

	/** Current armor regen penalty (0.0 - 0.9) */
	float ArmorRegenPenalty = 0.0f;

	/** Additional regen multiplier from buffs/debuffs */
	float RegenModifier = 1.0f;

	/** Time remaining before regen resumes */
	float RegenPauseRemaining = 0.0f;

	bool bWasDepleted = false;
};
