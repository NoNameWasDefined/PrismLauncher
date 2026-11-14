// File overview: Update task that makes sure the instance's game folders exist.

#pragma once

#include "tasks/Task.h"

class MinecraftInstance;
class FoldersTask : public Task {
    Q_OBJECT
   public:
    FoldersTask(MinecraftInstance* inst);
    virtual ~FoldersTask() = default;

    void executeTask() override;

   private:
    MinecraftInstance* m_inst;
};
