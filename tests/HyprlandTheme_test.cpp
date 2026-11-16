// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QFile>
#include <QCheckBox>
#include <QPushButton>
#include <QTableWidget>
#include <QXmlStreamReader>
#include "ui/themes/HyprlandTheme.h"
#include "ui/dialogs/SodiumPresetDialog.h"

class HyprlandThemeTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { Q_INIT_RESOURCE(hyprland); }
    void paletteAndStylesheet()
    {
        HyprlandTheme theme;
        QCOMPARE(theme.id(), QStringLiteral("hyprland"));
        const auto palette = theme.colorScheme();
        QCOMPARE(palette.color(QPalette::Window), QColor("#0c111b"));
        QCOMPARE(palette.color(QPalette::Highlight), QColor("#4ee6d5"));
        QVERIFY(palette.color(QPalette::Disabled, QPalette::Text) != palette.color(QPalette::Text));
        const auto qss = theme.appStyleSheet();
        QCOMPARE(qss.count('{'), qss.count('}'));
        QVERIFY(qss.contains("QPushButton#elysiumCreate:disabled"));
        QVERIFY(qss.contains("QWidget#elysiumModernHome"));
    }
    void embeddedIcons()
    {
        QFile index(QStringLiteral(":/icons/hyprland/index.theme"));
        QVERIFY(index.open(QIODevice::ReadOnly));
        QVERIFY(index.readAll().contains("Inherits=multimc"));
        for (const auto& name : QStringList{"launch", "new", "settings", "accounts", "viewfolder", "status-bad", "appearance"}) {
            QFile svg(QStringLiteral(":/icons/hyprland/scalable/%1.svg").arg(name));
            QVERIFY2(svg.open(QIODevice::ReadOnly), qPrintable(name));
            QXmlStreamReader reader(svg.readAll());
            while (!reader.atEnd()) reader.readNext();
            QVERIFY2(!reader.hasError(), qPrintable(reader.errorString()));
        }
    }
    void partialSodiumRowCannotPrepare()
    {
        // Deferred network reload is never entered in this synchronous test.
        SodiumPresetDialog dialog;
        auto lights = dialog.findChild<QCheckBox*>(QStringLiteral("sodiumDynamicLights"));
        auto shaders = dialog.findChild<QCheckBox*>(QStringLiteral("sodiumComplementaryShaders"));
        QVERIFY(!lights); // LDL is automatic, not a user toggle
        QVERIFY(shaders && shaders->isChecked());
        shaders->setChecked(false);
        auto table = dialog.findChild<QTableWidget*>(QStringLiteral("sodiumPresetTable"));
        auto install = dialog.findChild<QPushButton*>(QStringLiteral("sodiumPresetInstall"));
        QVERIFY(table);
        QVERIFY(install);
        table->setRowCount(1);
        table->setItem(0, 0, new QTableWidgetItem(QStringLiteral("26.3")));
        table->setCurrentCell(0, 0);
        table->selectRow(0); // status column does not exist yet: must not dereference null
        QVERIFY(!install->isEnabled());
        auto status = new QTableWidgetItem(QStringLiteral("Unavailable"));
        status->setData(Qt::UserRole, false);
        table->setItem(0, 3, status);
        table->clearSelection(); table->selectRow(0);
        QVERIFY(!install->isEnabled());
        status->setData(Qt::UserRole, true);
        table->clearSelection(); table->selectRow(0);
        QVERIFY(install->isEnabled());
        table->setRowCount(0);
        QVERIFY(!install->isEnabled());
    }
};
QTEST_MAIN(HyprlandThemeTest)
#include "HyprlandTheme_test.moc"
