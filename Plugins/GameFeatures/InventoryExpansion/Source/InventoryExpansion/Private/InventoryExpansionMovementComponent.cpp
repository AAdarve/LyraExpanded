// Copyright AndresAdarve. All Rights Reserved.

#include "InventoryExpansionMovementComponent.h"

#include "AbilitySystemGlobals.h"
#include "InventoryExpansionAttributeSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryExpansionMovementComponent)

UInventoryExpansionMovementComponent::UInventoryExpansionMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

float UInventoryExpansionMovementComponent::GetMaxSpeed() const
{
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (MovementMode == MOVE_Walking)
		{
			// Honor Lyra's movement-stopped tag (mirrors ULyraCharacterMovementComponent::GetMaxSpeed).
			if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
			{
				return 0.0f;
			}

			const float MaxSpeedFromAttribute = ASC->GetNumericAttribute(UInventoryExpansionAttributeSet::GetMovementSpeedAttribute());
			if (MaxSpeedFromAttribute > 0.0f)
			{
				return MaxSpeedFromAttribute;
			}
		}
	}

	return Super::GetMaxSpeed();
}
