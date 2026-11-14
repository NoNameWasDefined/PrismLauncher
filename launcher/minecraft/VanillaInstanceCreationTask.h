// File overview: InstanceCreationTask for a plain new instance: writes instance.cfg and an mmc-pack.json with the
// chosen Minecraft version (and optional mod loader).

#pragma once

#include "InstanceCreationTask.h"

#include <utility>

class VanillaCreationTask final : public InstanceCreationTask {
    Q_OBJECT
   public:
    VanillaCreationTask(BaseVersion::Ptr version) : InstanceCreationTask(), m_version(std::move(version)) {}
    VanillaCreationTask(BaseVersion::Ptr version, QString loader, BaseVersion::Ptr loader_version);

    InstancePtr createInstance() override;

   private:
    // Version to update to / create of the instance.
    BaseVersion::Ptr m_version;

    bool m_using_loader = false;
    QString m_loader;
    BaseVersion::Ptr m_loader_version;
};
