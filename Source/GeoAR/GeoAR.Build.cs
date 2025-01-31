// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using UnrealBuildTool;

public class GeoAR : ModuleRules
{
	public GeoAR(ReadOnlyTargetRules Target) : base(Target)
	{
        bLegacyPublicIncludePaths = false;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bEnableUndefinedIdentifierWarnings = false;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Http", "UMG", "RealmakerUtil" });

		PrivateDependencyModuleNames.AddRange(new string[] {
			"AugmentedReality",
			"ProceduralMeshComponent",
            "GdalPlugin",
            "StreetMapRuntime",
            "FusionPositioningSystem",
            
            "RENDERCORE",
            "RHI",
            "SlateCore",
            "OpenGLDrv",
            
            "Json",
            "JsonUtilities",
            "ArCamera",
            "AndroidService",
            "Eigen"
        });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
