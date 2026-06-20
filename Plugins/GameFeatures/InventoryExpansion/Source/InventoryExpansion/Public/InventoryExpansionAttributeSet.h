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
 */
UCLASS(BlueprintType)
class UInventoryExpansionAttributeSet : public ULyraAttributeSet
{
	GENERATED_BODY()

public:
	UInventoryExpansionAttributeSet();

	ATTRIBUTE_ACCESSORS(ThisClass, MovementSpeed);

	//~UAttributeSet interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	//~End of UAttributeSet interface

protected:

	UFUNCTION()
	void OnRep_MovementSpeed(const FGameplayAttributeData& OldValue);

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	// Max walk speed (cm/s) read by UInventoryExpansionMovementComponent::GetMaxSpeed().
	// Default matches Lyra's MaxWalkSpeed (engine default 600). Adjust if your mannequin BP overrides it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MovementSpeed, Category="InventoryGame", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData MovementSpeed;
};
