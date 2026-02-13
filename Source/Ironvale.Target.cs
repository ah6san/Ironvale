// =============================================================================
// Ironvale.Target.cs — Game build target (shipping / development)
// =============================================================================

using UnrealBuildTool;
using System.Collections.Generic;

public class IronvaleTarget : TargetRules
{
	public IronvaleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.AddRange(new string[] { "Ironvale" });
	}
}
