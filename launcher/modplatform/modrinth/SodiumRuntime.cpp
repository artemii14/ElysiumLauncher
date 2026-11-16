// SPDX-License-Identifier: GPL-3.0-only
#include "SodiumRuntime.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <archive.h>
#include <archive_entry.h>
#include <toml++/toml.h>
#include <memory>
#include <optional>

namespace Elysium::Sodium {
namespace {
struct SemVersion { QList<quint64> numbers; QStringList pre; };
std::optional<SemVersion> parseVersion(QString text)
{
    text = text.section('+', 0, 0);
    const auto match = QRegularExpression("^([0-9]+(?:\\.[0-9]+)*)(?:-([0-9A-Za-z.-]*))?$").match(text);
    if (!match.hasMatch()) return {};
    SemVersion result;
    for (const auto& part : match.captured(1).split('.')) {
        bool ok = false;
        const auto number = part.toULongLong(&ok);
        if (!ok) return {};
        result.numbers.append(number);
    }
    // Fabric uses a trailing '-' as the lowest prerelease sentinel:
    // ">=26.2- <26.3-" includes stable 26.2. It is not an invalid SemVer.
    if (text.contains('-')) result.pre = match.captured(2).split('.');
    return result;
}
int compare(const SemVersion& a, const SemVersion& b)
{
    for (int i = 0; i < qMax(a.numbers.size(), b.numbers.size()); ++i) {
        if (a.numbers.value(i) != b.numbers.value(i)) return a.numbers.value(i) < b.numbers.value(i) ? -1 : 1;
    }
    if (a.pre.isEmpty() != b.pre.isEmpty()) return a.pre.isEmpty() ? 1 : -1;
    for (int i = 0; i < qMin(a.pre.size(), b.pre.size()); ++i) {
        if (a.pre[i] == b.pre[i]) continue;
        bool an = false, bn = false;
        const auto av = a.pre[i].toULongLong(&an), bv = b.pre[i].toULongLong(&bn);
        if (an && bn) return av < bv ? -1 : 1;
        if (an != bn) return an ? -1 : 1;
        return a.pre[i] < b.pre[i] ? -1 : 1;
    }
    return a.pre.size() == b.pre.size() ? 0 : a.pre.size() < b.pre.size() ? -1 : 1;
}
RangeMatch fabricTerm(const QString& version, const QString& term)
{
    if (term == "*") return RangeMatch::Yes;
    const auto match = QRegularExpression("^(>=|<=|>|<|=|~|\\^)?(.+)$").match(term);
    if (!match.hasMatch()) return RangeMatch::Unknown;
    const auto op = match.captured(1), wanted = match.captured(2);
    const auto actual = parseVersion(version);
    if (wanted.contains('*') || wanted.contains('x') || wanted.contains('X')) {
        if (!op.isEmpty() && op != "=") return RangeMatch::Unknown;
        const auto parts = wanted.split('.');
        if (!actual) return RangeMatch::Unknown;
        bool wildcard = false;
        for (int i = 0; i < parts.size(); ++i) {
            if (parts[i] == "*" || parts[i].compare("x", Qt::CaseInsensitive) == 0) { wildcard = true; continue; }
            bool ok = false;
            const auto number = parts[i].toULongLong(&ok);
            if (!ok || wildcard) return RangeMatch::Unknown;
            if (actual->numbers.value(i) != number) return RangeMatch::No;
        }
        return RangeMatch::Yes;
    }
    const auto required = parseVersion(wanted);
    if (!actual || !required) {
        if (op.isEmpty() || op == "=") return version == wanted ? RangeMatch::Yes : RangeMatch::No;
        return RangeMatch::Unknown;
    }
    const int c = compare(*actual, *required);
    bool yes = false;
    if (op.isEmpty() || op == "=") yes = c == 0;
    else if (op == ">=") yes = c >= 0;
    else if (op == "<=") yes = c <= 0;
    else if (op == ">") yes = c > 0;
    else if (op == "<") yes = c < 0;
    else if (op == "~" || op == "^") {
        auto upper = *required;
        upper.pre.clear();
        int increment = op == "~" ? qMin(1, int(upper.numbers.size()) - 1) : 0;
        if (op == "^") {
            while (increment + 1 < upper.numbers.size() && upper.numbers[increment] == 0) ++increment;
        }
        ++upper.numbers[increment];
        for (int i = increment + 1; i < upper.numbers.size(); ++i) upper.numbers[i] = 0;
        yes = c >= 0 && compare(*actual, upper) < 0;
    } else return RangeMatch::Unknown;
    return yes ? RangeMatch::Yes : RangeMatch::No;
}
}
RangeMatch matchesRuntimeRange(const QString& version, const QJsonValue& predicate, bool maven)
{
    if (predicate.isArray()) {
        bool unknown = false;
        for (const auto& item : predicate.toArray()) {
            const auto result = matchesRuntimeRange(version, item, maven);
            if (result == RangeMatch::Yes) return result;
            unknown |= result == RangeMatch::Unknown;
        }
        return unknown ? RangeMatch::Unknown : RangeMatch::No;
    }
    if (!predicate.isString() || predicate.toString().trimmed().isEmpty()) return RangeMatch::Unknown;
    const auto text = predicate.toString().trimmed();
    if (maven) {
        const auto exact = QRegularExpression("^\\[([^,\\[\\]]+)\\]$").match(text);
        if (exact.hasMatch()) return fabricTerm(version, "=" + exact.captured(1));
        const auto range = QRegularExpression("^([\\[(])([^,]*),([^,]*)([\\])])$").match(text);
        if (!range.hasMatch()) return RangeMatch::Unknown; // do not guess unions or soft requirements
        for (int side = 0; side < 2; ++side) {
            const auto bound = range.captured(side + 2).trimmed();
            if (bound.isEmpty()) continue;
            const QString op = side == 0 ? (range.captured(1) == "[" ? ">=" : ">") : (range.captured(4) == "]" ? "<=" : "<");
            const auto result = fabricTerm(version, op + bound);
            if (result != RangeMatch::Yes) return result;
        }
        return RangeMatch::Yes;
    }
    // Fabric strings are AND; arrays are OR. Whitespace around operators is accepted.
    auto normalized = text;
    normalized.replace(QRegularExpression("(>=|<=|>|<|=|~|\\^)\\s+"), "\\1");
    bool unknown = false;
    for (const auto& term : normalized.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts)) {
        const auto result = fabricTerm(version, term);
        if (result == RangeMatch::No) return result;
        unknown |= result == RangeMatch::Unknown;
    }
    return unknown ? RangeMatch::Unknown : RangeMatch::Yes;
}

QList<RuntimeMod> readRuntimeMods(const QByteArray& jar, const QString& loader, QString& error)
{
    error.clear();
    std::unique_ptr<archive, decltype(&archive_read_free)> reader(archive_read_new(), &archive_read_free);
    archive_read_support_format_zip(reader.get());
    if (archive_read_open_memory(reader.get(), jar.constData(), jar.size()) != ARCHIVE_OK) {
        error = "Invalid mod JAR"; return {};
    }
    QMap<QString, QByteArray> metadata;
    archive_entry* entry = nullptr;
    int count = 0, status = ARCHIVE_OK;
    while ((status = archive_read_next_header(reader.get(), &entry)) == ARCHIVE_OK) {
        if (++count > 20000) { error = "Too many entries in mod JAR"; return {}; }
        const auto path = QString::fromUtf8(archive_entry_pathname(entry));
        if (path != "fabric.mod.json" && path != "META-INF/neoforge.mods.toml" && path != "META-INF/mods.toml" && path != "META-INF/MANIFEST.MF") {
            archive_read_data_skip(reader.get()); continue;
        }
        constexpr int Limit = 1024 * 1024;
        QByteArray bytes(Limit + 1, '\0');
        qsizetype used = 0;
        while (used < bytes.size()) {
            const auto size = archive_read_data(reader.get(), bytes.data() + used, bytes.size() - used);
            if (size < 0) { error = "Cannot read mod metadata"; return {}; }
            if (size == 0) break;
            used += size;
        }
        if (used > Limit || metadata.contains(path)) { error = "Oversized or duplicate mod metadata"; return {}; }
        bytes.resize(used); metadata[path] = bytes;
    }
    if (status != ARCHIVE_EOF) { error = "Truncated mod JAR"; return {}; }
    QList<RuntimeMod> result;
    if (loader == "fabric") {
        QJsonParseError parseError;
        const auto doc = QJsonDocument::fromJson(metadata.value("fabric.mod.json"), &parseError);
        const auto obj = doc.object();
        if (parseError.error != QJsonParseError::NoError || obj["id"].toString().isEmpty() || obj["version"].toString().isEmpty()) {
            error = "Missing or invalid fabric.mod.json"; return {};
        }
        for (const auto& key : QStringList{"depends", "breaks"}) {
            if (obj.contains(key) && !obj[key].isObject()) { error = "Invalid Fabric dependency object"; return {}; }
        }
        RuntimeMod mod{obj["id"].toString(), obj["version"].toString(), obj["depends"].toObject(), obj["breaks"].toObject(), {}, false};
        for (const auto& alias : obj["provides"].toArray()) mod.provides.append(alias.toString());
        result.append(mod);
    } else if (loader == "neoforge") {
        const auto bytes = metadata.value("META-INF/neoforge.mods.toml", metadata.value("META-INF/mods.toml"));
        if (bytes.isEmpty()) { error = "Missing NeoForge mod metadata"; return {}; }
#if TOML_EXCEPTIONS
        toml::table table;
        try { table = toml::parse(bytes.toStdString()); }
        catch (const toml::parse_error&) { error = "Invalid NeoForge metadata"; return {}; }
#else
        auto parsed = toml::parse(bytes.toStdString());
        if (!parsed) { error = "Invalid NeoForge metadata"; return {}; }
        auto table = std::move(parsed).table();
#endif
        const auto mods = table["mods"].as_array();
        if (!mods) { error = "Missing NeoForge mod list"; return {}; }
        for (const auto& item : *mods) {
            const auto modTable = item.as_table();
            if (!modTable) { error = "Invalid NeoForge mod entry"; return {}; }
            RuntimeMod mod;
            mod.mavenRanges = true;
            mod.id = QString::fromStdString((*modTable)["modId"].value_or(std::string()));
            mod.version = QString::fromStdString((*modTable)["version"].value_or(std::string()));
            if (mod.version == "${file.jarVersion}") {
                const auto manifest = QString::fromUtf8(metadata.value("META-INF/MANIFEST.MF"));
                mod.version = QRegularExpression("(?:^|\\n)Implementation-Version: ([^\\r\\n]+)").match(manifest).captured(1);
            }
            if (mod.id.isEmpty() || mod.version.isEmpty() || mod.version.contains("${")) { error = "Unresolved NeoForge mod version"; return {}; }
            const auto deps = table["dependencies"][mod.id.toStdString()].as_array();
            if (deps) for (const auto& dep : *deps) {
                const auto depTable = dep.as_table();
                if (!depTable) { error = "Invalid NeoForge dependency"; return {}; }
                const auto side = (*depTable)["side"].value_or(std::string("BOTH"));
                if (side == "SERVER") continue;
                const auto id = QString::fromStdString((*depTable)["modId"].value_or(std::string()));
                const auto range = QString::fromStdString((*depTable)["versionRange"].value_or(std::string("(,)")));
                const auto type = (*depTable)["type"].value_or(std::string());
                if (id.isEmpty()) continue;
                if (type == "incompatible") mod.breaks[id] = range;
                else if (type.empty() || type == "required" || type == "optional") mod.depends[id] = range;
                else if (type != "discouraged") { error = "Unknown NeoForge dependency type"; return {}; }
            }
            result.append(mod);
        }
    }
    if (result.isEmpty()) error = "No supported mod metadata";
    return result;
}
QString runtimeConflict(const QList<RuntimeMod>& mods, const QMap<QString, QString>& builtins)
{
    auto versions = builtins;
    for (const auto& mod : mods) {
        if (versions.contains(mod.id)) return QString("Duplicate runtime mod ID: %1").arg(mod.id);
        versions[mod.id] = mod.version;
        for (const auto& alias : mod.provides) {
            if (versions.contains(alias)) return QString("Duplicate provided mod ID: %1").arg(alias);
            versions[alias] = mod.version;
        }
    }
    for (const auto& mod : mods) {
        for (bool breaking : {false, true}) {
            const auto rules = breaking ? mod.breaks : mod.depends;
            for (auto rule = rules.begin(); rule != rules.end(); ++rule) {
                if (!versions.contains(rule.key())) continue;
                const auto match = matchesRuntimeRange(versions.value(rule.key()), rule.value(), mod.mavenRanges);
                if (match == RangeMatch::Unknown || (breaking ? match == RangeMatch::Yes : match == RangeMatch::No))
                    return QString("%1 %2: %3 %4 %5 (%6)").arg(mod.id, mod.version,
                        breaking ? QString("breaks") : QString("requires"), rule.key(),
                        QString::fromUtf8(QJsonDocument(QJsonArray{rule.value()}).toJson(QJsonDocument::Compact)), versions.value(rule.key()));
            }
        }
    }
    return {};
}
}
