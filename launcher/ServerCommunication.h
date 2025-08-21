#pragma once

#include "BaseInstance.h"

// Should not be a BaseInstance, because BaseInstance is for Minecraft instances
class ServerUtils{ // Communication with sync server
   public:
    QByteArray CreateManifest(InstancePtr instance);
    std::pair<int, QByteArray> PostManifest(InstancePtr instance);
    int SyncModpack(InstancePtr instance);
};
