// =============================================================================
// IronvaleStatics.h — Static utility functions shared across systems
// Project Ironvale
//
// Pure functions with no side effects — safe to call from any thread.
// All math is engine-agnostic and cross-platform.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "IronvaleTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IronvaleStatics.generated.h"

UCLASS()
class IRONVALE_API UIronvaleStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// =========================================================================
	// COMBAT UTILITIES
	// =========================================================================

	/**
	 * Convert a 2D input vector (mouse delta or right stick) to an attack direction.
	 * Uses angular zones: the stick/mouse direction maps to one of 7 zones.
	 * A small deadzone at center triggers Thrust.
	 *
	 * @param InputVector  Normalized 2D input (X = horizontal, Y = vertical)
	 * @param DeadZone     Magnitude below which input is treated as Thrust
	 * @return The attack direction corresponding to the input angle
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	static EIronvaleAttackDirection InputVectorToAttackDirection(FVector2D InputVector, float DeadZone = 0.3f);

	/**
	 * Get the "opposite" guard direction for an attack — used for AI to determine
	 * where to block, and for the parry system.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	static EIronvaleAttackDirection GetOppositeDirection(EIronvaleAttackDirection Direction);

	/**
	 * Get the montage section name for a given attack direction.
	 * Convention: "Attack_TopLeft", "Attack_Thrust", etc.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	static FName GetMontageSection(EIronvaleAttackDirection Direction);

	// =========================================================================
	// DAMAGE UTILITIES
	// =========================================================================

	/**
	 * Calculate damage effectiveness multiplier: weapon damage type vs armor type.
	 * Returns a multiplier (0.0 = completely ineffective, 2.0 = very effective).
	 *
	 * Matrix:
	 *   Slash  vs Cloth=1.5, Leather=1.2, Mail=0.6, Plate=0.3
	 *   Blunt  vs Cloth=1.0, Leather=1.0, Mail=1.0, Plate=1.5
	 *   Pierce vs Cloth=1.3, Leather=1.0, Mail=1.4, Plate=0.5
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	static float GetDamageTypeVsArmorMultiplier(EIronvaleDamageType DamageType, EIronvaleArmorType ArmorType);

	// =========================================================================
	// NEEDS / SURVIVAL UTILITIES
	// =========================================================================

	/**
	 * Calculate movement speed multiplier based on encumbrance ratio.
	 * Below soft cap: 1.0 (no penalty).
	 * Between soft and hard cap: linear interpolation from 1.0 to 0.5.
	 * At or above hard cap: 0.3 (barely moving).
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Needs")
	static float CalculateEncumbranceSpeedMultiplier(float CurrentWeight, float MaxWeight);

	// =========================================================================
	// TIME UTILITIES
	// =========================================================================

	/**
	 * Convert a game-hour float (0-24) to a time-of-day enum.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	static EIronvaleTimeOfDay HourToTimeOfDay(float GameHour);

	/**
	 * Get a human-readable time string from game hours (e.g., "14:30").
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|World")
	static FString GameHourToTimeString(float GameHour);

	// =========================================================================
	// REPUTATION UTILITIES
	// =========================================================================

	/**
	 * Calculate shop price multiplier based on reputation.
	 * Positive rep = cheaper prices (down to 0.8x at max rep).
	 * Negative rep = expensive prices (up to 1.5x at min rep).
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Reputation")
	static float GetReputationPriceMultiplier(float ReputationValue);
};
