// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include "minecraft/OptiFineProfile.h"
#include "minecraft/OneSixVersionFormat.h"

class OptiFineProfileTest : public QObject {
    Q_OBJECT
    QJsonObject profile() const
    {
        return {{"id", "1.20.1-OptiFine_HD_U_I6"}, {"inheritsFrom", "1.20.1"},
                {"mainClass", "net.minecraft.launchwrapper.Launch"},
                {"libraries", QJsonArray{
                    QJsonObject{{"name", "optifine:OptiFine:1.20.1_HD_U_I6"}},
                    QJsonObject{{"name", "optifine:launchwrapper-of:2.3"}}}}};
    }
private slots:
    void convertsToPrismComponent()
    {
        const auto plan = Elysium::planOptiFineImport(profile(), "1.20.1");
        QVERIFY2(plan.valid(), qPrintable(plan.error));
        QCOMPARE(plan.libraries.size(), 2);
        QCOMPARE(plan.libraries[0].relativePath,
                 QStringLiteral("optifine/OptiFine/1.20.1_HD_U_I6/OptiFine-1.20.1_HD_U_I6.jar"));
        auto parsed = OneSixVersionFormat::versionFileFromJson(QJsonDocument(plan.component), "optifine.json", false);
        QCOMPARE(parsed->uid, QStringLiteral("optifine.OptiFine"));
        QCOMPARE(parsed->mainClass, QStringLiteral("net.minecraft.launchwrapper.Launch"));
        QCOMPARE(parsed->addTweakers, QStringList{"optifine.OptiFineTweaker"});
        QCOMPARE(parsed->libraries.size(), 2);
        QVERIFY(parsed->libraries[0]->isLocal());
    }
    void rejectsDifferentMinecraftVersion()
    {
        QVERIFY(!Elysium::planOptiFineImport(profile(), "1.21").valid());
        QVERIFY(!Elysium::planOptiFineImport(profile(), "").valid());
    }
    void rejectsForeignEntryPoint()
    {
        auto json = profile();
        json["mainClass"] = "other.Main";
        QVERIFY(!Elysium::planOptiFineImport(json, "1.20.1").valid());
    }
    void rejectsUnsafeOrUnsupportedLibraries_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("parent") << "optifine:OptiFine:..";
        QTest::newRow("slash") << "optifine:OptiFine:../../outside";
        QTest::newRow("backslash") << "optifine:OptiFine:..\\outside";
        QTest::newRow("drive") << "C:optifine:OptiFine:1";
        QTest::newRow("classifier") << "optifine:OptiFine:1:other";
        QTest::newRow("foreign") << "other:payload:1";
        QTest::newRow("missing") << "";
    }
    void rejectsUnsafeOrUnsupportedLibraries()
    {
        QFETCH(QString, name);
        auto json = profile();
        json["libraries"] = QJsonArray{QJsonObject{{"name", name}}};
        QVERIFY(!Elysium::planOptiFineImport(json, "1.20.1").valid());
    }
    void rejectsMissingAndDuplicateLibraries()
    {
        auto json = profile();
        auto libraries = json["libraries"].toArray();
        libraries.removeLast();
        json["libraries"] = libraries;
        QVERIFY(!Elysium::planOptiFineImport(json, "1.20.1").valid());
        json = profile();
        libraries = json["libraries"].toArray();
        libraries.append(libraries.first());
        json["libraries"] = libraries;
        QVERIFY(!Elysium::planOptiFineImport(json, "1.20.1").valid());
    }
    void neverCopiesRemoteUrlsOrJvmArguments()
    {
        auto json = profile();
        json["+jvmArgs"] = QJsonArray{"-javaagent:untrusted.jar"};
        auto libraries = json["libraries"].toArray();
        auto library = libraries[0].toObject();
        library["url"] = "https://untrusted.invalid/";
        library["downloads"] = QJsonObject{{"artifact", QJsonObject{{"path", "../outside"}}}};
        libraries[0] = library;
        json["libraries"] = libraries;
        auto plan = Elysium::planOptiFineImport(json, "1.20.1");
        QVERIFY(plan.valid());
        QVERIFY(!plan.component.contains("+jvmArgs"));
        auto converted = plan.component["libraries"].toArray()[0].toObject();
        QVERIFY(!converted.contains("url"));
        QVERIFY(!converted.contains("downloads"));
        QCOMPARE(converted["MMC-hint"].toString(), QStringLiteral("local"));
    }
};
QTEST_GUILESS_MAIN(OptiFineProfileTest)
#include "OptiFineProfile_test.moc"
