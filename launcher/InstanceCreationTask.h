#pragma once

#include "BaseVersion.h"
#include "InstanceTask.h"
#include "BaseInstance.h"
#include "minecraft/MinecraftInstance.h"

class InstanceCreationTask : public InstanceTask {
  Q_OBJECT
 public:
  InstanceCreationTask() = default;
  virtual ~InstanceCreationTask() = default;

 protected:
  void executeTask() final override;

  /**
   * Tries to update an already existing instance.
   *
   * This can be implemented by subclasses to provide a way of updating an
   * already existing instance, according to that implementation's concept of
   * 'identity' (i.e. instances that are updates / downgrades of one another).
   *
   * If this returns true, createInstance() will not run, so you should do all
   * update steps in here. Otherwise, createInstance() is run as normal.
   */
  virtual bool updateInstance() { return false; };

  /**
   * Creates a new instance.
   *
   * Returns the created instance.
   */
   InstancePtr createInstance() override {
       return std::make_shared<MinecraftInstance>(m_globalSettings, m_globalSettings, m_stagingPath);
   }

  QString getError() const { return m_error_message; }

 protected:
  void setError(const QString& message) { m_error_message = message; };

 protected:
  bool m_abort = false;

  QStringList m_files_to_remove;

 private:
  QString m_error_message;
};
