// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemCognito : ModuleRules
{
	public OnlineSubsystemCognito(ReadOnlyTargetRules Target) : base(Target)
    {
		PrivateDefinitions.Add("ONLINESUBSYSTEMCOGNITO_PACKAGE=1");
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		
		PublicIncludePaths.AddRange(
			new string[] {
			"$(ProjectDir)/Plugins/BaAwsSdk/Source/ThirdParty/BaAwsSdkCoreLibrary/include"
			}
			);

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "OnlineSubsystemUtils",
				"BAMultiplayer"
			}
            );

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"Sockets", 
				"OnlineSubsystem", 
                "InputCore",
				"Json",
                "BaAwsSdkCoreLibrary",
                "BAMultiplayer",
                "Projects",
            }
			);
	}
}
