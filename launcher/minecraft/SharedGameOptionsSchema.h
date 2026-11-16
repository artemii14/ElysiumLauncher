// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QMap>
#include <QVariant>
#include <QStringList>
#include <QVector>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>

namespace SharedGameOptions {
// INI values are strings after restarting the launcher. Do not persist a raw
// QVariantMap and then rely on QVariant::toMap() to parse its stored form.
inline QString encode(const QVariantMap& values)
{
    return QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(values)).toJson(QJsonDocument::Compact));
}
inline QVariantMap decode(const QVariant& stored)
{
    // Accept in-memory maps from older builds as well as the persistent JSON form.
    if (stored.userType() == QMetaType::QVariantMap) return stored.toMap();
    return QJsonDocument::fromJson(stored.toString().toUtf8()).object().toVariantMap();
}

struct Option {
    QString key, title, group;
    int minimum = 0, maximum = 1, initial = 0;
    double divisor = 1;
    QStringList choices;
};
inline QStringList bindingValues()
{
    QStringList keys{"key.keyboard.unknown", "key.mouse.left", "key.mouse.right", "key.mouse.middle"};
    for (char key = 'a'; key <= 'z'; ++key) keys << "key.keyboard." + QString(QChar(key));
    for (int key = 0; key <= 9; ++key) keys << "key.keyboard." + QString::number(key);
    for (int key = 1; key <= 12; ++key) keys << "key.keyboard.f" + QString::number(key);
    for (const auto& key : {"space", "left.shift", "right.shift", "left.control", "right.control", "left.alt", "right.alt",
                            "tab", "enter", "backspace", "up", "down", "left", "right", "insert", "delete", "home", "end",
                            "page.up", "page.down", "grave.accent", "minus", "equal", "left.bracket", "right.bracket",
                            "backslash", "semicolon", "apostrophe", "comma", "period", "slash"})
        keys << "key.keyboard." + QString::fromLatin1(key);
    for (int key = 4; key <= 8; ++key) keys << "key.mouse." + QString::number(key);
    return keys;
}
inline const QVector<Option>& schema()
{
    static const QVector<Option> options = [] {
        QVector<Option> o;
        QString group;
        auto n = [&](QString key, QString title, int min, int max, int initial, double divisor = 1) {
            o.append({key, title, group, min, max, initial, divisor, {}});
        };
        auto b = [&](QString key, QString title, bool initial) { n(key, title, 0, 1, initial); };
        auto c = [&](QString key, QString title, QStringList choices, int initial = 0) {
            o.append({key, title, group, 0, int(choices.size()) - 1, initial, 1, choices});
        };
        group = QStringLiteral("Графика");
        n("renderDistance", "Дальность прорисовки (чанки)", 2, 32, 12);
        n("simulationDistance", "Дальность симуляции (чанки)", 5, 32, 12);
        n("maxFps", "Лимит FPS (260 — без ограничения)", 10, 260, 120);
        n("fov", "Поле зрения (градусы)", 30, 110, 70);
        n("gamma", "Яркость (%)", 0, 100, 50, 100);
        n("guiScale", "Масштаб интерфейса (0 — авто)", 0, 8, 0);
        n("entityDistanceScaling", "Дальность сущностей (%)", 50, 500, 100, 100);
        c("graphicsMode", "Графика", {"Быстро", "Красиво", "Потрясающе"}, 1);
        c("ao", "Мягкое освещение", {"Выкл.", "Минимум", "Максимум"}, 2);
        c("particles", "Частицы", {"Все", "Уменьшено", "Минимум"});
        c("renderClouds", "Облака", {"Выкл.", "Быстро", "Красиво"}, 2);
        n("mipmapLevels", "Уровни MIP-текстур", 0, 4, 4);
        b("enableVsync", "Вертикальная синхронизация", false);
        b("fullscreen", "Полноэкранный режим", false);
        b("entityShadows", "Тени сущностей", true);
        b("bobView", "Покачивание камеры", true);
        n("biomeBlendRadius", "Смешивание биомов", 0, 7, 2);
        group = QStringLiteral("Звук");
        for (const auto& item : QVector<QPair<QString, QString>>{
                 {"master", "Общая громкость"}, {"music", "Музыка"}, {"record", "Пластинки"},
                 {"weather", "Погода"}, {"block", "Блоки"}, {"hostile", "Враждебные существа"},
                 {"neutral", "Мирные существа"}, {"player", "Игроки"}, {"ambient", "Окружение"}, {"voice", "Голос"}})
            n("soundCategory_" + item.first, item.second + " (%)", 0, 100, 100, 100);
        b("showSubtitles", "Субтитры", false);
        group = QStringLiteral("Управление");
        n("mouseSensitivity", "Чувствительность мыши (%)", 0, 200, 100, 200);
        n("mouseWheelSensitivity", "Скорость прокрутки (%)", 1, 1000, 100, 100);
        b("invertYMouse", "Инверсия мыши по Y", false);
        b("rawMouseInput", "Прямой ввод мыши", true);
        b("discrete_mouse_scroll", "Дискретная прокрутка", false);
        b("touchscreen", "Сенсорное управление", false);
        b("autoJump", "Автопрыжок", false);
        b("toggleCrouch", "Приседание переключателем", false);
        b("toggleSprint", "Бег переключателем", false);
        group = QStringLiteral("Чат и интерфейс");
        c("chatVisibility", "Чат", {"Показывать", "Только команды", "Скрыть"});
        b("chatColors", "Цвета чата", true);
        b("chatLinks", "Ссылки в чате", true);
        b("chatLinksPrompt", "Подтверждать открытие ссылок", true);
        b("autoSuggestions", "Подсказки команд", true);
        n("chatOpacity", "Непрозрачность чата (%)", 0, 100, 100, 100);
        n("textBackgroundOpacity", "Непрозрачность фона текста (%)", 0, 100, 50, 100);
        n("chatScale", "Масштаб чата (%)", 0, 100, 100, 100);
        n("chatWidth", "Ширина чата (%)", 0, 100, 100, 100);
        n("chatHeightFocused", "Высота открытого чата (%)", 0, 100, 100, 100);
        n("chatHeightUnfocused", "Высота закрытого чата (%)", 0, 100, 44, 100);
        n("chatLineSpacing", "Интервал строк чата (%)", 0, 100, 0, 100);
        n("chatDelay", "Задержка сообщений (сек.)", 0, 6, 0);
        b("advancedItemTooltips", "Расширенные подсказки предметов", false);
        b("heldItemTooltips", "Название предмета над панелью", true);
        b("reducedDebugInfo", "Сокращённый экран отладки", false);
        c("attackIndicator", "Индикатор атаки", {"Выкл.", "У прицела", "У панели"}, 1);
        group = QStringLiteral("Доступность");
        n("screenEffectScale", "Эффекты экрана (%)", 0, 100, 100, 100);
        n("fovEffectScale", "Изменение FOV при движении (%)", 0, 100, 100, 100);
        n("darknessEffectScale", "Эффект темноты (%)", 0, 100, 100, 100);
        n("damageTiltStrength", "Наклон камеры при уроне (%)", 0, 100, 100, 100);
        n("glintStrength", "Интенсивность блеска (%)", 0, 100, 75, 100);
        n("glintSpeed", "Скорость блеска (%)", 0, 100, 50, 100);
        b("hideLightningFlashes", "Скрывать вспышки молний", false);
        b("darkMojangStudiosBackground", "Тёмный экран загрузки", false);
        b("hideSplashTexts", "Скрывать сплэши", false);
        b("pauseOnLostFocus", "Пауза при потере фокуса", true);
        b("forceUnicodeFont", "Шрифт Unicode", false);
        c("narrator", "Диктор", {"Выкл.", "Всё", "Чат", "Система"});
        c("lang", "Язык игры", {"Русский", "English (US)", "English (UK)", "Українська", "Suomi", "Deutsch", "Français", "Español", "Polski", "Português (Brasil)", "简体中文", "日本語"});
        group = QStringLiteral("Клавиши");
        const auto bindings = bindingValues();
        QStringList labels;
        for (const auto& key : bindings) {
            if (key == "key.keyboard.unknown") labels << "Не назначено";
            else if (key.startsWith("key.mouse.")) labels << "Мышь: " + key.mid(10);
            else labels << key.mid(13).toUpper();
        }
        auto binding = [&](QString key, QString title, QString initial) {
            c("key_key." + key, title, labels, bindings.indexOf(initial));
        };
        binding("forward", "Вперёд", "key.keyboard.w");
        binding("back", "Назад", "key.keyboard.s");
        binding("left", "Влево", "key.keyboard.a");
        binding("right", "Вправо", "key.keyboard.d");
        binding("jump", "Прыжок", "key.keyboard.space");
        binding("sneak", "Приседание", "key.keyboard.left.shift");
        binding("sprint", "Бег", "key.keyboard.left.control");
        binding("attack", "Атаковать / разрушить", "key.mouse.left");
        binding("use", "Использовать / поставить", "key.mouse.right");
        binding("pickItem", "Выбрать блок", "key.mouse.middle");
        binding("inventory", "Инвентарь", "key.keyboard.e");
        binding("drop", "Выбросить предмет", "key.keyboard.q");
        binding("swapOffhand", "Сменить руку", "key.keyboard.f");
        binding("chat", "Открыть чат", "key.keyboard.t");
        binding("command", "Ввести команду", "key.keyboard.slash");
        binding("playerlist", "Список игроков", "key.keyboard.tab");
        binding("togglePerspective", "Сменить камеру", "key.keyboard.f5");
        binding("screenshot", "Снимок экрана", "key.keyboard.f2");
        binding("advancements", "Достижения", "key.keyboard.l");
        for (int slot = 1; slot <= 9; ++slot)
            binding("hotbar." + QString::number(slot), "Слот " + QString::number(slot), "key.keyboard." + QString::number(slot));
        group = QStringLiteral("Внешний вид");
        c("mainHand", "Основная рука", {"Левая", "Правая"}, 1);
        for (const auto& part : QVector<QPair<QString, QString>>{{"cape", "Плащ"}, {"jacket", "Куртка"},
                     {"left_sleeve", "Левый рукав"}, {"right_sleeve", "Правый рукав"},
                     {"left_pants_leg", "Левая штанина"}, {"right_pants_leg", "Правая штанина"}, {"hat", "Головной убор"}})
            b("modelPart_" + part.first, part.second, true);
        return o;
    }();
    return options;
}
inline int value(const Option& option, const QVariantMap& saved)
{
    // QVariant(bool).toInt() and old numeric settings both migrate without loss.
    return qBound(option.minimum, saved.value(option.key, option.initial).toInt(), option.maximum);
}
inline QMap<QString, QString> serialize(const QVariantMap& saved, const QString& minecraftVersion = QString())
{
    QMap<QString, QString> result;
    for (const auto& option : schema()) {
        const int v = value(option, saved);
        QString text;
        if (option.key.startsWith("key_key.")) text = bindingValues().at(v);
        else if (option.key == "mainHand") text = v ? "right" : "left";
        else if (option.key == "fov") text = QString::number((v - 70) / 40.0);
        else if (option.key == "renderClouds") text = QStringList{"false", "fast", "true"}.at(v);
        else if (option.key == "lang") text = QStringList{"ru_ru", "en_us", "en_gb", "uk_ua", "fi_fi", "de_de", "fr_fr", "es_es", "pl_pl", "pt_br", "zh_cn", "ja_jp"}.at(v);
        else if (option.maximum == 1 && option.choices.isEmpty()) text = v ? "true" : "false";
        else text = QString::number(v / option.divisor, 'g', 12);
        result.insert(option.key, text);
    }
    const auto version = QVersionNumber::fromString(minecraftVersion);
    // Smooth lighting changed from an integer (0/1/2) to a boolean in 1.19.3.
    // A wrong type can prevent Minecraft from reading its options correctly.
    if (version.isNull() || QVersionNumber::compare(version, QVersionNumber(1, 19, 3)) >= 0)
        result["ao"] = result.value("ao").toInt() > 0 ? "true" : "false";
    if (!version.isNull() && QVersionNumber::compare(version, QVersionNumber(1, 16)) < 0) {
        result["fancyGraphics"] = result.value("graphicsMode").toInt() > 0 ? "true" : "false";
        result.remove("graphicsMode");
    }
    return result;
}
}
