// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QRegularExpression>
#include <QSet>
#include <QString>

namespace Elysium {
struct OptiFineLibrary {
    QString relativePath;
    QString fileName;
};
struct OptiFinePlan {
    QJsonObject component;
    QList<OptiFineLibrary> libraries;
    QString error;
    bool valid() const { return error.isEmpty() && !component.isEmpty(); }
};

// Only converts the LaunchWrapper profile emitted by the official installer.
// Never imports remote URLs, arbitrary entry points, JVM arguments or paths.
inline OptiFinePlan planOptiFineImport(const QJsonObject& profile, const QString& minecraftVersion)
{
    OptiFinePlan plan;
    auto fail = [&plan](const QString& error) {
        plan.error = error;
        plan.component = {};
        return plan;
    };
    if (minecraftVersion.isEmpty() || profile.value("inheritsFrom").toString() != minecraftVersion)
        return fail(QStringLiteral("OptiFine and the selected instance must use the same Minecraft version."));
    if (profile.value("mainClass").toString() != "net.minecraft.launchwrapper.Launch")
        return fail(QStringLiteral("Only official LaunchWrapper-based OptiFine profiles are supported."));
    const auto id = profile.value("id").toString();
    if (!id.contains("OptiFine", Qt::CaseInsensitive) || id.size() > 180)
        return fail(QStringLiteral("This is not an OptiFine profile."));
    const auto libraries = profile.value("libraries").toArray();
    if (libraries.isEmpty() || libraries.size() > 16)
        return fail(QStringLiteral("Missing or unsupported OptiFine libraries."));
    static const QRegularExpression coordinate(QStringLiteral("^[A-Za-z0-9_.-]+:[A-Za-z0-9_.-]+:[A-Za-z0-9_.-]+$"));
    QJsonArray localLibraries;
    QSet<QString> fileNames;
    bool hasOptiFine = false;
    bool hasWrapper = false;
    for (const auto& value : libraries) {
        const auto name = value.toObject().value("name").toString();
        if (!coordinate.match(name).hasMatch())
            return fail(QStringLiteral("Invalid library coordinate."));
        const auto parts = name.split(':');
        const auto group = parts[0];
        const auto artifact = parts[1];
        const auto version = parts[2];
        if (group.startsWith('.') || group.endsWith('.') || group.contains("..") ||
            artifact == "." || artifact == ".." || version == "." || version == "..")
            return fail(QStringLiteral("Unsafe library coordinate."));
        const bool optiFine = group == "optifine" && artifact == "OptiFine";
        const bool wrapper = (group == "optifine" && artifact == "launchwrapper-of") ||
                             (group == "net.minecraft" && artifact == "launchwrapper");
        if (!optiFine && !wrapper)
            return fail(QStringLiteral("This profile contains unsupported libraries. No files were imported."));
        hasOptiFine |= optiFine;
        hasWrapper |= wrapper;
        const auto fileName = artifact + '-' + version + ".jar";
        if (fileNames.contains(fileName))
            return fail(QStringLiteral("Duplicate library filename."));
        fileNames.insert(fileName);
        auto path = group;
        path.replace('.', '/');
        plan.libraries.append({path + '/' + artifact + '/' + version + '/' + fileName, fileName});
        localLibraries.append(QJsonObject{{"name", name}, {"MMC-hint", "local"}});
    }
    if (!hasOptiFine || !hasWrapper)
        return fail(QStringLiteral("Both OptiFine and LaunchWrapper libraries are required."));
    plan.component = QJsonObject{
        {"formatVersion", 1}, {"uid", "optifine.OptiFine"}, {"name", "OptiFine"}, {"version", id},
        {"mainClass", "net.minecraft.launchwrapper.Launch"},
        {"+tweakers", QJsonArray{"optifine.OptiFineTweaker"}},
        {"requires", QJsonArray{QJsonObject{{"uid", "net.minecraft"}, {"equals", minecraftVersion}}}},
        {"libraries", localLibraries}
    };
    return plan;
}
} // namespace Elysium
