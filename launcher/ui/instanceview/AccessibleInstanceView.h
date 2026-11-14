// File overview: Accessibility (screen reader) support for the instance view.

#pragma once

#include <QObject>
#include <QString>
class QAccessibleInterface;

QAccessibleInterface* groupViewAccessibleFactory(const QString& classname, QObject* object);
