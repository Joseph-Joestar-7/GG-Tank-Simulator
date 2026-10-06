// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class WW2Tank_PanzerIV_AdvEditorTarget : TargetRules
{
	public WW2Tank_PanzerIV_AdvEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;

		ExtraModuleNames.AddRange( new string[] { "WW2Tank_PanzerIV_Adv" } );
	}
}
