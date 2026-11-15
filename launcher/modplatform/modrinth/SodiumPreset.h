// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QUrl>
#include <QStringList>
#include <utility>
#include <algorithm>
#include <functional>

namespace Elysium::Sodium {
inline QStringList projects() { return {"sodium", "iris", "zoomify", "ok-zoomer", "sodium-extra", "modmenu", "fabric-api"}; }
inline QStringList coreProjects(const QString& loader)
{
    QStringList roots{"iris", "sodium"};
    if (loader == "fabric") roots << "fabric-api";
    return roots;
}
inline bool stableMinecraft(const QString& game)
{
    const auto match = QRegularExpression("^1\\.(\\d+)(?:\\.\\d+)?$").match(game);
    return (match.hasMatch() && match.captured(1).toInt() >= 14) || QRegularExpression("^[2-9][0-9]\\.\\d+(?:\\.\\d+)?$").match(game).hasMatch();
}
inline QJsonObject primaryFile(const QJsonObject& version)
{
    QJsonObject fallback;
    for (const auto& value : version["files"].toArray()) {
        const auto file = value.toObject();
        const auto name = file["filename"].toString();
        const auto url = QUrl(file["url"].toString());
        const auto hash = file["hashes"].toObject()["sha512"].toString();
        if (!QRegularExpression("^[A-Za-z0-9_.+ -]+\\.jar$").match(name).hasMatch() || name.contains("..") ||
            url.scheme() != "https" || url.host() != "cdn.modrinth.com" || !url.userInfo().isEmpty() ||
            url.port(443) != 443 || !QRegularExpression("^[a-fA-F0-9]{128}$").match(hash).hasMatch()) continue;
        if (file["primary"].toBool()) return file;
        if (fallback.isEmpty()) fallback = file;
    }
    return fallback;
}
inline bool supports(const QJsonObject& version, const QString& game, const QString& loader)
{
    return version["game_versions"].toArray().contains(game) && version["loaders"].toArray().contains(loader) && !primaryFile(version).isEmpty();
}
inline QJsonArray newestFirst(QJsonArray array)
{
    QList<QJsonObject> sorted;
    for (const auto& value : array) sorted << value.toObject();
    std::stable_sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a["date_published"].toString() > b["date_published"].toString(); });
    QJsonArray result;
    for (const auto& value : sorted) result.append(value);
    return result;
}

// Resolve Modrinth required/pinned and incompatible dependencies with bounded
// backtracking. Unavailable optional roots may be omitted without changing core.
class Resolver {
public:
    using Versions = std::function<QJsonArray(const QString&)>;
    using Version = std::function<QJsonObject(const QString&)>;
    using Selection = QMap<QString, QJsonObject>;
    using Compatible = std::function<bool(const Selection&)>;
    struct Need { QString project; QString pin; };
    Resolver(QString game, QString loader, Versions versions, Version version, Compatible compatible = {})
        : game(std::move(game)), loader(std::move(loader)), versions(std::move(versions)), version(std::move(version)), compatible(std::move(compatible)) {}
    bool resolve(const QStringList& roots, Selection& result)
    {
        QList<Need> pending;
        for (const auto& root : roots) pending << Need{canonical(root), {}};
        budget = 4096;
        return search(pending, result, {}, 0);
    }
    bool addOptional(const QString& project, Selection& result)
    {
        auto trial = result;
        budget = 2048;
        if (!search({Need{canonical(project), {}}}, trial, {}, 0)) return false;
        result = trial;
        return true;
    }
private:
    QString canonical(const QString& project)
    {
        const auto list = versions(project);
        return list.isEmpty() ? project : list.first().toObject()["project_id"].toString(project);
    }
    bool conflicts(const Selection& selected, const QList<Need>& banned)
    {
        for (const auto& ban : banned)
            if (selected.contains(ban.project) && (ban.pin.isEmpty() || selected[ban.project]["id"].toString() == ban.pin)) return true;
        for (const auto& mod : selected) {
            for (const auto& value : mod["dependencies"].toArray()) {
                const auto dep = value.toObject();
                if (dep["dependency_type"] != "incompatible") continue;
                auto project = dep["project_id"].toString();
                const auto pin = dep["version_id"].toString();
                if (project.isEmpty() && !pin.isEmpty()) project = version(pin)["project_id"].toString();
                if (selected.contains(project) && (pin.isEmpty() || selected[project]["id"].toString() == pin)) return true;
            }
        }
        return false;
    }
    bool search(QList<Need> pending, Selection& selected, QList<Need> banned, int depth)
    {
        if (--budget < 0 || depth > 128 || selected.size() > 96 || conflicts(selected, banned)) return false;
        if (compatible && !compatible(selected)) return false;
        if (pending.isEmpty()) return true;
        auto need = pending.takeFirst();
        if (selected.contains(need.project)) {
            if (!need.pin.isEmpty() && selected[need.project]["id"].toString() != need.pin) return false;
            return search(pending, selected, banned, depth + 1);
        }
        const auto choices = need.pin.isEmpty() ? newestFirst(versions(need.project)) : QJsonArray{version(need.pin)};
        for (const auto& choice : choices) {
            const auto candidate = choice.toObject();
            if (candidate["project_id"].toString() != need.project ||
                (!need.pin.isEmpty() && candidate["id"].toString() != need.pin) || !supports(candidate, game, loader)) continue;
            auto trial = selected;
            trial[need.project] = candidate;
            auto next = pending;
            auto excludes = banned;
            bool valid = true;
            for (const auto& value : candidate["dependencies"].toArray()) {
                const auto dep = value.toObject();
                const auto type = dep["dependency_type"].toString();
                if (type != "required" && type != "incompatible") continue;
                auto project = dep["project_id"].toString();
                const auto pin = dep["version_id"].toString();
                if (project.isEmpty() && !pin.isEmpty()) project = version(pin)["project_id"].toString();
                if (project.isEmpty()) { valid = false; break; }
                if (type == "required") next << Need{project, pin};
                else excludes << Need{project, pin};
            }
            if (valid && search(next, trial, excludes, depth + 1)) { selected = trial; return true; }
        }
        return false;
    }
    QString game, loader;
    Versions versions;
    Version version;
    Compatible compatible;
    int budget = 0;
};
} // namespace Elysium::Sodium
