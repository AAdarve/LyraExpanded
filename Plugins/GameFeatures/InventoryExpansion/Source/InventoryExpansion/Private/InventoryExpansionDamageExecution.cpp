// Copyright AndresAdarve. All Rights Reserved.

#include "InventoryExpansionDamageExecution.h"

#include "InventoryExpansionAttributeSet.h"

#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/LyraGameplayEffectContext.h"
#include "AbilitySystem/LyraAbilitySourceInterface.h"
#include "Engine/World.h"
#include "LyraLogChannels.h"
#include "Teams/LyraTeamSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryExpansionDamageExecution)

// Mirrors ULyraDamageExecution's capture statics, adding the target's Defense attribute. BaseDamage is captured
// from the Source (the attacker's LyraCombatSet); Defense from the Target (the victim's InventoryExpansion set),
// non-snapshot so the live value at apply time is used.
struct FInventoryExpansionDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;
	FGameplayEffectAttributeCaptureDefinition DefenseDef;

	FInventoryExpansionDamageStatics()
	{
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(ULyraCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		DefenseDef = FGameplayEffectAttributeCaptureDefinition(UInventoryExpansionAttributeSet::GetDefenseAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
	}
};

static FInventoryExpansionDamageStatics& DamageStatics()
{
	static FInventoryExpansionDamageStatics Statics;
	return Statics;
}


UInventoryExpansionDamageExecution::UInventoryExpansionDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().BaseDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);
}

void UInventoryExpansionDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FLyraGameplayEffectContext* TypedContext = FLyraGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	check(TypedContext);

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BaseDamageDef, EvaluateParameters, BaseDamage);

	const AActor* EffectCauser = TypedContext->GetEffectCauser();
	const FHitResult* HitActorResult = TypedContext->GetHitResult();

	AActor* HitActor = nullptr;
	FVector ImpactLocation = FVector::ZeroVector;
	FVector ImpactNormal = FVector::ZeroVector;
	FVector StartTrace = FVector::ZeroVector;
	FVector EndTrace = FVector::ZeroVector;

	// Calculation of hit actor, surface, zone, and distance all rely on whether the calculation has a hit result or not.
	// Effects just being added directly w/o having been targeted will always come in without a hit result, which must default
	// to some fallback information.
	if (HitActorResult)
	{
		const FHitResult& CurHitResult = *HitActorResult;
		HitActor = CurHitResult.HitObjectHandle.FetchActor();
		if (HitActor)
		{
			ImpactLocation = CurHitResult.ImpactPoint;
			ImpactNormal = CurHitResult.ImpactNormal;
			StartTrace = CurHitResult.TraceStart;
			EndTrace = CurHitResult.TraceEnd;
		}
	}

	// Handle case of no hit result or hit result not actually returning an actor
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!HitActor)
	{
		HitActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
		if (HitActor)
		{
			ImpactLocation = HitActor->GetActorLocation();
		}
	}

	// Apply rules for team damage/self damage/etc...
	float DamageInteractionAllowedMultiplier = 0.0f;
	if (HitActor)
	{
		ULyraTeamSubsystem* TeamSubsystem = HitActor->GetWorld()->GetSubsystem<ULyraTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			DamageInteractionAllowedMultiplier = TeamSubsystem->CanCauseDamage(EffectCauser, HitActor) ? 1.0 : 0.0;
		}
	}

	// Determine distance
	double Distance = WORLD_MAX;

	if (TypedContext->HasOrigin())
	{
		Distance = FVector::Dist(TypedContext->GetOrigin(), ImpactLocation);
	}
	else if (EffectCauser)
	{
		Distance = FVector::Dist(EffectCauser->GetActorLocation(), ImpactLocation);
	}
	else
	{
		UE_LOG(LogLyraAbilitySystem, Error, TEXT("Damage Calculation cannot deduce a source location for damage coming from %s; Falling back to WORLD_MAX dist!"), *GetPathNameSafe(Spec.Def));
	}

	// Apply ability source modifiers.
	// NOTE: FLyraGameplayEffectContext::GetAbilitySource()/GetPhysicalMaterial() are not exported from LyraGame
	// (no LYRAGAME_API), so reproduce them with the base context's exported/inline API. ULyraGameplayAbility sets
	// the ability source from the source object (the weapon instance, which implements ILyraAbilitySourceInterface)
	// and also AddSourceObject()s it, so GetSourceObject() yields the same object; the physical material is just
	// the hit result's surface.
	float PhysicalMaterialAttenuation = 1.0f;
	float DistanceAttenuation = 1.0f;
	if (const ILyraAbilitySourceInterface* AbilitySource = Cast<ILyraAbilitySourceInterface>(TypedContext->GetSourceObject()))
	{
		if (const UPhysicalMaterial* PhysMat = (HitActorResult ? HitActorResult->PhysMaterial.Get() : nullptr))
		{
			PhysicalMaterialAttenuation = AbilitySource->GetPhysicalMaterialAttenuation(PhysMat, SourceTags, TargetTags);
		}

		DistanceAttenuation = AbilitySource->GetDistanceAttenuation(Distance, SourceTags, TargetTags);
	}
	DistanceAttenuation = FMath::Max(DistanceAttenuation, 0.0f);

	// Clamping is done when damage is converted to -health
	const float DamageDone = FMath::Max(BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation * DamageInteractionAllowedMultiplier, 0.0f);

	// Mitigate by the target's Defense (percentage damage reduction, 0..90). This is the only addition over
	// ULyraDamageExecution; a target without UInventoryExpansionAttributeSet captures Defense == 0 (no change).
	float Defense = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvaluateParameters, Defense);
	const float DamageMultiplier = 1.0f - (FMath::Clamp(Defense, 0.0f, 90.0f) / 100.0f);

	const float FinalDamage = FMath::Max(DamageDone * DamageMultiplier, 0.0f);

	if (FinalDamage > 0.0f)
	{
		// Apply a damage modifier, this gets turned into - health on the target
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(ULyraHealthSet::GetDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
	}
#endif // #if WITH_SERVER_CODE
}
