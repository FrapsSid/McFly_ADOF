// FullScreenPassModule.h

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FFullScreenPassSceneViewExtension;

class FFullScreenPassModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedPtr<FFullScreenPassSceneViewExtension, ESPMode::ThreadSafe> ViewExtension;
};