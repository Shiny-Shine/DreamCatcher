using UnrealBuildTool;

public class DCRifleMigrationTools : ModuleRules
{
	public DCRifleMigrationTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[] { "AssetTools", "BlueprintEditorLibrary", "BlueprintGraph", "KismetCompiler", "SourceControl", "UnrealEd", "UMG", "UMGEditor" });
	}
}
