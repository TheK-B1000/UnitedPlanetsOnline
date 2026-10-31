// White Tiger Studios 2024

using UnrealBuildTool;
using System.Collections.Generic;

public class WarOfGodsTarget : TargetRules
{
	public WarOfGodsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "WarOfGods" });
	}
}
