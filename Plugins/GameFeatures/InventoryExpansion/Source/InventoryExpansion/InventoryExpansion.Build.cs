// Copyright AndresAdarve. All Rights Reserved.

using UnrealBuildTool;

public class InventoryExpansion : ModuleRules
{
	public InventoryExpansion(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"NarrativeInventory",
				"NarrativeEquipment",
				"LyraGame",
				// Public because our component headers inherit IGameFrameworkInitStateInterface
				// and expose FGameplayTag / UGameFrameworkComponentManager in their signatures.
				"GameplayTags",
				"ModularGameplay",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameplayAbilities",
				"GameplayMessageRuntime",
			}
			);
	}
}
