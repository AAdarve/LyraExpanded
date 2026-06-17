// Copyright AndresAdarve. All Rights Reserved.

#include "InventoryExpansionComponent.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "LyraGameplayTags.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

const FName UInventoryExpansionComponent::NAME_ActorFeatureName("NarrativeInventoryExpansion");

void UInventoryExpansionComponent::OnRegister()
{
	Super::OnRegister();

	// Register with the init state system; this only matters in a game world.
	RegisterInitStateFeature();
}

void UInventoryExpansionComponent::BeginPlay()
{
	// Let the Narrative base run its own setup (e.g. granting default items).
	Super::BeginPlay();

	// Listen for the pawn extension feature so we can advance once the pawn core is initialized.
	BindOnActorInitStateChanged(ULyraPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);

	// Notify that we have spawned, then try to advance the rest of the chain.
	ensure(TryToChangeInitState(LyraGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UInventoryExpansionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

bool UInventoryExpansionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = Cast<APawn>(GetOwner());

	if (!CurrentState.IsValid() && DesiredState == LyraGameplayTags::InitState_Spawned)
	{
		// As long as we are on a valid pawn, we count as spawned.
		return Pawn != nullptr;
	}
	else if (CurrentState == LyraGameplayTags::InitState_Spawned && DesiredState == LyraGameplayTags::InitState_DataAvailable)
	{
		if (!Pawn)
		{
			return false;
		}

		// The player state is required.
		if (!Pawn->GetPlayerState())
		{
			return false;
		}

		// On authority or when locally controlled we also need a controller.
		if (Pawn->HasAuthority() || Pawn->IsLocallyControlled())
		{
			if (!Pawn->GetController())
			{
				return false;
			}
		}

		return true;
	}
	else if (CurrentState == LyraGameplayTags::InitState_DataAvailable && DesiredState == LyraGameplayTags::InitState_DataInitialized)
	{
		// Wait for the pawn extension (ASC, pawn data) to be initialized before we are.
		return Manager->HasFeatureReachedInitState(Pawn, ULyraPawnExtensionComponent::NAME_ActorFeatureName, LyraGameplayTags::InitState_DataInitialized);
	}
	else if (CurrentState == LyraGameplayTags::InitState_DataInitialized && DesiredState == LyraGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UInventoryExpansionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState)
{
	if (DesiredState == LyraGameplayTags::InitState_GameplayReady)
	{
		// The pawn is fully initialized. Run the server-side mirror entry point.
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			OnPawnGameplayReady();
		}
	}
}

void UInventoryExpansionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
	if (Params.FeatureName == ULyraPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == LyraGameplayTags::InitState_DataInitialized)
		{
			// The pawn extension says the core is initialized; try to progress our own chain.
			CheckDefaultInitialization();
		}
	}
}

void UInventoryExpansionComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { LyraGameplayTags::InitState_Spawned, LyraGameplayTags::InitState_DataAvailable, LyraGameplayTags::InitState_DataInitialized, LyraGameplayTags::InitState_GameplayReady };

	ContinueInitStateChain(StateChain);
}

void UInventoryExpansionComponent::OnPawnGameplayReady_Implementation()
{
	// TODO (Phase 3): Read the pawn's Lyra inventory (ULyraInventoryManagerComponent),
	// resolve each item's UInventoryFragment_NarrativeItem::NarrativeItemClass and feed it
	// into this Narrative inventory via TryAddItemFromClass(). Left empty so the derived
	// Blueprint can implement the mirror for now.
}
