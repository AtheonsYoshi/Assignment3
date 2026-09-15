using UnrealBuildTool;
using System.Collections.Generic;

public class Assignment3EditorTarget : TargetRules
{
	public Assignment3EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Assignment3");
	}
}
