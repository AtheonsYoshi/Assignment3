using UnrealBuildTool;
using System.Collections.Generic;

public class Assignment3Target : TargetRules
{
	public Assignment3Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Assignment3");
	}
}
