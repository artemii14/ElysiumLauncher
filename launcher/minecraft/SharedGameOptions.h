// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QFile>
#include <QSaveFile>
#include <QMap>
#include <QStringList>
#include <QVersionNumber>

namespace SharedGameOptions {
// Replace only managed keys; preserve mod options, key bindings and version markers.
inline QByteArray merge(const QByteArray& original, const QMap<QString, QString>& values)
{
    QStringList output;
    for (const auto& line : QString::fromUtf8(original).split('\n')) {
        const auto key = line.section(':', 0, 0);
        if (!values.contains(key) && !line.isEmpty()) output << line;
    }
    for (auto it = values.cbegin(); it != values.cend(); ++it)
        output << it.key() + ':' + it.value();
    return (output.join('\n') + '\n').toUtf8();
}
inline bool apply(const QString& path, const QMap<QString, QString>& values, QString& error, const QString& minecraftVersion = QString())
{
    QFile input(path);
    QByteArray original;
    if (input.exists()) {
        if (!input.open(QIODevice::ReadOnly)) { error = input.errorString(); return false; }
        original = input.readAll();
        if (input.error() != QFileDevice::NoError) { error = input.errorString(); return false; }
        input.close();
        // Keep the first pre-sync settings for manual recovery. Never overwrite this backup.
        if (!QFile::exists(path + ".before-elysium") && !input.copy(path + ".before-elysium")) {
            error = input.errorString(); return false;
        }
    }
    QSaveFile output(path);
    auto managed = values;
    // Without a version marker a fresh options file is treated as legacy input
    // by Minecraft's data fixer. Use the supplied 26.3 marker only for that exact
    // release; never overwrite an existing marker or guess other data versions.
    if (minecraftVersion == QStringLiteral("26.3")) {
        bool hasVersion = false;
        for (const auto& line : original.split('\n'))
            if (line.startsWith("version:")) { hasVersion = true; break; }
        if (!hasVersion) managed.insert("version", "5023");
    }
    const auto data = merge(original, managed);
    if (!output.open(QIODevice::WriteOnly) || output.write(data) != data.size() || !output.commit()) {
        error = output.errorString(); return false;
    }
    QFile written(path);
    if (!written.open(QIODevice::ReadOnly) || written.readAll() != data) {
        error = QStringLiteral("Minecraft options verification failed: ") + path;
        return false;
    }
    return true;
}
}
