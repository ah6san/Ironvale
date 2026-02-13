// =============================================================================
// IronvaleEditor.Target.cs — Editor build target
// =============================================================================

using UnrealBuildTool;
using System.Collections.Generic;

public class IronvaleEditorTarget : TargetRules
{
	public IronvaleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.AddRange(new string[] { "Ironvale" });
	}
}
