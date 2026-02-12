// =============================================================================
// IronvaleDamageCalculator.h — Static damage math: weapon vs armor
// Project Ironvale
//
// Pure static functions — no state, no side effects, safe to call anywhere.
// Takes attack data + armor layers and produces a FIronvaleDamageResult.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleCombatTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IronvaleDamageCalculator.generated.h"

UCLASS()
class IRONVALE_API UIronvaleDamageCalculator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Calculate final damage from an attack against armor layers.
	 *
	 * Process:
	 * 1. Start with base weapon damage * charge multiplier
	 * 2. For each armor layer (outside-in: plate → mail → leather → cloth):
	 *    a. Calculate weapon-vs-armor effectiveness multiplier
	 *    b. Reduce damage by layer's effective armor rating * effectiveness
	 *    c. Calculate durability damage to the armor layer
	 * 3. Apply headshot / critical multiplier
	 * 4. Calculate stagger (based on weapon stagger power minus armor mitigation)
	 * 5. Calculate weapon durability damage
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	static FIronvaleDamageResult CalculateDamage(
		const FIronvaleAttackData& Attack,
		const TArray<FIronvaleArmorLayer>& ArmorLayers,
		EIronvaleArmorZone HitZone);

	/**
	 * Get the headshot/zone damage multiplier.
	 * Head = 2.0x, Torso = 1.0x, Arms = 0.75x, Legs = 0.8x
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	static float GetZoneDamageMultiplier(EIronvaleArmorZone Zone);

	/**
	 * Calculate how much durability damage an armor layer takes from a hit.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	static float CalculateArmorDurabilityDamage(
		float IncomingDamage,
		EIronvaleDamageType DamageType,
		EIronvaleArmorType ArmorType);

	/**
	 * Calculate weapon durability damage from striking armor.
	 * Striking plate with a sword dulls it faster than striking cloth.
	 */
	UFUNCTION(BlueprintPure, Category = "Ironvale|Combat")
	static float CalculateWeaponDurabilityDamage(
		EIronvaleDamageType DamageType,
		EIronvaleArmorType HardestArmorHit);
};
