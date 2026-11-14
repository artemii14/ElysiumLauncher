// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QWidget>
#include <QStringList>
#include <QTimer>
#include <QPoint>
#include <QList>
#include <QPointer>
class QAbstractButton;
class QBoxLayout;
class QLayout;
class QPainter;
class QLabel;

class MidnightSurface : public QWidget {
public:
    explicit MidnightSurface(QWidget* parent = nullptr);
    static void paintBackground(QPainter& painter, const QRect& area, const QPalette& palette);
protected:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;
private:
    void syncTimer();
    QTimer m_timer;

};

// Reorders existing buttons in their responsive layout, never absolute pixels.
// Disabled by default; only local registered buttons are accepted as drops.
class HomeButtonOrder : public QObject {
public:
    HomeButtonOrder(QWidget* root, const QString& key);
    void restore();
protected:
    bool eventFilter(QObject* object, QEvent* event) override;
private:
    struct Group { QString key; QBoxLayout* layout; QList<QAbstractButton*> buttons; QList<int> positions; };
    void collect(QLayout* layout);
    void arrange(Group& group, const QStringList& ids);
    void save();
    bool unlocked() const;
    QString m_key;
    QString m_session;
    QList<Group> m_groups;
    QPointer<QAbstractButton> m_pressed;
    QPoint m_start;
    int m_counter = 0;
};

// Whole action panels use their own drag handles, independent of button clicks.
class HomePanelOrder : public QObject {
public:
    HomePanelOrder(QBoxLayout* layout, const QString& key);
    QBoxLayout* addPanel(const QString& id, const QString& title);
    void restore();
protected:
    bool eventFilter(QObject*, QEvent*) override;
private:
    struct Panel { QString id; QWidget* widget; QLabel* handle; int position; };
    void arrange(const QStringList& ids);
    void save();
    void refreshHandles();
    QBoxLayout* m_layout;
    QString m_key;
    QString m_session;
    QList<Panel> m_panels;
    QPointer<QWidget> m_pressed;
    QPoint m_start;
};
