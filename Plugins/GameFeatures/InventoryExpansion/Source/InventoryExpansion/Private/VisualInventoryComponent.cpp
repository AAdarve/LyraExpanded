// Copyright AndresAdarve. All Rights Reserved.

#include "VisualInventoryComponent.h"

#include "InventoryFragment_ItemDisplay.h"
#include "LyraLogChannels.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

#include "Inventory/LyraInventoryManagerComponent.h"
#include "Inventory/LyraInventoryItemInstance.h"
#include "Inventory/LyraInventoryItemDefinition.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Equipment/LyraEquipmentInstance.h"

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

	// Runtime data, gathered once: equipped-state (from equipment) and stack counts (from inventory).
	TMap<ULyraInventoryItemInstance*, ULyraEquipmentInstance*> EquippedMap;
	BuildEquippedMap(Equipment, EquippedMap);

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

		FVisualInventoryItem Row;
		Row.Instance = Instance;
		Row.Display = const_cast<UInventoryFragment_ItemDisplay*>(Display);

		const int32* Count = CountMap.Find(Instance);
		Row.StackCount = Count ? *Count : 1;

		if (ULyraEquipmentInstance** Equip = EquippedMap.Find(Instance))
		{
			Row.bEquipped = true;
			Row.EquipmentInstance = *Equip;
		}

		Results.Add(Row);
	}

	return Results;
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
