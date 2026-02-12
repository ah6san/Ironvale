// =============================================================================
// IronvaleCombatTypes.h — Combat-specific data structures
// Project Ironvale
//
// Structs used by the combat system for attacks, blocks, and hit processing.
// Separated from IronvaleTypes.h to keep combat internals self-contained.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleCombatTypes.generated.h"

/**
 * Data for a pending or in-progress attack.
 */
USTRUCT(BlueprintType)
struct FIronvaleAttackData
{
	GENERATED_BODY()

	/** Direction of this attack */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EIronvaleAttackDirection Direction = EIronvaleAttackDirection::None;

	/** Weapon damage type */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EIronvaleDamageType DamageType = EIronvaleDamageType::Slash;

	/** Base damage from weapon */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float BaseDamage = 0.0f;

	/** Charge level (0.0 = quick, 1.0 = fully charged). Charged attacks deal more damage. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float ChargeLevel = 0.0f;

	/** Stagger power from weapon */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float StaggerPower = 0.0f;

	/** Stamina cost for this attack */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float StaminaCost = 0.0f;

	/** Whether this attack has passed the feint-cancel window (committed) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsCommitted = false;

	/** Whether this attack is a riposte (bonus damage, faster) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsRiposte = false;

	/** Timestamp when attack started (for timing calculations) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float StartTime = 0.0f;

	/** The attacking actor */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TWeakObjectPtr<AActor> Attacker;
};

/**
 * Result of a block attempt.
 */
USTRUCT(BlueprintType)
struct FIronvaleBlockResult
{
	GENERATED_BODY()

	/** Was the block successful? */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bBlocked = false;

	/** Was it a perfect parry (within the parry timing window)? */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPerfectParry = false;

	/** Was the block direction correct (matching the attack direction)? */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCorrectDirection = false;

	/** Stamina cost of the block */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float StaminaCost = 0.0f;

	/** Residual damage that passed through the block (if any) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float ResidualDamage = 0.0f;

	/** Stagger applied to the blocker */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float StaggerToBlocker = 0.0f;

	/** Stagger applied back to the attacker (on perfect parry) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float StaggerToAttacker = 0.0f;
};

/**
 * Detailed hit information — produced by weapon trace, consumed by damage calculation.
 */
USTRUCT(BlueprintType)
struct FIronvaleHitData
{
	GENERATED_BODY()

	/** The actor that was hit */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	TWeakObjectPtr<AActor> HitActor;

	/** World-space hit location */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FVector HitLocation = FVector::ZeroVector;

	/** Hit normal */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FVector HitNormal = FVector::UpVector;

	/** Bone name hit (for skeletal mesh hit detection) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FName HitBoneName;

	/** Armor zone determined from bone/location */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EIronvaleArmorZone HitZone = EIronvaleArmorZone::Torso;

	/** The attack data that generated this hit */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FIronvaleAttackData AttackData;

	/** Impact velocity for physics response */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FVector ImpactVelocity = FVector::ZeroVector;
};

/**
 * Input buffer entry — stores recent combat inputs for combo detection.
 */
USTRUCT(BlueprintType)
struct FIronvaleCombatInputEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EIronvaleAttackDirection Direction = EIronvaleAttackDirection::None;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float Timestamp = 0.0f;

	/** True if this was a charged (held) input */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCharged = false;
};

/**
 * Combo definition — a sequence of attack directions that triggers a special move.
 */
USTRUCT(BlueprintType)
struct FIronvaleComboDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FName ComboID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FText DisplayName;

	/** Sequence of attack directions required to trigger */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TArray<EIronvaleAttackDirection> Sequence;

	/** Maximum time between inputs (seconds) before the combo resets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float MaxTimeBetweenInputs = 1.0f;

	/** Damage multiplier applied to the final hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float DamageMultiplier = 1.5f;

	/** Extra stamina cost for the combo finisher */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float BonusStaminaCost = 10.0f;

	/** Montage section name for the combo finisher animation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	FName FinisherMontageSection;

	/** Weapon types this combo is available for */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TArray<EIronvaleWeaponType> ValidWeaponTypes;
};

/**
 * Weapon vs armor type effectiveness lookup — DataTable row for designer tuning.
 */
USTRUCT(BlueprintType)
struct FIronvaleDamageEffectivenessRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	EIronvaleDamageType DamageType = EIronvaleDamageType::Slash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	EIronvaleArmorType ArmorType = EIronvaleArmorType::None;

	/** Damage multiplier: > 1.0 = effective, < 1.0 = ineffective */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat",
		meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float Multiplier = 1.0f;
};
