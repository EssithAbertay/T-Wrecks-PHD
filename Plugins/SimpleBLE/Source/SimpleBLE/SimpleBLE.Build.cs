// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class SimpleBLE : ModuleRules
{
	public SimpleBLE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		bUseRTTI = true;
		bEnableExceptions = true;
		bUseUnity = false;
	
		PublicIncludePaths.AddRange(
			new string[] {
				// SimpleBLE public headers live under Public/simpleble/*
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicAdditionalLibraries.Add("$(PluginDir)/Binaries/ThirdParty/simpleble/Win64/simpleble.lib");

			PublicSystemLibraries.Add("WindowsApp.lib");      
			PublicSystemLibraries.Add("RuntimeObject.lib");   
		}
	}
}
