// Copyright AndresAdarve. All Rights Reserved.

#include "InventoryExpansionAttributeSet.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryExpansionAttributeSet)

class FLifetimeProperty;

UInventoryExpansionAttributeSet::UInventoryExpansionAttributeSet()
	: MovementSpeed(600.0f)
{
}

void UInventoryExpansionAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MovementSpeed, COND_None, REPNOTIFY_Always);
}

void UInventoryExpansionAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MovementSpeed, OldValue);
}

void UInventoryExpansionAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UInventoryExpansionAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UInventoryExpansionAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetMovementSpeedAttribute())
	{
		// Never allow a negative speed; GetMaxSpeed() falls back to Super when this is <= 0.
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}
