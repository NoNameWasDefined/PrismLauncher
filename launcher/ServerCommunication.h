#pragma once

#include "BaseInstance.h"

// TODO: rename this class
class ServerInstance : public BaseInstance {
   public:
    QByteArray CreateManifest(InstancePtr instance);
    std::pair<int, QByteArray> PostManifest(InstancePtr instance);
    int SyncModpack(InstancePtr instance);
};
