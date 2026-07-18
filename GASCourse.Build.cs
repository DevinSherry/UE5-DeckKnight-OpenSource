using UnrealBuildTool;

public class GASCourse : ModuleRules
{
	public GASCourse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		SetupGameplayDebuggerSupport(Target);
		SetupIrisSupport(Target);
		
		PublicIncludePaths.Add("GASCourse/Public");
		PrivateIncludePaths.Add("GASCourse/Private");

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
			"NiagaraUIRenderer",
			"MassCommon",
			"MassMovement",
			"MassEntity",
			"MassSpawner",
			"MassRepresentation",
			"GameplayMessageRuntime", 
			"ImGui",
			"StateTreeModule",
			"StructUtils",
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AIModule",
			"NetCore",
			"MotionWarping",
			"StateTreeModule",
			"GameplayStateTreeModule", 
			"Slate",
			"PropertyBindingUtils",
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