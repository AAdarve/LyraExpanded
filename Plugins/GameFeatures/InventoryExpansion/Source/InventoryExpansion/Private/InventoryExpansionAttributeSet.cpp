// Copyright AndresAdarve. All Rights Reserved.

#include "InventoryExpansionAttributeSet.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryExpansionAttributeSet)

class FLifetimeProperty;

UInventoryExpansionAttributeSet::UInventoryExpansionAttributeSet()
	: MovementSpeed(600.0f)
	, JumpPower(700.0f)
	, Defense(0.0f)
{
}

void UInventoryExpansionAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MovementSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, JumpPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Defense, COND_None, REPNOTIFY_Always);
}

void UInventoryExpansionAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MovementSpeed, OldValue);
}

void UInventoryExpansionAttributeSet::OnRep_JumpPower(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, JumpPower, OldValue);
}

void UInventoryExpansionAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Defense, OldValue);
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
	else if (Attribute == GetJumpPowerAttribute())
	{
		// Never allow a negative jump velocity; DoJump() falls back to the BP value when this is <= 0.
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetDefenseAttribute())
	{
		// Defense is a percentage of damage reduction; cap it at 90% so damage is never fully negated.
		NewValue = FMath::Clamp(NewValue, 0.0f, 90.0f);
	}
}
