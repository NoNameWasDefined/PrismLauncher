// File overview: Implementation of ServerCommunication.h. (This fork) Client for the modpack sync server: builds a
// manifest of an instance (loaders + mods) and uploads/downloads it. See the detailed comments inside.

#include "Application.h"
#include "BaseInstance.h"
#include "ServerCommunication.h"
#include <iostream>
using namespace std;

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QFileInfoList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QUrl>

// Reads <instance>/mmc-pack.json (the component list: Minecraft, loaders...) and returns its root object,
// or an empty object if the file is missing/invalid. See minecraft/PackProfile.h for the file format.
// (PackProfile itself could give the same data: std::dynamic_pointer_cast<MinecraftInstance>(inst)->getPackProfile().)
QJsonObject loadMmcPack(const QString& rootDir) {
  QString jsonPath = rootDir + "/mmc-pack.json";
  QFile jsonFile(jsonPath);
  if (!jsonFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Could not open" << jsonPath;
    return {};
  }

  QByteArray data = jsonFile.readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (!doc.isObject()) {
    qWarning() << "Invalid JSON in" << jsonPath;
    return {};
  }

  return doc.object();
}

QByteArray ServerUtils::CreateManifest(InstancePtr instance) {
  // --- Extract name from instance ---
  QString instanceName = instance->settings()->get("name").toString();

  // --- Get more instance info from Multi-mc's json files (Minecraft and Forge
  // versions)---
  QString rootDir = instance->instanceRoot();
  QJsonObject pack = loadMmcPack(rootDir);
  QJsonArray components = pack["components"].toArray();
  // NOTE: assumes LWJGL is always first. Usually true, but not guaranteed (old instances, custom orders,
  // or an empty list -> removeAt(0) on an empty array). Filtering on uid ("org.lwjgl"/"org.lwjgl3") is safer.
  components.removeAt(0);  // Remove the first element (usually LWJGL)

  // --- Mods folder ---
  QString modsPath = instance->modsRoot() +
                     "/.index";  // Go into the .index folder (keeps an index of
                                 // mods, with versions and download urls)
  QDir indexDir(modsPath);
  if (!indexDir.exists()) {
    qWarning() << ".index directory does not exist:" << modsPath;
    return QByteArray();  // Empty if vanilla instance
  }

  // --- Collect mods into JSON array ---
  // Each mod installed from Modrinth/CurseForge has a packwiz metadata file "<slug>.pw.toml" in mods/.index,
  // written by modplatform/packwiz/Packwiz.cpp. Mods dropped in by hand have none and are skipped.
  QJsonArray modsArray;
  QFileInfoList tomlFiles = indexDir.entryInfoList(
      QStringList() << "*.toml", QDir::Files | QDir::NoDotAndDotDot);

  for (const QFileInfo& fileInfo : tomlFiles) {
    QFile tomlFile(fileInfo.absoluteFilePath());
    if (!tomlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qWarning() << "Failed to open TOML file:" << fileInfo.fileName();
      continue;
    }

    QString content = tomlFile.readAll();
    tomlFile.close();

    // Extract needed values with regex (since Qt doesn't have native TOML parser)
    // NOTE: the launcher already links toml++ (used by Packwiz.cpp); Packwiz::V1::getIndexForMod() parses these
    // files properly and returns name, version, url... which would be more robust than regexes.
    QJsonObject modEntry;

    // NOTE: this pattern is not anchored to the start of a line, so it also matches inside
    // "filename = '...'". toml++ writes keys in alphabetical order, so "filename" comes before "name" and is
    // probably what gets captured here. Anchoring it ("^name\\s*=..." with
    // QRegularExpression::MultilineOption) would fix it.
    QRegularExpression nameRe("name\\s*=\\s*'([^']*)'");  // Get name
    QRegularExpressionMatch nameMatch = nameRe.match(content);
    if (nameMatch.hasMatch()) {
      modEntry["name"] = nameMatch.captured(1);
    }

    QRegularExpression versionRe(
        "x-prismlauncher-version-number\\s*=\\s*'([^']*)'");  // Get mods
                                                              // version
    QRegularExpressionMatch versionMatch = versionRe.match(content);
    if (versionMatch.hasMatch()) {
      modEntry["version"] = versionMatch.captured(1);
    }

    // The [download] table's url. CurseForge mods whose authors block third-party downloads have no url.
    QRegularExpression urlRe("url\\s*=\\s*'([^']*)'");  // Get download URL
    QRegularExpressionMatch urlMatch = urlRe.match(content);
    if (urlMatch.hasMatch()) {
      modEntry["url"] = urlMatch.captured(1);
    }

    modsArray.append(modEntry);
  }

  // --- Build manifest JSON ---
  QJsonObject manifest;
  manifest["instance_name"] = instanceName;
  manifest["loadersVersion"] = components;
  manifest["mods"] = modsArray;

  // --- Serialize to compact JSON ---
  QJsonDocument manifestDoc(manifest);
  return manifestDoc.toJson(QJsonDocument::Compact);
}

// Upload the manifest. Blocking: see the class comment in ServerCommunication.h.
int ServerUtils::PutManifest(InstancePtr instance) {
  // Get the global Application instance's network manager and settings manager
  // (debug print left over; qDebug() would go to the launcher log instead of stdout)
  cout << "CREATING MANIFEST";
  auto net = APPLICATION->network();
  auto settings = APPLICATION->settings();

  // Get credentials and instance name to build url
  QString username = settings->get("ServerId").toString();
  QString password = settings->get("ServerPsswd").toString();
  QString serverUrl = settings->get("ModpackSyncServerURL").toString();
  QString instanceName = instance->settings()->get("name").toString();

  if (serverUrl == ""){
    return 0
  }

  // Build Put url with credentials
  // The instance name goes into the URL path as-is; QUrl percent-encodes spaces etc., but a name containing
  // "/" or "?" would change the path. Credentials in the URL become an HTTP basic-auth header.
  QUrl url(serverUrl + "/modpacks/" + instanceName + "/manifest.json");
  url.setUserName(username);
  url.setPassword(password);

  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  // May be empty (no mods/.index folder): an empty body is uploaded in that case.
  QByteArray manifestData = ServerUtils::CreateManifest(instance);
  QNetworkReply* reply = net->put(request, manifestData);

  // Use event loop to block until finished
  // (runs a nested event loop: other UI events are processed meanwhile, so the user can still click around)
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  QByteArray response = reply->readAll();
  reply->deleteLater();

  return code;
}

std::pair<int, QByteArray> ServerUtils::GetManifest(InstancePtr instance) {
  // Get the global Application instance's network manager and settings manager
  auto net = APPLICATION->network();
  auto settings = APPLICATION->settings();

  // Get credentials and instance name to build url
  QString username = settings->get("ServerId").toString();
  QString password = settings->get("ServerPsswd").toString();
  QString serverUrl = settings->get("ModpackSyncServerURL").toString();
  QString instanceName = instance->settings()->get("name").toString();

  if (serverUrl == ""){
    return 0
  }

  // Build Put url with credentials
  QUrl url(serverUrl + "/modpacks/" + instanceName + "/manifest.json");
  url.setUserName(username);
  url.setPassword(password);

  // NOTE: unused here (the GET sends no body); can be removed.
  QByteArray manifestData = ServerUtils::CreateManifest(instance);

  // Send GET request for the manifest.json
  QNetworkRequest request(url);
  QNetworkReply* reply = net->get(request);

  // Use event loop to block until finished
  QEventLoop loop;
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  QByteArray response = reply->readAll();
  reply->deleteLater();

  return std::make_pair(code, response);
}

// Called before each launch. Today it only logs what the server answered; nothing is changed locally.
// A full sync would compare `serverManifest` with `localManifest` and download/remove mods (e.g. with
// ResourceDownloadTask) and update loaders (PackProfile::setComponentVersion) before the launch continues.
int ServerUtils::SyncModpack(InstancePtr instance) {
  auto [code, response] = GetManifest(instance);
  // Parsed but not used yet.
  QJsonDocument serverManifest = QJsonDocument::fromJson(response);

  // Built but not used yet.
  auto localManifest = CreateManifest(instance);
  QString instanceName = instance->name();
  switch (code) {
    case 204:
      qDebug() << "No update for instance:" << instanceName;
      break;
    case 210:
      qDebug() << "Updating modpack" << instanceName;
      break;
    case 300:
      qDebug() << "Manifest for" << instanceName << ":" << response;
      break;
    default:
      qWarning() << "Server error:" << code << response;
  };
  qDebug() << "Server replied with:" << response;
  return 1;
}
