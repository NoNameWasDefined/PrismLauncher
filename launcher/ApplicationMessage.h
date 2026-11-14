// File overview: Small serializable message passed between launcher processes (e.g. a second launcher start asking the
// running one to launch an instance or import a pack).

#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>

struct ApplicationMessage {
    QString command;
    QHash<QString, QString> args;

    QByteArray serialize();
    void parse(const QByteArray& input);
};
