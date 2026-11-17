// SPDX-License-Identifier: GPL-3.0-only
// Independent Hyprland-inspired styling, not an official Hyprland component.
#pragma once
#include "DarkTheme.h"
#include <QObject>

class HyprlandTheme final : public DarkTheme {
    QColor m_accent{"#a997ff"};
public:
    void setAccent(const QColor& color) { m_accent = color.isValid() ? color : QColor("#a997ff"); }
    QString id() override { return QStringLiteral("hyprland"); }
    QString name() override { return QStringLiteral("Elysium • Hyprland"); }
    QString tooltip() override { return QObject::tr("Tiled workspace, muted lavender borders and compact controls. No compositor required."); }
    QColor fadeColor() override { return QColor("#171722"); }
    double fadeAmount() override { return 0.3; }
    QPalette colorScheme() override
    {
        auto palette = DarkTheme::colorScheme();
        palette.setColor(QPalette::Window, QColor("#171722"));
        palette.setColor(QPalette::Base, QColor("#20202f"));
        palette.setColor(QPalette::AlternateBase, QColor("#292839"));
        palette.setColor(QPalette::Button, QColor("#292839"));
        for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::ToolTipText})
            palette.setColor(role, QColor("#ece8f5"));
        palette.setColor(QPalette::Highlight, m_accent);
        palette.setColor(QPalette::HighlightedText, QColor("#191722"));
        palette.setColor(QPalette::Link, m_accent.lighter(118));
        palette.setColor(QPalette::PlaceholderText, QColor("#a8a2b8"));
        palette.setColor(QPalette::ToolTipBase, QColor("#292839"));
        for (auto role : {QPalette::Text, QPalette::WindowText, QPalette::ButtonText})
            palette.setColor(QPalette::Disabled, role, QColor("#817b94"));
        palette.setColor(QPalette::Inactive, QPalette::Highlight, QColor("#48415f"));
        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor("#ece8f5"));
        return palette;
    }
    QString appStyleSheet() override
    {
        return QStringLiteral(R"QSS(
QToolTip { color: #ece8f5; background: #292839; border: 1px solid #a997ff; padding: 7px; }
QToolBar { background: #20202f; border: none; spacing: 6px; padding: 6px; }
QPushButton, QToolButton { color: #ece8f5; background: #292839; border: 1px solid #4b435f; border-radius: 9px; padding: 8px 12px; }
QPushButton:hover, QToolButton:hover { background: #373148; border-color: #a997ff; }
QPushButton:pressed, QToolButton:pressed, QToolButton:checked { background: #50415f; border-color: #c5b6df; }
QPushButton:focus, QToolButton:focus { border: 1px solid #c5b6df; }
QPushButton:disabled, QToolButton:disabled { color: #817b94; background: #20202f; border-color: #343043; }
QPushButton#elysiumCreate, QWidget#elysiumRail QPushButton#elysiumPrimaryPlay { color: #191722; background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #a997ff,stop:1 #c5b6df); border: 1px solid #a997ff; font-weight: bold; }
QPushButton#elysiumCreate:disabled, QWidget#elysiumRail QPushButton#elysiumPrimaryPlay:disabled { color: #817b94; background: #292839; border-color: #343043; }
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: #20202f; color: #ece8f5; border: 1px solid #4b435f; border-radius: 8px; padding: 7px; selection-background-color: #a997ff; selection-color: #191722; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border-color: #a997ff; }
QComboBox QAbstractItemView { background: #20202f; color: #ece8f5; selection-background-color: #50415f; selection-color: #ece8f5; }
QAbstractItemView { selection-background-color: #50415f; selection-color: #ece8f5; alternate-background-color: #292839; }
QHeaderView::section { background: #292839; color: #ccc0dd; padding: 7px; border: none; border-bottom: 1px solid #4b435f; }
QGroupBox { border: 1px solid #4b435f; border-radius: 10px; margin-top: 12px; padding-top: 12px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; color: #c5b6df; padding: 0 5px; }
QMenu { background: #20202f; color: #ece8f5; border: 1px solid #655477; padding: 6px; }
QMenu::item { padding: 7px 25px; border-radius: 5px; }
QMenu::item:selected { background: #50415f; }
QMenu::item:disabled { color: #817b94; }
QMenuBar, QStatusBar { background: #171722; color: #ccc0dd; }
QMenuBar::item:selected { background: #50415f; }
QWidget#elysiumRail { background: #20202f; border: 1px solid #655477; border-radius: 12px; }
QWidget#elysiumActionPanel { background: #252435; border: 1px solid #4b435f; border-radius: 9px; }
QWidget#elysiumCollection { background: #20202f; border: 1px solid #655477; border-radius: 12px; }
QWidget#elysiumWorkspaceBar { background: #292839; border: 1px solid #4b435f; border-radius: 10px; }
QLabel#elysiumWorkspaceBrand { color: #ece8f5; font-size: 15px; font-weight: bold; letter-spacing: 2px; }
QLabel#elysiumWorkspaceHint { color: #aaa3ba; font-size: 11px; }
QLabel#elysiumSelectedName { color: #ece8f5; font-size: 16px; font-weight: bold; }
QWidget#elysiumActionPanel QLabel#elysiumEyebrow { background: #302b41; border-radius: 5px; color: #c5b6df; }
QScrollArea { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 9px; margin: 2px; }
QScrollBar::handle:vertical { background: #655477; border-radius: 4px; min-height: 30px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QWidget#elysiumRail QPushButton, QWidget#elysiumRail QToolButton { text-align: left; padding: 8px; background: transparent; border: 1px solid transparent; }
QWidget#elysiumRail QPushButton:hover, QWidget#elysiumRail QToolButton:hover { background: #373148; border-color: #a997ff; }
QWidget#elysiumRail QToolButton::menu-indicator { subcontrol-position: right center; right: 5px; }
QWidget#elysiumHeader { background: #252435; border: none; border-bottom: 1px solid #4b435f; border-top-left-radius: 10px; border-top-right-radius: 10px; }
QWidget#elysiumModernHome, QWidget#elysiumCompactHome, QLabel#elysiumEmpty { background: transparent; }
QLabel#elysiumEyebrow { color: #c5b6df; font-size: 11px; font-weight: bold; }
QLabel#elysiumTitle { color: #ece8f5; font-size: 26px; font-weight: bold; }
QLabel#elysiumSubtitle, QLabel#elysiumCount { color: #aaa3ba; }
QLabel#elysiumEmpty { color: #aaa3ba; font-size: 16px; padding: 24px; }
QProgressBar { background: #292839; border: 1px solid #4b435f; border-radius: 5px; text-align: center; color: #ece8f5; }
QProgressBar::chunk { background: #a997ff; border-radius: 4px; }
QTabBar::tab { background: #20202f; color: #aaa3ba; border-bottom: 2px solid transparent; padding: 9px 14px; }
QTabBar::tab:selected { color: #a997ff; border-bottom-color: #a997ff; }
QSplitter::handle { background: #4b435f; }
)QSS").replace(QStringLiteral("#a997ff"), m_accent.name())
              .replace(QStringLiteral("#c5b6df"), m_accent.lighter(118).name());
    }
};
