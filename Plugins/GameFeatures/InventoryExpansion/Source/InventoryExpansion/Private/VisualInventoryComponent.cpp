// Copyright AndresAdarve. All Rights Reserved.

#include "VisualInventoryComponent.h"

#include "InventoryExpansionGameplayTags.h"
#include "InventoryFragment_ItemDisplay.h"
#include "SlottedEquipmentManagerComponent.h"
#include "LyraLogChannels.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "UObject/UnrealType.h"

#include "Inventory/LyraInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraEquipmentInstance.h"
#include "Equipment/LyraQuickBarComponent.h"
#include "Equipment/LyraPickupDefinition.h"

namespace
{
	// Resolves the display fragment off an item without calling ULyraInventoryItemInstance::FindFragmentByClass,
	// which is not exported from the LyraGame module. GetItemDef() is inline and the definition's Fragments
	// array is public, so we iterate the definition CDO directly (the same thing FindFragmentByClass does).
	const UInventoryFragment_ItemDisplay* FindDisplayFragment(const ULyraInventoryItemInstance* Instance)
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
			if (const UInventoryFragment_ItemDisplay* Display = Cast<UInventoryFragment_ItemDisplay>(Fragment))
			{
				return Display;
			}
		}

		return nullptr;
	}

	// Builds one UI row from a Lyra item plus the already-gathered runtime maps (equipped-state + stack counts).
	// Shared by GetDisplayItems (inventory list) and GetQuickBarItems (weapon slots). Display may be null
	// (e.g. a QuickBar weapon without a display fragment); the row still carries the live Instance.
	FVisualInventoryItem MakeRow(
		ULyraInventoryItemInstance* Instance,
		const UInventoryFragment_ItemDisplay* Display,
		const TMap<ULyraInventoryItemInstance*, ULyraEquipmentInstance*>& EquippedMap,
		const TMap<ULyraInventoryItemInstance*, int32>& QuickBarIndexMap,
		const TMap<ULyraInventoryItemInstance*, int32>& CountMap)
	{
		FVisualInventoryItem Row;
		Row.Instance = Instance;
		Row.Display = const_cast<UInventoryFragment_ItemDisplay*>(Display);
		if (Display)
		{
			Row.SlotTag = Display->EquipmentSlot;
			Row.PickupDefinition = Display->PickupDefinition;
		}

		const int32* Count = CountMap.Find(Instance);
		Row.StackCount = Count ? *Count : 1;

		if (ULyraEquipmentInstance* const* Equip = EquippedMap.Find(Instance))
		{
			// Active weapon / body slot: a live equipment instance is attached to the pawn.
			Row.bEquipped = true;
			Row.EquipmentInstance = *Equip;
		}

		// QuickBar occupancy (active or holstered). Weapons are keyed by slot index, not by the display
		// fragment's EquipmentSlot, so stamp the index + derived Equipment.Slot.Weapon.N tag here. This runs
		// independently of the equipped block above (an active weapon is in both), so weapon rows are
		// identifiable through every build path, not just GetQuickBarItems.
		if (const int32* QbIndex = QuickBarIndexMap.Find(Instance))
		{
			Row.bEquipped = true;
			Row.QuickBarSlotIndex = *QbIndex;
			Row.SlotTag = UVisualInventoryComponent::WeaponSlotTagForIndex(*QbIndex);
		}

		return Row;
	}
}

void UVisualInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen (read-only) for body-slot equip/unequip so per-slot widgets can refresh without polling.
	if (UWorld* World = GetWorld())
	{
		UGameplayMessageSubsystem& Messaging = UGameplayMessageSubsystem::Get(World);

		SlotChangedListenerHandle = Messaging.RegisterListener(
			InventoryExpansionTags::Equipment_Message_SlotChanged, this, &UVisualInventoryComponent::HandleSlotEquipmentChanged);

		// Also listen to Lyra's QuickBar so weapon slots refresh through the same delegates. The channel tag
		// is defined static (non-exported) in LyraQuickBarComponent.cpp, so resolve it by its stable name.
		const FGameplayTag QuickBarChannel = FGameplayTag::RequestGameplayTag(FName("Lyra.QuickBar.Message.SlotsChanged"));
		QuickBarSlotsListenerHandle = Messaging.RegisterListener(
			QuickBarChannel, this, &UVisualInventoryComponent::HandleQuickBarSlotsChanged);
	}
}

void UVisualInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister() resolves the subsystem from the handle's own weak pointer (safe during teardown).
	if (SlotChangedListenerHandle.IsValid())
	{
		SlotChangedListenerHandle.Unregister();
	}
	if (QuickBarSlotsListenerHandle.IsValid())
	{
		QuickBarSlotsListenerHandle.Unregister();
	}

	Super::EndPlay(EndPlayReason);
}

void UVisualInventoryComponent::HandleSlotEquipmentChanged(FGameplayTag Channel, const FSlottedEquipmentChangedMessage& Message)
{
	// The message bus is world-global, so ignore changes that belong to a different player's pawn.
	if (Message.OwnerActor != GetPawn<APawn>())
	{
		return;
	}

	const FVisualInventoryItem Row = BuildRowForInstance(Message.Item);
	if (Message.bEquipped)
	{
		OnItemEquippedToSlot.Broadcast(Message.SlotTag, Row);
	}
	else
	{
		OnItemUnequippedFromSlot.Broadcast(Message.SlotTag, Row);
	}
}

void UVisualInventoryComponent::HandleQuickBarSlotsChanged(FGameplayTag Channel, const FLyraQuickBarSlotsChangedMessage& Message)
{
	// The QuickBar lives on the controller (like this component), so its message Owner is our controller.
	if (Message.Owner != GetOwner())
	{
		return;
	}

	// Diff per index against the last-seen weapon slots. A slot whose occupant changed fires unequip for
	// the old item and equip for the new one, each tagged with the matching Equipment.Slot.Weapon.N occupant.
	const int32 Count = FMath::Max(KnownQuickBarSlots.Num(), Message.Slots.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		ULyraInventoryItemInstance* Old = KnownQuickBarSlots.IsValidIndex(Index) ? KnownQuickBarSlots[Index].Get() : nullptr;
		ULyraInventoryItemInstance* New = Message.Slots.IsValidIndex(Index) ? Message.Slots[Index] : nullptr;
		if (Old == New)
		{
			continue;
		}

		const FGameplayTag SlotTag = WeaponSlotTagForIndex(Index);
		if (!SlotTag.IsValid())
		{
			continue; // QuickBar has more than the 3 mapped weapon slots; ignore the extras.
		}

		if (Old)
		{
			FVisualInventoryItem Row = BuildRowForInstance(Old);
			Row.QuickBarSlotIndex = Index;
			OnItemUnequippedFromSlot.Broadcast(SlotTag, Row);
		}
		if (New)
		{
			FVisualInventoryItem Row = BuildRowForInstance(New);
			Row.QuickBarSlotIndex = Index;
			OnItemEquippedToSlot.Broadcast(SlotTag, Row);
		}
	}

	KnownQuickBarSlots = Message.Slots;
}

FGameplayTag UVisualInventoryComponent::WeaponSlotTagForIndex(int32 QuickBarIndex)
{
	switch (QuickBarIndex)
	{
	case 0: return InventoryExpansionTags::Equipment_Slot_Weapon_1;
	case 1: return InventoryExpansionTags::Equipment_Slot_Weapon_2;
	case 2: return InventoryExpansionTags::Equipment_Slot_Weapon_3;
	default: return FGameplayTag();
	}
}

int32 UVisualInventoryComponent::IndexForWeaponSlotTag(FGameplayTag WeaponSlotTag)
{
	if (WeaponSlotTag.MatchesTagExact(InventoryExpansionTags::Equipment_Slot_Weapon_1)) { return 0; }
	if (WeaponSlotTag.MatchesTagExact(InventoryExpansionTags::Equipment_Slot_Weapon_2)) { return 1; }
	if (WeaponSlotTag.MatchesTagExact(InventoryExpansionTags::Equipment_Slot_Weapon_3)) { return 2; }
	return -1;
}

FVisualInventoryItem UVisualInventoryComponent::BuildRowForInstance(ULyraInventoryItemInstance* Instance) const
{
	if (!Instance)
	{
		return FVisualInventoryItem();
	}

	ULyraInventoryManagerComponent* Inventory = nullptr;
	ULyraEquipmentManagerComponent* Equipment = nullptr;
	if (!ResolveManagers(Inventory, Equipment))
	{
		return FVisualInventoryItem();
	}

	TMap<ULyraInventoryItemInstance*, ULyraEquipmentInstance*> EquippedMap;
	BuildEquippedMap(Equipment, EquippedMap);

	TMap<ULyraInventoryItemInstance*, int32> QuickBarIndexMap;
	BuildQuickBarIndexMap(QuickBarIndexMap);

	TMap<ULyraInventoryItemInstance*, int32> CountMap;
	BuildStackCountMap(Inventory, CountMap);

	return MakeRow(Instance, FindDisplayFragment(Instance), EquippedMap, QuickBarIndexMap, CountMap);
}

TArray<FVisualInventoryItem> UVisualInventoryComponent::GetDisplayItems() const
{
	TArray<FVisualInventoryItem> Results;

	ULyraInventoryManagerComponent* Inventory = nullptr;
	ULyraEquipmentManagerComponent* Equipment = nullptr;
	if (!ResolveManagers(Inventory, Equipment))
	{
		return Results;
	}

	// Runtime data, gathered once: equipped-state (from equipment + QuickBar) and stack counts (from inventory).
	TMap<ULyraInventoryItemInstance*, ULyraEquipmentInstance*> EquippedMap;
	BuildEquippedMap(Equipment, EquippedMap);

	TMap<ULyraInventoryItemInstance*, int32> QuickBarIndexMap;
	BuildQuickBarIndexMap(QuickBarIndexMap);

	TMap<ULyraInventoryItemInstance*, int32> CountMap;
	BuildStackCountMap(Inventory, CountMap);

	const TArray<ULyraInventoryItemInstance*> Items = Inventory->GetAllItems();
	Results.Reserve(Items.Num());
	for (ULyraInventoryItemInstance* Instance : Items)
	{
		if (!Instance)
		{
			continue;
		}

		const UInventoryFragment_ItemDisplay* Display = FindDisplayFragment(Instance);
		if (!Display || !Display->bShowInInventory)
		{
			continue;
		}

		Results.Add(MakeRow(Instance, Display, EquippedMap, QuickBarIndexMap, CountMap));
	}

	return Results;
}

FVisualInventoryItem UVisualInventoryComponent::GetEquippedItemForSlot(FGameplayTag InSlot) const
{
	if (!InSlot.IsValid())
	{
		return FVisualInventoryItem();
	}

	// Weapon occupant slots resolve against the QuickBar (the item in that index), so the same read path
	// serves all paperdoll cells - 4 body + 3 weapon - keyed purely by occupant tag.
	const int32 WeaponIndex = IndexForWeaponSlotTag(InSlot);
	if (WeaponIndex != -1)
	{
		const AController* OwnerController = GetController<AController>();
		ULyraQuickBarComponent* QuickBar = OwnerController ? OwnerController->FindComponentByClass<ULyraQuickBarComponent>() : nullptr;
		if (!QuickBar)
		{
			return FVisualInventoryItem();
		}

		const TArray<ULyraInventoryItemInstance*> Slots = QuickBar->GetSlots();
		ULyraInventoryItemInstance* Instance = Slots.IsValidIndex(WeaponIndex) ? Slots[WeaponIndex] : nullptr;
		if (!Instance)
		{
			return FVisualInventoryItem(); // empty weapon slot
		}

		FVisualInventoryItem Row = BuildRowForInstance(Instance);
		Row.QuickBarSlotIndex = WeaponIndex;
		return Row;
	}

	// Body slots: exclusivity is guaranteed by USlottedEquipmentManagerComponent, so at most one row matches.
	for (const FVisualInventoryItem& Row : GetDisplayItems())
	{
		if (Row.bEquipped && Row.SlotTag.MatchesTagExact(InSlot))
		{
			return Row;
		}
	}

	return FVisualInventoryItem();
}

TArray<FVisualInventoryItem> UVisualInventoryComponent::GetQuickBarItems() const
{
	TArray<FVisualInventoryItem> Results;

	ULyraInventoryManagerComponent* Inventory = nullptr;
	ULyraEquipmentManagerComponent* Equipment = nullptr;
	if (!ResolveManagers(Inventory, Equipment))
	{
		return Results;
	}

	const AController* OwnerController = GetController<AController>();
	ULyraQuickBarComponent* QuickBar = OwnerController ? OwnerController->FindComponentByClass<ULyraQuickBarComponent>() : nullptr;
	if (!QuickBar)
	{
		return Results;
	}

	TMap<ULyraInventoryItemInstance*, ULyraEquipmentInstance*> EquippedMap;
	BuildEquippedMap(Equipment, EquippedMap);

	TMap<ULyraInventoryItemInstance*, int32> CountMap;
	BuildStackCountMap(Inventory, CountMap);

	const TArray<ULyraInventoryItemInstance*> Slots = QuickBar->GetSlots();

	// Every occupied QuickBar slot counts as equipped, even the non-active ones (which have no equipment
	// instance yet). MakeRow ORs this map into bEquipped and stamps QuickBarSlotIndex + SlotTag from it.
	TMap<ULyraInventoryItemInstance*, int32> QuickBarIndexMap;
	QuickBarIndexMap.Reserve(Slots.Num());
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index])
		{
			QuickBarIndexMap.Add(Slots[Index], Index);
		}
	}

	Results.Reserve(Slots.Num());
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		ULyraInventoryItemInstance* Instance = Slots[Index];
		if (!Instance)
		{
			continue; // empty QuickBar slot
		}

		// MakeRow stamps QuickBarSlotIndex + SlotTag from QuickBarIndexMap.
		Results.Add(MakeRow(Instance, FindDisplayFragment(Instance), EquippedMap, QuickBarIndexMap, CountMap));
	}

	return Results;
}

int32 UVisualInventoryComponent::GetActiveQuickBarIndex() const
{
	const AController* OwnerController = GetController<AController>();
	if (const ULyraQuickBarComponent* QuickBar = OwnerController ? OwnerController->FindComponentByClass<ULyraQuickBarComponent>() : nullptr)
	{
		return QuickBar->GetActiveSlotIndex();
	}

	return -1;
}

bool UVisualInventoryComponent::ResolveManagers(ULyraInventoryManagerComponent*& OutInventory, ULyraEquipmentManagerComponent*& OutEquipment) const
{
	OutInventory = nullptr;
	OutEquipment = nullptr;

	// This component lives on the controller. The inventory manager is a sibling controller component
	// (inventory is a property of the player, so it survives pawn re-possession). The equipment manager
	// lives on the possessed pawn, since equipment is spawned/attached to the physical body.
	const AController* OwnerController = GetController<AController>();
	if (!OwnerController)
	{
		return false;
	}

	OutInventory = OwnerController->FindComponentByClass<ULyraInventoryManagerComponent>();

	if (const APawn* OwnerPawn = GetPawn<APawn>())
	{
		OutEquipment = OwnerPawn->FindComponentByClass<ULyraEquipmentManagerComponent>();
	}

	return (OutInventory != nullptr) && (OutEquipment != nullptr);
}

void UVisualInventoryComponent::BuildEquippedMap(const ULyraEquipmentManagerComponent* Equipment, TMap<ULyraInventoryItemInstance*, ULyraEquipmentInstance*>& OutEquipped) const
{
	if (!Equipment)
	{
		return;
	}

	const TArray<ULyraEquipmentInstance*> Equipped = Equipment->GetEquipmentInstancesOfType(ULyraEquipmentInstance::StaticClass());
	for (ULyraEquipmentInstance* Equip : Equipped)
	{
		if (!Equip)
		{
			continue;
		}

		// The inventory item that this equipment was equipped from is stored as the instigator
		// (see ULyraQuickBarComponent::EquipItemInSlot -> SetInstigator).
		if (ULyraInventoryItemInstance* Source = Cast<ULyraInventoryItemInstance>(Equip->GetInstigator()))
		{
			OutEquipped.Add(Source, Equip);
		}
	}
}

void UVisualInventoryComponent::BuildQuickBarIndexMap(TMap<ULyraInventoryItemInstance*, int32>& OutItems) const
{
	const AController* OwnerController = GetController<AController>();
	ULyraQuickBarComponent* QuickBar = OwnerController ? OwnerController->FindComponentByClass<ULyraQuickBarComponent>() : nullptr;
	if (!QuickBar)
	{
		return;
	}

	const TArray<ULyraInventoryItemInstance*> Slots = QuickBar->GetSlots();
	OutItems.Reserve(Slots.Num());
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index])
		{
			OutItems.Add(Slots[Index], Index);
		}
	}
}

void UVisualInventoryComponent::BuildStackCountMap(const ULyraInventoryManagerComponent* Inventory, TMap<ULyraInventoryItemInstance*, int32>& OutCounts) const
{
	if (!Inventory)
	{
		return;
	}

	// Lyra keeps the per-stack count in the private FLyraInventoryEntry::StackCount, friended only to the
	// manager. We must not modify Lyra, so we read it through reflection over the replicated InventoryList
	// UPROPERTY. Property handles are resolved once and cached.
	static const FStructProperty* ListProp = CastField<FStructProperty>(
		ULyraInventoryManagerComponent::StaticClass()->FindPropertyByName(TEXT("InventoryList")));
	if (!ListProp)
	{
		UE_LOG(LogLyra, Warning, TEXT("VisualInventoryComponent: could not reflect ULyraInventoryManagerComponent::InventoryList; stack counts default to 1."));
		return;
	}

	static const FArrayProperty* EntriesProp = CastField<FArrayProperty>(ListProp->Struct->FindPropertyByName(TEXT("Entries")));
	static const FStructProperty* EntryStructProp = EntriesProp ? CastField<FStructProperty>(EntriesProp->Inner) : nullptr;
	static const FObjectProperty* InstProp = EntryStructProp ? CastField<FObjectProperty>(EntryStructProp->Struct->FindPropertyByName(TEXT("Instance"))) : nullptr;
	static const FIntProperty* CountProp = EntryStructProp ? CastField<FIntProperty>(EntryStructProp->Struct->FindPropertyByName(TEXT("StackCount"))) : nullptr;

	if (!EntriesProp || !EntryStructProp || !InstProp || !CountProp)
	{
		UE_LOG(LogLyra, Warning, TEXT("VisualInventoryComponent: could not reflect FLyraInventoryEntry layout; stack counts default to 1."));
		return;
	}

	const void* ListPtr = ListProp->ContainerPtrToValuePtr<void>(Inventory);
	FScriptArrayHelper ArrayHelper(EntriesProp, EntriesProp->ContainerPtrToValuePtr<void>(ListPtr));
	for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
	{
		const void* EntryPtr = ArrayHelper.GetRawPtr(Index);
		if (ULyraInventoryItemInstance* Inst = Cast<ULyraInventoryItemInstance>(InstProp->GetObjectPropertyValue_InContainer(EntryPtr)))
		{
			OutCounts.Add(Inst, CountProp->GetPropertyValue_InContainer(EntryPtr));
		}
	}
}

FString UVisualInventoryComponent::GetItemStringVariable(const UInventoryFragment_ItemDisplay* Display, int32 Quantity, const FString& VariableName)
{
	if (!Display)
	{
		return FString();
	}

	if (VariableName == TEXT("DisplayName"))
	{
		return Display->DisplayName.ToString();
	}
	if (VariableName == TEXT("Weight"))
	{
		return FString::SanitizeFloat(Display->Weight);
	}
	if (VariableName == TEXT("RechargeDuration"))
	{
		return FString::SanitizeFloat(Display->UseRechargeDuration);
	}
	if (VariableName == TEXT("StackWeight"))
	{
		return FString::SanitizeFloat(Display->Weight * Quantity);
	}
	if (VariableName == TEXT("Quantity"))
	{
		return FString::FromInt(Quantity);
	}
	if (VariableName == TEXT("MaxStackSize"))
	{
		return FString::FromInt(Display->bStackable ? Display->MaxStackSize : 1);
	}
	if (VariableName == TEXT("BaseValue"))
	{
		return FString::FromInt(Display->BaseValue);
	}

	return FString();
}

FText UVisualInventoryComponent::GetParsedDescription(const FVisualInventoryItem& Item)
{
	if (!Item.Display)
	{
		return FText::GetEmpty();
	}

	FString Result = Item.Display->Description.ToString();

	// Replace {Variable} tokens left-to-right, capped to avoid pathological loops.
	int32 Safety = 0;
	int32 OpenIndex = INDEX_NONE;
	while ((Safety++ < 50) && Result.FindChar(TEXT('{'), OpenIndex))
	{
		int32 CloseIndex = INDEX_NONE;
		if (!Result.FindChar(TEXT('}'), CloseIndex) || CloseIndex < OpenIndex)
		{
			break;
		}

		const FString VariableName = Result.Mid(OpenIndex + 1, CloseIndex - OpenIndex - 1);
		const FString Value = GetItemStringVariable(Item.Display, Item.StackCount, VariableName);
		Result = Result.Left(OpenIndex) + Value + Result.Mid(CloseIndex + 1);
	}

	return FText::FromString(Result);
}

float UVisualInventoryComponent::GetStackWeight(const FVisualInventoryItem& Item)
{
	return Item.Display ? (Item.Display->Weight * Item.StackCount) : 0.f;
}

int32 UVisualInventoryComponent::GetStackSpace(const FVisualInventoryItem& Item)
{
	if (!Item.Display)
	{
		return 0;
	}

	const int32 EffectiveMax = Item.Display->bStackable ? Item.Display->MaxStackSize : 1;
	return FMath::Max(0, EffectiveMax - Item.StackCount);
}

bool UVisualInventoryComponent::IsStackFull(const FVisualInventoryItem& Item)
{
	if (!Item.Display)
	{
		return true;
	}

	const int32 EffectiveMax = Item.Display->bStackable ? Item.Display->MaxStackSize : 1;
	return Item.StackCount >= EffectiveMax;
}

TSoftObjectPtr<ULyraPickupDefinition> UVisualInventoryComponent::GetPickupDefinitionForItem(const ULyraInventoryItemInstance* Instance)
{
	if (const UInventoryFragment_ItemDisplay* Display = FindDisplayFragment(Instance))
	{
		return Display->PickupDefinition;
	}

	return nullptr;
}
