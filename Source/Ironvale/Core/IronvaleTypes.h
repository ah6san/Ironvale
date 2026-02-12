// =============================================================================
// IronvaleTypes.h — Shared enums, structs, and constants used across all systems
// Project Ironvale: First-person grounded medieval RPG
//
// This is the central type registry. Every system includes this header for
// shared vocabulary types. Keep it lean — only types used by 2+ systems belong here.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "IronvaleTypes.generated.h"

// =============================================================================
// COMBAT ENUMS
// =============================================================================

/**
 * Directional attack zones — KCD-style 5-direction + thrust system.
 * Maps to animation montage sections and input directions.
 */
UENUM(BlueprintType)
enum class EIronvaleAttackDirection : uint8
{
	None        UMETA(DisplayName = "None"),
	TopLeft     UMETA(DisplayName = "Top Left"),
	TopRight    UMETA(DisplayName = "Top Right"),
	Left        UMETA(DisplayName = "Left"),
	Right       UMETA(DisplayName = "Right"),
	BottomLeft  UMETA(DisplayName = "Bottom Left"),
	BottomRight UMETA(DisplayName = "Bottom Right"),
	Thrust      UMETA(DisplayName = "Thrust")
};

/**
 * Combat state machine states.
 * CombatComponent transitions between these based on input and game events.
 */
UENUM(BlueprintType)
enum class EIronvaleCombatState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	LockedOn    UMETA(DisplayName = "Locked On"),
	Attacking   UMETA(DisplayName = "Attacking"),
	Blocking    UMETA(DisplayName = "Blocking"),
	Parrying    UMETA(DisplayName = "Parrying"),
	Riposting   UMETA(DisplayName = "Riposting"),
	Staggered   UMETA(DisplayName = "Staggered"),
	Dodging     UMETA(DisplayName = "Dodging"),
	Dead        UMETA(DisplayName = "Dead")
};

/**
 * Damage types — determines effectiveness against different armor types.
 * Slash is good vs cloth/leather, Blunt vs plate, Pierce vs mail.
 */
UENUM(BlueprintType)
enum class EIronvaleDamageType : uint8
{
	Slash   UMETA(DisplayName = "Slash"),
	Blunt   UMETA(DisplayName = "Blunt"),
	Pierce  UMETA(DisplayName = "Pierce")
};

/**
 * Weapon categories — affects animations, damage type, speed, and reach.
 */
UENUM(BlueprintType)
enum class EIronvaleWeaponType : uint8
{
	Fists    UMETA(DisplayName = "Fists"),
	Sword    UMETA(DisplayName = "Sword"),
	Mace     UMETA(DisplayName = "Mace"),
	Axe      UMETA(DisplayName = "Axe"),
	Polearm  UMETA(DisplayName = "Polearm"),
	Dagger   UMETA(DisplayName = "Dagger"),
	Shield   UMETA(DisplayName = "Shield")
};

// =============================================================================
// ARMOR ENUMS
// =============================================================================

/**
 * Body zones for hit detection — each zone can have independent armor layers.
 */
UENUM(BlueprintType)
enum class EIronvaleArmorZone : uint8
{
	Head      UMETA(DisplayName = "Head"),
	Torso     UMETA(DisplayName = "Torso"),
	LeftArm   UMETA(DisplayName = "Left Arm"),
	RightArm  UMETA(DisplayName = "Right Arm"),
	Legs      UMETA(DisplayName = "Legs"),
	Feet      UMETA(DisplayName = "Feet")
};

/**
 * Armor material types — layered system (cloth under mail under plate).
 * Damage reduction depends on weapon damage type vs armor type.
 */
UENUM(BlueprintType)
enum class EIronvaleArmorType : uint8
{
	None     UMETA(DisplayName = "None"),
	Cloth    UMETA(DisplayName = "Cloth"),
	Leather  UMETA(DisplayName = "Leather"),
	Mail     UMETA(DisplayName = "Mail"),
	Plate    UMETA(DisplayName = "Plate")
};

// =============================================================================
// ITEM ENUMS
// =============================================================================

/**
 * Item categories for inventory organization, filtering, and slot validation.
 */
UENUM(BlueprintType)
enum class EIronvaleItemCategory : uint8
{
	Weapon           UMETA(DisplayName = "Weapon"),
	Armor            UMETA(DisplayName = "Armor"),
	Consumable       UMETA(DisplayName = "Consumable"),
	CraftingMaterial UMETA(DisplayName = "Crafting Material"),
	QuestItem        UMETA(DisplayName = "Quest Item"),
	Book             UMETA(DisplayName = "Book"),
	Misc             UMETA(DisplayName = "Miscellaneous")
};

/**
 * Item quality tiers — affects base stats, price, and visual appearance.
 */
UENUM(BlueprintType)
enum class EIronvaleItemQuality : uint8
{
	Broken    UMETA(DisplayName = "Broken"),
	Poor      UMETA(DisplayName = "Poor"),
	Common    UMETA(DisplayName = "Common"),
	Fine      UMETA(DisplayName = "Fine"),
	Superior  UMETA(DisplayName = "Superior"),
	Masterwork UMETA(DisplayName = "Masterwork")
};

// =============================================================================
// PLAYER NEEDS ENUMS
// =============================================================================

/**
 * Player survival needs — each decays over time and applies debuffs at thresholds.
 */
UENUM(BlueprintType)
enum class EIronvaleNeedType : uint8
{
	Hunger       UMETA(DisplayName = "Hunger"),
	Thirst       UMETA(DisplayName = "Thirst"),
	Fatigue      UMETA(DisplayName = "Fatigue"),
	Cleanliness  UMETA(DisplayName = "Cleanliness")
};

// =============================================================================
// WORLD / ENVIRONMENT ENUMS
// =============================================================================

/**
 * Weather states — drives visual FX, audio ambience, AI schedules, and gameplay modifiers.
 */
UENUM(BlueprintType)
enum class EIronvaleWeatherState : uint8
{
	Clear   UMETA(DisplayName = "Clear"),
	Cloudy  UMETA(DisplayName = "Cloudy"),
	Rain    UMETA(DisplayName = "Rain"),
	Storm   UMETA(DisplayName = "Storm"),
	Fog     UMETA(DisplayName = "Fog"),
	Snow    UMETA(DisplayName = "Snow")
};

/**
 * Time-of-day periods — broadcast to systems that need dawn/dusk/night behavior.
 */
UENUM(BlueprintType)
enum class EIronvaleTimeOfDay : uint8
{
	Dawn      UMETA(DisplayName = "Dawn"),       // 5:00 - 7:00
	Morning   UMETA(DisplayName = "Morning"),    // 7:00 - 11:00
	Midday    UMETA(DisplayName = "Midday"),     // 11:00 - 14:00
	Afternoon UMETA(DisplayName = "Afternoon"),  // 14:00 - 17:00
	Dusk      UMETA(DisplayName = "Dusk"),       // 17:00 - 19:00
	Night     UMETA(DisplayName = "Night")       // 19:00 - 5:00
};

/**
 * World biome types — used for encounter selection, audio, and visual theming.
 */
UENUM(BlueprintType)
enum class EIronvaleBiome : uint8
{
	Forest     UMETA(DisplayName = "Forest"),
	Valley     UMETA(DisplayName = "Valley"),
	Mountain   UMETA(DisplayName = "Mountain"),
	Swamp      UMETA(DisplayName = "Swamp"),
	Town       UMETA(DisplayName = "Town"),
	Castle     UMETA(DisplayName = "Castle"),
	Road       UMETA(DisplayName = "Road"),
	Dungeon    UMETA(DisplayName = "Dungeon")
};

// =============================================================================
// NPC ENUMS
// =============================================================================

/**
 * NPC archetypes — determine default behavior trees, dialogue pools, and schedules.
 */
UENUM(BlueprintType)
enum class EIronvaleNPCArchetype : uint8
{
	Villager  UMETA(DisplayName = "Villager"),
	Guard     UMETA(DisplayName = "Guard"),
	Merchant  UMETA(DisplayName = "Merchant"),
	Priest    UMETA(DisplayName = "Priest"),
	Noble     UMETA(DisplayName = "Noble"),
	Child     UMETA(DisplayName = "Child"),
	Bandit    UMETA(DisplayName = "Bandit"),
	Traveler  UMETA(DisplayName = "Traveler")
};

/**
 * NPC activities for the schedule system.
 */
UENUM(BlueprintType)
enum class EIronvaleActivity : uint8
{
	Work     UMETA(DisplayName = "Work"),
	Eat      UMETA(DisplayName = "Eat"),
	Sleep    UMETA(DisplayName = "Sleep"),
	Leisure  UMETA(DisplayName = "Leisure"),
	Patrol   UMETA(DisplayName = "Patrol"),
	Pray     UMETA(DisplayName = "Pray"),
	Shop     UMETA(DisplayName = "Shop"),
	Travel   UMETA(DisplayName = "Travel"),
	Flee     UMETA(DisplayName = "Flee"),
	Combat   UMETA(DisplayName = "Combat")
};

// =============================================================================
// QUEST ENUMS
// =============================================================================

/**
 * Quest states for tracking progression.
 */
UENUM(BlueprintType)
enum class EIronvaleQuestState : uint8
{
	NotStarted  UMETA(DisplayName = "Not Started"),
	Active      UMETA(DisplayName = "Active"),
	Completed   UMETA(DisplayName = "Completed"),
	Failed      UMETA(DisplayName = "Failed"),
	Abandoned   UMETA(DisplayName = "Abandoned")
};

/**
 * Quest objective types.
 */
UENUM(BlueprintType)
enum class EIronvaleObjectiveType : uint8
{
	Kill           UMETA(DisplayName = "Kill"),
	Collect        UMETA(DisplayName = "Collect"),
	TalkTo         UMETA(DisplayName = "Talk To"),
	ReachLocation  UMETA(DisplayName = "Reach Location"),
	Escort         UMETA(DisplayName = "Escort"),
	Survive        UMETA(DisplayName = "Survive"),
	UseItem        UMETA(DisplayName = "Use Item"),
	Custom         UMETA(DisplayName = "Custom")
};

// =============================================================================
// DIALOGUE ENUMS
// =============================================================================

/**
 * Dialogue condition types for branching logic.
 */
UENUM(BlueprintType)
enum class EIronvaleDialogueConditionType : uint8
{
	Flag        UMETA(DisplayName = "World Flag"),
	Stat        UMETA(DisplayName = "Player Stat"),
	Reputation  UMETA(DisplayName = "Faction Reputation"),
	QuestStage  UMETA(DisplayName = "Quest Stage"),
	HasItem     UMETA(DisplayName = "Has Item"),
	HasTag      UMETA(DisplayName = "Has Gameplay Tag"),
	TimeOfDay   UMETA(DisplayName = "Time of Day"),
	Weather     UMETA(DisplayName = "Weather"),
	NeedLevel   UMETA(DisplayName = "Need Level")
};

/**
 * Comparison operators for dialogue conditions.
 */
UENUM(BlueprintType)
enum class EIronvaleComparisonOp : uint8
{
	Equal          UMETA(DisplayName = "=="),
	NotEqual       UMETA(DisplayName = "!="),
	GreaterThan    UMETA(DisplayName = ">"),
	LessThan       UMETA(DisplayName = "<"),
	GreaterOrEqual UMETA(DisplayName = ">="),
	LessOrEqual    UMETA(DisplayName = "<=")
};

/**
 * Actions that dialogue nodes can trigger.
 */
UENUM(BlueprintType)
enum class EIronvaleDialogueActionType : uint8
{
	SetFlag           UMETA(DisplayName = "Set World Flag"),
	ModifyReputation  UMETA(DisplayName = "Modify Reputation"),
	StartQuest        UMETA(DisplayName = "Start Quest"),
	AdvanceQuest      UMETA(DisplayName = "Advance Quest"),
	FailQuest         UMETA(DisplayName = "Fail Quest"),
	GiveItem          UMETA(DisplayName = "Give Item"),
	RemoveItem        UMETA(DisplayName = "Remove Item"),
	GiveGold          UMETA(DisplayName = "Give Gold"),
	ScheduleNPC       UMETA(DisplayName = "Schedule NPC Action"),
	PlayAnimation     UMETA(DisplayName = "Play Animation"),
	TeleportPlayer    UMETA(DisplayName = "Teleport Player"),
	ModifyNeed        UMETA(DisplayName = "Modify Player Need"),
	OpenShop          UMETA(DisplayName = "Open Shop Interface")
};

// =============================================================================
// INTERACTION ENUMS
// =============================================================================

/**
 * Types of world interactions available to the player.
 */
UENUM(BlueprintType)
enum class EIronvaleInteractionType : uint8
{
	Examine  UMETA(DisplayName = "Examine"),
	PickUp   UMETA(DisplayName = "Pick Up"),
	Open     UMETA(DisplayName = "Open"),
	Sit      UMETA(DisplayName = "Sit"),
	Sleep    UMETA(DisplayName = "Sleep"),
	Use      UMETA(DisplayName = "Use"),       // Forge, workbench, etc.
	Read     UMETA(DisplayName = "Read"),
	Talk     UMETA(DisplayName = "Talk"),
	Harvest  UMETA(DisplayName = "Harvest"),    // Herbs, ore, wood
	Fish     UMETA(DisplayName = "Fish"),
	Lockpick UMETA(DisplayName = "Lockpick")
};

// =============================================================================
// SHARED STRUCTS
// =============================================================================

/**
 * Result of a damage calculation — passed from combat system to health/stagger systems.
 */
USTRUCT(BlueprintType)
struct FIronvaleDamageResult
{
	GENERATED_BODY()

	/** Final damage after all reductions */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float FinalDamage = 0.0f;

	/** Stagger value applied to target (may trigger stagger state if exceeds threshold) */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float StaggerDamage = 0.0f;

	/** Whether the attack penetrated armor */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bArmorPenetrated = false;

	/** Zone that was hit */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EIronvaleArmorZone HitZone = EIronvaleArmorZone::Torso;

	/** Direction the attack came from */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EIronvaleAttackDirection AttackDirection = EIronvaleAttackDirection::None;

	/** Damage type dealt */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	EIronvaleDamageType DamageType = EIronvaleDamageType::Slash;

	/** Whether this was a critical/headshot */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bCriticalHit = false;

	/** Durability damage to the defender's armor in the hit zone */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float ArmorDurabilityDamage = 0.0f;

	/** Durability damage to the attacker's weapon */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float WeaponDurabilityDamage = 0.0f;
};

/**
 * Reputation entry — stored per faction in the reputation subsystem.
 */
USTRUCT(BlueprintType)
struct FIronvaleReputationEntry
{
	GENERATED_BODY()

	/** Faction identifier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation")
	FName FactionID;

	/** Reputation value: -100 (hated) to +100 (revered) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reputation", meta = (ClampMin = "-100.0", ClampMax = "100.0"))
	float Value = 0.0f;

	/** Game-time when this reputation was last modified (for decay/cooldown systems) */
	UPROPERTY(BlueprintReadOnly, Category = "Reputation")
	float LastModifiedGameTime = 0.0f;
};

/**
 * Need status snapshot — returned by NeedsComponent for UI and other systems.
 */
USTRUCT(BlueprintType)
struct FIronvaleNeedStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Needs")
	EIronvaleNeedType NeedType = EIronvaleNeedType::Hunger;

	/** Current value: 100 = fully satisfied, 0 = critical */
	UPROPERTY(BlueprintReadOnly, Category = "Needs")
	float CurrentValue = 100.0f;

	/** Rate of decay per game-hour */
	UPROPERTY(BlueprintReadOnly, Category = "Needs")
	float DecayRate = 1.0f;

	/** Active debuff tags from threshold breaches */
	UPROPERTY(BlueprintReadOnly, Category = "Needs")
	FGameplayTagContainer ActiveDebuffs;
};

/**
 * Armor layer data — represents one piece of armor in one zone.
 * Characters can have multiple layers per zone (cloth + mail + plate).
 */
USTRUCT(BlueprintType)
struct FIronvaleArmorLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	EIronvaleArmorType ArmorType = EIronvaleArmorType::None;

	/** Damage reduction rating for this layer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor", meta = (ClampMin = "0.0"))
	float ArmorRating = 0.0f;

	/** Current durability (0 = broken, provides no protection) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor", meta = (ClampMin = "0.0"))
	float CurrentDurability = 100.0f;

	/** Max durability for this piece */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor", meta = (ClampMin = "0.0"))
	float MaxDurability = 100.0f;

	/** Effective armor rating accounting for durability */
	float GetEffectiveRating() const
	{
		if (MaxDurability <= 0.0f) return 0.0f;
		float DurabilityFactor = FMath::Clamp(CurrentDurability / MaxDurability, 0.0f, 1.0f);
		return ArmorRating * DurabilityFactor;
	}
};

// =============================================================================
// CONSTANTS
// =============================================================================

namespace IronvaleConstants
{
	/** Number of directional attack zones (excluding None) */
	constexpr int32 NUM_ATTACK_DIRECTIONS = 7;

	/** Number of armor body zones */
	constexpr int32 NUM_ARMOR_ZONES = 6;

	/** Default parry window in seconds */
	constexpr float DEFAULT_PARRY_WINDOW = 0.2f;

	/** Default riposte window after successful parry, in seconds */
	constexpr float DEFAULT_RIPOSTE_WINDOW = 0.4f;

	/** Maximum inventory weight before hard-cap (cannot move) */
	constexpr float MAX_CARRY_WEIGHT = 200.0f;

	/** Soft encumbrance threshold (movement penalty starts) */
	constexpr float SOFT_ENCUMBRANCE_RATIO = 0.75f;

	/** Reputation range */
	constexpr float MIN_REPUTATION = -100.0f;
	constexpr float MAX_REPUTATION = 100.0f;

	/** Need value range */
	constexpr float NEED_MIN = 0.0f;
	constexpr float NEED_MAX = 100.0f;

	/** Game time: 1 real second = this many game-seconds (default: 1 real hour = 1 game day) */
	constexpr float DEFAULT_TIME_SCALE = 24.0f;
}
