// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "EquipmentComponent.h"

#include "EquipmentExpansionComponent.generated.h"

/**
 * Blueprintable subclass of the Narrative Equipment component.
 *
 * The base UEquipmentComponent is exposed to Blueprints only via
 * meta=(BlueprintSpawnableComponent) (add-to-actor) but is NOT marked Blueprintable,
 * so it cannot be picked as a parent class for a Blueprint. This thin subclass adds
 * the Blueprintable specifier while staying IS-A UEquipmentComponent, so the
 * Narrative equippable items (which Cast to the exact base type) keep working.
 *
 * It is intended as the prototyping surface for mirroring Lyra's equipped state into
 * the Narrative inventory view.
 */
UCLASS(Blueprintable, ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class INVENTORYEXPANSION_API UEquipmentExpansionComponent : public UEquipmentComponent
{
	GENERATED_BODY()
};
