// SPDX-License-Identifier: GPL-3.0-only
// Independent Hyprland-inspired styling, not an official Hyprland component.
#pragma once
#include "DarkTheme.h"
#include <QObject>

class HyprlandTheme final : public DarkTheme {
public:
    QString id() override { return QStringLiteral("hyprland"); }
    QString name() override { return QStringLiteral("Elysium • Hyprland"); }
    QString tooltip() override { return QObject::tr("Hyprland-inspired dark panels, cyan borders and rounded controls. No compositor required."); }
    QColor fadeColor() override { return QColor("#0c111b"); }
    double fadeAmount() override { return 0.3; }
    QPalette colorScheme() override
    {
        auto palette = DarkTheme::colorScheme();
        palette.setColor(QPalette::Window, QColor("#0c111b"));
        palette.setColor(QPalette::Base, QColor("#101827"));
        palette.setColor(QPalette::AlternateBase, QColor("#172235"));
        palette.setColor(QPalette::Button, QColor("#172235"));
        for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::ToolTipText})
            palette.setColor(role, QColor("#e3eef5"));
        palette.setColor(QPalette::Highlight, QColor("#4ee6d5"));
        palette.setColor(QPalette::HighlightedText, QColor("#08171b"));
        palette.setColor(QPalette::Link, QColor("#74c7ff"));
        palette.setColor(QPalette::PlaceholderText, QColor("#91a4b9"));
        palette.setColor(QPalette::ToolTipBase, QColor("#172235"));
        for (auto role : {QPalette::Text, QPalette::WindowText, QPalette::ButtonText})
            palette.setColor(QPalette::Disabled, role, QColor("#7d8da1"));
        palette.setColor(QPalette::Inactive, QPalette::Highlight, QColor("#315d6b"));
        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, QColor("#e3eef5"));
        return palette;
    }
    QString appStyleSheet() override
    {
        return QStringLiteral(R"QSS(
QToolTip { color: #e3eef5; background: #172235; border: 1px solid #4ee6d5; padding: 7px; }
QToolBar { background: #101827; border: none; spacing: 6px; padding: 6px; }
QPushButton, QToolButton { color: #e3eef5; background: #172235; border: 1px solid #2c4157; border-radius: 9px; padding: 8px 12px; }
QPushButton:hover, QToolButton:hover { background: #1c3443; border-color: #4ee6d5; }
QPushButton:pressed, QToolButton:pressed, QToolButton:checked { background: #214952; border-color: #74c7ff; }
QPushButton:focus, QToolButton:focus { border: 1px solid #74c7ff; }
QPushButton:disabled, QToolButton:disabled { color: #7d8da1; background: #101827; border-color: #243144; }
QPushButton#elysiumCreate { color: #08171b; background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #4ee6d5,stop:1 #74c7ff); border: 1px solid #4ee6d5; font-weight: bold; }
QPushButton#elysiumCreate:disabled { color: #7d8da1; background: #172235; border-color: #243144; }
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: #101827; color: #e3eef5; border: 1px solid #2c4157; border-radius: 8px; padding: 7px; selection-background-color: #4ee6d5; selection-color: #08171b; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border-color: #4ee6d5; }
QComboBox QAbstractItemView { background: #101827; color: #e3eef5; selection-background-color: #214952; selection-color: #e3eef5; }
QAbstractItemView { selection-background-color: #214952; selection-color: #e3eef5; alternate-background-color: #172235; }
QHeaderView::section { background: #172235; color: #b7d5e7; padding: 7px; border: none; border-bottom: 1px solid #2c4157; }
QGroupBox { border: 1px solid #2c4157; border-radius: 10px; margin-top: 12px; padding-top: 12px; }
QGroupBox::title { subcontrol-origin: margin; left: 12px; color: #74c7ff; padding: 0 5px; }
QMenu { background: #101827; color: #e3eef5; border: 1px solid #3a6a79; padding: 6px; }
QMenu::item { padding: 7px 25px; border-radius: 5px; }
QMenu::item:selected { background: #214952; }
QMenu::item:disabled { color: #7d8da1; }
QMenuBar, QStatusBar { background: #0c111b; color: #b7d5e7; }
QMenuBar::item:selected { background: #214952; }
QWidget#elysiumRail { background: transparent; border-left: 1px solid #2c4157; }
QWidget#elysiumActionPanel { background: transparent; border: 1px solid #2c4157; border-radius: 11px; }
QWidget#elysiumRail QPushButton, QWidget#elysiumRail QToolButton { text-align: left; padding: 8px; background: transparent; border: 1px solid transparent; }
QWidget#elysiumRail QPushButton:hover, QWidget#elysiumRail QToolButton:hover { background: #1c3443; border-color: #4ee6d5; }
QWidget#elysiumRail QToolButton::menu-indicator { subcontrol-position: right center; right: 5px; }
QWidget#elysiumHeader { background: transparent; border-bottom: 1px solid #2c4157; }
QWidget#elysiumModernHome, QWidget#elysiumCompactHome, QLabel#elysiumEmpty { background: transparent; }
QLabel#elysiumEyebrow { color: #74c7ff; font-size: 11px; font-weight: bold; }
QLabel#elysiumTitle { color: #e3eef5; font-size: 26px; font-weight: bold; }
QLabel#elysiumSubtitle, QLabel#elysiumCount { color: #a6bccf; }
QLabel#elysiumEmpty { color: #a6bccf; font-size: 16px; padding: 24px; }
QProgressBar { background: #172235; border: 1px solid #2c4157; border-radius: 5px; text-align: center; color: #e3eef5; }
QProgressBar::chunk { background: #4ee6d5; border-radius: 4px; }
QTabBar::tab { background: #101827; color: #a6bccf; border-bottom: 2px solid transparent; padding: 9px 14px; }
QTabBar::tab:selected { color: #4ee6d5; border-bottom-color: #4ee6d5; }
QSplitter::handle { background: #2c4157; }
)QSS");
    }
};
