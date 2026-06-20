// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "AbilitySystem/Attributes/LyraAttributeSet.h"
#include "AbilitySystemComponent.h"

#include "InventoryExpansionAttributeSet.generated.h"

class UObject;
struct FFrame;

/**
 * UInventoryExpansionAttributeSet
 *
 *	Gameplay attributes for the InventoryGame experience.
 *
 *	MovementSpeed drives the pawn's max walk speed: UInventoryExpansionMovementComponent::GetMaxSpeed()
 *	reads this attribute instead of the static UCharacterMovementComponent::MaxWalkSpeed, so any
 *	GameplayEffect that modifies it (e.g. GE_SpeedBoost from the shoes) changes the actual locomotion
 *	speed in a replicated, GAS-driven way - no Tick, no manual SetMaxWalkSpeed.
 *
 *	JumpPower drives the pawn's jump Z velocity the same way: UInventoryExpansionMovementComponent::DoJump()
 *	reads this attribute and feeds it into UCharacterMovementComponent::JumpZVelocity before jumping, so a
 *	GameplayEffect (e.g. GE_JumpBoost from the boots) makes the pawn jump higher while equipped.
 *
 *	Defense is a percentage damage-reduction stat (points of % reduction) read by
 *	UInventoryExpansionDamageExecution: incoming damage is multiplied by (1 - Defense/100), so a
 *	GameplayEffect (e.g. GE_DefenseBoost from the helmet) makes the pawn harder to kill while equipped.
 */
UCLASS(BlueprintType)
class UInventoryExpansionAttributeSet : public ULyraAttributeSet
{
	GENERATED_BODY()

public:
	UInventoryExpansionAttributeSet();

	ATTRIBUTE_ACCESSORS(ThisClass, MovementSpeed);
	ATTRIBUTE_ACCESSORS(ThisClass, JumpPower);
	ATTRIBUTE_ACCESSORS(ThisClass, Defense);

	//~UAttributeSet interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	//~End of UAttributeSet interface

protected:

	UFUNCTION()
	void OnRep_MovementSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_JumpPower(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldValue);

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	// Max walk speed (cm/s) read by UInventoryExpansionMovementComponent::GetMaxSpeed().
	// Default matches Lyra's MaxWalkSpeed (engine default 600). Adjust if your mannequin BP overrides it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MovementSpeed, Category="InventoryGame", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MovementSpeed;

	// Jump Z velocity (cm/s) read by UInventoryExpansionMovementComponent::DoJump().
	// Default matches Lyra's JumpZVelocity (UE5 default 700). Adjust if your mannequin BP overrides it,
	// otherwise the baseline jump height changes once this attribute set is granted.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_JumpPower, Category="InventoryGame", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData JumpPower;

	// Percentage damage reduction (0..90) applied by UInventoryExpansionDamageExecution: incoming damage is
	// multiplied by (1 - Defense/100). Default 0 = no mitigation. Buffed by the helmet's GE_DefenseBoost.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Defense, Category="InventoryGame", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Defense;
};
