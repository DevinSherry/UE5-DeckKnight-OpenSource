using UnrealBuildTool;
using System.Collections.Generic;

public class GASCourseShippingWithDebugTarget : TargetRules
{
    public GASCourseShippingWithDebugTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;

        DefaultBuildSettings   = BuildSettingsVersion.V5;
        IncludeOrderVersion    = EngineIncludeOrderVersion.Latest;

        ExtraModuleNames.Add("GASCourse");

        // Build as "Shipping" but preserve debug data
        Configuration = UnrealTargetConfiguration.Shipping;

        // ---------------------------
        //    SAFE SHIPPING DEBUGGING
        // ---------------------------

        bUsePDBFiles = true;                 // Always exists
        bUseLoggingInShipping = true;       // Always exists
        bUseChecksInShipping  = true;       // Always exists
        bBuildWithEditorOnlyData = false;   // Always exists
        bUseUnityBuild = false;             // Always exists

        // UE-specific defines: always safe
        GlobalDefinitions.Add("UE_BUILD_DEBUG_WITH_SHIPPING=1");
        GlobalDefinitions.Add("UE_ALLOW_CPP_DEBUG_IN_SHIPPING=1");
        GlobalDefinitions.Add("UE_ALLOW_LOGGING_IN_SHIPPING=1");
        GlobalDefinitions.Add("ALLOW_CONSOLE_IN_SHIPPING=1");
    }
}
