// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include "translations/BundledRussianTranslator.h"
#include "translations/ElysiumTranslator.h"
class BundledRussianTranslatorTest : public QObject {
    Q_OBJECT
private slots:
    void offlineCatalogWorks()
    {
        Q_INIT_RESOURCE(russiantranslations);
        BundledRussianTranslator translator;
        QVERIFY(!translator.isEmpty());
        QCOMPARE(translator.translate("MainWindow", "Settings"), QString::fromUtf8("Настройки"));
        QCOMPARE(translator.translate("LanguagePage", "Language"), QString::fromUtf8("Язык"));
        QVERIFY(translator.translate("Unknown", "Untranslated string").isNull());
        QCoreApplication::installTranslator(&translator);
        QCOMPARE(QCoreApplication::translate("MainWindow", "Settings"), QString::fromUtf8("Настройки"));
        QCoreApplication::removeTranslator(&translator);
        QCOMPARE(QCoreApplication::translate("MainWindow", "Settings"), QString("Settings"));
    }
};
QTEST_GUILESS_MAIN(BundledRussianTranslatorTest)
#include "BundledRussianTranslator_test.moc"
