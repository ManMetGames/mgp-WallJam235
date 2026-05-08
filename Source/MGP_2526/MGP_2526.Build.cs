// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MGP_2526 : ModuleRules
{
	public MGP_2526(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"CableComponent",
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MGP_2526",
			"MGP_2526/Variant_Platforming",
			"MGP_2526/Variant_Platforming/Animation",
			"MGP_2526/Variant_Combat",
			"MGP_2526/Variant_Combat/AI",
			"MGP_2526/Variant_Combat/Animation",
			"MGP_2526/Variant_Combat/Gameplay",
			"MGP_2526/Variant_Combat/Interfaces",
			"MGP_2526/Variant_Combat/UI",
			"MGP_2526/Variant_SideScrolling",
			"MGP_2526/Variant_SideScrolling/AI",
			"MGP_2526/Variant_SideScrolling/Gameplay",
			"MGP_2526/Variant_SideScrolling/Interfaces",
			"MGP_2526/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
