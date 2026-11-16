// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QStringList>

namespace Elysium::Sodium {
struct RuntimeMod {
    QString id, version;
    QJsonObject depends, breaks;
    QStringList provides;
    bool mavenRanges = false;
};
// No extraction or execution: read bounded metadata directly from a verified JAR.
QList<RuntimeMod> readRuntimeMods(const QByteArray& jar, const QString& loader, QString& error);
enum class RangeMatch { Yes, No, Unknown };
RangeMatch matchesRuntimeRange(const QString& version, const QJsonValue& predicate, bool maven = false);
// Validate constraints between present mods (and explicitly supplied builtins).
// Modrinth remains responsible for discovering missing required projects.
QString runtimeConflict(const QList<RuntimeMod>& mods, const QMap<QString, QString>& builtins = {});
}
