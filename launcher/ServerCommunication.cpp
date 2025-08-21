
#include "Application.h"
#include "BaseInstance.h"
#include "ServerCommunication.h"

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

    // Extract needed values with regex (since Qt doesn't have native TOML
    // parser)
    QJsonObject modEntry;

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

int ServerUtils::PostManifest(InstancePtr instance) {
  // Get the global Application instance's network manager and settings manager
  auto net = APPLICATION->network();
  auto settings = APPLICATION->settings();

  // Get instance name to build url
  QString instanceName = instance->settings()->get("name").toString();

  // Build request URL
  QString serverUrl = settings->get("ModpackSyncServerURL").toString();
  QUrl url(serverUrl + "/" + instanceName + "/manifest.json");

  QByteArray manifestData = ServerInstance::CreateManifest(instance);

  // Send POST request with manifest.json
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QNetworkReply* reply = net->post(request, manifestData);

  // Use event loop to block until finished
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

  // Get instance name to build url
  QString instanceName = instance->settings()->get("name").toString();

  // Build request URL
  QString serverUrl = settings->get("ModpackSyncServerURL").toString();
  QUrl url(serverUrl + "/" + instanceName + "/manifest.json");

  QByteArray manifestData = ServerInstance::CreateManifest(instance);

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

int ServerUtils::SyncModpack(InstancePtr instance) {
  auto [code, response] = checkManifest(instance);
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
  return 0;
}