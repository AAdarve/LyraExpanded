// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "GameplayEffectExecutionCalculation.h"

#include "InventoryExpansionDamageExecution.generated.h"

class UObject;

/**
 * UInventoryExpansionDamageExecution
 *
 *	Drop-in replacement for ULyraDamageExecution that additionally mitigates incoming damage by the target's
 *	UInventoryExpansionAttributeSet::Defense attribute (a percentage of damage reduction, 0..90).
 *
 *	It reproduces Lyra's stock damage math (BaseDamage x DistanceAttenuation x PhysicalMaterialAttenuation x
 *	team multiplier) so distance falloff, surface attenuation and friendly-fire rules keep working exactly the
 *	same, then multiplies the result by (1 - Defense/100) before writing it to the Damage meta-attribute.
 *
 *	To enable defense on a weapon, swap the calculation class in that weapon's GE_Damage_* "Executions" array
 *	from ULyraDamageExecution to this class - the weapon ability/asset references are unchanged. If the target
 *	has no UInventoryExpansionAttributeSet granted, the captured Defense is 0 and damage is unmitigated.
 */
UCLASS()
class UInventoryExpansionDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:

	UInventoryExpansionDamageExecution();

protected:

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
