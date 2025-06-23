using UnrealBuildTool;

public class MounteaDialogueSystemDeveloper : ModuleRules
{
	public MounteaDialogueSystemDeveloper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

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
				"MounteaDialogueSystem", 
			}
		);

		if (Target.bBuildEditor == true) 
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"EditorStyle",
					"BlueprintGraph",
					"ApplicationCore",
				}
			);
		}
	}
}