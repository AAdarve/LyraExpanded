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
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"GameplayAbilities",
				"GameplayMessageRuntime",
				"ModularGameplay",
			}
			);
	}
}
