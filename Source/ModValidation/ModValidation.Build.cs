using UnrealBuildTool;

public class ModValidation : ModuleRules
{
	public ModValidation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DataValidation", // UEditorValidatorBase / UEditorValidatorSubsystem
			"BlueprintGraph",  // UK2Node_CallFunction
			"UnrealEd"         // editor-only blueprint utilities
		});
	}
}
