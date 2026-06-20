// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Character/LyraCharacterMovementComponent.h"

#include "InventoryExpansionMovementComponent.generated.h"

class UObject;

/**
 * UInventoryExpansionMovementComponent
 *
 *	Lyra character movement component whose only changes are:
 *	 - GetMaxSpeed() reads the UInventoryExpansionAttributeSet::MovementSpeed attribute instead of MaxWalkSpeed.
 *	 - DoJump() feeds the UInventoryExpansionAttributeSet::JumpPower attribute into JumpZVelocity, so equipping
 *	   the boots (which buff JumpPower via a GameplayEffect) makes the pawn jump higher.
 *
 *	Assign it to the pawn via the Blueprint's "Component Class Overrides" on the inherited
 *	CharacterMovement component (same approach as B_Hero_Arena) - no C++ character subclass needed.
 */
UCLASS()
class UInventoryExpansionMovementComponent : public ULyraCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UInventoryExpansionMovementComponent(const FObjectInitializer& ObjectInitializer);

	//~UMovementComponent interface
	virtual float GetMaxSpeed() const override;
	//~End of UMovementComponent interface

	//~UCharacterMovementComponent interface
	virtual bool DoJump(bool bReplayingMoves, float DeltaTime) override;
	//~End of UCharacterMovementComponent interface
};
