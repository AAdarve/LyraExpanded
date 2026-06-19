// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "VisualInventoryComponent.generated.h"

class ULyraInventoryItemInstance;
class ULyraInventoryManagerComponent;
class ULyraEquipmentInstance;
class ULyraEquipmentManagerComponent;
class ULyraQuickBarComponent;
class UInventoryFragment_ItemDisplay;
struct FSlottedEquipmentChangedMessage;
struct FLyraQuickBarSlotsChangedMessage;

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

	// Body-equipment slot this item declares (from UInventoryFragment_ItemDisplay::EquipmentSlot).
	// Invalid/empty if the item is not body-equippable (e.g. weapons - see QuickBarSlotIndex).
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Display")
	FGameplayTag SlotTag;

	// QuickBar slot index this row occupies, or -1 if it is not in the QuickBar (only filled by GetQuickBarItems).
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Display")
	int32 QuickBarSlotIndex = -1;
};

/** Fired when a body slot is equipped/unequipped; carries the slot tag and the affected item row. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlottedEquipmentChangedSignature, FGameplayTag, SlotTag, FVisualInventoryItem, Item);

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
 * READ-ONLY: it never equips or unequips. The one push it does is convenience-only: it listens to the
 * USlottedEquipmentManagerComponent's GameplayMessage and re-broadcasts OnItemEquippedToSlot /
 * OnItemUnequippedFromSlot so per-slot widgets can refresh without polling. It still owns no authority.
 */
UCLASS(Blueprintable, ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class INVENTORYEXPANSION_API UVisualInventoryComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	// --- Push notifications (bind these on per-slot widgets to refresh on equip/unequip) ---

	/** An item was equipped into a body slot. Item is the freshly equipped row (use SlotTag to target the widget). */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Display")
	FOnSlottedEquipmentChangedSignature OnItemEquippedToSlot;

	/** An item was unequipped from a body slot. Item is the row that was removed (slot is now free). */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Display")
	FOnSlottedEquipmentChangedSignature OnItemUnequippedFromSlot;

	/**
	 * Builds the unified inventory view on demand. One row per Lyra inventory entry that has an
	 * UInventoryFragment_ItemDisplay and is flagged bShowInInventory, each marked with its current
	 * stack count and equipped-state. Returns empty if the managers aren't available.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Display")
	TArray<FVisualInventoryItem> GetDisplayItems() const;

	// --- Slot / QuickBar reads (read-only; this component never equips or unequips) ---

	/**
	 * The equipped item currently filling the given body slot, or an empty row if the slot is free.
	 * Filters GetDisplayItems() by bEquipped + exact slot-tag match. Decoupled from
	 * USlottedEquipmentManagerComponent (which guarantees one item per slot), so the bridge stays read-only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Display", meta = (GameplayTagFilter = "Equipment.Slot"))
	FVisualInventoryItem GetEquippedItemForSlot(FGameplayTag InSlot) const;

	/** One row per occupied QuickBar slot (weapons), each tagged with its QuickBarSlotIndex. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Display")
	TArray<FVisualInventoryItem> GetQuickBarItems() const;

	/** Active QuickBar slot index, or -1 if there is no QuickBar / nothing active. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Display")
	int32 GetActiveQuickBarIndex() const;

	// --- Weapon occupant-slot tag mapping (QuickBar index <-> Equipment.Slot.Weapon.N) ---

	/** The weapon occupant-slot tag (Equipment.Slot.Weapon.{1,2,3}) for a QuickBar index, or an invalid tag if out of range. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Display")
	static FGameplayTag WeaponSlotTagForIndex(int32 QuickBarIndex);

	/** The QuickBar index (0/1/2) for a weapon occupant-slot tag, or -1 if the tag is not a weapon slot. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Display", meta = (GameplayTagFilter = "Equipment.Slot.Weapon"))
	static int32 IndexForWeaponSlotTag(FGameplayTag WeaponSlotTag);

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
	 * Gathers every item instance occupying a QuickBar slot. Lyra only spawns a ULyraEquipmentInstance for the
	 * *active* slot (see ULyraQuickBarComponent::SetActiveSlotIndex), so weapons holstered in non-active slots
	 * never show up in BuildEquippedMap. From the player's point of view a weapon in a QuickBar slot is equipped
	 * regardless of which one is currently drawn, so callers OR this set into the equipped-state.
	 */
	void BuildQuickBarSet(TSet<ULyraInventoryItemInstance*>& OutItems) const;

	/**
	 * Reads each entry's private StackCount via UObject reflection over the manager's replicated
	 * InventoryList UPROPERTY (no Lyra source edit). Degrades gracefully (logs + leaves entries
	 * unmapped, callers fall back to 1) if Epic ever renames the underlying properties.
	 */
	void BuildStackCountMap(const ULyraInventoryManagerComponent* Inventory, TMap<ULyraInventoryItemInstance*, int32>& OutCounts) const;

	/** Resolves a {Variable} token against the item's display data + current quantity. */
	static FString GetItemStringVariable(const UInventoryFragment_ItemDisplay* Display, int32 Quantity, const FString& VariableName);

	/** Builds a single display row for one item (resolves managers + runtime maps). Empty row if unavailable. */
	FVisualInventoryItem BuildRowForInstance(ULyraInventoryItemInstance* Instance) const;

	/** GameplayMessage handler: re-broadcasts the equip/unequip delegates for this controller's own pawn. */
	void HandleSlotEquipmentChanged(FGameplayTag Channel, const FSlottedEquipmentChangedMessage& Message);

	/**
	 * GameplayMessage handler for Lyra.QuickBar.Message.SlotsChanged: diffs the weapon slots against the
	 * last-seen set and re-broadcasts OnItemEquippedToSlot / OnItemUnequippedFromSlot with the matching
	 * Equipment.Slot.Weapon.N occupant tag, so weapon widgets refresh through the same delegates as body slots.
	 */
	void HandleQuickBarSlotsChanged(FGameplayTag Channel, const FLyraQuickBarSlotsChangedMessage& Message);

	/** Listener handle for the Equipment.Message.SlotChanged channel (registered in BeginPlay). */
	FGameplayMessageListenerHandle SlotChangedListenerHandle;

	/** Listener handle for the Lyra.QuickBar.Message.SlotsChanged channel (registered in BeginPlay). */
	FGameplayMessageListenerHandle QuickBarSlotsListenerHandle;

	/** Last-seen QuickBar weapon slots on this controller, used by HandleQuickBarSlotsChanged to diff. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULyraInventoryItemInstance>> KnownQuickBarSlots;
};
