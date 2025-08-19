#include "ServerCommunication.h"
#include "Application.h"
#include "BaseInstance.h"

int SyncModpack(InstancePtr instance)
{
    // Get the global Application instance's network manager and Application's
    // setting manager
    auto net = APPLICATION->network();
    auto settings = APPLICATION->settings();

    // Extract name and type
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
        switch (statusCode) {
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

        reply->deleteLater();
    });

    return 0;
}