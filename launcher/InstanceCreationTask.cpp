// File overview: Implementation of InstanceCreationTask.h. Base task for creating a new instance (vanilla, Modrinth,
// CurseForge...). Subclasses implement createInstance() and optionally updateInstance(); executeTask() runs them and,
// in this fork, uploads the modpack manifest to the sync server afterwards.

#include "InstanceCreationTask.h"
#include "ServerCommunication.h"

#include <QDebug>
#include <QFile>
#include "FileSystem.h"

void InstanceCreationTask::executeTask() {
  setAbortable(true);

  // 1. Pack platforms can update an existing instance instead of creating a new one.
  if (updateInstance()) {
    emitSucceeded();
    return;
  }

  // When the user aborted in the update stage.
  if (m_abort) {
    emitAborted();
    return;
  }

  // 2. Build the instance in the staging folder (subclass-specific: write instance.cfg, mmc-pack.json,
  //    download mods...). A null result means failure.
  auto createdInstance = createInstance();
  if (!createdInstance) {
    if (m_abort)
      return;

    qWarning() << "Instance creation failed!";
    if (!m_error_message.isEmpty()) {
      qWarning() << "Reason: " << m_error_message;
      emitFailed(
          tr("Error while creating new instance:\n%1").arg(m_error_message));
    } else {
      emitFailed(tr("Error while creating new instance."));
    }

    return;
  }

  // If this is set, it means we're updating an instance. So, we now need to
  // remove the files scheduled to, and we'd better not let the user abort in
  // the middle of it, since it'd put the instance in an invalid state.
  if (shouldOverride()) {
    bool deleteFailed = false;

    setAbortable(false);
    setStatus(tr("Removing old conflicting files..."));
    qDebug() << "Removing old files";

    for (const QString& path : m_files_to_remove) {
      if (!QFile::exists(path))
        continue;

      qDebug() << "Removing" << path;

      if (!QFile::remove(path)) {
        qCritical() << "Could not remove" << path;
        deleteFailed = true;
      }
    }

    if (deleteFailed) {
      emitFailed(tr("Failed to remove old conflicting files."));
      return;
    }
  }
  if (!m_abort) {
    // 3. (This fork) Upload the manifest (loaders + mods) of the new instance to the sync server.
    //    The instance still lives in its staging folder at this point; InstanceList moves it into
    //    instances/ after emitSucceeded(). The HTTP status code returned by PutManifest is ignored, so
    //    a failed upload does not stop the creation.
    ServerUtils::PutManifest(createdInstance);
    emitSucceeded();
  }
}
