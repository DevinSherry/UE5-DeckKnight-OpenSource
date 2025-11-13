using UnrealBuildTool;

public class GASCourse : ModuleRules
{
	public GASCourse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		SetupGameplayDebuggerSupport(Target);
		SetupIrisSupport(Target);

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"PhysicsCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"UMG",
			"Niagara",
			"NavigationSystem",
			"TargetingSystem",
			"ModelViewViewModel",
			"PropertyBindingUtils"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AIModule",
			"NetCore",
			"MotionWarping",
			"StateTreeModule",
			"GameplayStateTreeModule", 
			"Slate"
		});

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new[]
			{
				"UnrealEd",
				"StructUtilsEditor",
				"BlueprintGraph",
				"AssetTools",
				"AssetRegistry",
				"Kismet",
				"StateTreeEditorModule" 
			});
		}
	}
}