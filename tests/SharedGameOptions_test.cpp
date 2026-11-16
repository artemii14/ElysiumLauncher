// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QTemporaryDir>
#include "minecraft/SharedGameOptions.h"
class SharedGameOptionsTest : public QObject {
    Q_OBJECT
private slots:
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
