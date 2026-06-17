// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"

#include "VisualInventoryComponent.generated.h"

class ULyraInventoryItemInstance;
class ULyraInventoryManagerComponent;
class ULyraEquipmentInstance;
class ULyraEquipmentManagerComponent;
class UInventoryFragment_ItemDisplay;

/**
 * One UI row describing a single Lyra inventory item.
 *
 * Thin view-model: it does NOT copy the item's visual fields - it carries a pointer to the
 * static display fragment (read visual data through Display->...) plus the runtime extras the
 * UI needs (current stack count, equipped-state). Built on demand by UVisualInventoryComponent.
 */
USTRUCT(BlueprintType)
struct FVisualInventoryItem
{
	GENERATED_BODY()

	// The live Lyra item this row represents (identity; use for drop/equip writes back into Lyra).
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Display")
	TObjectPtr<ULyraInventoryItemInstance> Instance = nullptr;

	// Static display data, read straight off the item definition's fragment (Display->Thumbnail, etc.).
	// Safe as a pointer: the fragment lives on the item definition CDO and stays loaded.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Display")
	TObjectPtr<UInventoryFragment_ItemDisplay> Display = nullptr;

	// Runtime quantity of this stack, read from Lyra's inventory entry.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Display")
	int32 StackCount = 0;

	// Runtime: is this exact item instance currently equipped?
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Display")
	bool bEquipped = false;

	// The equipment instance backing it when bEquipped (null otherwise).
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Display")
	TObjectPtr<ULyraEquipmentInstance> EquipmentInstance = nullptr;
};

/**
 * Controller component that produces a single, UI-ready view of the player's Lyra inventory + equipment.
 *
 * Lives on the controller (alongside ULyraInventoryManagerComponent), so it survives pawn re-possession.
 *
 * Lyra is the source of truth. This component owns no state of its own and replicates nothing:
 * it reads the controller's ULyraInventoryManagerComponent and the possessed pawn's ULyraEquipmentManagerComponent
 * on demand, pulls the UInventoryFragment_ItemDisplay off each item, marks equipped items, and returns a
 * finished list. The widgets call GetDisplayItems() when they need data and never touch the managers directly.
 *
 * PULL-ONLY: nothing here subscribes to messages, binds delegates, or ticks. Every call recomputes
 * fresh from current Lyra state.
 */
UCLASS(Blueprintable, ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class INVENTORYEXPANSION_API UVisualInventoryComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	/**
	 * Builds the unified inventory view on demand. One row per Lyra inventory entry that has an
	 * UInventoryFragment_ItemDisplay and is flagged bShowInInventory, each marked with its current
	 * stack count and equipped-state. Returns empty if the managers aren't available.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Display")
	TArray<FVisualInventoryItem> GetDisplayItems() const;

	// --- Optional display helpers (pure, quantity-aware), ported from UNarrativeItem ---

	// Description with {Variable} tokens substituted from the item's display data + current quantity.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Display")
	static FText GetParsedDescription(const FVisualInventoryItem& Item);

	// Total weight of the stack (per-unit weight * quantity).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Display")
	static float GetStackWeight(const FVisualInventoryItem& Item);

	// Remaining capacity in this stack.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Display")
	static int32 GetStackSpace(const FVisualInventoryItem& Item);

	// Whether the stack is at its maximum size.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Display")
	static bool IsStackFull(const FVisualInventoryItem& Item);

private:
	/** Resolves the inventory manager from the controller and the equipment manager from the pawn. Returns false if either is missing. */
	bool ResolveManagers(ULyraInventoryManagerComponent*& OutInventory, ULyraEquipmentManagerComponent*& OutEquipment) const;

	/** Maps each currently-equipped inventory item instance to the equipment instance backing it. */
	void BuildEquippedMap(const ULyraEquipmentManagerComponent* Equipment, TMap<ULyraInventoryItemInstance*, ULyraEquipmentInstance*>& OutEquipped) const;

	/**
	 * Reads each entry's private StackCount via UObject reflection over the manager's replicated
	 * InventoryList UPROPERTY (no Lyra source edit). Degrades gracefully (logs + leaves entries
	 * unmapped, callers fall back to 1) if Epic ever renames the underlying properties.
	 */
	void BuildStackCountMap(const ULyraInventoryManagerComponent* Inventory, TMap<ULyraInventoryItemInstance*, int32>& OutCounts) const;

	/** Resolves a {Variable} token against the item's display data + current quantity. */
	static FString GetItemStringVariable(const UInventoryFragment_ItemDisplay* Display, int32 Quantity, const FString& VariableName);
};
