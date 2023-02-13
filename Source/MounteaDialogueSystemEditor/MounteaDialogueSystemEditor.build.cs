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
				"Http", 
				"Json", 
				"JsonUtilities"
				// ... add private dependencies that you statically link with here ...
			}
		);
	}
}