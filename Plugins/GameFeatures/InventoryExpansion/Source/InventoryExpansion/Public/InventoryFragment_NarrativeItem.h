// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Inventory/LyraInventoryItemDefinition.h"
#include "Templates/SubclassOf.h"

#include "InventoryFragment_NarrativeItem.generated.h"

class UNarrativeItem;

/**
 * Bridge fragment that maps a Lyra inventory item definition to its Narrative
 * Inventory equivalent.
 *
 * Lyra remains the source of truth: this fragment simply records which
 * UNarrativeItem class represents this Lyra item inside the Narrative inventory
 * view. The sync logic (Phase 3) reads this fragment off each Lyra item instance
 * and calls TryAddItemFromClass on the Narrative component so the Narrative
 * widgets can display it.
 */
UCLASS()
class INVENTORYEXPANSION_API UInventoryFragment_NarrativeItem : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:
	// The Narrative item class that mirrors this Lyra item in the Narrative inventory.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Narrative)
	TSubclassOf<UNarrativeItem> NarrativeItemClass;
};
