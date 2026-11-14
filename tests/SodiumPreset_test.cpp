// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include "modplatform/modrinth/SodiumPreset.h"
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
