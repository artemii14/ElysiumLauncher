// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QColor>
#include <QDir>
#include <QFile>
#include <QSaveFile>

namespace AccentIcons {
// Only our decorative accent is replaced. Semantic error/warning/success colors
// and user-supplied instance artwork are deliberately left untouched.
inline QByteArray tint(QByteArray svg, const QColor& accent)
{
    return svg.replace("#68e8da", accent.name().toUtf8()).replace("#b7a8ff", accent.name().toUtf8());
}
inline QString build(const QString& root, const QString& source, const QColor& accent)
{
    if ((source != "hyprland" && source != "midnight") || !accent.isValid()) return {};
    const auto id = "elysium-" + source + "-" + accent.name().mid(1);
    const QDir target(root + '/' + id);
    if (QFile::exists(target.filePath("index.theme"))) return id;
    if (!target.mkpath("scalable")) return {};
    QDir originals(":/icons/" + source + "/scalable");
    const auto files = originals.entryList({"*.svg"}, QDir::Files);
    if (files.isEmpty()) return {};
    auto write = [](const QString& path, const QByteArray& data) {
        QSaveFile out(path);
        return out.open(QIODevice::WriteOnly) && out.write(data) == data.size() && out.commit();
    };
    for (const auto& name : files) {
        const auto path = target.filePath("scalable/" + name);
        if (QFile::exists(path)) continue;
        QFile input(originals.filePath(name));
        if (!input.open(QIODevice::ReadOnly) || !write(path, tint(input.readAll(), accent))) return {};
    }
    const QByteArray index = "[Icon Theme]\nName=Elysium accent icons\nInherits=" + source.toUtf8() +
        "\nDirectories=scalable\n\n[scalable]\nSize=24\nType=Scalable\nMinSize=16\nMaxSize=256\n";
    if (!write(target.filePath("index.theme"), index)) return {};
    return id;
}
}
