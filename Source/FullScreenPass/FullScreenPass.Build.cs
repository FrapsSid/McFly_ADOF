// FullScreenPass.Build.cs

using UnrealBuildTool;
using System.IO;

public class FullScreenPass : ModuleRules
{
    public FullScreenPass(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );
            
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "RHI",
                "RenderCore",
                "Renderer",
                "Projects",
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[]
            {
                "Renderer",
            }
        );

        string PluginPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));
        string ShaderPath = Path.Combine(PluginPath, "Shaders");
        
        if (Directory.Exists(ShaderPath))
        {
            RuntimeDependencies.Add(Path.Combine(ShaderPath, "*.usf"));
            RuntimeDependencies.Add(Path.Combine(ShaderPath, "*.ush"));
        }
    }
}