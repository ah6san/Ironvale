// =============================================================================
// IronvaleStatics.cpp — Static utility function implementations
// Project Ironvale
// =============================================================================

#include "Core/IronvaleStatics.h"

EIronvaleAttackDirection UIronvaleStatics::InputVectorToAttackDirection(FVector2D InputVector, float DeadZone)
{
	const float Magnitude = InputVector.Size();

	// Small input = thrust (stab forward)
	if (Magnitude < DeadZone)
	{
		return EIronvaleAttackDirection::Thrust;
	}

	// Normalize and compute angle in degrees (0 = right, 90 = up, etc.)
	const FVector2D Normalized = InputVector.GetSafeNormal();
	float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Normalized.Y, Normalized.X));

	// Convert to 0-360 range
	if (AngleDeg < 0.0f)
	{
		AngleDeg += 360.0f;
	}

	// Map angle to 6 directional zones (60 degrees each):
	//   TopRight:    30 - 90    (upper-right swing)
	//   TopLeft:     90 - 150   (upper-left swing)
	//   Left:        150 - 210  (left horizontal)
	//   BottomLeft:  210 - 270  (lower-left swing)
	//   BottomRight: 270 - 330  (lower-right swing)
	//   Right:       330 - 30   (right horizontal)
	if (AngleDeg >= 30.0f && AngleDeg < 90.0f)
	{
		return EIronvaleAttackDirection::TopRight;
	}
	else if (AngleDeg >= 90.0f && AngleDeg < 150.0f)
	{
		return EIronvaleAttackDirection::TopLeft;
	}
	else if (AngleDeg >= 150.0f && AngleDeg < 210.0f)
	{
		return EIronvaleAttackDirection::Left;
	}
	else if (AngleDeg >= 210.0f && AngleDeg < 270.0f)
	{
		return EIronvaleAttackDirection::BottomLeft;
	}
	else if (AngleDeg >= 270.0f && AngleDeg < 330.0f)
	{
		return EIronvaleAttackDirection::BottomRight;
	}
	else
	{
		return EIronvaleAttackDirection::Right;
	}
}

EIronvaleAttackDirection UIronvaleStatics::GetOppositeDirection(EIronvaleAttackDirection Direction)
{
	switch (Direction)
	{
	case EIronvaleAttackDirection::TopLeft:     return EIronvaleAttackDirection::BottomRight;
	case EIronvaleAttackDirection::TopRight:    return EIronvaleAttackDirection::BottomLeft;
	case EIronvaleAttackDirection::Left:        return EIronvaleAttackDirection::Right;
	case EIronvaleAttackDirection::Right:       return EIronvaleAttackDirection::Left;
	case EIronvaleAttackDirection::BottomLeft:  return EIronvaleAttackDirection::TopRight;
	case EIronvaleAttackDirection::BottomRight: return EIronvaleAttackDirection::TopLeft;
	case EIronvaleAttackDirection::Thrust:      return EIronvaleAttackDirection::Thrust; // Block center
	default:                                    return EIronvaleAttackDirection::None;
	}
}

FName UIronvaleStatics::GetMontageSection(EIronvaleAttackDirection Direction)
{
	switch (Direction)
	{
	case EIronvaleAttackDirection::TopLeft:     return FName("Attack_TopLeft");
	case EIronvaleAttackDirection::TopRight:    return FName("Attack_TopRight");
	case EIronvaleAttackDirection::Left:        return FName("Attack_Left");
	case EIronvaleAttackDirection::Right:       return FName("Attack_Right");
	case EIronvaleAttackDirection::BottomLeft:  return FName("Attack_BottomLeft");
	case EIronvaleAttackDirection::BottomRight: return FName("Attack_BottomRight");
	case EIronvaleAttackDirection::Thrust:      return FName("Attack_Thrust");
	default:                                    return FName("Attack_Right");
	}
}

float UIronvaleStatics::GetDamageTypeVsArmorMultiplier(EIronvaleDamageType DamageType, EIronvaleArmorType ArmorType)
{
	// Effectiveness matrix: rows = damage type, columns = armor type
	// Higher = more effective against that armor
	//
	// Design rationale:
	//   Slash cuts through cloth/leather but bounces off metal
	//   Blunt transfers force through all armor, best vs plate (concussive)
	//   Pierce finds gaps in mail but deflects off solid plate
	static const float Matrix[3][5] = {
		//            None  Cloth  Leather  Mail  Plate
		/* Slash  */ { 1.0f, 1.5f,  1.2f,   0.6f, 0.3f },
		/* Blunt  */ { 1.0f, 1.0f,  1.0f,   1.0f, 1.5f },
		/* Pierce */ { 1.0f, 1.3f,  1.0f,   1.4f, 0.5f }
	};

	const int32 DmgIdx = static_cast<int32>(DamageType);
	const int32 ArmIdx = static_cast<int32>(ArmorType);

	if (DmgIdx >= 0 && DmgIdx < 3 && ArmIdx >= 0 && ArmIdx < 5)
	{
		return Matrix[DmgIdx][ArmIdx];
	}

	return 1.0f;
}

float UIronvaleStatics::CalculateEncumbranceSpeedMultiplier(float CurrentWeight, float MaxWeight)
{
	if (MaxWeight <= 0.0f) return 1.0f;

	const float Ratio = CurrentWeight / MaxWeight;
	const float SoftCap = IronvaleConstants::SOFT_ENCUMBRANCE_RATIO;

	if (Ratio <= SoftCap)
	{
		// Below soft cap: no penalty
		return 1.0f;
	}
	else if (Ratio < 1.0f)
	{
		// Between soft and hard cap: lerp from 1.0 down to 0.5
		const float T = (Ratio - SoftCap) / (1.0f - SoftCap);
		return FMath::Lerp(1.0f, 0.5f, T);
	}
	else
	{
		// At or over hard cap: barely moving
		return 0.3f;
	}
}

EIronvaleTimeOfDay UIronvaleStatics::HourToTimeOfDay(float GameHour)
{
	// Wrap to 0-24
	GameHour = FMath::Fmod(GameHour, 24.0f);
	if (GameHour < 0.0f) GameHour += 24.0f;

	if (GameHour >= 5.0f && GameHour < 7.0f)   return EIronvaleTimeOfDay::Dawn;
	if (GameHour >= 7.0f && GameHour < 11.0f)  return EIronvaleTimeOfDay::Morning;
	if (GameHour >= 11.0f && GameHour < 14.0f) return EIronvaleTimeOfDay::Midday;
	if (GameHour >= 14.0f && GameHour < 17.0f) return EIronvaleTimeOfDay::Afternoon;
	if (GameHour >= 17.0f && GameHour < 19.0f) return EIronvaleTimeOfDay::Dusk;

	return EIronvaleTimeOfDay::Night; // 19:00 - 5:00
}

FString UIronvaleStatics::GameHourToTimeString(float GameHour)
{
	GameHour = FMath::Fmod(GameHour, 24.0f);
	if (GameHour < 0.0f) GameHour += 24.0f;

	const int32 Hours = FMath::FloorToInt32(GameHour);
	const int32 Minutes = FMath::FloorToInt32((GameHour - Hours) * 60.0f);

	return FString::Printf(TEXT("%02d:%02d"), Hours, Minutes);
}

float UIronvaleStatics::GetReputationPriceMultiplier(float ReputationValue)
{
	// Clamp to valid range
	ReputationValue = FMath::Clamp(ReputationValue,
		IronvaleConstants::MIN_REPUTATION,
		IronvaleConstants::MAX_REPUTATION);

	// Linear interpolation:
	//   Rep -100 → 1.5x prices (50% markup)
	//   Rep    0 → 1.0x prices (normal)
	//   Rep +100 → 0.8x prices (20% discount)
	if (ReputationValue >= 0.0f)
	{
		// Positive rep: lerp from 1.0 to 0.8
		const float T = ReputationValue / IronvaleConstants::MAX_REPUTATION;
		return FMath::Lerp(1.0f, 0.8f, T);
	}
	else
	{
		// Negative rep: lerp from 1.0 to 1.5
		const float T = FMath::Abs(ReputationValue) / FMath::Abs(IronvaleConstants::MIN_REPUTATION);
		return FMath::Lerp(1.0f, 1.5f, T);
	}
}
