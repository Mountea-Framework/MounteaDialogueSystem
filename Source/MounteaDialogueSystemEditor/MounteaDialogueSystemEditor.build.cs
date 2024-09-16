using UnrealBuildTool;
 
public class MounteaDialogueSystemEditor : ModuleRules
{
	public MounteaDialogueSystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bLegacyPublicIncludePaths = false;
		ShadowVariableWarningLevel = WarningLevel.Error;
		
		PrecompileForTargets = PrecompileTargetsType.None;
		bPrecompile = false;
		bUsePrecompiled = false;
		
		// Add the zip library
		PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "ThirdParty", "zip"));
		PublicDefinitions.Add("ZIP_STATIC");
		
		var MZPlatformValue = "0";
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			MZPlatformValue = "1";
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			MZPlatformValue = "2";
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			MZPlatformValue = "3";
		}

		PublicDefinitions.Add("MZ_PLATFORM=" + MZPlatformValue);

		PublicDependencyModuleNames.AddRange
		(new string[]
			{
				"Core", 
				"CoreUObject", 
				"Engine", 
				"UnrealEd",
				"AssetTools"
			}
		);
		
		PrivateDependencyModuleNames.AddRange
		(
			new string[]
			{
				"MounteaDialogueSystem",

				"AssetTools",
				"Slate",
				"SlateCore",
				"GraphEditor",
				"PropertyEditor",
				"EditorStyle",
				"Kismet",
				"KismetWidgets",
				"ApplicationCore",
				"ToolMenus",
				"DeveloperSettings",
				"Projects", 
				
				"WebBrowser",
				"HTTP", 
				"Json", 
				"JsonUtilities",
				
				"BlueprintGraph",
				"InputCore", 
				
				"MainFrame",
				
				"GameplayTags"
				// ... add private dependencies that you statically link with here ...
			}
		);
		
	}
}