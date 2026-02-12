// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MySkully : ModuleRules
{
	public MySkully(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "GeometryCollectionEngine",
			"FieldSystemEngine", "GameplayTasks", "StateTreeModule", "GameplayStateTreeModule", "NavigationSystem"
		});
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
	}
}
