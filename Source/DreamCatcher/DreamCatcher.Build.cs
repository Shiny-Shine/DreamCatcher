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
			"ApplicationCore",
			"EnhancedInput",

			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			
			"ModularGameplay",
			"GameFeatures",

			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			
			"ModularGameplayActors",
			"CommonUser",
			"CommonGame",
			"CommonUI",
			"UIExtension",
			"AsyncMixin",
			"GameSubtitles",
			"CommonLoadingScreen",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"GameplayMessageRuntime",
			"Niagara",
			"SlateCore",
			"CommonInput",
			"AudioMixer",
			"AudioModulation"
		});

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

		PublicDependencyModuleNames.Add("PhysicsCore");
		PrivateDependencyModuleNames.Add("NetCore");
		// Original GameInstance parent required by the imported weapon audio Blueprint.
		PrivateDependencyModuleNames.AddRange(new[] { "CoreOnline", "DTLSHandlerComponent" });

		SetupIrisSupport(Target);

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] {
		//     "Slate", "SlateCore"
		// });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section
		// in your uproject file with the Enabled attribute set to true.
	}
}
