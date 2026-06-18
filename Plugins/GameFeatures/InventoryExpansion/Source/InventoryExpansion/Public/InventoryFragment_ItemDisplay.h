// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Inventory/LyraInventoryItemDefinition.h"
#include "GameplayTagContainer.h"

#include "InventoryFragment_ItemDisplay.generated.h"

class UTexture2D;
class USoundBase;

/**
 * A stat row shown in the item-preview panel. Decoupled local mirror of Narrative's
 * FNarrativeItemStat so this fragment has no dependency on UNarrativeItem.
 */
USTRUCT(BlueprintType)
struct FExpandedItemStat
{
	GENERATED_BODY()

	// The stat's display name.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Stat")
	FText StatDisplayName;

	// Backing variable name, resolved against the item's display data (see GetParsedDescription helpers).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Stat")
	FString StringVariable;
};

/**
 * Static visual / descriptive data for an inventory item, attached to a
 * ULyraInventoryItemDefinition. Lyra remains the single source of truth for the inventory;
 * this fragment only carries what the UI needs to render an item (thumbnail, name, description,
 * stacking rules, etc.).
 *
 * Deliberately holds NO runtime/replicated state: current quantity, equipped-state, activation
 * and replication keys are owned by Lyra's item instance / inventory and surfaced to the UI via
 * UVisualInventoryComponent::FVisualInventoryItem, not stored here.
 */
UCLASS()
class INVENTORYEXPANSION_API UInventoryFragment_ItemDisplay : public ULyraInventoryItemFragment
{
	GENERATED_BODY()

public:
	// --- Visual / descriptive ---

	// Icon shown for this item in the inventory UI.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	// Sound played when the item is used.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	TObjectPtr<USoundBase> UseSound;

	// Name shown in the inventory UI.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FText DisplayName;

	// Description shown in the item-preview panel.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (MultiLine = true))
	FText Description;

	// Per-unit weight.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (ClampMin = 0.0, Units = "Kilograms"))
	float Weight = 0.f;

	// Descriptive gameplay tags (e.g. category) used for filtering/sorting in the UI.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	FGameplayTagContainer ItemTags;

	// Base/vendor value of the item.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
	int32 BaseValue = 0;

	// --- Equipment ---

	// Body-equipment slot this item belongs to, if equippable (invalid/empty tag = not body-equippable,
	// e.g. weapons handled by the QuickBar). Read by USlottedEquipmentManagerComponent (exact match) and
	// surfaced to the UI via UVisualInventoryComponent::FVisualInventoryItem::SlotTag. Pick one of
	// InventoryExpansionTags::Equipment_Slot_*.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (Categories = "Equipment.Slot"))
	FGameplayTag EquipmentSlot;

	// --- Usage ---

	// Whether using the item consumes it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Usage")
	bool bConsumeOnUse = false;

	// Label for the use action (e.g. "Equip", "Eat").
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Usage")
	FText UseActionText;

	// Cooldown between uses, in seconds.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Usage", meta = (ClampMin = 0, Units = "Seconds"))
	float UseRechargeDuration = 0.f;

	// --- Stacking rules (current quantity comes from Lyra at runtime, not stored here) ---

	// Whether multiple units of this item can share a single stack.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stacking")
	bool bStackable = true;

	// Maximum units in a single stack when bStackable.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stacking", meta = (ClampMin = 2, EditCondition = bStackable, EditConditionHides))
	int32 MaxStackSize = 2;

	// --- UI ---

	// Whether this item should appear in the inventory list (replaces Narrative's vendor-aware ShouldShowInInventory).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	bool bShowInInventory = true;

	// Custom stats shown in the item-preview panel.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TArray<FExpandedItemStat> Stats;
};
