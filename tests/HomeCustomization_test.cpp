// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QLabel>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QTemporaryDir>
#include "settings/INISettingsObject.h"
#include "ui/widgets/HomeCustomization.h"

class HomeCustomizationTest : public QObject {
    Q_OBJECT
private:
    static void registerSettings(SettingsObject& settings)
    {
        settings.registerSetting("ElysiumMoveButtons", false);
        settings.registerSetting("ElysiumHomePanelOrder", QString());
        settings.registerSetting("ElysiumAnimatedBackground", true);
        settings.registerSetting("ElysiumBackgroundStyle", QStringLiteral("waves"));
        settings.registerSetting("ElysiumAccentColor", QStringLiteral("#a997ff"));
    }
    static bool drop(QWidget* target, const QString& token)
    {
        QMimeData mime;
        mime.setData("application/x-elysium-home-panel", token.toUtf8());
        QDragEnterEvent enter(QPoint(4, 4), Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(target, &enter);
        if (!enter.isAccepted()) return false;
        QDropEvent event(QPointF(4, 4), Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(target, &event);
        return event.isAccepted();
    }
private slots:
    void panelDropOverButtonAndPersistence()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        INISettingsObject settings(directory.filePath("home.cfg"));
        registerSettings(settings);
        QWidget root;
        auto layout = new QVBoxLayout(&root);
        HomePanelOrder order(layout, QStringLiteral("modern"), &settings);
        auto first = order.addPanel(QStringLiteral("library"), QStringLiteral("Library"));
        auto second = order.addPanel(QStringLiteral("instance"), QStringLiteral("Instance"));
        auto target = new QPushButton(QStringLiteral("Play"));
        target->setAcceptDrops(true);
        second->addWidget(target);
        order.restore();
        const auto token = first->parentWidget()->property("elysiumPanelDragToken").toString();
        // Locked by default; dropping onto a descendant must not move anything.
        QVERIFY(!drop(target, token));
        QCOMPARE(layout->itemAt(0)->widget(), first->parentWidget());
        settings.set("ElysiumMoveButtons", true);
        QVERIFY(!drop(target, QStringLiteral("another-window:library")));
        QVERIFY(drop(target, token));
        QCOMPARE(layout->itemAt(0)->widget(), second->parentWidget());
        QVERIFY(!settings.get("ElysiumHomePanelOrder").toString().isEmpty());

        QWidget restoredRoot;
        auto restoredLayout = new QVBoxLayout(&restoredRoot);
        HomePanelOrder restored(restoredLayout, QStringLiteral("modern"), &settings);
        restored.addPanel(QStringLiteral("library"), QStringLiteral("Library"));
        auto restoredSecond = restored.addPanel(QStringLiteral("instance"), QStringLiteral("Instance"));
        restored.restore();
        QCOMPARE(restoredLayout->itemAt(0)->widget(), restoredSecond->parentWidget());
        settings.set("ElysiumHomePanelOrder", QString());
        restored.restore();
        QCOMPARE(restoredLayout->itemAt(0)->widget()->property("elysiumPanelId").toString(), QStringLiteral("library"));
    }
    void emptyLibraryDoesNotHideBackground()
    {
        QTemporaryDir directory;
        INISettingsObject settings(directory.filePath("home.cfg"));
        registerSettings(settings);
        MidnightSurface root(nullptr, &settings);
        root.setObjectName(QStringLiteral("elysiumModernHome"));
        root.resize(800, 600);
        auto layout = new QVBoxLayout(&root);
        auto empty = new QLabel(QStringLiteral("Create your first instance"));
        empty->setAlignment(Qt::AlignCenter);
        layout->addWidget(empty);
        QImage enabled(root.size(), QImage::Format_ARGB32_Premultiplied);
        root.render(&enabled);
        settings.set("ElysiumAnimatedBackground", false);
        QImage disabled(root.size(), QImage::Format_ARGB32_Premultiplied);
        root.render(&disabled);
        // A sample under the empty-label widget must show the scene beneath it.
        QVERIFY(enabled.pixel(100, 550) != disabled.pixel(100, 550));
    }
    void allAnimationPresetsRender()
    {
        QTemporaryDir directory;
        INISettingsObject settings(directory.filePath("home.cfg"));
        registerSettings(settings);
        QPalette palette;
        palette.setColor(QPalette::Window, QColor("#11131d"));
        for (const auto& preset : QStringList{"aurora", "stars", "waves", "rain", "orbit", "grid"}) {
            settings.set("ElysiumBackgroundStyle", preset);
            QImage image(QSize(320, 240), QImage::Format_ARGB32_Premultiplied);
            { QPainter painter(&image); MidnightSurface::paintBackground(painter, image.rect(), palette, nullptr, &settings); }
            int colored = 0;
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x)
                    if (image.pixel(x, y) != palette.color(QPalette::Window).rgba()) ++colored;
            QVERIFY2(colored > 50, qPrintable(preset));
        }
    }
    void backgroundCoversLeftAndRightAndCanBeDisabled()
    {
        QTemporaryDir directory;
        INISettingsObject settings(directory.filePath("home.cfg"));
        registerSettings(settings);
        QWidget root;
        root.setObjectName(QStringLiteral("elysiumModernHome"));
        root.resize(800, 600);
        QPalette palette;
        palette.setColor(QPalette::Window, QColor("#11131d"));
        QImage image(root.size(), QImage::Format_ARGB32_Premultiplied);
        {
            QPainter painter(&image);
            MidnightSurface::paintBackground(painter, image.rect(), palette, &root, &settings);
        }
        const auto base = palette.color(QPalette::Window).rgba();
        int left = 0, right = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixel(x, y) != base) { if (x < 400) ++left; else ++right; }
            }
        }
        QVERIFY(left > 10000);
        QVERIFY(right > 10000);
        settings.set("ElysiumAnimatedBackground", false);
        {
            QPainter painter(&image);
            MidnightSurface::paintBackground(painter, image.rect(), palette, &root, &settings);
        }
        for (int y = 0; y < image.height(); y += 20)
            for (int x = 0; x < image.width(); x += 20) QCOMPARE(image.pixel(x, y), base);
    }
};
QTEST_MAIN(HomeCustomizationTest)
#include "HomeCustomization_test.moc"
