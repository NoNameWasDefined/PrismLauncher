// File overview: (This fork) InstanceCreationTask for .json packs, i.e. sync-server manifests (see ServerCommunication.h
// for the format): sets up the loaders from "loadersVersion" and downloads every mod listed in "mods".

#pragma once

#include <QJsonObject>
#include <QString>
#include <QUrl>
#include <QVector>

#include "BaseInstance.h"
#include "InstanceCreationTask.h"

/*
 * Created by InstanceImportTask::processJSONPack() when the imported file is JSON instead of an archive.
 * Modeled on ModrinthCreationTask, minus everything the manifest doesn't carry (no hashes, no overrides,
 * no managed-pack ID, so no in-place updates).
 *
 * createInstance():
 *   1. read the manifest: components from "loadersVersion", one download per entry of "mods" that has a "url";
 *   2. write instance.cfg + mmc-pack.json in the staging folder (PackProfile::setComponentVersion);
 *   3. download the mods into <instance>/minecraft/mods;
 *   4. run EnsureMetadataTask (Modrinth or CurseForge, from the download host) so the mods get their
 *      mods/.index/*.pw.toml files, which ServerUtils::CreateManifest() reads to rebuild the manifest later.
 *
 * The manifest is not uploaded back to the server after the import (m_uploadManifest = false): the instance
 * was built from the server's copy, and mods whose metadata couldn't be found would be missing from the upload.
 */
class SyncPackCreationTask final : public InstanceCreationTask {
    Q_OBJECT
    struct ModFile {
        QString fileName;  // file name inside mods/
        QUrl url;
    };

   public:
    SyncPackCreationTask(QString staging_path, SettingsObjectPtr global_settings, QJsonObject manifest)
        : InstanceCreationTask(), m_manifest(std::move(manifest))
    {
        setStagingPath(staging_path);
        setParentSettings(global_settings);
        m_uploadManifest = false;
    }

    bool abort() override;

    InstancePtr createInstance() override;

   private:
    bool parseManifest();

   private:
    QJsonObject m_manifest;

    // uid -> version, in manifest order ("net.minecraft" first is not required).
    QVector<QPair<QString, QString>> m_components;
    QVector<ModFile> m_files;

    Task::Ptr m_task;
};
