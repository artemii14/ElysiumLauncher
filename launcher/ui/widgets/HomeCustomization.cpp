// SPDX-License-Identifier: GPL-3.0-only
#include "HomeCustomization.h"
#include "Application.h"
#include "settings/SettingsObject.h"
#include <QAbstractButton>
#include <QApplication>
#include <QBoxLayout>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QUuid>
#include <cmath>
#include <algorithm>

namespace {
constexpr auto Mime = "application/x-elysium-home-button";
QColor accent()
{
    QColor color(APPLICATION->settings()->get("ElysiumAccentColor").toString());
    return color.isValid() ? color : QColor("#a997ff");
}
}

MidnightSurface::MidnightSurface(QWidget* parent) : QWidget(parent)
{
    m_timer.setInterval(50); // restrained 20 FPS; no work when hidden or inactive
    connect(&m_timer, &QTimer::timeout, this, [this] { m_phase += 0.006; update(); syncTimer(); });
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this] { syncTimer(); });
    connect(APPLICATION, &Application::globalSettingsApplied, this, [this] { syncTimer(); update(); });
}
void MidnightSurface::syncTimer()
{
    const bool active = isVisible() && !window()->isMinimized() &&
        qApp->applicationState() == Qt::ApplicationActive &&
        APPLICATION->settings()->get("ElysiumAnimatedBackground").toBool();
    if (active) m_timer.start(); else m_timer.stop();
}
void MidnightSurface::showEvent(QShowEvent* event) { QWidget::showEvent(event); syncTimer(); }
void MidnightSurface::hideEvent(QHideEvent* event) { m_timer.stop(); QWidget::hideEvent(event); }
void MidnightSurface::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());
    if (!APPLICATION->settings()->get("ElysiumAnimatedBackground").toBool()) return;
    for (int i = 0; i < 3; ++i) {
        const QPointF center(width() * (0.5 + 0.38 * std::sin(m_phase + i * 2.1)),
                             height() * (0.5 + 0.32 * std::cos(m_phase * 0.7 + i * 1.8)));
        QRadialGradient gradient(center, qMax(width(), height()) * 0.6);
        auto color = accent(); color.setAlpha(24 - i * 4);
        gradient.setColorAt(0, color); color.setAlpha(0); gradient.setColorAt(1, color);
        painter.fillRect(rect(), gradient);
    }
}

HomeButtonOrder::HomeButtonOrder(QWidget* root, const QString& key)
    : QObject(root), m_key(key), m_session(QUuid::createUuid().toString())
{
    collect(root->layout());
    restore();
    connect(APPLICATION, &Application::globalSettingsApplied, this, [this] { restore(); });
}
bool HomeButtonOrder::unlocked() const { return APPLICATION->settings()->get("ElysiumMoveButtons").toBool(); }
void HomeButtonOrder::collect(QLayout* layout)
{
    if (!layout) return;
    auto box = qobject_cast<QBoxLayout*>(layout);
    Group group{QString::number(m_counter), box, {}, {}};
    for (int i = 0; i < layout->count(); ++i) {
        const auto item = layout->itemAt(i);
        if (auto child = item->layout()) collect(child);
        if (auto widget = item->widget()) {
            if (auto button = qobject_cast<QAbstractButton*>(widget)) {
                if (box && button->objectName() != "elysiumSelectedIcon") {
                    button->setProperty("elysiumOrderId", QString::number(m_counter++));
                    group.buttons << button;
                    group.slots << i;
                    button->installEventFilter(this);
                    button->setAcceptDrops(true);
                }
            } else collect(widget->layout());
        }
    }
    if (box && group.buttons.size() > 1) m_groups << group;
}
void HomeButtonOrder::arrange(Group& group, const QStringList& ids)
{
    QList<QAbstractButton*> ordered;
    for (const auto& id : ids)
        for (auto button : group.buttons)
            if (button->property("elysiumOrderId").toString() == id && !ordered.contains(button)) ordered << button;
    for (auto button : group.buttons) if (!ordered.contains(button)) ordered << button;
    for (auto button : group.buttons) group.layout->removeWidget(button);
    for (int i = 0; i < ordered.size(); ++i) group.layout->insertWidget(group.slots[i], ordered[i]);
    group.buttons = ordered;
}
void HomeButtonOrder::restore()
{
    const auto json = QJsonDocument::fromJson(APPLICATION->settings()->get("ElysiumHomeButtonOrder").toString().toUtf8()).object();
    for (auto& group : m_groups) {
        QStringList ids;
        const auto key = m_key + ":" + group.key;
        for (const auto& value : json[key].toArray()) ids << value.toString();
        if (ids.isEmpty()) {
            // IDs are assigned in default source order, not in current visual order.
            for (auto button : group.buttons) ids << button->property("elysiumOrderId").toString();
            std::sort(ids.begin(), ids.end(), [](const QString& a, const QString& b) { return a.toInt() < b.toInt(); });
        }
        arrange(group, ids);
    }
}
void HomeButtonOrder::save()
{
    auto settings = APPLICATION->settings();
    auto json = QJsonDocument::fromJson(settings->get("ElysiumHomeButtonOrder").toString().toUtf8()).object();
    for (const auto& group : m_groups) {
        QJsonArray ids;
        for (auto button : group.buttons) ids.append(button->property("elysiumOrderId").toString());
        json[m_key + ":" + group.key] = ids;
    }
    settings->set("ElysiumHomeButtonOrder", QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)));
}
bool HomeButtonOrder::eventFilter(QObject* object, QEvent* event)
{
    auto button = qobject_cast<QAbstractButton*>(object);
    if (!button || !unlocked()) return QObject::eventFilter(object, event);
    if (event->type() == QEvent::MouseButtonPress) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) { m_pressed = button; m_start = mouse->position().toPoint(); return true; }
    }
    if (event->type() == QEvent::MouseMove && m_pressed == button) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if ((mouse->position().toPoint() - m_start).manhattanLength() >= QApplication::startDragDistance()) {
            auto mime = new QMimeData;
            mime->setData(Mime, (m_session + ":" + button->property("elysiumOrderId").toString()).toUtf8());
            QDrag drag(button); drag.setMimeData(mime); drag.setPixmap(button->grab());
            m_pressed.clear();
            drag.exec(Qt::MoveAction);
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonRelease && m_pressed == button) { m_pressed.clear(); return true; }
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::Drop) {
        auto drop = static_cast<QDropEvent*>(event);
        const auto data = QString::fromUtf8(drop->mimeData()->data(Mime));
        if (!data.startsWith(m_session + ":")) return false;
        const auto id = data.mid(m_session.size() + 1);
        for (auto& group : m_groups) {
            if (!group.buttons.contains(button)) continue;
            QAbstractButton* source = nullptr;
            for (auto candidate : group.buttons) if (candidate->property("elysiumOrderId").toString() == id) source = candidate;
            if (!source) return false;
            drop->acceptProposedAction();
            if (event->type() == QEvent::Drop) {
                QStringList ids;
                auto reordered = group.buttons;
                reordered.move(reordered.indexOf(source), reordered.indexOf(button));
                for (auto item : reordered) ids << item->property("elysiumOrderId").toString();
                arrange(group, ids); save();
            }
            return true;
        }
    }
    return QObject::eventFilter(object, event);
}
