// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QHash>
#include <QStringList>
#include <QTranslator>

// A small offline supplement to the downloaded upstream catalogs. Returning a
// null QString preserves Qt's normal fallback for strings outside this catalog.
class ElysiumTranslator final : public QTranslator {
public:
    explicit ElysiumTranslator(QObject* parent) : QTranslator(parent) {}
    QString language;
    bool isEmpty() const override { return false; }
    QString translate(const char*, const char* source, const char*, int) const override
    {
        const auto text = QString::fromUtf8(source);
        const auto lang = language.left(2);
        static const QHash<QString, QString> english = {
            {QStringLiteral("Изменить иконку сборки"), QStringLiteral("Change instance icon")},
            {QStringLiteral("БИБЛИОТЕКА"), QStringLiteral("LIBRARY")},
            {QStringLiteral("Создать сборку…"), QStringLiteral("Create instance…")},
            {QStringLiteral("Установить vanilla…"), QStringLiteral("Install vanilla…")},
            {QStringLiteral("Установить Sodium…"), QStringLiteral("Install Sodium…")},
            {QStringLiteral("ВЫБРАННАЯ СБОРКА"), QStringLiteral("SELECTED INSTANCE")},
            {QStringLiteral("Играть"), QStringLiteral("Play")},
            {QStringLiteral("Настроить сборку…"), QStringLiteral("Edit instance…")},
            {QStringLiteral("Настроить сборку"), QStringLiteral("Edit instance")},
            {QStringLiteral("Импорт OptiFine…"), QStringLiteral("Import OptiFine…")},
            {QStringLiteral("Импорт локального профиля OptiFine в выбранную vanilla-сборку"), QStringLiteral("Import a local OptiFine profile into the selected vanilla instance")},
            {QStringLiteral("ЛАУНЧЕР"), QStringLiteral("LAUNCHER")},
            {QStringLiteral("Лаунчер"), QStringLiteral("Launcher")},
            {QStringLiteral("Учётные записи"), QStringLiteral("Accounts")},
            {QStringLiteral("Папки"), QStringLiteral("Folders")},
            {QStringLiteral("Настройки"), QStringLiteral("Settings")},
            {QStringLiteral("Эмулятор локальной сети"), QStringLiteral("Virtual LAN")},
            {QStringLiteral("Эмулятор локальной сети…"), QStringLiteral("Virtual LAN…")},
            {QStringLiteral("Подключение к виртуальной сети через установленный ZeroTier One"), QStringLiteral("Connect to a virtual network using the installed ZeroTier One client")},
            {QStringLiteral("Справка"), QStringLiteral("Help")},
            {QStringLiteral("Запуск Minecraft"), QStringLiteral("Launch Minecraft")},
            {QStringLiteral("Выберите сборку для запуска или установите новую версию Minecraft."), QStringLiteral("Choose an instance or install a new Minecraft version.")},
            {QStringLiteral("Готовая сборка Minecraft"), QStringLiteral("Minecraft instance")},
            {QStringLiteral("Выбери сборку…"), QStringLiteral("Choose an instance…")},
            {QStringLiteral("Остановить"), QStringLiteral("Stop")},
            {QStringLiteral("+ Vanilla автоматически"), QStringLiteral("+ Automatic vanilla")},
            {QStringLiteral("Sodium-сборка…"), QStringLiteral("Sodium instance…")},
            {QStringLiteral("Модпаки / Forge / Fabric…"), QStringLiteral("Modpacks / Forge / Fabric…")},
            {QStringLiteral("Иконка сборки %1"), QStringLiteral("Icon for %1")},
            {QStringLiteral("Нет выбранной сборки. Установите Minecraft или добавьте сборку."), QStringLiteral("No instance selected. Install Minecraft or add an instance.")},
            {QStringLiteral("версия не указана"), QStringLiteral("version unknown")},
            {QStringLiteral("Игра запущена"), QStringLiteral("Game running")},
            {QStringLiteral("Готово к запуску"), QStringLiteral("Ready to play")},
            {QStringLiteral("Выбери Minecraft — сборка будет создана автоматически"), QStringLiteral("Choose Minecraft — an instance will be created automatically")}
        };
        static const QHash<QString, QString> russian = {
            {QStringLiteral("Neon rain"), QStringLiteral("Неоновый дождь")},
            {QStringLiteral("Orbits"), QStringLiteral("Орбиты")},
            {QStringLiteral("Neon grid"), QStringLiteral("Неоновая сетка")},
            {QStringLiteral("Hyprland-inspired dark panels, cyan borders and rounded controls. No compositor required."), QStringLiteral("В стиле Hyprland: тёмные панели, бирюзовые рамки и скругления. Композитор не требуется.")},
            {QStringLiteral("Layout locked. Enable Allow moving buttons in Settings → Appearance, then Apply."), QStringLiteral("Раскладка заблокирована. Включите «Разрешить перемещение» в Настройки → Внешний вид и нажмите «Применить».")},
            {QStringLiteral("Make room for a new world.\nCreate your first instance to install Minecraft or a modpack."), QStringLiteral("Здесь начнётся новый мир.\nСоздайте сборку Minecraft или установите модпак.")},
            {QStringLiteral("No matching instances.\nTry another name or clear the search."), QStringLiteral("Сборки не найдены.\nИзмените запрос или очистите поиск.")},
            {QStringLiteral("Search instances or groups…  Ctrl+K"), QStringLiteral("Поиск сборок или групп…  Ctrl+K")},
            {QStringLiteral("%1 of %2 instances"), QStringLiteral("Сборки: %1 из %2")},
            {QStringLiteral("Background preset"), QStringLiteral("Готовый анимированный фон")},
            {QStringLiteral("Aurora"), QStringLiteral("Северное сияние")},
            {QStringLiteral("Starfield"), QStringLiteral("Звёздное небо")},
            {QStringLiteral("Waves"), QStringLiteral("Волны")},
            {QStringLiteral("Reset button and panel order"), QStringLiteral("Сбросить порядок кнопок и блоков")},
            {QStringLiteral("Drag panel headers to move whole blocks, or drag individual buttons. Turn this off to lock the layout."), QStringLiteral("Перетаскивайте заголовки блоков или отдельные кнопки. Выключите перемещение для блокировки раскладки.")},
            {QStringLiteral("Drag this header onto another panel to move the whole block."), QStringLiteral("Перетащите заголовок на другой блок, чтобы переместить блок целиком.")},
            {QStringLiteral("Home screen"), QStringLiteral("Главный экран")},
            {QStringLiteral("Animated background"), QStringLiteral("Анимированный фон")},
            {QStringLiteral("Allow moving buttons"), QStringLiteral("Разрешить перемещение")},
            {QStringLiteral("Drag buttons onto each other to reorder. Turn this off to use them normally."), QStringLiteral("Перетащите кнопку на другую, чтобы изменить порядок. Выключите перемещение для обычного использования кнопок.")},
            {QStringLiteral("Reset button order"), QStringLiteral("Сбросить порядок кнопок")},
            {QStringLiteral("Accent color"), QStringLiteral("Цвет акцента")},
            {QStringLiteral("Accent color (Midnight): %1"), QStringLiteral("Цвет акцента (Midnight): %1")},
            {QStringLiteral("Midnight surfaces, violet accents and comfortable spacing."), QStringLiteral("Тёмная тема с настраиваемым акцентом и удобными отступами.")}
        };
        static const QHash<QString, QStringList> controls = {
            {QStringLiteral("Home screen"), {QStringLiteral("Startseite"), QStringLiteral("Écran d’accueil"), QStringLiteral("Pantalla de inicio"), QStringLiteral("Aloitusnäyttö")}},
            {QStringLiteral("Animated background"), {QStringLiteral("Animierter Hintergrund"), QStringLiteral("Arrière-plan animé"), QStringLiteral("Fondo animado"), QStringLiteral("Animoitu tausta")}},
            {QStringLiteral("Allow moving buttons"), {QStringLiteral("Verschieben der Schaltflächen erlauben"), QStringLiteral("Autoriser le déplacement des boutons"), QStringLiteral("Permitir mover botones"), QStringLiteral("Salli painikkeiden siirtäminen")}},
            {QStringLiteral("Reset button order"), {QStringLiteral("Reihenfolge zurücksetzen"), QStringLiteral("Réinitialiser l’ordre des boutons"), QStringLiteral("Restablecer orden de botones"), QStringLiteral("Palauta painikkeiden järjestys")}},
            {QStringLiteral("Accent color"), {QStringLiteral("Akzentfarbe"), QStringLiteral("Couleur d’accentuation"), QStringLiteral("Color de acento"), QStringLiteral("Korostusväri")}},
            {QStringLiteral("Accent color (Midnight): %1"), {QStringLiteral("Akzentfarbe (Midnight): %1"), QStringLiteral("Couleur d’accentuation (Midnight) : %1"), QStringLiteral("Color de acento (Midnight): %1"), QStringLiteral("Korostusväri (Midnight): %1")}}
        };
        if (lang == QStringLiteral("ru")) return russian.value(text);
        const int index = QStringList{QStringLiteral("de"), QStringLiteral("fr"), QStringLiteral("es"), QStringLiteral("fi")}.indexOf(lang);
        if (index >= 0 && controls.contains(text)) return controls.value(text).at(index);
        // English fallback for Elysium's historical Russian-only home labels.
        return english.value(text);
    }
};
