// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MounteaDialogueSystem : ModuleRules
{
	public MounteaDialogueSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;
		ShadowVariableWarningLevel = WarningLevel.Error;
		
		PublicIncludePaths.AddRange
		(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange
		(
			new string[] 
			{
				"MounteaDialogueSystem/Private"
			}
		);
			
		
		PublicDependencyModuleNames.AddRange
		(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG"
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange
		(
			new string[]
			{
				"Engine",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"DeveloperSettings",
				"UMG",
				"Projects",
				"NetCore",
				"InputCore"
			}
		);
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
		
		DynamicallyLoadedModuleNames.AddRange
		(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
