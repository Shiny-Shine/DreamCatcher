// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DreamCatcher : ModuleRules
{
	public DreamCatcher(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
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
			"DreamCatcher",
			"DreamCatcher/Variant_Platforming",
			"DreamCatcher/Variant_Platforming/Animation",
			"DreamCatcher/Variant_Combat",
			"DreamCatcher/Variant_Combat/AI",
			"DreamCatcher/Variant_Combat/Animation",
			"DreamCatcher/Variant_Combat/Gameplay",
			"DreamCatcher/Variant_Combat/Interfaces",
			"DreamCatcher/Variant_Combat/UI",
			"DreamCatcher/Variant_SideScrolling",
			"DreamCatcher/Variant_SideScrolling/AI",
			"DreamCatcher/Variant_SideScrolling/Gameplay",
			"DreamCatcher/Variant_SideScrolling/Interfaces",
			"DreamCatcher/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
