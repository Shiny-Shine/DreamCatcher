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
		// Original AI development lookup and opt-in WorldSettings map validation.
		PrivateDependencyModuleNames.Add("AssetRegistry");
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("MessageLog");
			// Narrow editor repair for the HARU dash montage/profile connection.
			PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "BlueprintGraph" });
		}

		PublicIncludePaths.AddRange(new string[] {
			"DreamCatcher"
		});

		PublicDependencyModuleNames.Add("PhysicsCore");
		// R13 original settings UI and its replay/settings dependencies.
		PublicDependencyModuleNames.AddRange(new[] { "GameSettings", "NetworkReplayStreaming", "PropertyPath", "DataRegistry" });
		PrivateDependencyModuleNames.AddRange(new[] { "PlatformDLC", "RHI", "RenderCore" });
		PrivateDependencyModuleNames.Add("NetCore");
		// Original Lyra Frontend flow; used only by the separate 2D menu Experience.
		PublicDependencyModuleNames.Add("ControlFlows");
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
