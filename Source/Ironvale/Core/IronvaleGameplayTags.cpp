// =============================================================================
// IronvaleGameplayTags.cpp — Native gameplay tag definitions
// Project Ironvale
// =============================================================================

#include "Core/IronvaleGameplayTags.h"

namespace IronvaleTags
{
	// --- Combat Tags ---
	UE_DEFINE_GAMEPLAY_TAG(Combat_State_Idle,       "Combat.State.Idle");
	UE_DEFINE_GAMEPLAY_TAG(Combat_State_LockedOn,   "Combat.State.LockedOn");
	UE_DEFINE_GAMEPLAY_TAG(Combat_State_Attacking,  "Combat.State.Attacking");
	UE_DEFINE_GAMEPLAY_TAG(Combat_State_Blocking,   "Combat.State.Blocking");
	UE_DEFINE_GAMEPLAY_TAG(Combat_State_Parrying,   "Combat.State.Parrying");
	UE_DEFINE_GAMEPLAY_TAG(Combat_State_Staggered,  "Combat.State.Staggered");
	UE_DEFINE_GAMEPLAY_TAG(Combat_State_Dead,       "Combat.State.Dead");

	UE_DEFINE_GAMEPLAY_TAG(Combat_DamageType_Slash,  "Combat.DamageType.Slash");
	UE_DEFINE_GAMEPLAY_TAG(Combat_DamageType_Blunt,  "Combat.DamageType.Blunt");
	UE_DEFINE_GAMEPLAY_TAG(Combat_DamageType_Pierce, "Combat.DamageType.Pierce");

	UE_DEFINE_GAMEPLAY_TAG(Combat_HitZone_Head,     "Combat.HitZone.Head");
	UE_DEFINE_GAMEPLAY_TAG(Combat_HitZone_Torso,    "Combat.HitZone.Torso");
	UE_DEFINE_GAMEPLAY_TAG(Combat_HitZone_LeftArm,  "Combat.HitZone.LeftArm");
	UE_DEFINE_GAMEPLAY_TAG(Combat_HitZone_RightArm, "Combat.HitZone.RightArm");
	UE_DEFINE_GAMEPLAY_TAG(Combat_HitZone_Legs,     "Combat.HitZone.Legs");

	// --- Needs / Debuff Tags ---
	UE_DEFINE_GAMEPLAY_TAG(Needs_Debuff_Hungry,     "Needs.Debuff.Hungry");
	UE_DEFINE_GAMEPLAY_TAG(Needs_Debuff_Starving,   "Needs.Debuff.Starving");
	UE_DEFINE_GAMEPLAY_TAG(Needs_Debuff_Thirsty,    "Needs.Debuff.Thirsty");
	UE_DEFINE_GAMEPLAY_TAG(Needs_Debuff_Dehydrated, "Needs.Debuff.Dehydrated");
	UE_DEFINE_GAMEPLAY_TAG(Needs_Debuff_Tired,      "Needs.Debuff.Tired");
	UE_DEFINE_GAMEPLAY_TAG(Needs_Debuff_Exhausted,  "Needs.Debuff.Exhausted");
	UE_DEFINE_GAMEPLAY_TAG(Needs_Debuff_Dirty,      "Needs.Debuff.Dirty");
	UE_DEFINE_GAMEPLAY_TAG(Needs_Debuff_Filthy,     "Needs.Debuff.Filthy");

	// --- Item Tags ---
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Weapon,          "Item.Type.Weapon");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Armor,            "Item.Type.Armor");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_Consumable,       "Item.Type.Consumable");
	UE_DEFINE_GAMEPLAY_TAG(Item_Type_QuestItem,        "Item.Type.QuestItem");
	UE_DEFINE_GAMEPLAY_TAG(Item_Quality_Masterwork,    "Item.Quality.Masterwork");
	UE_DEFINE_GAMEPLAY_TAG(Item_Special_Unbreakable,   "Item.Special.Unbreakable");
	UE_DEFINE_GAMEPLAY_TAG(Item_Special_TwoHanded,     "Item.Special.TwoHanded");

	// --- Appearance / Social Tags ---
	UE_DEFINE_GAMEPLAY_TAG(Appearance_NobleClothes,   "Appearance.NobleClothes");
	UE_DEFINE_GAMEPLAY_TAG(Appearance_PeasantClothes, "Appearance.PeasantClothes");
	UE_DEFINE_GAMEPLAY_TAG(Appearance_FullPlate,      "Appearance.FullPlate");
	UE_DEFINE_GAMEPLAY_TAG(Appearance_Dirty,          "Appearance.Dirty");
	UE_DEFINE_GAMEPLAY_TAG(Appearance_Armed,          "Appearance.Armed");
	UE_DEFINE_GAMEPLAY_TAG(Appearance_Hooded,         "Appearance.Hooded");

	// --- NPC Reaction Tags ---
	UE_DEFINE_GAMEPLAY_TAG(NPC_Reaction_Friendly,   "NPC.Reaction.Friendly");
	UE_DEFINE_GAMEPLAY_TAG(NPC_Reaction_Neutral,    "NPC.Reaction.Neutral");
	UE_DEFINE_GAMEPLAY_TAG(NPC_Reaction_Suspicious, "NPC.Reaction.Suspicious");
	UE_DEFINE_GAMEPLAY_TAG(NPC_Reaction_Hostile,    "NPC.Reaction.Hostile");
	UE_DEFINE_GAMEPLAY_TAG(NPC_Reaction_Afraid,     "NPC.Reaction.Afraid");

	// --- World / Environment Tags ---
	UE_DEFINE_GAMEPLAY_TAG(World_Interior,   "World.Interior");
	UE_DEFINE_GAMEPLAY_TAG(World_Exterior,   "World.Exterior");
	UE_DEFINE_GAMEPLAY_TAG(World_Town,       "World.Town");
	UE_DEFINE_GAMEPLAY_TAG(World_Wilderness, "World.Wilderness");
	UE_DEFINE_GAMEPLAY_TAG(World_Dungeon,    "World.Dungeon");
}
