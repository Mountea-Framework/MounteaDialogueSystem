using UnrealBuildTool;
 
public class MounteaDialogueSystemDeveloper : ModuleRules
{
	public MounteaDialogueSystemDeveloper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"BlueprintGraph"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Projects",
				"Engine",
				"Slate",
				"SlateCore", 
				"Projects", 
				"EditorStyle",
				"BlueprintGraph",
				
				"MounteaDialogueSystem", 
				"MounteaDialogueSystemEditor"
			}
		);
	}
}