// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QTemporaryDir>
#include "minecraft/SharedGameOptions.h"
#include "minecraft/SharedGameOptionsSchema.h"
class SharedGameOptionsTest : public QObject {
    Q_OBJECT
private slots:
    void schemaIsCompleteAndValid()
    {
        QSet<QString> keys;
        QSet<QString> groups;
        for (const auto& option : SharedGameOptions::schema()) {
            QVERIFY2(!keys.contains(option.key), qPrintable(option.key));
            QVERIFY(option.initial >= option.minimum);
            QVERIFY(option.initial <= option.maximum);
            QVERIFY(option.divisor > 0);
            if (!option.choices.isEmpty()) QCOMPARE(option.choices.size(), option.maximum + 1);
            keys.insert(option.key);
            groups.insert(option.group);
        }
        QVERIFY(keys.size() >= 100);
        QCOMPARE(groups.size(), 8);
    }
    void serializesMinecraftFormatsAndClampsCorruptSettings()
    {
        const auto values = SharedGameOptions::serialize({{"fov", 90}, {"mouseSensitivity", 100},
            {"soundCategory_master", 35}, {"enableVsync", true}, {"maxFps", 99999},
            {"graphicsMode", -10}, {"lang", 999}, {"mainHand", 0}});
        QCOMPARE(values.value("fov"), QString("0.5"));
        QCOMPARE(values.value("mouseSensitivity"), QString("0.5"));
        QCOMPARE(values.value("soundCategory_master"), QString("0.35"));
        QCOMPARE(values.value("enableVsync"), QString("true"));
        QCOMPARE(values.value("maxFps"), QString("260"));
        QCOMPARE(values.value("graphicsMode"), QString("0"));
        QCOMPARE(values.value("mainHand"), QString("left"));
        QCOMPARE(values.value("lang"), QString("ja_jp"));
        QCOMPARE(values.value("key_key.attack"), QString("key.mouse.left"));
        QCOMPARE(values.value("key_key.jump"), QString("key.keyboard.space"));
        QCOMPARE(values.value("renderClouds"), QString("true"));
        QCOMPARE(values.value("modelPart_hat"), QString("true"));
        QCOMPARE(values.value("chatLinksPrompt"), QString("true"));
        QVERIFY(!values.contains("version"));
        QVERIFY(!values.contains("resourcePacks"));
    }
    void createsMissingOptionsFile()
    {
        QTemporaryDir dir;
        QString error;
        const auto path = dir.filePath("options.txt");
        QVERIFY2(SharedGameOptions::apply(path, SharedGameOptions::serialize({}), error), qPrintable(error));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("mouseSensitivity:0.5\n"));
    }
    void preservesUnmanagedAndDeduplicates()
    {
        const QByteArray original("version:4000\r\nfov:1\r\nkey_key.attack:key.mouse.left\r\nfov:0\r\nmod:value:extra\n");
        const QMap<QString, QString> values{{"fov", "0.5"}, {"maxFps", "120"}};
        auto result = SharedGameOptions::merge(original, values);
        QVERIFY(result.contains("version:4000"));
        QVERIFY(result.contains("key_key.attack:key.mouse.left"));
        QVERIFY(result.contains("mod:value:extra"));
        QCOMPARE(result.count("fov:"), 1);
        QVERIFY(result.contains("fov:0.5\n"));
        QCOMPARE(SharedGameOptions::merge(result, values), result);
    }
    void backsUpOnlyOnce()
    {
        QTemporaryDir dir;
        const auto path = dir.filePath("options.txt");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("fov:1\nuntouched:true\n"); file.close();
        QString error;
        QVERIFY2(SharedGameOptions::apply(path, {{"fov", "0"}}, error), qPrintable(error));
        QVERIFY2(SharedGameOptions::apply(path, {{"fov", "0.5"}}, error), qPrintable(error));
        QFile backup(path + ".before-elysium");
        QVERIFY(backup.open(QIODevice::ReadOnly));
        QCOMPARE(backup.readAll(), QByteArray("fov:1\nuntouched:true\n"));
        QVERIFY(!SharedGameOptions::apply(dir.filePath("missing/options.txt"), {{"fov", "0"}}, error));
    }
};
QTEST_GUILESS_MAIN(SharedGameOptionsTest)
#include "SharedGameOptions_test.moc"
