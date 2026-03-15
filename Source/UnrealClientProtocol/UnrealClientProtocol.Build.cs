// MIT License - Copyright (c) 2025 Italink

using UnrealBuildTool;

public class UnrealClientProtocol : ModuleRules
{
	public UnrealClientProtocol(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings",
			"Sockets",
			"Networking",
			"Json",
			"JsonUtilities",
		});

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }
    }
}
