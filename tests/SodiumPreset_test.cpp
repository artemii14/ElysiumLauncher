// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include "modplatform/modrinth/SodiumPreset.h"
#include "modplatform/modrinth/SodiumRuntime.h"
#include <archive.h>
#include <archive_entry.h>
using namespace Elysium::Sodium;

class SodiumPresetTest : public QObject {
    Q_OBJECT
    QJsonObject mod(QString project, QString id, QString date = "2026-01-01") const
    {
        return {{"project_id", project}, {"id", id}, {"date_published", date},
            {"loaders", QJsonArray{"fabric"}}, {"game_versions", QJsonArray{"1.21.1"}},
            {"dependencies", QJsonArray{}},
            {"files", QJsonArray{QJsonObject{{"filename", project + ".jar"}, {"url", "https://cdn.modrinth.com/data/test/mod.jar"},
                {"primary", true}, {"hashes", QJsonObject{{"sha512", QString(128, 'a')}}}}}}};
    }
    QByteArray jar(const QString& path, const QByteArray& metadata) const
    {
        QByteArray bytes(1024 * 1024, '\0');
        size_t used = 0;
        auto writer = archive_write_new();
        archive_write_set_format_zip(writer);
        archive_write_open_memory(writer, bytes.data(), bytes.size(), &used);
        auto entry = archive_entry_new();
        const auto name = path.toUtf8();
        archive_entry_set_pathname(entry, name.constData());
        archive_entry_set_size(entry, metadata.size());
        archive_entry_set_filetype(entry, AE_IFREG);
        archive_entry_set_perm(entry, 0644);
        archive_write_header(writer, entry);
        archive_write_data(writer, metadata.constData(), metadata.size());
        archive_entry_free(entry);
        archive_write_close(writer);
        archive_write_free(writer);
        bytes.resize(used);
        return bytes;
    }
private slots:
    void runtimeRanges()
    {
        QCOMPARE(matchesRuntimeRange("1.11.7+mc26.3", "<=1.12.0"), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("1.12.1+mc26.3", "<=1.12.0"), RangeMatch::No);
        QCOMPARE(matchesRuntimeRange("1.12.0-beta.2", ">=1.12.0"), RangeMatch::No);
        QCOMPARE(matchesRuntimeRange("0.9.3-beta.5+mc26.3", ">=0.9.3-beta.4 <0.10"), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("1.11.7", QJsonArray{"1.10.x", ">=1.11 <1.12"}), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("1.11.7", "[1.12,)", true), RangeMatch::No);
        QCOMPARE(matchesRuntimeRange("1.12.1", "[1.12,)", true), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("1.12.0", "(,1.12.0]", true), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("1.12.0", "[1.12.0]", true), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("1.12.0", "???"), RangeMatch::No);
        QCOMPARE(matchesRuntimeRange("1.12.0", QJsonObject{}), RangeMatch::Unknown);
    }
    void fabricJarRulesCatchScreenshotConflict()
    {
        QString error;
        const auto sodium = readRuntimeMods(jar("fabric.mod.json", R"({"id":"sodium","version":"0.9.3-beta.5+mc26.3","breaks":{"iris":"<=1.12.0"}})"), "fabric", error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        RuntimeMod iris; iris.id = "iris"; iris.version = "1.11.7+mc26.3";
        QVERIFY(runtimeConflict(sodium + QList<RuntimeMod>{iris}).contains("iris"));
        iris.version = "1.12.1+mc26.3";
        QVERIFY(runtimeConflict(sodium + QList<RuntimeMod>{iris}).isEmpty());
        QVERIFY(readRuntimeMods(QByteArray("not a jar"), "fabric", error).isEmpty());
        QVERIFY(!error.isEmpty());
    }
    void neoForgeJarRanges()
    {
        QString error;
        const auto mods = readRuntimeMods(jar("META-INF/neoforge.mods.toml", R"(
[[mods]]
modId="sodium"
version="0.9.3-beta.5"
[[dependencies.sodium]]
modId="iris"
type="incompatible"
versionRange="(,1.12.0]"
side="CLIENT"
)"), "neoforge", error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        RuntimeMod iris; iris.id = "iris"; iris.version = "1.11.7";
        QVERIFY(!runtimeConflict(mods + QList<RuntimeMod>{iris}).isEmpty());
        iris.version = "1.12.1";
        QVERIFY(runtimeConflict(mods + QList<RuntimeMod>{iris}).isEmpty());
    }
    void runtimeValidationBacktracksFor26x()
    {
        for (const auto& game : QStringList{"26.2", "26.3"}) {
            auto sodiumNew = mod("sodium", "new", "2026-02-01");
            auto sodiumOld = mod("sodium", "old");
            auto iris = mod("iris", "iris1");
            sodiumNew["game_versions"] = QJsonArray{game};
            sodiumOld["game_versions"] = QJsonArray{game};
            iris["game_versions"] = QJsonArray{game};
            QMap<QString, QJsonArray> data{{"sodium", {sodiumNew, sodiumOld}}, {"iris", {iris}}};
            auto compatible = [](const Resolver::Selection& selected) {
                QList<RuntimeMod> mods;
                if (selected.contains("sodium")) {
                    RuntimeMod sodium; sodium.id = "sodium"; sodium.version = "0.9.3-beta.5";
                    if (selected["sodium"]["id"] == "new") sodium.breaks["iris"] = "<=1.12.0";
                    mods.append(sodium);
                }
                if (selected.contains("iris")) { RuntimeMod irisMod; irisMod.id = "iris"; irisMod.version = "1.11.7"; mods.append(irisMod); }
                return runtimeConflict(mods).isEmpty();
            };
            Resolver solver(game, "fabric", [&](const QString& p) { return data.value(p); }, [](const QString&) { return QJsonObject{}; }, compatible);
            Resolver::Selection selected;
            QVERIFY(solver.resolve({"iris", "sodium"}, selected));
            QCOMPARE(selected["sodium"]["id"].toString(), QString("old"));
            data["sodium"] = QJsonArray{sodiumNew};
            selected.clear();
            QVERIFY(!solver.resolve({"iris", "sodium"}, selected));
            QVERIFY(selected.isEmpty());
        }
    }

    void stableVersions()
    {
        QVERIFY(stableMinecraft("1.14"));
        QVERIFY(stableMinecraft("1.21.11"));
        QVERIFY(stableMinecraft("26.3"));
        QVERIFY(!stableMinecraft("1.13.2"));
        QVERIFY(!stableMinecraft("1.21-pre1"));
        QVERIFY(!stableMinecraft("26.3-snapshot-1"));
    }
    void filtersLoaderAndMinecraft()
    {
        const auto value = mod("sodium", "v1");
        QVERIFY(supports(value, "1.21.1", "fabric"));
        QVERIFY(!supports(value, "1.21.1", "neoforge"));
        QVERIFY(!supports(value, "1.21", "fabric"));
    }
    void validatesFiles()
    {
        auto value = mod("sodium", "v1");
        auto file = primaryFile(value);
        QVERIFY(!file.isEmpty());
        for (const QString& name : {"../escape.jar", "a/b.jar", "a\\b.jar", "a.exe", "CON:bad.jar"}) {
            auto unsafe = file; unsafe["filename"] = name;
            value["files"] = QJsonArray{unsafe};
            QVERIFY(primaryFile(value).isEmpty());
        }
        file["url"] = "http://cdn.modrinth.com/mod.jar";
        value["files"] = QJsonArray{file};
        QVERIFY(primaryFile(value).isEmpty());
    }
    void pinnedDependencyBacktracks()
    {
        auto sodiumNew = mod("sodium", "new", "2026-02-01");
        auto sodiumOld = mod("sodium", "old");
        auto iris = mod("iris", "iris1");
        iris["dependencies"] = QJsonArray{QJsonObject{{"project_id", "sodium"}, {"version_id", "old"}, {"dependency_type", "required"}}};
        QMap<QString, QJsonArray> data{{"sodium", {sodiumNew, sodiumOld}}, {"iris", {iris}}};
        Resolver solver("1.21.1", "fabric", [&](const QString& p) { return data.value(p); },
            [&](const QString& id) { return id == "old" ? sodiumOld : QJsonObject{}; });
        Resolver::Selection selected;
        QVERIFY(solver.resolve({"sodium", "iris"}, selected));
        QCOMPARE(selected["sodium"]["id"].toString(), QString("old"));
        QCOMPARE(selected.size(), 2);
    }
    void optionalDoesNotBreakCore()
    {
        auto sodium = mod("sodium", "v1");
        auto extra = mod("extra", "v2");
        extra["dependencies"] = QJsonArray{QJsonObject{{"project_id", "sodium"}, {"dependency_type", "incompatible"}}};
        QMap<QString, QJsonArray> data{{"sodium", {sodium}}, {"extra", {extra}}};
        Resolver solver("1.21.1", "fabric", [&](const QString& p) { return data.value(p); }, [](const QString&) { return QJsonObject{}; });
        Resolver::Selection selected;
        QVERIFY(solver.resolve({"sodium"}, selected));
        QVERIFY(!solver.addOptional("extra", selected));
        QCOMPARE(selected.size(), 1);
        QVERIFY(!solver.addOptional("missing", selected));
        QCOMPARE(selected.size(), 1);
    }
    void rejectsIncorrectPinnedResponse()
    {
        auto sodium = mod("sodium", "v1");
        auto iris = mod("iris", "iris1");
        iris["dependencies"] = QJsonArray{QJsonObject{{"project_id", "sodium"}, {"version_id", "expected"}, {"dependency_type", "required"}}};
        Resolver solver("1.21.1", "fabric", [&](const QString& p) { return p == "iris" ? QJsonArray{iris} : QJsonArray{sodium}; },
            [&](const QString&) { return sodium; });
        Resolver::Selection selected;
        QVERIFY(!solver.resolve({"iris"}, selected));
        QVERIFY(selected.isEmpty());
    }
    void missingRequiredFails()
    {
        auto sodium = mod("sodium", "v1");
        sodium["dependencies"] = QJsonArray{QJsonObject{{"project_id", "missing"}, {"dependency_type", "required"}}};
        Resolver solver("1.21.1", "fabric", [&](const QString& p) { return p == "sodium" ? QJsonArray{sodium} : QJsonArray{}; },
            [](const QString&) { return QJsonObject{}; });
        Resolver::Selection selected;
        QVERIFY(!solver.resolve({"sodium"}, selected));
        QVERIFY(selected.isEmpty());
    }
};
QTEST_GUILESS_MAIN(SodiumPresetTest)
#include "SodiumPreset_test.moc"
