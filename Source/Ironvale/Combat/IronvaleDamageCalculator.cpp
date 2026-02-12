// =============================================================================
// IronvaleDamageCalculator.cpp — Damage calculation implementation
// Project Ironvale
// =============================================================================

#include "Combat/IronvaleDamageCalculator.h"
#include "Core/IronvaleStatics.h"

FIronvaleDamageResult UIronvaleDamageCalculator::CalculateDamage(
	const FIronvaleAttackData& Attack,
	const TArray<FIronvaleArmorLayer>& ArmorLayers,
	EIronvaleArmorZone HitZone)
{
	FIronvaleDamageResult Result;
	Result.HitZone = HitZone;
	Result.AttackDirection = Attack.Direction;
	Result.DamageType = Attack.DamageType;

	// Step 1: Base damage (already includes charge multiplier from CombatComponent)
	float RemainingDamage = Attack.BaseDamage;
	float TotalStagger = Attack.StaggerPower;
	EIronvaleArmorType HardestArmor = EIronvaleArmorType::None;

	// Step 2: Process armor layers (outermost first — array is sorted cloth→plate,
	// so iterate in reverse for outside-in processing)
	float TotalArmorDurabilityDamage = 0.0f;

	for (int32 i = ArmorLayers.Num() - 1; i >= 0; --i)
	{
		const FIronvaleArmorLayer& Layer = ArmorLayers[i];

		if (Layer.ArmorType == EIronvaleArmorType::None) continue;
		if (Layer.CurrentDurability <= 0.0f) continue; // Broken armor provides no protection

		// Track hardest armor hit (for weapon durability)
		if (static_cast<int32>(Layer.ArmorType) > static_cast<int32>(HardestArmor))
		{
			HardestArmor = Layer.ArmorType;
		}

		// Effectiveness multiplier: how well this weapon type works against this armor
		const float Effectiveness = UIronvaleStatics::GetDamageTypeVsArmorMultiplier(
			Attack.DamageType, Layer.ArmorType);

		// Effective armor rating (accounts for durability degradation)
		const float EffectiveRating = Layer.GetEffectiveRating();

		// Damage reduction: higher effectiveness means MORE damage gets through armor
		// (i.e., lower armor effectiveness against this weapon type)
		// A slash against plate (multiplier 0.3) means only 30% of damage gets through
		// But the armor also absorbs based on its rating
		const float Absorbed = EffectiveRating * (2.0f - Effectiveness);
		RemainingDamage = FMath::Max(0.0f, RemainingDamage - Absorbed);

		// Stagger is reduced by armor but less so than damage
		TotalStagger *= FMath::Lerp(1.0f, 0.5f, EffectiveRating / 100.0f);

		// Armor durability damage
		const float LayerDurabilityDamage = CalculateArmorDurabilityDamage(
			Attack.BaseDamage, Attack.DamageType, Layer.ArmorType);
		TotalArmorDurabilityDamage += LayerDurabilityDamage;

		// If damage is fully absorbed, remaining layers take no damage
		if (RemainingDamage <= 0.0f) break;
	}

	// Step 3: Zone multiplier (headshots deal more damage)
	const float ZoneMultiplier = GetZoneDamageMultiplier(HitZone);
	RemainingDamage *= ZoneMultiplier;

	// Check for critical hit (head zone with significant damage)
	if (HitZone == EIronvaleArmorZone::Head && RemainingDamage > 5.0f)
	{
		Result.bCriticalHit = true;
	}

	// Step 4: Determine if armor was penetrated (damage got through all layers)
	Result.bArmorPenetrated = (ArmorLayers.Num() > 0 && RemainingDamage > 0.0f);

	// Step 5: Final values
	Result.FinalDamage = FMath::Max(0.0f, RemainingDamage);
	Result.StaggerDamage = FMath::Max(0.0f, TotalStagger);
	Result.ArmorDurabilityDamage = TotalArmorDurabilityDamage;
	Result.WeaponDurabilityDamage = CalculateWeaponDurabilityDamage(Attack.DamageType, HardestArmor);

	return Result;
}

float UIronvaleDamageCalculator::GetZoneDamageMultiplier(EIronvaleArmorZone Zone)
{
	switch (Zone)
	{
	case EIronvaleArmorZone::Head:     return 2.0f;
	case EIronvaleArmorZone::Torso:    return 1.0f;
	case EIronvaleArmorZone::LeftArm:  return 0.75f;
	case EIronvaleArmorZone::RightArm: return 0.75f;
	case EIronvaleArmorZone::Legs:     return 0.8f;
	case EIronvaleArmorZone::Feet:     return 0.6f;
	default:                           return 1.0f;
	}
}

float UIronvaleDamageCalculator::CalculateArmorDurabilityDamage(
	float IncomingDamage, EIronvaleDamageType DamageType, EIronvaleArmorType ArmorType)
{
	// Base durability damage is proportional to incoming force
	float BaseDurDamage = IncomingDamage * 0.1f;

	// Blunt weapons damage armor more (denting plate, breaking mail links)
	// Slash weapons damage armor less (glancing off metal)
	switch (DamageType)
	{
	case EIronvaleDamageType::Blunt:  BaseDurDamage *= 1.5f; break;
	case EIronvaleDamageType::Slash:  BaseDurDamage *= 0.8f; break;
	case EIronvaleDamageType::Pierce: BaseDurDamage *= 1.0f; break;
	}

	return FMath::Max(0.5f, BaseDurDamage); // Minimum durability damage per hit
}

float UIronvaleDamageCalculator::CalculateWeaponDurabilityDamage(
	EIronvaleDamageType DamageType, EIronvaleArmorType HardestArmorHit)
{
	float BaseDurDamage = 1.0f;

	// Striking harder armor wears weapons faster
	switch (HardestArmorHit)
	{
	case EIronvaleArmorType::None:    BaseDurDamage = 0.5f; break;
	case EIronvaleArmorType::Cloth:   BaseDurDamage = 0.5f; break;
	case EIronvaleArmorType::Leather: BaseDurDamage = 0.8f; break;
	case EIronvaleArmorType::Mail:    BaseDurDamage = 1.5f; break;
	case EIronvaleArmorType::Plate:   BaseDurDamage = 2.5f; break;
	}

	// Slash weapons degrade faster against hard armor than blunt
	if (DamageType == EIronvaleDamageType::Slash &&
		(HardestArmorHit == EIronvaleArmorType::Mail || HardestArmorHit == EIronvaleArmorType::Plate))
	{
		BaseDurDamage *= 1.5f;
	}

	return BaseDurDamage;
}
