// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QJsonDocument>
#include "modplatform/modrinth/SodiumPreset.h"
#include "modplatform/modrinth/SodiumRuntime.h"
#include "SodiumRuntimeFixtures.h"
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
private slots:
    void lambDynamicLightsDoesNotBlockCore()
    {
        QVERIFY(!projects().contains("lambdynamiclights"));
        QCOMPARE(coreProjects("fabric"), QStringList({"iris", "sodium", "fabric-api"}));
        QCOMPARE(coreProjects("neoforge"), QStringList({"iris", "sodium"}));
    }
    void shaderArchivesAreFilteredAndPlacedSafely()
    {
        auto shader = mod("shader", "shader-v1");
        shader["loaders"] = QJsonArray{"iris", "optifine"};
        shader["game_versions"] = QJsonArray{"26.3"};
        auto file = primaryFile(shader);
        file["filename"] = "ComplementaryUnbound_r5.zip";
        file["size"] = 8 * 1024 * 1024;
        shader["files"] = QJsonArray{file};
        QVERIFY(primaryFile(shader).isEmpty()); // never put a shader ZIP in mods
        QVERIFY(!shaderFile(shader, "26.3").isEmpty());
        QVERIFY(shaderFile(shader, "26.2").isEmpty());
        QCOMPARE(packFile(file, "shaderpacks")["path"].toString(), QString("shaderpacks/ComplementaryUnbound_r5.zip"));
        QVERIFY(packFile(file, "mods").isEmpty());
        QVERIFY(packFile(file, "../shaderpacks").isEmpty());
        file["filename"] = "../escape.zip";
        QVERIFY(packFile(file, "shaderpacks").isEmpty());
        shader["loaders"] = QJsonArray{"optifine"};
        QVERIFY(shaderFile(shader, "26.3").isEmpty());
        shader["loaders"] = QJsonArray{"iris"};
        file["filename"] = "Reimagined.zip";
        file["size"] = 129 * 1024 * 1024;
        shader["files"] = QJsonArray{file};
        QVERIFY(shaderFile(shader, "26.3").isEmpty());
        file["size"] = 1024;
        file["url"] = "https://example.org/shader.zip";
        shader["files"] = QJsonArray{file};
        QVERIFY(shaderFile(shader, "26.3").isEmpty());
        QCOMPARE(shaderProjects(), QStringList({"complementary-unbound", "complementary-reimagined"}));
    }
    void runtimeRanges()
    {
        QCOMPARE(matchesRuntimeRange("26.2", ">=26.2- <26.3-"), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("26.3", ">=26.2- <26.3-"), RangeMatch::No);
        QCOMPARE(matchesRuntimeRange("26.3", ">=26.3-"), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("26.3-beta.1", ">=26.3-"), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("1.21.11", ">=1.21.11- <26.1-"), RangeMatch::Yes);
        QCOMPARE(matchesRuntimeRange("26.2", "~26.2-"), RangeMatch::Yes);
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
        const auto sodium = readRuntimeMods(runtimeJar("fabric.mod.json", QJsonDocument(QJsonObject{{"id", "sodium"}, {"version", "0.9.3-beta.5+mc26.3"}, {"breaks", QJsonObject{{"iris", "<=1.12.0"}}}}).toJson()), "fabric", error);
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
        const auto mods = readRuntimeMods(runtimeJar("META-INF/neoforge.mods.toml", "\n[[mods]]\nmodId=\"sodium\"\nversion=\"0.9.3-beta.5\"\n[[dependencies.sodium]]\nmodId=\"iris\"\ntype=\"incompatible\"\nversionRange=\"(,1.12.0]\"\nside=\"CLIENT\"\n"), "neoforge", error);
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
