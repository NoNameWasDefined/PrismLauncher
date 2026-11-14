// File overview: Base task for creating a new instance (vanilla, Modrinth, CurseForge...). Subclasses implement
// createInstance() and optionally updateInstance(); executeTask() runs them and, in this fork, uploads the modpack
// manifest to the sync server afterwards.

#pragma once

#include "BaseVersion.h"
#include "InstanceTask.h"
#include "BaseInstance.h"
#include "minecraft/MinecraftInstance.h"

/*
 * Base for "create an instance from X" tasks: VanillaCreationTask (minecraft/), FlameCreationTask and
 * ModrinthCreationTask (modplatform/). Flow of executeTask() (InstanceCreationTask.cpp):
 *
 *   updateInstance()  -> true: an existing instance was updated in place, done.
 *   createInstance()  -> builds the new instance's files inside m_stagingPath and returns it (null = failure,
 *                        with the reason in setError()).
 *   if shouldOverride(): delete m_files_to_remove (files of the old version of an updated pack).
 *   (this fork) ServerUtils::PutManifest(instance): upload the new pack's manifest to the sync server.
 *   emitSucceeded()   -> InstanceList then moves the staging folder into instances/ (the task was wrapped by
 *                        InstanceList::wrapInstanceTask(), called from MainWindow::instanceFromInstanceTask()),
 *                        which is when the instance appears in the UI.
 *
 * Upstream Prism's createInstance() returns a bool; this fork returns the InstancePtr so the manifest upload
 * can use it. Keep that in mind when merging upstream changes to the subclasses.
 */
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
   *
   * Subclasses override this; this default only builds an empty MinecraftInstance object on the staging
   * folder. Note it passes m_globalSettings as both the global and the instance settings object.
   */
   virtual InstancePtr createInstance() {
       return std::make_shared<MinecraftInstance>(m_globalSettings, m_globalSettings, m_stagingPath);
   }

  QString getError() const { return m_error_message; }

 protected:
  void setError(const QString& message) { m_error_message = message; };

 protected:
  // Set by subclasses' abort(); checked between steps.
  bool m_abort = false;

  // When updating an existing pack: files from the old version to delete once the new files are in place.
  QStringList m_files_to_remove;

 private:
  QString m_error_message;
};
