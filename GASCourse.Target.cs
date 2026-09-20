// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GASCourseTarget : TargetRules
{
    public GASCourseTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.AddRange(new string[] { "GASCourse" });

        // You already had this; it's fine.
        BuildEnvironment = TargetBuildEnvironment.Unique;

        // Test retains the in-game targeting history visualizer and its world primitives.
        if (Configuration == UnrealTargetConfiguration.Test)
        {
            GlobalDefinitions.Add("UE_ENABLE_DEBUG_DRAWING=1");
        }

        // Prevent editor-only data in Shipping
        bBuildWithEditorOnlyData = false;
        bUseLoggingInShipping = true;
        bUseConsoleInShipping = true;
        bUseExecCommandsInShipping = true;
        GlobalDefinitions.Add("UE_TRACE_ENABLED=1");
        GlobalDefinitions.Add("ALLOW_CONSOLE_IN_SHIPPING=1");
    }
}
