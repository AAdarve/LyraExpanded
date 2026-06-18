// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Equipment/LyraEquipmentManagerComponent.h"
#include "GameplayTagContainer.h"

#include "SlottedEquipmentManagerComponent.generated.h"

class AActor;
class ULyraInventoryItemInstance;
class ULyraEquipmentInstance;

/**
 * One body-equipment slot occupancy: which equipment instance currently fills a given slot tag,
 * plus the inventory item it was equipped from (cached so the equipped/unequipped notification can
 * be rebuilt on clients without depending on the equipment instance's instigator being resolved yet).
 */
USTRUCT()
struct FSlottedEquipmentEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag SlotTag;

	UPROPERTY()
	TObjectPtr<ULyraEquipmentInstance> Instance = nullptr;

	UPROPERTY()
	TObjectPtr<ULyraInventoryItemInstance> SourceItem = nullptr;
};

/**
 * GameplayMessageSubsystem payload broadcast (on the channel InventoryExpansionTags::Equipment_Message_SlotChanged)
 * whenever a body slot is equipped or unequipped. Fires on the authority and, via OnRep, on each owning client.
 * The read-only UVisualInventoryComponent listens and re-broadcasts its Blueprint-assignable delegates.
 */
USTRUCT(BlueprintType)
struct FSlottedEquipmentChangedMessage
{
	GENERATED_BODY()

	// Pawn that owns the slotted equipment manager (used by listeners to filter to their own pawn).
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Slots")
	TObjectPtr<AActor> OwnerActor = nullptr;

	// The body slot that changed.
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Slots")
	FGameplayTag SlotTag;

	// The inventory item equipped into / unequipped from the slot (stays in the inventory either way).
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Slots")
	TObjectPtr<ULyraInventoryItemInstance> Item = nullptr;

	// True = the item was equipped into the slot; false = it was unequipped (slot now free).
	UPROPERTY(BlueprintReadOnly, Category = "Equipment|Slots")
	bool bEquipped = false;
};

/**
 * USlottedEquipmentManagerComponent
 *
 * Adds named body-equipment slots (Equipment.Slot.*) on top of Lyra's flat equipment list.
 * Subclasses ULyraEquipmentManagerComponent so it remains the pawn's single equipment manager:
 * the QuickBar still finds it via FindComponentByClass<ULyraEquipmentManagerComponent>() (IS-A) and
 * keeps driving weapons through the inherited EquipItem/UnequipItem. This subclass only governs
 * body slots, enforcing one item per slot: equipping into an occupied slot unequips the current
 * occupant first.
 *
 * AUTHORITY: all mutations run on the server (the equip requests are Server RPCs). Equipping does
 * NOT move the item out of the inventory - it spawns a ULyraEquipmentInstance whose instigator is
 * the source inventory item (mirroring ULyraQuickBarComponent), so the read-only
 * UVisualInventoryComponent can mark it equipped without this component having any UI authority.
 */
UCLASS(Blueprintable, ClassGroup = (Inventory), meta = (BlueprintSpawnableComponent))
class INVENTORYEXPANSION_API USlottedEquipmentManagerComponent : public ULyraEquipmentManagerComponent
{
	GENERATED_BODY()

public:
	USlottedEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Note: GetLifetimeReplicatedProps is auto-declared by UHT (this class has a replicated property);
	// we only provide the definition in the .cpp, matching ULyraEquipmentManagerComponent.

	// NOTE: these requests are declared 'const' only because ULyraEquipmentManagerComponent is a
	// UCLASS(..., Const), which makes UHT generate every UFUNCTION (including RPC stubs) as const.
	// They genuinely mutate state; the _Implementation const-casts to run the authority logic.

	/**
	 * Equip an inventory item into the body slot declared by its UInventoryFragment_ItemDisplay::EquipmentSlot.
	 * If that slot is already occupied, the current occupant is unequipped first (it stays in the inventory).
	 * No-op for items without a valid Equipment.Slot tag (e.g. weapons - those go through the QuickBar).
	 * Server-authoritative: safe to call from clients (routes to the server).
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment|Slots")
	void RequestEquipToSlot(ULyraInventoryItemInstance* Item) const;

	/**
	 * Unequip whatever occupies the given body slot. The backing item remains in the inventory.
	 * Server-authoritative: safe to call from clients (routes to the server).
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment|Slots", meta = (GameplayTagFilter = "Equipment.Slot"))
	void RequestUnequipSlot(FGameplayTag SlotTag) const;

	/** The equipment instance currently occupying a body slot, or null. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment|Slots", meta = (GameplayTagFilter = "Equipment.Slot"))
	ULyraEquipmentInstance* GetInstanceInSlot(FGameplayTag SlotTag) const;

	/** Whether a body slot is currently occupied. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment|Slots", meta = (GameplayTagFilter = "Equipment.Slot"))
	bool IsSlotOccupied(FGameplayTag SlotTag) const;

private:
	/** Authority-side equip/unequip; assumes HasAuthority(). */
	void EquipToSlot_Authority(ULyraInventoryItemInstance* Item);
	void UnequipSlot_Authority(FGameplayTag SlotTag);

	/** Index into SlottedEquipment for a slot tag (exact match), or INDEX_NONE. */
	int32 FindSlotIndex(const FGameplayTag& SlotTag) const;

	/** Broadcasts FSlottedEquipmentChangedMessage on the GameplayMessageSubsystem (server + each client). */
	void BroadcastSlotChange(const FGameplayTag& SlotTag, ULyraInventoryItemInstance* Item, bool bEquipped) const;

	/** Client-side: diffs the replicated array against the last-seen state and broadcasts per add/remove. */
	UFUNCTION()
	void OnRep_SlottedEquipment();

	/** Replicated occupancy of the body slots (references instances the base already replicates as subobjects). */
	UPROPERTY(ReplicatedUsing = OnRep_SlottedEquipment)
	TArray<FSlottedEquipmentEntry> SlottedEquipment;

	/** Last-seen SlottedEquipment on this client, used by OnRep to compute what was added/removed. */
	UPROPERTY(Transient)
	TArray<FSlottedEquipmentEntry> ClientKnownSlots;
};
