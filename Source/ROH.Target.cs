using UnrealBuildTool;
using System.Collections.Generic;

public class ROHTarget : TargetRules
{
	public ROHTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ROH");
	}
}
