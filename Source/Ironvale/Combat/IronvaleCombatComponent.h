// =============================================================================
// IronvaleCombatComponent.h — KCD-style directional melee combat state machine
// Project Ironvale
//
// This is the core combat logic component. It manages:
//   - Combat state machine (Idle → LockedOn → Attacking/Blocking/Parrying...)
//   - Attack initiation, charge, and release
//   - Block and parry timing windows
//   - Input buffering for responsive feel
//   - Combo detection and execution
//   - Feinting (cancel before commit point)
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleCombatTypes.h"
#include "IronvaleCombatComponent.generated.h"

class UIronvaleStaminaComponent;
class UIronvaleHealthComponent;
class UIronvaleEquipmentComponent;
class AIronvaleWeaponActor;

UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleCombatComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =========================================================================
	// STATE MACHINE
	// =========================================================================

	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	EIronvaleCombatState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Combat")
	void ForceState(EIronvaleCombatState NewState);

	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	bool IsInCombat() const { return CurrentState != EIronvaleCombatState::Idle && CurrentState != EIronvaleCombatState::Dead; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	bool CanAttack() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	bool CanBlock() const;

	// =========================================================================
	// ATTACK INPUT
	// =========================================================================

	/** Start an attack in the given direction. Hold to charge. */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Combat")
	void StartAttack(EIronvaleAttackDirection Direction);

	/** Release attack (ends charge, commits attack) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Combat")
	void ReleaseAttack();

	/** Cancel attack before commit point (feint) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Combat")
	bool TryFeint();

	// =========================================================================
	// BLOCK / PARRY INPUT
	// =========================================================================

	/** Start blocking */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Combat")
	void StartBlock();

	/** Stop blocking */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Combat")
	void StopBlock();

	/** Get current block direction (mirrors the opponent's attack direction) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	EIronvaleAttackDirection GetBlockDirection() const { return CurrentBlockDirection; }

	// =========================================================================
	// HIT PROCESSING (called by weapon trace system)
	// =========================================================================

	/** Process an incoming hit against this character */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Combat")
	FIronvaleDamageResult ProcessIncomingHit(const FIronvaleHitData& HitData);

	/** Get the current attack data (for weapon trace to use) */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	const FIronvaleAttackData& GetCurrentAttackData() const { return CurrentAttack; }

	// =========================================================================
	// STAGGER
	// =========================================================================

	/** Apply stagger to this character */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Combat")
	void ApplyStagger(float StaggerAmount);

	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	float GetCurrentStagger() const { return CurrentStagger; }

	// =========================================================================
	// CONFIGURATION
	// =========================================================================

	/** Parry window duration in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	float ParryWindowDuration = IronvaleConstants::DEFAULT_PARRY_WINDOW;

	/** Riposte window after successful parry */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	float RiposteWindowDuration = IronvaleConstants::DEFAULT_RIPOSTE_WINDOW;

	/** Time before attack animation commits (feint cancel window) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	float FeintCancelWindow = 0.15f;

	/** Max charge time before auto-release */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	float MaxChargeTime = 1.5f;

	/** Stagger threshold — exceeding this causes stagger state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	float StaggerThreshold = 50.0f;

	/** Stagger recovery rate per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	float StaggerRecoveryRate = 20.0f;

	/** Stagger duration (locked in stagger state for this long) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	float StaggerDuration = 1.0f;

	/** Combo definitions — loaded from DataTable or set in Blueprint */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	TArray<FIronvaleComboDefinition> ComboDefinitions;

	/** Input buffer window — inputs this old can still trigger combos */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ironvale|Combat|Config")
	float InputBufferWindow = 0.5f;

protected:
	// --- State ---
	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Combat")
	EIronvaleCombatState CurrentState = EIronvaleCombatState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Combat")
	FIronvaleAttackData CurrentAttack;

	UPROPERTY(BlueprintReadOnly, Category = "Ironvale|Combat")
	EIronvaleAttackDirection CurrentBlockDirection = EIronvaleAttackDirection::None;

	float BlockStartTime = 0.0f;
	float StaggerTimer = 0.0f;
	float ChargeStartTime = 0.0f;
	float CurrentStagger = 0.0f;
	bool bIsCharging = false;
	bool bRiposteAvailable = false;
	float RiposteWindowTimer = 0.0f;

	// --- Input Buffer ---
	TArray<FIronvaleCombatInputEntry> InputBuffer;

	// --- Component refs (cached on BeginPlay) ---
	UIronvaleStaminaComponent* StaminaComp = nullptr;
	UIronvaleHealthComponent* HealthComp = nullptr;
	UIronvaleEquipmentComponent* EquipmentComp = nullptr;

	// --- State transitions ---
	void TransitionTo(EIronvaleCombatState NewState);
	void OnEnterState(EIronvaleCombatState State);
	void OnExitState(EIronvaleCombatState State);

	// --- Internal ---
	void UpdateCharge(float DeltaTime);
	void UpdateStagger(float DeltaTime);
	void UpdateRiposteWindow(float DeltaTime);
	void CommitAttack();
	FIronvaleBlockResult EvaluateBlock(const FIronvaleAttackData& IncomingAttack);
	void AddToInputBuffer(EIronvaleAttackDirection Direction, bool bCharged);
	void CheckCombos();
	void ClearExpiredInputs();

	/** Get weapon data from equipment component */
	const FIronvaleItemData* GetEquippedWeaponData() const;
};
