#pragma once

#include "BaseInstance.h"

// Should not be a BaseInstance, because BaseInstance is for Minecraft instances
class ServerUtils{ // Communication with sync server
   public:
    static QByteArray CreateManifest(InstancePtr instance);
    static std::pair<int, QByteArray> PutManifest(InstancePtr instance);
    static int SyncModpack(InstancePtr instance);
    static std::pair<int, QByteArray> GetManifest(InstancePtr instance);
};
