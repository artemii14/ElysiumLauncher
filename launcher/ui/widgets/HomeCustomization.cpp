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
#include <QElapsedTimer>
#include <QLabel>
#include <QPainterPath>

namespace {
constexpr auto Mime = "application/x-elysium-home-button";

}

MidnightSurface::MidnightSurface(QWidget* parent, SettingsObject* settings)
    : QWidget(parent), m_settings(settings ? settings : APPLICATION->settings())
{
    m_timer.setInterval(50);
    // Viewports can have Show/Hide consumed by QAbstractScrollArea. Observe
    // delivery before its filters, so their animation timer still starts.
    qApp->installEventFilter(this);
    connect(&m_timer, &QTimer::timeout, this, [this] { update(); });
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this] { syncTimer(); });
    if (!settings) connect(APPLICATION, &Application::globalSettingsApplied, this, [this] { syncTimer(); update(); });
}
void MidnightSurface::syncTimer()
{
    const bool active = isVisible() && !window()->isMinimized() &&
        qApp->applicationState() == Qt::ApplicationActive &&
        m_settings->get("ElysiumAnimatedBackground").toBool();
    if (active && !m_timer.isActive()) m_timer.start();
    if (!active) m_timer.stop();
}
void MidnightSurface::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    syncTimer();
}
void MidnightSurface::hideEvent(QHideEvent* event) { m_timer.stop(); QWidget::hideEvent(event); }
bool MidnightSurface::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == QEvent::Hide) m_timer.stop();
    if ((object == window() && event->type() == QEvent::WindowStateChange) ||
        (object == this && (event->type() == QEvent::Show || event->type() == QEvent::ParentChange)))
        QTimer::singleShot(0, this, [this] { syncTimer(); update(); });
    return QWidget::eventFilter(object, event);
}
void MidnightSurface::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    paintBackground(painter, rect(), palette(), this, m_settings);
}
void MidnightSurface::paintBackground(QPainter& painter, const QRect& area, const QPalette& palette, const QWidget* target, SettingsObject* settings)
{
    if (!settings) settings = APPLICATION->settings();
    painter.save();
    painter.setClipRect(area);
    painter.fillRect(area, palette.window());
    if (!settings->get("ElysiumAnimatedBackground").toBool()) { painter.restore(); return; }
    static QElapsedTimer clock;
    if (!clock.isValid()) clock.start();
    const double time = clock.elapsed() / 1000.0;
    const auto style = settings->get("ElysiumBackgroundStyle").toString();
    // Every surface paints a slice of one scene, including the empty library.
    const QWidget* scene = target;
    for (auto parent = target; parent; parent = parent->parentWidget()) {
        if (parent->objectName() == QStringLiteral("elysiumModernHome") ||
            parent->objectName() == QStringLiteral("elysiumCompactHome")) { scene = parent; break; }
    }
    const double width = scene ? scene->width() : area.width();
    const double height = scene ? scene->height() : area.height();
    if (target && scene) painter.translate(-target->mapTo(scene, QPoint(0, 0)));
    else painter.translate(area.topLeft());
    painter.setRenderHint(QPainter::Antialiasing);
    QColor color(settings->get("ElysiumAccentColor").toString());
    if (!color.isValid()) color = QColor("#a997ff");
    if (style == QStringLiteral("stars")) {
        painter.setPen(Qt::NoPen);
        for (int i = 0; i < 70; ++i) {
            auto star = color.lighter(140);
            star.setAlpha(90 + static_cast<int>(65 * (1 + std::sin(time + i))));
            painter.setBrush(star);
            const double x = std::fmod(i * 0.61803398875, 1.0) * width;
            const double y = std::fmod(i * 0.41421356237 + time * (0.008 + (i % 3) * 0.003), 1.0) * height;
            const double radius = 1.0 + (i % 3) * 0.5;
            painter.drawEllipse(QPointF(x, y), radius, radius);
        }
    } else if (style == QStringLiteral("rain")) {
        for (int i = 0; i < 90; ++i) {
            auto drop = color; drop.setAlpha(55 + (i % 5) * 18);
            painter.setPen(QPen(drop, 1.0 + (i % 2) * 0.5));
            const double x = std::fmod(i * 0.61803398875, 1.0) * width;
            const double y = std::fmod(i * 0.41421356237 + time * (0.11 + (i % 5) * 0.025), 1.0) * (height + 36) - 36;
            painter.drawLine(QPointF(x, y), QPointF(x - 5, y + 18 + i % 14));
        }
    } else if (style == QStringLiteral("orbit")) {
        const QPointF center(width * 0.5, height * 0.5);
        painter.setBrush(Qt::NoBrush);
        for (int ring = 0; ring < 3; ++ring) {
            const double radius = qMin(width, height) * (0.18 + ring * 0.1);
            auto line = color; line.setAlpha(42);
            painter.setPen(QPen(line, 1.2));
            painter.drawEllipse(center, radius * 1.55, radius);
            const double angle = time * (0.45 - ring * 0.08) + ring * 2.1;
            const QPointF point = center + QPointF(std::cos(angle) * radius * 1.55, std::sin(angle) * radius);
            QRadialGradient glow(point, 22);
            auto bright = color; bright.setAlpha(190); glow.setColorAt(0, bright);
            bright.setAlpha(0); glow.setColorAt(1, bright);
            painter.setPen(Qt::NoPen);
            painter.setBrush(glow);
            painter.drawEllipse(point, 22, 22);
            painter.setBrush(Qt::NoBrush);
        }
    } else if (style == QStringLiteral("grid")) {
        const double step = qMax(32.0, qMax(width, height) / 24.0);
        const double offset = std::fmod(time * 12, step);
        auto line = color; line.setAlpha(32);
        painter.setPen(QPen(line, 1));
        for (double x = offset - step; x < width; x += step)
            painter.drawLine(QPointF(x, 0), QPointF(x, height));
        for (double y = offset - step; y < height; y += step)
            painter.drawLine(QPointF(0, y), QPointF(width, y));
        const double scan = (0.5 + 0.5 * std::sin(time * 0.4)) * height;
        line.setAlpha(125); painter.setPen(QPen(line, 2));
        painter.drawLine(QPointF(0, scan), QPointF(width, scan));
    } else if (style == QStringLiteral("waves")) {
        painter.setPen(Qt::NoPen);
        for (int layer = 0; layer < 4; ++layer) {
            QPainterPath path;
            path.moveTo(0, height);
            for (int x = 0; x <= static_cast<int>(width) + 12; x += 12) {
                const double y = height * (0.45 + layer * 0.1) +
                    std::sin(x / 160.0 + time * 0.4 + layer) * height * 0.12;
                path.lineTo(x, y);
            }
            path.lineTo(width, height); path.closeSubpath();
            auto wave = color; wave.setAlpha(26 + layer * 9);
            painter.fillPath(path, wave);
        }
    } else {
        // Aurora: broad, visible color blooms instead of almost invisible dots.
        for (int i = 0; i < 3; ++i) {
            const QPointF center(width * (0.5 + 0.38 * std::sin(time * 0.22 + i * 2.1)),
                                 height * (0.5 + 0.32 * std::cos(time * 0.17 + i * 1.8)));
            QRadialGradient gradient(center, qMax(width, height) * 0.7);
            auto glow = color; glow.setAlpha(100 - i * 15);
            gradient.setColorAt(0, glow); glow.setAlpha(0); gradient.setColorAt(1, glow);
            painter.fillRect(QRectF(0, 0, width, height), gradient);
        }
    }
    painter.restore();
}

HomeButtonOrder::HomeButtonOrder(QWidget* root, const QString& key, SettingsObject* settings)
    : QObject(root), m_settings(settings ? settings : APPLICATION->settings()), m_key(key), m_session(QUuid::createUuid().toString())
{
    collect(root->layout());
    restore();
    if (!settings) connect(APPLICATION, &Application::globalSettingsApplied, this, [this] { restore(); });
}
bool HomeButtonOrder::unlocked() const { return m_settings->get("ElysiumMoveButtons").toBool(); }
void HomeButtonOrder::collect(QLayout* layout)
{
    if (!layout) return;
    auto box = qobject_cast<QBoxLayout*>(layout);
    const auto panelId = layout->parentWidget() ? layout->parentWidget()->property("elysiumPanelId").toString() : QString();
    Group group{panelId.isEmpty() ? QString::number(m_counter) : panelId, box, {}, {}};
    for (int i = 0; i < layout->count(); ++i) {
        const auto item = layout->itemAt(i);
        if (auto child = item->layout()) collect(child);
        if (auto widget = item->widget()) {
            if (auto button = qobject_cast<QAbstractButton*>(widget)) {
                if (box && button->objectName() != "elysiumSelectedIcon") {
                    // Panel order must not change a button's persisted identity.
                    button->setProperty("elysiumOrderId", QString::number(i));
                    ++m_counter;
                    group.buttons << button;
                    group.positions << i;
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
    for (int i = 0; i < ordered.size(); ++i) group.layout->insertWidget(group.positions[i], ordered[i]);
    group.buttons = ordered;
}
void HomeButtonOrder::restore()
{
    const auto json = QJsonDocument::fromJson(m_settings->get("ElysiumHomeButtonOrder").toString().toUtf8()).object();
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
    auto settings = m_settings;
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
    if (!button || !unlocked()) { m_pressed.clear(); return QObject::eventFilter(object, event); }
    if (event->type() == QEvent::MouseButtonPress) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) { m_pressed = button; m_start = mouse->position().toPoint(); return true; }
    }
    if (event->type() == QEvent::MouseMove && m_pressed == button) {
        auto mouse = static_cast<QMouseEvent*>(event);
        if ((mouse->buttons() & Qt::LeftButton) &&
            (mouse->position().toPoint() - m_start).manhattanLength() >= QApplication::startDragDistance()) {
            auto mime = new QMimeData;
            mime->setData(Mime, (m_session + ":" + button->property("elysiumOrderId").toString()).toUtf8());
            QDrag drag(button); drag.setMimeData(mime); drag.setPixmap(button->grab());
            m_pressed.clear();
            drag.exec(Qt::MoveAction);
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonRelease && m_pressed == button) { m_pressed.clear(); return true; }
    if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove || event->type() == QEvent::Drop) {
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

HomePanelOrder::HomePanelOrder(QBoxLayout* layout, const QString& key, SettingsObject* settings)
    : QObject(layout->parentWidget()), m_settings(settings ? settings : APPLICATION->settings()), m_layout(layout), m_key(key), m_session(QUuid::createUuid().toString())
{
    qApp->installEventFilter(this);
    if (!settings) connect(APPLICATION, &Application::globalSettingsApplied, this, [this] { m_pressed.clear(); restore(); });
}
QBoxLayout* HomePanelOrder::addPanel(const QString& id, const QString& title)
{
    auto panel = new QWidget(m_layout->parentWidget());
    panel->setObjectName(QStringLiteral("elysiumActionPanel"));
    panel->setProperty("elysiumPanelId", id);
    auto content = new QVBoxLayout(panel);
    content->setContentsMargins(4, 4, 4, 8);
    content->setSpacing(6);
    auto handle = new QLabel(title, panel);
    handle->setObjectName(QStringLiteral("elysiumEyebrow"));
    handle->setMinimumHeight(30);
    handle->setMouseTracking(true);
    handle->setProperty("panelTitle", title);
    handle->setMargin(6);
    content->addWidget(handle);
    panel->setAcceptDrops(true);
    handle->setAcceptDrops(true);
    panel->setProperty("elysiumPanelDragToken", m_session + ":" + id);
    m_panels.append({id, panel, handle, m_layout->count()});
    m_layout->addWidget(panel);
    refreshHandles();
    return content;
}
void HomePanelOrder::refreshHandles()
{
    const bool unlocked = m_settings->get("ElysiumMoveButtons").toBool();
    for (const auto& panel : m_panels) {
        const auto title = panel.handle->property("panelTitle").toString();
        panel.handle->setText((unlocked ? QStringLiteral("↕  ") : QStringLiteral("≡  ")) + title);
        panel.handle->setCursor(unlocked ? Qt::OpenHandCursor : Qt::ArrowCursor);
        panel.handle->setToolTip(unlocked ? tr("Drag this header onto another panel to move the whole block.") : tr("Layout locked. Enable Allow moving buttons in Settings → Appearance, then Apply."));
    }
}
void HomePanelOrder::arrange(const QStringList& ids)
{
    QList<Panel> ordered;
    QStringList seen;
    for (const auto& id : ids) {
        for (const auto& panel : m_panels) {
            if (panel.id == id && !seen.contains(id)) { ordered.append(panel); seen.append(id); }
        }
    }
    for (const auto& panel : m_panels) if (!seen.contains(panel.id)) ordered.append(panel);
    QList<int> positions;
    for (const auto& panel : m_panels) { positions.append(panel.position); m_layout->removeWidget(panel.widget); }
    std::sort(positions.begin(), positions.end());
    for (int i = 0; i < ordered.size(); ++i) {
        ordered[i].position = positions[i];
        m_layout->insertWidget(positions[i], ordered[i].widget);
    }
    m_panels = ordered;
}
void HomePanelOrder::restore()
{
    const auto json = QJsonDocument::fromJson(m_settings->get("ElysiumHomePanelOrder").toString().toUtf8()).object();
    QStringList ids;
    for (const auto& item : json[m_key].toArray()) ids.append(item.toString());
    if (ids.isEmpty()) {
        // Explicit semantic defaults, not translated text or current visual order.
        ids = m_key == QStringLiteral("modern")
            ? QStringList{QStringLiteral("library"), QStringLiteral("instance"), QStringLiteral("launcher")}
            : QStringList{QStringLiteral("play"), QStringLiteral("install"), QStringLiteral("extras")};
    }
    arrange(ids);
    refreshHandles();
}
void HomePanelOrder::save()
{
    auto settings = m_settings;
    auto json = QJsonDocument::fromJson(settings->get("ElysiumHomePanelOrder").toString().toUtf8()).object();
    QJsonArray ids;
    for (const auto& panel : m_panels) ids.append(panel.id);
    json[m_key] = ids;
    settings->set("ElysiumHomePanelOrder", QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)));
}
bool HomePanelOrder::eventFilter(QObject* object, QEvent* event)
{
    const auto type = event->type();
    if (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease && type != QEvent::MouseMove &&
        type != QEvent::DragEnter && type != QEvent::DragMove && type != QEvent::Drop) return false;
    if (!m_settings->get("ElysiumMoveButtons").toBool()) { m_pressed.clear(); return false; }
    auto target = qobject_cast<QWidget*>(object);
    if (!target) return false;
    for (const auto& panel : m_panels) {
        if (target != panel.widget && !panel.widget->isAncestorOf(target)) continue;
        if (object == panel.handle && event->type() == QEvent::MouseButtonPress) {
            const auto mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() != Qt::LeftButton) return false;
            m_pressed = panel.handle; m_start = mouse->position().toPoint();
            panel.handle->grabMouse(); return true;
        }
        if (object == m_pressed && event->type() == QEvent::MouseMove) {
            const auto mouse = static_cast<QMouseEvent*>(event);
            if (!(mouse->buttons() & Qt::LeftButton)) { m_pressed.clear(); return false; }
            if ((mouse->position().toPoint() - m_start).manhattanLength() < QApplication::startDragDistance()) return true;
            const auto id = panel.id;
            auto widget = panel.widget;
            panel.handle->releaseMouse();
            m_pressed.clear();
            QDrag drag(widget);
            auto mime = new QMimeData;
            mime->setData("application/x-elysium-home-panel", (m_session + ":" + id).toUtf8());
            drag.setMimeData(mime);
            // A small handle preview avoids allocating huge images for long panels.
            drag.setPixmap(panel.handle->grab());
            drag.exec(Qt::MoveAction);
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease && m_pressed) {
            m_pressed->releaseMouse(); m_pressed.clear(); return true;
        }
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove || event->type() == QEvent::Drop) {
            const auto drop = static_cast<QDropEvent*>(event);
            const auto data = QString::fromUtf8(drop->mimeData()->data("application/x-elysium-home-panel"));
            if (!data.startsWith(m_session + ":")) return false;
            const auto source = data.mid(m_session.size() + 1);
            QStringList ids;
            for (const auto& entry : m_panels) ids.append(entry.id);
            if (!ids.contains(source)) return false;
            drop->setDropAction(Qt::MoveAction); drop->accept();
            if (event->type() == QEvent::Drop) {
                ids.move(ids.indexOf(source), ids.indexOf(panel.id));
                arrange(ids); save();
            }
            return true;
        }
    }
    return false;
}
