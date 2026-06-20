// Copyright AndresAdarve. All Rights Reserved.

#include "InventoryExpansionGameplayTags.h"

namespace InventoryExpansionTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Head, "Equipment.Slot.Head", "Body equipment slot: head/helmet.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Torso, "Equipment.Slot.Torso", "Body equipment slot: torso/chest.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Legs, "Equipment.Slot.Legs", "Body equipment slot: legs/pants.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Feet, "Equipment.Slot.Feet", "Body equipment slot: feet/boots.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Weapon, "Equipment.Slot.Weapon", "Weapon category (authored on weapon item fragments). Weapons equip via the QuickBar, not the slotted component.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Weapon_1, "Equipment.Slot.Weapon.1", "Weapon occupant slot 1 (QuickBar index 0).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Weapon_2, "Equipment.Slot.Weapon.2", "Weapon occupant slot 2 (QuickBar index 1).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Weapon_3, "Equipment.Slot.Weapon.3", "Weapon occupant slot 3 (QuickBar index 2).");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Message_SlotChanged, "Equipment.Message.SlotChanged", "Broadcast when a body equipment slot is equipped/unequipped.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InventoryGame_Attribute_MovementSpeed, "InventoryGame.Attribute.MovementSpeed", "Movement speed attribute of the InventoryGame experience (UInventoryExpansionAttributeSet::MovementSpeed).");
}
