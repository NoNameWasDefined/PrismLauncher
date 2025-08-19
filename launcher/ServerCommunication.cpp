#include "Application.h"
#include "ServerCommunication.h"
#include "BaseInstance.h"

int SyncModpack(BaseInstance instance) {
    // Get the global Application instance’s network manager and Application's setting manager
    auto net = APPLICATION->network();
    auto settings = APPLICATION->settings();

    // Extract name and type
    QString instanceName = instance.settings()->get("name").toString();
    QString instanceType = instance.settings()->get("InstanceType").toString();;

    // Read app setting
    QString serverUrl = settings->get("ModpackSyncServerURL").toString();

    // Example: request the manifest
    QNetworkRequest request(QUrl(serverUrl + "/manifest.json?=" + instanceName + "&" + instanceType));
    auto reply = net->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        QByteArray response = reply->readAll();
        qDebug() << "Manifest from server:" << response;
        reply->deleteLater();
    });

    return 0;
}