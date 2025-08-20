#include "ServerCommunication.h"
#include "Application.h"
#include "BaseInstance.h"
#include "ServerCommunication.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QUrl>

QByteArray CreateManifest(InstancePtr instance) {
    // Extract instance metadata
    QString instanceName = instance->settings()->get("name").toString();
    QString instanceType = instance->settings()->get("InstanceType").toString();
    
    // Get mods folder path
    QString modsPath = instance->modsRoot();
    QDir modsDir(modsPath);
    if (!modsDir.exists()) {
        qWarning() << "Mods directory does not exist:" << modsPath;
        return QByteArray(); // Return empty QByteArray (for vanilla)
    }
    
    // Collect all mod files
    QJsonArray modsArray;
    QFileInfoList modFiles = modsDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo & fileInfo : modFiles) {
        QJsonObject modEntry;
        modEntry["fileName"] = fileInfo.fileName();
        modEntry["size"] = static_cast<qint64>(fileInfo.size());
        modEntry["lastModified"] = fileInfo.lastModified().toString(Qt::ISODate);
        modsArray.append(modEntry);
    }
    
    QJsonObject manifest;
    manifest["instanceName"] = instanceName;
    manifest["instanceType"] = instanceType;
    manifest["mods"] = modsArray;
    
    QJsonDocument manifestDoc(manifest);
    QByteArray manifestData = manifestDoc.toJson(QJsonDocument::Compact);
    return manifestData;
}

std::pair<int, QByteArray> PostManifest(InstancePtr instance)
{
    // Get the global Application instance's network manager and settings manager
    auto net = APPLICATION->network();
    auto settings = APPLICATION->settings();
    
    // Build request URL
    QString serverUrl = settings->get("ModpackSyncServerURL").toString();
    QUrl url(serverUrl + "/compareMods");
    
    QByteArray manifestData = CreateManifest(instance);
    
    // Send POST request with manifest.json
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = net->post(request, manifestData);
    
    // Use event loop to block until finished
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray response = reply->readAll();
    reply->deleteLater();
    
    return std::make_pair(code, response);
}

int SyncModpack(InstancePtr instance) {
    auto [code, response] = PostManifest(instance);
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
            qWarning() << "Server error:" << statusCode << response;
    };
    qDebug() << "Server replied with:" << response;
    return 0;
}