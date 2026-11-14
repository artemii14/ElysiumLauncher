// SPDX-License-Identifier: GPL-3.0-only
// ElysiumLauncher theme. Upstream themes remain available in Settings.
#pragma once

#include "DarkTheme.h"
#include <QObject>

class ElysiumTheme final : public DarkTheme {
public:
    QString id() override { return QStringLiteral("elysium"); }
    QString name() override { return QObject::tr("Elysium • Midnight"); }
    QString tooltip() override { return QObject::tr("Midnight surfaces, violet accents and comfortable spacing."); }
    QColor fadeColor() override { return QColor("#11131d"); }
    double fadeAmount() override { return 0.35; }
    QPalette colorScheme() override
    {
        QPalette palette = DarkTheme::colorScheme();
        palette.setColor(QPalette::Window, QColor("#11131d"));
        palette.setColor(QPalette::WindowText, QColor("#edeef7"));
        palette.setColor(QPalette::Base, QColor("#171a27"));
        palette.setColor(QPalette::AlternateBase, QColor("#1e2233"));
        palette.setColor(QPalette::Text, QColor("#edeef7"));
        palette.setColor(QPalette::Button, QColor("#252a40"));
        palette.setColor(QPalette::ButtonText, QColor("#edeef7"));
        palette.setColor(QPalette::Highlight, QColor("#a997ff"));
        palette.setColor(QPalette::HighlightedText, QColor("#11131d"));
        palette.setColor(QPalette::Link, QColor("#b7a8ff"));
        palette.setColor(QPalette::PlaceholderText, QColor("#9ca3bd"));
        palette.setColor(QPalette::ToolTipBase, QColor("#252a40"));
        palette.setColor(QPalette::ToolTipText, QColor("#edeef7"));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#7a8199"));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#7a8199"));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor("#7a8199"));
        return palette;
    }
    QString appStyleSheet() override
    {
        return QStringLiteral(R"QSS(
QToolTip { color: #edeef7; background: #252a40; border: 1px solid #6d609e; padding: 6px; }
QToolBar { background: #151824; border: none; spacing: 6px; padding: 8px; }
QToolBar::separator { background: #30364e; width: 1px; margin: 8px; }
QToolButton { border: 1px solid transparent; border-radius: 7px; padding: 7px; }
QToolButton:hover, QToolButton:checked { background: #30334e; border-color: #6d609e; }
QToolButton:focus, QPushButton:focus { border: 1px solid #b7a8ff; }
QPushButton { background: #292d43; border: 1px solid #3b415c; border-radius: 7px; padding: 7px 14px; }
QPushButton:hover { background: #383d59; border-color: #a997ff; }
QPushButton:disabled { background: #1b1e2b; border-color: #292d43; }
QPushButton#elysiumCreate { background: #a997ff; color: #11131d; font-weight: bold; border: none; }
QPushButton#elysiumCreate:hover { background: #c2b5ff; }
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: #1b1f2f; border: 1px solid #3b415c; border-radius: 6px; padding: 7px; selection-background-color: #a997ff; selection-color: #11131d; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #b7a8ff; }
QMenu { background: #1b1f2f; border: 1px solid #3b415c; padding: 5px; }
QMenu::item { padding: 7px 24px; border-radius: 4px; }
QMenu::item:selected { background: #383d59; }
QMenuBar { background: #11131d; }
QMenuBar::item:selected { background: #383d59; }
QStatusBar { background: #11131d; color: #a5adc7; border-top: 1px solid #292d43; padding: 4px; }
QLabel#elysiumEyebrow { color: #b7a8ff; font-size: 11px; font-weight: bold; }
QLabel#elysiumTitle { font-size: 26px; font-weight: bold; }
QLabel#elysiumSubtitle, QLabel#elysiumCount { color: #a5adc7; }
QLabel#elysiumEmpty { color: #a5adc7; font-size: 16px; padding: 32px; }
QWidget#elysiumRail { background: #151824; border-right: 1px solid #292d43; }
QWidget#elysiumRail QPushButton, QWidget#elysiumRail QToolButton { text-align: left; padding: 8px 10px; background: transparent; border: 1px solid transparent; border-radius: 7px; }
QWidget#elysiumRail QPushButton:hover, QWidget#elysiumRail QToolButton:hover { background: #252a40; border-color: #3b415c; }
QWidget#elysiumRail QPushButton:disabled { color: #72798f; }
QWidget#elysiumRail QToolButton::menu-indicator { subcontrol-position: right center; right: 8px; }
QWidget#elysiumModeBar { background: #11131d; border-bottom: 1px solid #292d43; }
QWidget#elysiumCompactHome { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #151b2d, stop:0.65 #20203b, stop:1 #171a27); }
QWidget#elysiumCompactHome QPushButton { padding: 12px 16px; }
QWidget#elysiumHeader { background: #171a27; border-bottom: 1px solid #292d43; }
QProgressBar { border: none; background: #252a40; border-radius: 4px; text-align: center; }
QProgressBar::chunk { background: #a997ff; border-radius: 4px; }
QTabBar::tab { padding: 9px 14px; }
QTabBar::tab:selected { color: #c2b5ff; }
)QSS");
    }
};
