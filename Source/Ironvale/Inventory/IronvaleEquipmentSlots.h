// =============================================================================
// IronvaleEquipmentSlots.h — Equipment slot definitions
// Project Ironvale
//
// Defines all equipment slots and their validation rules.
// Slots determine where gear can be worn and how it maps to visual sockets.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleEquipmentSlots.generated.h"

/**
 * Equipment slot identifiers.
 * Each slot corresponds to a body location and accepts specific item categories.
 */
UENUM(BlueprintType)
enum class EIronvaleEquipmentSlot : uint8
{
	Head      UMETA(DisplayName = "Head"),
	Torso     UMETA(DisplayName = "Torso"),
	Hands     UMETA(DisplayName = "Hands"),
	Legs      UMETA(DisplayName = "Legs"),
	Feet      UMETA(DisplayName = "Feet"),
	MainHand  UMETA(DisplayName = "Main Hand"),
	OffHand   UMETA(DisplayName = "Off Hand"),
	Ring1     UMETA(DisplayName = "Ring 1"),
	Ring2     UMETA(DisplayName = "Ring 2"),
	Cloak     UMETA(DisplayName = "Cloak"),
	MAX       UMETA(Hidden)
};

/**
 * Maps equipment slot to the skeletal mesh socket name for attachment.
 * Also defines which armor zone the slot protects (for damage calculation).
 */
USTRUCT(BlueprintType)
struct FIronvaleEquipmentSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EIronvaleEquipmentSlot Slot = EIronvaleEquipmentSlot::Torso;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName DisplayName;

	/** Skeletal mesh socket name for attaching equipped item visuals */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName AttachSocketName;

	/** Which armor zone this slot protects (for combat damage lookup) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EIronvaleArmorZone ProtectedZone = EIronvaleArmorZone::Torso;

	/** Item categories allowed in this slot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<EIronvaleItemCategory> AllowedCategories;
};

/**
 * Static helper for equipment slot metadata.
 */
namespace IronvaleEquipmentSlotHelper
{
	/** Get the default slot info table. Call once at init, cache the result. */
	inline TArray<FIronvaleEquipmentSlotInfo> GetDefaultSlotInfos()
	{
		TArray<FIronvaleEquipmentSlotInfo> Infos;

		auto AddSlot = [&](EIronvaleEquipmentSlot Slot, const FName& Name, const FName& Socket,
			EIronvaleArmorZone Zone, TArray<EIronvaleItemCategory> Categories)
		{
			FIronvaleEquipmentSlotInfo Info;
			Info.Slot = Slot;
			Info.DisplayName = Name;
			Info.AttachSocketName = Socket;
			Info.ProtectedZone = Zone;
			Info.AllowedCategories = MoveTemp(Categories);
			Infos.Add(Info);
		};

		AddSlot(EIronvaleEquipmentSlot::Head, "Head", "socket_head",
			EIronvaleArmorZone::Head, { EIronvaleItemCategory::Armor });
		AddSlot(EIronvaleEquipmentSlot::Torso, "Torso", "socket_torso",
			EIronvaleArmorZone::Torso, { EIronvaleItemCategory::Armor });
		AddSlot(EIronvaleEquipmentSlot::Hands, "Hands", "socket_hands",
			EIronvaleArmorZone::LeftArm, { EIronvaleItemCategory::Armor });
		AddSlot(EIronvaleEquipmentSlot::Legs, "Legs", "socket_legs",
			EIronvaleArmorZone::Legs, { EIronvaleItemCategory::Armor });
		AddSlot(EIronvaleEquipmentSlot::Feet, "Feet", "socket_feet",
			EIronvaleArmorZone::Feet, { EIronvaleItemCategory::Armor });
		AddSlot(EIronvaleEquipmentSlot::MainHand, "Main Hand", "socket_weapon_r",
			EIronvaleArmorZone::Torso, { EIronvaleItemCategory::Weapon });
		AddSlot(EIronvaleEquipmentSlot::OffHand, "Off Hand", "socket_weapon_l",
			EIronvaleArmorZone::Torso, { EIronvaleItemCategory::Weapon, EIronvaleItemCategory::Armor });
		AddSlot(EIronvaleEquipmentSlot::Ring1, "Ring 1", "socket_ring_r",
			EIronvaleArmorZone::Torso, { EIronvaleItemCategory::Misc });
		AddSlot(EIronvaleEquipmentSlot::Ring2, "Ring 2", "socket_ring_l",
			EIronvaleArmorZone::Torso, { EIronvaleItemCategory::Misc });
		AddSlot(EIronvaleEquipmentSlot::Cloak, "Cloak", "socket_cloak",
			EIronvaleArmorZone::Torso, { EIronvaleItemCategory::Armor });

		return Infos;
	}
}
