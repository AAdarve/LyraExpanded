// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Inventory/LyraInventoryItemDefinition.h"
#include "Templates/SubclassOf.h"

#include "InventoryFragment_EquippableTyped.generated.h"

class ULyraEquipmentDefinition;

/**
 * How an equippable item is meant to be equipped. Drives equip routing so weapons and body cosmetics
 * never share the same equip path (Lyra's QuickBar is weapon-only).
 */
UENUM(BlueprintType)
enum class EInventoryExpansionEquippableKind : uint8
{
	// Equips through Lyra's ULyraQuickBarComponent (active-slot weapon flow).
	Weapon   UMETA(DisplayName="Weapon"),

	// Equips through USlottedEquipmentManagerComponent (tag-based body slot).
	Cosmetic UMETA(DisplayName="Cosmetic"),
};

/**
 * UInventoryFragment_EquippableTyped
 *
 *	Plugin-owned equippable fragment that mirrors Lyra's UInventoryFragment_EquippableItem (it carries the
 *	same EquipmentDefinition) but adds an explicit Kind. It deliberately does NOT derive from Lyra's
 *	fragment: a standalone fragment keeps cosmetics OUT of Lyra's weapon paths for free, because the
 *	QuickBar/pickup look up UInventoryFragment_EquippableItem (FindFragmentByClass) and will not match this
 *	type. Body cosmetics carry this fragment; USlottedEquipmentManagerComponent reads its EquipmentDefinition.
 *	Weapons keep Lyra's UInventoryFragment_EquippableItem so the QuickBar still equips them unchanged.
 *
 *	Default is Cosmetic on purpose: the QuickBar path is the one that misbehaves with non-weapons, so an
 *	un-set field keeps an item on the safe (slotted) side.
 */
UCLASS(meta=(DisplayName="Equippable (Typed)"))
class INVENTORYEXPANSION_API UInventoryFragment_EquippableTyped : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:

	// Equipment spawned/granted when this item is equipped (same role as Lyra's EquippableItem fragment).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	TSubclassOf<ULyraEquipmentDefinition> EquipmentDefinition;

	// Authoring hint for which equip system owns this item: Weapon -> QuickBar, Cosmetic -> slotted body equipment.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equipment")
	EInventoryExpansionEquippableKind Kind = EInventoryExpansionEquippableKind::Cosmetic;

	UFUNCTION(BlueprintPure, Category="Equipment")
	bool IsWeapon() const { return Kind == EInventoryExpansionEquippableKind::Weapon; }
};
