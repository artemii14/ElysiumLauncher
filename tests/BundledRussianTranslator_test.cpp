// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include "translations/BundledRussianTranslator.h"
#include "translations/ElysiumTranslator.h"
#include "translations/LiveTranslation.h"
#include "minecraft/SharedGameOptionsSchema.h"
#include <QVBoxLayout>
#include <QPushButton>
class BundledRussianTranslatorTest : public QObject {
    Q_OBJECT
private slots:
    void everyBundledLanguageLoads()
    {
        Q_INIT_RESOURCE(russiantranslations);
        for (const auto& code : {"ru", "de", "fr", "es", "pt_BR", "uk", "zh"}) {
            BundledTranslator translator(nullptr, code);
            QVERIFY2(!translator.isEmpty(), code);
            QVERIFY2(!translator.translate("LanguagePage", "Language").isEmpty(), code);
        }
    }
    void customCatalogIsCompleteAndKeepsPlaceholders()
    {
        for (auto it = CustomLanguage::catalog().cbegin(); it != CustomLanguage::catalog().cend(); ++it) {
            QCOMPARE(it.value().size(), 8);
            for (const auto& text : it.value()) {
                QVERIFY2(!text.isEmpty(), qPrintable(it.key()));
                QCOMPARE(text.count("%1"), it.key().count("%1"));
            }
        }
        for (const auto& option : SharedGameOptions::schema()) {
            QVERIFY2(!CustomLanguage::canonical(option.group).isEmpty(), qPrintable(option.group));
            // Hotbar titles contain an argument and are bound using the template.
            if (!option.key.startsWith("key_key.hotbar."))
                QVERIFY2(!CustomLanguage::canonical(option.title).isEmpty(), qPrintable(option.title));
        }
    }
    void liveWidgetsFollowLanguageWithoutChangingIds()
    {
        CustomLanguage::language = "ru";
        QWidget root;
        auto layout = new QVBoxLayout(&root);
        auto play = new QPushButton(QString::fromUtf8("Играть"));
        auto choice = new QComboBox;
        choice->addItem(QString::fromUtf8("Графика"), "graphics-id");
        auto user = new QLabel(QString::fromUtf8("Играть"));
        user->setProperty("elysiumUserContent", true);
        layout->addWidget(play); layout->addWidget(choice); layout->addWidget(user);
        LiveTranslation binding(&root);
        for (const auto& code : {"en", "de", "fr", "es", "pt_BR", "uk", "zh", "ru"}) {
            CustomLanguage::language = code;
            QSignalSpy changed(choice, &QComboBox::currentTextChanged);
            QEvent event(QEvent::LanguageChange);
            QCoreApplication::sendEvent(&root, &event);
            QCOMPARE(play->text(), CustomLanguage::text(QString::fromUtf8("Играть")));
            QCOMPARE(choice->currentData().toString(), QString("graphics-id"));
            QCOMPARE(user->text(), QString::fromUtf8("Играть"));
            QCOMPARE(changed.count(), 0);
        }
    }
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
QTEST_MAIN(BundledRussianTranslatorTest)
#include "BundledRussianTranslator_test.moc"
