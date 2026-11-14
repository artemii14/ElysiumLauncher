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

class MidnightSurface : public QWidget {
public:
    explicit MidnightSurface(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent*) override;
    void hideEvent(QHideEvent*) override;
private:
    void syncTimer();
    QTimer m_timer;
    double m_phase = 0;
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
