// =============================================================================
// Ironvale.Build.cs — Main module build rules
// Project Ironvale: First-person grounded medieval RPG
// =============================================================================

using UnrealBuildTool;

public class Ironvale : ModuleRules
{
	public Ironvale(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Cross-platform: these modules are engine-native and work on both Windows and macOS
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"GameplayTasks",
			"AIModule",
			"NavigationSystem",
			"Slate",
			"SlateCore",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AnimGraphRuntime"  // For animation blending and state machine support
		});

		// Include paths for subfolder organization
		PublicIncludePaths.AddRange(new string[]
		{
			"Ironvale",
			"Ironvale/Core",
			"Ironvale/Characters",
			"Ironvale/Combat",
			"Ironvale/Inventory",
			"Ironvale/Needs",
			"Ironvale/Dialogue",
			"Ironvale/AI",
			"Ironvale/Quests",
			"Ironvale/World",
			"Ironvale/Immersion",
			"Ironvale/UI"
		});
	}
}
