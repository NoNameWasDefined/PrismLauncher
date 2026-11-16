// File overview: Implementation of SyncPackCreationTask.h. (This fork) Builds an instance from a sync-server manifest
// (.json pack): loaders from "loadersVersion", mods downloaded from their "url".

#include "SyncPackCreationTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "Json.h"

#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/Mod.h"
#include "modplatform/EnsureMetadataTask.h"

#include "net/ApiDownload.h"
#include "net/NetJob.h"
#include "settings/INISettingsObject.h"

#include <QEventLoop>
#include <QJsonArray>
#include <algorithm>

bool SyncPackCreationTask::abort()
{
    if (!canAbort())
        return false;

    m_abort = true;
    if (m_task)
        m_task->abort();
    return Task::abort();
}

// Fills m_components and m_files from the manifest. Mods without a download URL (e.g. CurseForge mods whose
// authors block third-party downloads) are skipped with a warning instead of failing the whole import.
bool SyncPackCreationTask::parseManifest()
{
    try {
        auto components = Json::requireIsArrayOf<QJsonObject>(m_manifest, "loadersVersion", "manifest");
        for (const auto& component : components) {
            auto uid = Json::requireString(component, "uid", "loadersVersion");
            // LWJGL is a dependency of Minecraft: PackProfile resolves it on its own.
            if (uid.startsWith("org.lwjgl"))
                continue;
            m_components.append({ uid, Json::requireString(component, "version", uid) });
        }

        // "mods" is optional: a vanilla or loader-only pack has none.
        const auto mods = m_manifest["mods"].toArray();
        for (const QJsonValue value : mods) {
            auto mod = value.toObject();
            auto name = mod["name"].toString();
            QUrl url(mod["url"].toString());
            if (!url.isValid() || url.isEmpty()) {
                logWarning(tr("The mod '%1' has no download link and was skipped.").arg(name));
                continue;
            }
            // Download URLs end with the jar's file name (Modrinth and CurseForge CDNs both do).
            auto fileName = url.fileName();
            if (fileName.isEmpty())
                fileName = name + ".jar";
            m_files.append({ FS::RemoveInvalidPathChars(fileName), url });
        }
    } catch (const JSONValidationError& e) {
        setError(tr("Could not understand pack manifest:\n") + e.cause());
        return false;
    }

    bool hasMinecraft =
        std::any_of(m_components.begin(), m_components.end(), [](const auto& component) { return component.first == "net.minecraft"; });
    if (!hasMinecraft) {
        setError(tr("The pack manifest does not specify a Minecraft version."));
        return false;
    }
    return true;
}

InstancePtr SyncPackCreationTask::createInstance()
{
    if (!parseManifest())
        return nullptr;

    QString configPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(configPath);
    auto instance = std::make_shared<MinecraftInstance>(m_globalSettings, instanceSettings, m_stagingPath);

    auto components = instance->getPackProfile();
    components->buildingFromScratch();
    for (const auto& [uid, version] : m_components)
        components->setComponentVersion(uid, version, uid == "net.minecraft");

    if (m_instIcon != "default")
        instance->setIconKey(m_instIcon);

    instance->setName(name());
    instance->saveNow();

    QEventLoop loop;
    auto downloadMods = makeShared<NetJob>(tr("Mod Download"), APPLICATION->network());

    auto modsPath = instance->modsRoot();
    auto modsUrl = QUrl::fromLocalFile(modsPath);
    QStringList filePaths;
    for (const auto& file : m_files) {
        auto filePath = FS::PathCombine(modsPath, file.fileName);
        if (!modsUrl.isParentOf(QUrl::fromLocalFile(filePath))) {
            // This means we somehow got out of the mods folder, so abort here to prevent exploits
            setError(tr("One of the files has a path that leads to an arbitrary location (%1). This is a security risk and isn't allowed.")
                         .arg(file.fileName));
            return nullptr;
        }
        qDebug() << "Will try to download" << file.url << "to" << filePath;
        downloadMods->addNetAction(Net::ApiDownload::makeFile(file.url, filePath));
        filePaths.append(filePath);
    }

    bool ended_well = false;

    connect(downloadMods.get(), &NetJob::succeeded, this, [&ended_well]() { ended_well = true; });
    connect(downloadMods.get(), &NetJob::failed, [this, &ended_well](const QString& reason) {
        ended_well = false;
        setError(reason);
    });
    connect(downloadMods.get(), &NetJob::finished, &loop, &QEventLoop::quit);
    connect(downloadMods.get(), &NetJob::progress, [this](qint64 current, qint64 total) {
        setDetails(tr("%1 out of %2 complete").arg(current).arg(total));
        setProgress(current, total);
    });
    connect(downloadMods.get(), &NetJob::stepProgress, this, &SyncPackCreationTask::propagateStepProgress);

    setStatus(tr("Downloading mods..."));
    downloadMods->start();
    m_task = downloadMods;

    loop.exec();

    if (!ended_well)
        return nullptr;

    // Write mods/.index/*.pw.toml for the downloaded mods, asking the platform each one was downloaded from.
    // Failures are not fatal: the mod is installed, it just won't appear in this instance's next manifest.
    QList<Resource*> modrinthMods, flameMods;
    for (int i = 0; i < m_files.size(); i++) {
        auto mod = new Mod(filePaths[i]);
        ModDetails d;
        d.mod_id = filePaths[i];
        mod->setDetails(d);
        if (m_files[i].url.host().endsWith("forgecdn.net"))
            flameMods.append(mod);
        else
            modrinthMods.append(mod);
    }

    QDir indexFolder = FS::PathCombine(modsPath, ".index");
    auto ensureMetadata = [this, &indexFolder](QList<Resource*>& mods, ModPlatform::ResourceProvider provider) {
        if (mods.isEmpty() || m_abort)
            return;
        QEventLoop ensureMetaLoop;
        auto ensureMetadataTask = makeShared<EnsureMetadataTask>(mods, indexFolder, provider);
        connect(ensureMetadataTask.get(), &Task::finished, &ensureMetaLoop, &QEventLoop::quit);
        connect(ensureMetadataTask.get(), &Task::progress, [this](qint64 current, qint64 total) {
            setDetails(tr("%1 out of %2 complete").arg(current).arg(total));
            setProgress(current, total);
        });
        connect(ensureMetadataTask.get(), &Task::stepProgress, this, &SyncPackCreationTask::propagateStepProgress);

        ensureMetadataTask->start();
        m_task = ensureMetadataTask;

        ensureMetaLoop.exec();
    };
    ensureMetadata(modrinthMods, ModPlatform::ResourceProvider::MODRINTH);
    ensureMetadata(flameMods, ModPlatform::ResourceProvider::FLAME);

    qDeleteAll(modrinthMods);
    qDeleteAll(flameMods);

    if (m_abort)
        return nullptr;

    return instance;
}
