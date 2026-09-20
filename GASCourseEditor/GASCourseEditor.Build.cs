using UnrealBuildTool;

public class GASCourseEditor : ModuleRules
{
	public GASCourseEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add("GASCourseEditor/Public");
		PrivateIncludePaths.Add("GASCourseEditor/Private");

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GASCourse"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"InputCore",
			"Slate",
			"SlateCore",
			"UnrealEd",
			"GraphEditor",
			"BlueprintGraph",
			"KismetCompiler",
			"PropertyEditor",
			"EditorSubsystem",
			"GameplayAbilities",
			"DataValidation",
			"RewindDebuggerInterface",
			"TraceServices",
			// StateTreeModule is already public on GASCourse, but this module includes its headers
			// directly. StateTreeEditorModule is only a private editor dependency over there, so the
			// editor-side types (UStateTreeEditorData, UStateTreeState, FStateTreeEditorNode) are not
			// reachable without naming it here.
			"StateTreeModule",
			"StateTreeEditorModule"
		});
	}
}
