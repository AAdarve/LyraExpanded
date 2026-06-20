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

bool UInventoryExpansionMovementComponent::DoJump(bool bReplayingMoves, float DeltaTime)
{
	// Drive the jump velocity from the JumpPower attribute. Super::DoJump() applies Velocity.Z = JumpZVelocity,
	// so updating JumpZVelocity here (right before jumping) lets a GameplayEffect on JumpPower change jump height
	// in a replicated, GAS-driven way. Falls back to the component's configured JumpZVelocity when unavailable.
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		const float JumpPowerFromAttribute = ASC->GetNumericAttribute(UInventoryExpansionAttributeSet::GetJumpPowerAttribute());
		if (JumpPowerFromAttribute > 0.0f)
		{
			JumpZVelocity = JumpPowerFromAttribute;
		}
	}

	return Super::DoJump(bReplayingMoves, DeltaTime);
}
