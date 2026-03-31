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
			"Niagara",
			"NiagaraEditor",
			"RHI",
			"Slate",
			"SlateCore",
			"InputCore",
			"AdvancedPreviewScene",
			"EditorScriptingUtilities",
			"ToolMenus",
			"ImageCore",
			"ImageWrapper",

			"GeometryScriptingCore",
			"GeometryScriptingEditor",
			"GeometryFramework",
			"DynamicMesh",
			"MeshDescription",
			"StaticMeshDescription",
			"StaticMeshEditor",

			"GeometryCore",
			"MeshConversion",
		});

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDependencyModuleNames.Add("LiveCoding");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelTBB", "OpenVDB");
		}
	}
}
