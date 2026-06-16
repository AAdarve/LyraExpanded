// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "InventoryComponent.h"

#include "InventoryExpansionComponent.generated.h"

/**
 * Blueprintable subclass of the Narrative Inventory component.
 *
 * The base UNarrativeInventoryComponent is exposed to Blueprints only via
 * meta=(BlueprintSpawnableComponent) (add-to-actor) but is NOT marked Blueprintable,
 * so it cannot be picked as a parent class for a Blueprint. This thin subclass adds
 * the Blueprintable specifier while staying IS-A UNarrativeInventoryComponent, so the
 * Narrative items/widgets (which are coupled to the exact base type) keep working.
 *
 * It is intended as the prototyping surface for the Lyra -> Narrative inventory bridge.
 */
UCLASS(Blueprintable, ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class INVENTORYEXPANSION_API UInventoryExpansionComponent : public UNarrativeInventoryComponent
{
	GENERATED_BODY()
};
