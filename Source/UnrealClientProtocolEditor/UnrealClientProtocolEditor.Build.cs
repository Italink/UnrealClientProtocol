// MIT License - Copyright (c) 2025 Italink

using UnrealBuildTool;

public class UnrealClientProtocolEditor : ModuleRules
{
	public UnrealClientProtocolEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[]
		{
			ModuleDirectory + "/Public",
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateIncludePaths.AddRange(new string[]
		{
			ModuleDirectory + "/Private",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealClientProtocol",
			"UnrealEd",
			"MaterialEditor",
			"BlueprintGraph",
			"KismetCompiler",
			"UMG",
			"UMGEditor",
			"Json",
			"JsonUtilities",
			"AssetRegistry",
			"AssetTools",
		});
	}
}
