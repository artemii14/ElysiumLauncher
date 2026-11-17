// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include "ui/themes/AccentIcons.h"
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
        QCOMPARE(palette.color(QPalette::Window), QColor("#171722"));
        QCOMPARE(palette.color(QPalette::Highlight), QColor("#a997ff"));
        QVERIFY(palette.color(QPalette::Disabled, QPalette::Text) != palette.color(QPalette::Text));
        const auto qss = theme.appStyleSheet();
        QCOMPARE(qss.count('{'), qss.count('}'));
        QVERIFY(qss.contains("QPushButton#elysiumCreate:disabled"));
        QVERIFY(qss.contains("QWidget#elysiumModernHome"));
    }
    void customAccentReachesPaletteAndPanels()
    {
        HyprlandTheme theme;
        theme.setAccent(QColor("#ed9bc4"));
        QCOMPARE(theme.colorScheme().color(QPalette::Highlight), QColor("#ed9bc4"));
        QVERIFY(theme.appStyleSheet().contains("#ed9bc4"));
        QVERIFY(theme.appStyleSheet().contains("elysiumCollection"));
        QVERIFY(theme.appStyleSheet().contains("elysiumWorkspaceBar"));
        theme.setAccent(QColor());
        QCOMPARE(theme.colorScheme().color(QPalette::Highlight), QColor("#a997ff"));
    }
    void iconsFollowAccentWithoutRecoloringStatus()
    {
        QTemporaryDir dir;
        const auto pink = AccentIcons::build(dir.path(), "hyprland", QColor("#ed9bc4"));
        const auto green = AccentIcons::build(dir.path(), "hyprland", QColor("#88ddaa"));
        QVERIFY(!pink.isEmpty());
        QVERIFY(pink != green);
        QFile icon(dir.filePath(pink + "/scalable/settings.svg"));
        QVERIFY(icon.open(QIODevice::ReadOnly));
        QVERIFY(icon.readAll().contains("#ed9bc4"));
        QFile status(dir.filePath(pink + "/scalable/status-bad.svg"));
        QVERIFY(status.open(QIODevice::ReadOnly));
        QVERIFY(status.readAll().contains("#fb7185"));
        QVERIFY(AccentIcons::build(dir.path(), "external-theme", QColor("red")).isEmpty());
        const auto oldPaths = QIcon::themeSearchPaths();
        const auto oldName = QIcon::themeName();
        QIcon::setThemeSearchPaths(QStringList{dir.path()} + oldPaths);
        QIcon::setThemeName(pink);
        auto liveIcon = QIcon::fromTheme("settings");
        QVERIFY(!liveIcon.isNull());
        const auto before = liveIcon.pixmap(24, 24).toImage();
        QIcon::setThemeName(green);
        const auto after = liveIcon.pixmap(24, 24).toImage();
        QVERIFY(before != after); // existing widgets must update too, not only new icons
        QIcon::setThemeName(oldName);
        QIcon::setThemeSearchPaths(oldPaths);
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
