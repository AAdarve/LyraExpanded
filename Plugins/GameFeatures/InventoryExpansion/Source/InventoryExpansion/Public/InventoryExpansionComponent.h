// Copyright AndresAdarve. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "InventoryComponent.h"

#include "InventoryExpansionComponent.generated.h"

class UGameFrameworkComponentManager;
struct FActorInitStateChangedParams;
struct FGameplayTag;

/**
 * Blueprintable subclass of the Narrative Inventory component.
 *
 * The base UNarrativeInventoryComponent is exposed to Blueprints only via
 * meta=(BlueprintSpawnableComponent) (add-to-actor) but is NOT marked Blueprintable,
 * so it cannot be picked as a parent class for a Blueprint. This thin subclass adds
 * the Blueprintable specifier while staying IS-A UNarrativeInventoryComponent, so the
 * Narrative items/widgets (which are coupled to the exact base type) keep working.
 *
 * It also participates in Lyra's Init State framework as its own feature, so it does not
 * mirror Lyra's inventory until the owning pawn is fully initialized (GameplayReady). The
 * actual mirror logic is implemented by the derived Blueprint via OnPawnGameplayReady.
 */
UCLASS(Blueprintable, ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class INVENTORYEXPANSION_API UInventoryExpansionComponent : public UNarrativeInventoryComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()

public:

	/** The name of this feature inside the Init State framework. Must be unique on the owning pawn. */
	static const FName NAME_ActorFeatureName;

	//~ Begin IGameFrameworkInitStateInterface interface
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface

protected:

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Server-only. Fired once the owning pawn reaches Init State GameplayReady.
	 * Implement the Lyra -> Narrative inventory mirror in the derived Blueprint.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = "Narrative|Bridge")
	void OnPawnGameplayReady();
	virtual void OnPawnGameplayReady_Implementation();
};
