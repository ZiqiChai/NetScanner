#pragma once
#include <QStringList>
#include <QList>
#include <stdexcept>
QStringList parseTargets(const QString &text);
QList<quint16> parsePorts(const QString &text);
