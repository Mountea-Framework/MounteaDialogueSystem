using UnrealBuildTool;
 
public class MounteaDialogueSystemDeveloper : ModuleRules
{
	public MounteaDialogueSystemDeveloper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"BlueprintGraph",
				"KismetCompiler",
				"Kismet",
				"SlateCore"
			}
		);
		
		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"GraphEditor"
				}
			);
		}

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"K2Nodes", "MounteaDialogueSystem"
			}
		);
 
		PublicIncludePaths.AddRange(new string[] {"MounteaDialogueSystemDeveloper/Public"});
		PrivateIncludePaths.AddRange(new string[] {"MounteaDialogueSystemDeveloper/Private"});
	}
}