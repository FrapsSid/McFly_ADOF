// FullScreenPassModule.cpp

#include "FullScreenPassModule.h"
#include "FullScreenPassSceneViewExtension.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FFullScreenPassModule"

void FFullScreenPassModule::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("ADOF: Module Starting..."));
    
    // Get plugin base directory
    FString PluginBaseDir;
    IPluginManager& PluginManager = IPluginManager::Get();
    TSharedPtr<IPlugin> Plugin = PluginManager.FindPlugin(TEXT("FullScreenPass"));
    
    if (Plugin.IsValid())
    {
        PluginBaseDir = Plugin->GetBaseDir();
    }
    else
    {
        PluginBaseDir = FPaths::ProjectPluginsDir() / TEXT("FullScreenPass");
    }
    
    // Setup shader directory - MUST match the path in IMPLEMENT_GLOBAL_SHADER
    FString ShaderDir = FPaths::Combine(PluginBaseDir, TEXT("Shaders"));
    ShaderDir = FPaths::ConvertRelativePathToFull(ShaderDir);
    
    UE_LOG(LogTemp, Warning, TEXT("ADOF: Shader directory: %s"), *ShaderDir);
    
    if (FPaths::DirectoryExists(ShaderDir))
    {
        // This path "/Plugin/ADOF" must match FullScreenPassShaders.cpp
        AddShaderSourceDirectoryMapping(TEXT("/Plugin/ADOF"), ShaderDir);
        UE_LOG(LogTemp, Warning, TEXT("ADOF: Mapped /Plugin/ADOF -> %s"), *ShaderDir);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ADOF: Shader directory NOT found!"));
    }
    
    // Create view extension
    ViewExtension = FSceneViewExtensions::NewExtension<FFullScreenPassSceneViewExtension>();
    if (ViewExtension.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ADOF: View Extension Created!"));
    }
}

void FFullScreenPassModule::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("ADOF: Module Shutting Down"));
    ViewExtension.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFullScreenPassModule, FullScreenPass)