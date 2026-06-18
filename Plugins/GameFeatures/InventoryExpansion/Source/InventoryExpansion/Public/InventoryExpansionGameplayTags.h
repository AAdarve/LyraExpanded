// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * Native gameplay tags owned by the InventoryExpansion plugin.
 *
 * The Equipment.Slot.* hierarchy identifies equipment slots of the player paperdoll. Two flavours:
 *
 *  - Body slots (Head/Torso/Legs/Feet): leaf tags that are BOTH the item's category (authored on
 *    UInventoryFragment_ItemDisplay::EquipmentSlot) AND its occupant slot - one item per slot, governed
 *    by USlottedEquipmentManagerComponent (matched exact).
 *
 *  - Weapon slots: here "category" and "occupant" differ. Equipment.Slot.Weapon is the *category* a
 *    weapon item authors on its fragment (it tells the UI/drag-drop "this is a weapon"); weapons are
 *    still equipped exclusively through Lyra's ULyraQuickBarComponent, NOT the slotted component.
 *    Equipment.Slot.Weapon.{1,2,3} are the *occupant* slots, mapped to QuickBar index 0/1/2, and are
 *    what the equip/unequip delegates report so per-weapon widgets know which cell changed.
 *
 * An invalid/empty FGameplayTag on a fragment means "not equippable".
 */
namespace InventoryExpansionTags
{
	// Body slots: category == occupant (one item each).
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Head);
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Torso);
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Legs);
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Feet);

	// Weapon category (authored on weapon fragments) + the three occupant slots (QuickBar index 0/1/2).
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Weapon);
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Weapon_1);
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Weapon_2);
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_Weapon_3);

	// GameplayMessageSubsystem channel: a body slot was equipped/unequipped (see FSlottedEquipmentChangedMessage).
	// Kept outside the Equipment.Slot.* branch so it never pollutes the slot tag picker.
	INVENTORYEXPANSION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Message_SlotChanged);
}
