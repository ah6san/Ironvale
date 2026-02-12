// =============================================================================
// IronvaleGameplayTags.h — Native gameplay tag declarations
// Project Ironvale
//
// Defines gameplay tags in C++ for compile-time safety. Tags are organized
// hierarchically: System.Category.Specific (e.g., "Combat.State.Attacking").
//
// Usage: #include "Core/IronvaleGameplayTags.h"
//        FGameplayTag MyTag = IronvaleTags::Combat_State_Attacking;
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace IronvaleTags
{
	// --- Combat Tags ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_Idle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_LockedOn);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_Attacking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_Blocking);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_Parrying);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_Staggered);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_Dead);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_DamageType_Slash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_DamageType_Blunt);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_DamageType_Pierce);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_HitZone_Head);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_HitZone_Torso);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_HitZone_LeftArm);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_HitZone_RightArm);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_HitZone_Legs);

	// --- Needs / Debuff Tags ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Needs_Debuff_Hungry);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Needs_Debuff_Starving);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Needs_Debuff_Thirsty);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Needs_Debuff_Dehydrated);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Needs_Debuff_Tired);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Needs_Debuff_Exhausted);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Needs_Debuff_Dirty);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Needs_Debuff_Filthy);

	// --- Item Tags ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Type_Weapon);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Type_Armor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Type_Consumable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Type_QuestItem);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Quality_Masterwork);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Special_Unbreakable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Special_TwoHanded);

	// --- Appearance / Social Tags (for dialogue conditions) ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Appearance_NobleClothes);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Appearance_PeasantClothes);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Appearance_FullPlate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Appearance_Dirty);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Appearance_Armed);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Appearance_Hooded);

	// --- NPC Reaction Tags ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NPC_Reaction_Friendly);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NPC_Reaction_Neutral);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NPC_Reaction_Suspicious);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NPC_Reaction_Hostile);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(NPC_Reaction_Afraid);

	// --- World / Environment Tags ---
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(World_Interior);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(World_Exterior);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(World_Town);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(World_Wilderness);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(World_Dungeon);
}
