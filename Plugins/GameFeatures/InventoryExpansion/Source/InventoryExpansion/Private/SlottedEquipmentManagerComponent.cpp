// Copyright AndresAdarve. All Rights Reserved.

#include "SlottedEquipmentManagerComponent.h"

#include "InventoryExpansionGameplayTags.h"
#include "InventoryFragment_EquippableTyped.h"
#include "InventoryFragment_ItemDisplay.h"
#include "LyraLogChannels.h"

#include "GameFramework/Actor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"

#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/LyraEquipmentDefinition.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryItemDefinition.h"

namespace
{
	// Resolves a fragment off an item without calling ULyraInventoryItemInstance::FindFragmentByClass,
	// which is not exported from the LyraGame module. GetItemDef() is inline and the definition's Fragments
	// array is public, so we iterate the definition CDO directly (same pattern as VisualInventoryComponent).
	template <typename FragmentT>
	const FragmentT* FindFragment(const ULyraInventoryItemInstance* Instance)
	{
		if (!Instance)
		{
			return nullptr;
		}

		const TSubclassOf<ULyraInventoryItemDefinition> ItemDef = Instance->GetItemDef();
		if (!ItemDef)
		{
			return nullptr;
		}

		const ULyraInventoryItemDefinition* DefCDO = GetDefault<ULyraInventoryItemDefinition>(ItemDef);
		if (!DefCDO)
		{
			return nullptr;
		}

		for (const ULyraInventoryItemFragment* Fragment : DefCDO->Fragments)
		{
			if (const FragmentT* Typed = Cast<FragmentT>(Fragment))
			{
				return Typed;
			}
		}

		return nullptr;
	}
}

USlottedEquipmentManagerComponent::USlottedEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Replication is enabled by the base (SetIsReplicatedByDefault(true)); nothing extra needed here.
}

void USlottedEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, SlottedEquipment);
}

// const because UHT forces it (base class is UCLASS(..., Const)); the work is genuinely mutating.
void USlottedEquipmentManagerComponent::RequestEquipToSlot_Implementation(ULyraInventoryItemInstance* Item) const
{
	const_cast<USlottedEquipmentManagerComponent*>(this)->EquipToSlot_Authority(Item);
}

void USlottedEquipmentManagerComponent::RequestUnequipSlot_Implementation(FGameplayTag SlotTag) const
{
	const_cast<USlottedEquipmentManagerComponent*>(this)->UnequipSlot_Authority(SlotTag);
}

void USlottedEquipmentManagerComponent::RequestEquipToSlotIfFree_Implementation(ULyraInventoryItemInstance* Item) const
{
	const_cast<USlottedEquipmentManagerComponent*>(this)->EquipToSlot_Authority(Item, /*bOnlyIfSlotFree=*/true);
}

void USlottedEquipmentManagerComponent::EquipToSlot_Authority(ULyraInventoryItemInstance* Item, bool bOnlyIfSlotFree)
{
	const AActor* Owner = GetOwner();
	if (!Item || !Owner || !Owner->HasAuthority())
	{
		return;
	}

	// The item declares its body slot via the display fragment. No slot tag => not body-equippable
	// (e.g. weapons, which the QuickBar handles).
	const UInventoryFragment_ItemDisplay* Display = FindFragment<UInventoryFragment_ItemDisplay>(Item);
	if (!Display || !Display->EquipmentSlot.IsValid())
	{
		UE_LOG(LogLyra, Warning, TEXT("SlottedEquipment: item '%s' has no valid Equipment.Slot tag; not body-equippable."), *GetNameSafe(Item));
		return;
	}
	const FGameplayTag SlotTag = Display->EquipmentSlot;

	// Defensive: weapons declare the Equipment.Slot.Weapon category but are equipped exclusively through
	// Lyra's ULyraQuickBarComponent. This body-slot component must never equip them (it would pull the
	// weapon out of the QuickBar flow), so reject anything under the weapon branch.
	if (SlotTag.MatchesTag(InventoryExpansionTags::Equipment_Slot_Weapon))
	{
		UE_LOG(LogLyra, Warning, TEXT("SlottedEquipment: item '%s' is a weapon (slot '%s'); weapons are handled by the QuickBar, not the slotted component."),
			*GetNameSafe(Item), *SlotTag.ToString());
		return;
	}

	// The equipment to spawn comes from the plugin's typed equippable fragment (cosmetics carry this one;
	// weapons keep Lyra's UInventoryFragment_EquippableItem and equip through the QuickBar instead).
	const UInventoryFragment_EquippableTyped* Equippable = FindFragment<UInventoryFragment_EquippableTyped>(Item);
	if (!Equippable || Equippable->EquipmentDefinition == nullptr)
	{
		UE_LOG(LogLyra, Warning, TEXT("SlottedEquipment: item '%s' targets slot '%s' but has no EquippableTyped/EquipmentDefinition."),
			*GetNameSafe(Item), *SlotTag.ToString());
		return;
	}

	// "Equip only if free" mode (e.g. equip-on-pickup): leave an occupied slot untouched and bail out.
	if (bOnlyIfSlotFree && IsSlotOccupied(SlotTag))
	{
		return;
	}

	// One item per slot: free the slot first. The previous occupant stays in the inventory (state model).
	UnequipSlot_Authority(SlotTag);

	ULyraEquipmentInstance* NewInstance = EquipItem(Equippable->EquipmentDefinition);
	if (!NewInstance)
	{
		return;
	}

	// Mirror ULyraQuickBarComponent: the source inventory item is the instigator, so the read-only
	// UVisualInventoryComponent can mark this item equipped (BuildEquippedMap maps by instigator).
	NewInstance->SetInstigator(Item);

	FSlottedEquipmentEntry& Entry = SlottedEquipment.AddDefaulted_GetRef();
	Entry.SlotTag = SlotTag;
	Entry.Instance = NewInstance;
	Entry.SourceItem = Item;

	// Authority-side notification (OnRep handles remote clients). Keep ClientKnownSlots in sync so a
	// listen-server host doesn't double-fire if OnRep ever runs locally.
	ClientKnownSlots = SlottedEquipment;
	BroadcastSlotChange(SlotTag, Item, /*bEquipped=*/true);
}

void USlottedEquipmentManagerComponent::UnequipSlot_Authority(FGameplayTag SlotTag)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const int32 Index = FindSlotIndex(SlotTag);
	if (Index == INDEX_NONE)
	{
		return;
	}

	ULyraEquipmentInstance* Instance = SlottedEquipment[Index].Instance;
	ULyraInventoryItemInstance* SourceItem = SlottedEquipment[Index].SourceItem;
	SlottedEquipment.RemoveAt(Index);

	if (Instance)
	{
		// Destroys equipment actors / removes granted abilities. The backing inventory item is untouched.
		UnequipItem(Instance);
	}

	ClientKnownSlots = SlottedEquipment;
	BroadcastSlotChange(SlotTag, SourceItem, /*bEquipped=*/false);
}

int32 USlottedEquipmentManagerComponent::FindSlotIndex(const FGameplayTag& SlotTag) const
{
	for (int32 Index = 0; Index < SlottedEquipment.Num(); ++Index)
	{
		if (SlottedEquipment[Index].SlotTag.MatchesTagExact(SlotTag))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

ULyraEquipmentInstance* USlottedEquipmentManagerComponent::GetInstanceInSlot(FGameplayTag SlotTag) const
{
	const int32 Index = FindSlotIndex(SlotTag);
	return (Index != INDEX_NONE) ? SlottedEquipment[Index].Instance : nullptr;
}

bool USlottedEquipmentManagerComponent::IsSlotOccupied(FGameplayTag SlotTag) const
{
	return FindSlotIndex(SlotTag) != INDEX_NONE;
}

void USlottedEquipmentManagerComponent::OnRep_SlottedEquipment()
{
	// Entries present last time but gone now -> unequipped (by SlotTag + Instance, so a slot swap fires both).
	for (const FSlottedEquipmentEntry& Old : ClientKnownSlots)
	{
		const bool bStillPresent = SlottedEquipment.ContainsByPredicate([&Old](const FSlottedEquipmentEntry& E)
		{
			return E.SlotTag.MatchesTagExact(Old.SlotTag) && E.Instance == Old.Instance;
		});

		if (!bStillPresent)
		{
			BroadcastSlotChange(Old.SlotTag, Old.SourceItem, /*bEquipped=*/false);
		}
	}

	// Entries present now but not last time -> equipped.
	for (const FSlottedEquipmentEntry& Cur : SlottedEquipment)
	{
		const bool bWasKnown = ClientKnownSlots.ContainsByPredicate([&Cur](const FSlottedEquipmentEntry& E)
		{
			return E.SlotTag.MatchesTagExact(Cur.SlotTag) && E.Instance == Cur.Instance;
		});

		if (!bWasKnown)
		{
			BroadcastSlotChange(Cur.SlotTag, Cur.SourceItem, /*bEquipped=*/true);
		}
	}

	ClientKnownSlots = SlottedEquipment;
}

void USlottedEquipmentManagerComponent::BroadcastSlotChange(const FGameplayTag& SlotTag, ULyraInventoryItemInstance* Item, bool bEquipped) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FSlottedEquipmentChangedMessage Message;
	Message.OwnerActor = GetOwner();
	Message.SlotTag = SlotTag;
	Message.Item = Item;
	Message.bEquipped = bEquipped;

	UGameplayMessageSubsystem::Get(World).BroadcastMessage(InventoryExpansionTags::Equipment_Message_SlotChanged, Message);
}
