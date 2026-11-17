// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2022 Tayou <git@tayou.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QDialog>

#include <translations/TranslationsModel.h>
#include <QTextCursor>

class QTextCharFormat;
class QCheckBox;
class QPushButton;
class QGroupBox;
class QComboBox;
class QLabel;
class SettingsObject;

namespace Ui {
class AppearanceWidget;
}

class AppearanceWidget : public QWidget {
    Q_OBJECT

   public:
    explicit AppearanceWidget(bool simple, QWidget* parent = 0);
    virtual ~AppearanceWidget();

   public:
    void applySettings();
    void loadSettings();
    void retranslateUi();

   private:
    void applyIconTheme(int index);
    void applyWidgetTheme(int index);
    void applyCatTheme(int index);
    void loadThemeSettings();

    void updateConsolePreview();
    void updateCatPreview();

    Ui::AppearanceWidget* m_ui;
    QTextCharFormat m_defaultFormat;
    bool m_themesOnly;
    QCheckBox* m_animation;
    QComboBox* m_backgroundStyle;
    QLabel* m_backgroundLabel;
    QCheckBox* m_moveButtons;
    QPushButton* m_accent;
    QPushButton* m_resetOrder;
    QPushButton* m_resetColors;
    QGroupBox* m_homeAppearance;
    QString m_accentColor;
    bool m_resetHomeOrder = false;
};
