// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "CustomLanguage.h"
#include <QAbstractButton>
#include <QAction>
#include <QComboBox>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTableWidget>
#include <QRegularExpression>
#include <QSignalBlocker>

// Binds only known static catalog strings. Never translates user input, instance
// names, persisted IDs, combo data, key codes or mod/Minecraft version strings.
class LiveTranslation final : public QObject {
    struct Entry { QPointer<QObject> object; QByteArray property; QString source, prefix; QStringList args; int index = -1; QString lastText; };
    QList<Entry> m_entries;
    void add(QObject* object, const QByteArray& property, QString text, int index = -1)
    {
        if (text.isEmpty()) return;
        QString prefix;
        for (const auto& p : {QStringLiteral("≡  "), QStringLiteral("↕  ")})
            if (text.startsWith(p)) { prefix = p; text.remove(0, p.size()); break; }
        auto source = CustomLanguage::canonical(text);
        QStringList args;
        if (source.isEmpty()) {
            struct Pattern { QString source; QRegularExpression expression; };
            static const auto patterns = [] {
                QList<Pattern> result;
                for (auto it = CustomLanguage::catalog().cbegin(); it != CustomLanguage::catalog().cend(); ++it) {
                    if (!it.key().contains("%1")) continue;
                    for (const auto& form : it.value()) {
                        auto expression = QRegularExpression::escape(form);
                        expression.replace("%1", "(.+)");
                        result.append({it.key(), QRegularExpression("^" + expression + "$")});
                    }
                }
                return result;
            }();
            for (const auto& pattern : patterns) {
                const auto match = pattern.expression.match(text);
                if (match.hasMatch()) { source = pattern.source; args << match.captured(1); break; }
            }
        }
        if (!source.isEmpty()) m_entries.append({object, property, source, prefix, args, index, prefix + text});
    }
public:
    explicit LiveTranslation(QWidget* root) : QObject(root)
    {
        auto objects = root->findChildren<QObject*>();
        objects.prepend(root);
        for (auto object : objects) {
            if (object->property("elysiumUserContent").toBool() || object->objectName() == "elysiumSelectedName") continue;
            if (qobject_cast<QAbstractButton*>(object) || qobject_cast<QLabel*>(object) || qobject_cast<QAction*>(object))
                add(object, "text", object->property("text").toString());
            if (object->inherits("QGroupBox")) add(object, "title", object->property("title").toString());
            if (qobject_cast<QLineEdit*>(object)) add(object, "placeholderText", object->property("placeholderText").toString());
            if (object == root) add(object, "windowTitle", root->windowTitle());
            add(object, "toolTip", object->property("toolTip").toString());
            add(object, "accessibleName", object->property("accessibleName").toString());
            if (auto tabs = qobject_cast<QTabWidget*>(object))
                for (int i = 0; i < tabs->count(); ++i) add(tabs, "tab", tabs->tabText(i), i);
            if (auto combo = qobject_cast<QComboBox*>(object); combo && qobject_cast<QStandardItemModel*>(combo->model()))
                for (int i = 0; i < combo->count(); ++i) add(combo, "item", combo->itemText(i), i);
            if (auto table = qobject_cast<QTableWidget*>(object))
                for (int i = 0; i < table->columnCount(); ++i)
                    if (auto header = table->horizontalHeaderItem(i)) add(table, "header", header->text(), i);
            if (object->property("panelTitle").isValid()) add(object, "panelTitle", object->property("panelTitle").toString());
        }
        root->installEventFilter(this);
        refresh();
    }
    void refresh()
    {
        for (auto& entry : m_entries) {
            if (!entry.object) continue;
            if (entry.property == "text") {
                auto current = entry.object->property("text").toString();
                if (current != entry.lastText) {
                    auto plain = current;
                    if (plain.startsWith(QStringLiteral("≡  ")) || plain.startsWith(QStringLiteral("↕  "))) {
                        entry.prefix = plain.left(3); plain.remove(0, 3);
                    }
                    const auto changedSource = CustomLanguage::canonical(plain);
                    if (changedSource.isEmpty()) continue; // dynamic status/argument changed; never restore stale text
                    entry.source = changedSource;
                    entry.args.clear();
                }
            }
            auto text = CustomLanguage::text(entry.source);
            for (const auto& arg : entry.args) text = text.arg(arg);
            text.prepend(entry.prefix);
            entry.lastText = text;
            QSignalBlocker block(entry.object);
            if (entry.property == "tab") static_cast<QTabWidget*>(entry.object.data())->setTabText(entry.index, text);
            else if (entry.property == "item") static_cast<QComboBox*>(entry.object.data())->setItemText(entry.index, text);
            else if (entry.property == "header") {
                if (auto item = static_cast<QTableWidget*>(entry.object.data())->horizontalHeaderItem(entry.index)) item->setText(text);
            } else entry.object->setProperty(entry.property.constData(), text);
        }
    }
protected:
    bool eventFilter(QObject*, QEvent* event) override
    {
        if (event->type() == QEvent::LanguageChange) refresh();
        return false;
    }
};
