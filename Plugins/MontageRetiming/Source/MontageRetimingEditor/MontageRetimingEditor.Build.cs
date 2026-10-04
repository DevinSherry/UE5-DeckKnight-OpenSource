using UnrealBuildTool;

public class MontageRetimingEditor : ModuleRules
{
    public MontageRetimingEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "MontageRetiming", "UnrealEd",
            "Slate", "SlateCore", "InputCore", "AnimationEditor", "Persona", "AnimGraph", "EditorFramework"
        });
    }
}
