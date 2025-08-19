#include "Application.h"
#include "BaseInstance.h"
#include "ServerCommunication.h"

int SyncModpack(InstancePtr instance)
{
    // Get the global Application instance's network manager and Application's
    // setting manager
    auto net = APPLICATION->network();
    auto settings = APPLICATION->settings();

    // Extract name and type - CORRECTION: utiliser -> au lieu de .
    QString instanceName = instance->settings()->get("name").toString();
    QString instanceType = instance->settings()->get("InstanceType").toString();

    // Read app setting
    QString serverUrl = settings->get("ModpackSyncServerURL").toString();

    // Request manifest
    QUrl url(serverUrl + "/manifest.json?name=" + instanceName + "&type=" + instanceType);
    QNetworkRequest request(url);
    auto reply = net->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [reply, instanceName]() {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray response = reply->readAll();

        if (statusCode == 204) {
            qDebug() << "No update for instance:" << instanceName;
        } else if (statusCode == 210) {
            qDebug() << "Updating modpack" << instanceName;
        } else if (statusCode >= 200 && statusCode < 300) {
            qDebug() << "Manifest for" << instanceName << ":" << response;
        } else {
            qWarning() << "Server error:" << statusCode << response;
        }

        reply->deleteLater();
    });

    return 0;
}