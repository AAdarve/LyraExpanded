// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Inventory/LyraInventoryItemDefinition.h"
#include "Templates/SubclassOf.h"

#include "InventoryFragment_EquippableBody.generated.h"

class ULyraEquipmentDefinition;

/**
 * UInventoryFragment_EquippableBody
 *
 *	Marks an item as a BODY COSMETIC: equipment that grants abilities/effects (and a cosmetic mesh) through a
 *	tag-based body slot, equipped by USlottedEquipmentManagerComponent. This fragment is carried EXCLUSIVELY by
 *	body cosmetics; weapons must NOT carry it.
 *
 *	It deliberately does NOT derive from Lyra's UInventoryFragment_EquippableItem. Staying off that type is what
 *	keeps cosmetics out of Lyra's weapon paths for free: ULyraQuickBarComponent looks up
 *	UInventoryFragment_EquippableItem via FindFragmentByClass and will never match this type.
 *
 *	Conversely, weapons keep Lyra's UInventoryFragment_EquippableItem so the QuickBar equips them (mesh,
 *	animations, reticle) unchanged. A weapon carrying only a plugin fragment would never be equipped by the
 *	QuickBar - that was the bug this fragment's predecessor caused.
 *
 *	The body SLOT TAG itself is not stored here: it lives on UInventoryFragment_ItemDisplay::EquipmentSlot,
 *	which both USlottedEquipmentManagerComponent and the UI already read. This fragment only carries the
 *	equipment definition to spawn.
 */
UCLASS(meta=(DisplayName="Equippable (Body)"))
class INVENTORYEXPANSION_API UInventoryFragment_EquippableBody : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:

	// Equipment spawned/granted (abilities + effects + cosmetic mesh) when this item is equipped into its body slot.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition;
};
