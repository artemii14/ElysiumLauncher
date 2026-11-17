// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QFile>
#include <QHash>
#include <QCoreApplication>
#include <QStringList>

namespace CustomLanguage {
inline QString language = QStringLiteral("en");
inline const QHash<QString, QStringList>& catalog()
{
    static const auto data = [] {
        QHash<QString, QStringList> entries;
        QFile file(":/translations/elysium.tsv");
        if (!file.open(QIODevice::ReadOnly)) return entries;
        file.readLine();
        while (!file.atEnd()) {
            auto row = QString::fromUtf8(file.readLine()).trimmed().split('\t');
            if (row.size() == 8) entries.insert(row.first(), row);
        }
        return entries;
    }();
    return data;
}
inline QString canonical(const QString& text)
{
    if (text.isEmpty()) return {};
    static const auto reverse = [] {
        QHash<QString, QString> index;
        // Sort keys for deterministic handling of shared translated labels.
        auto keys = catalog().keys();
        keys.sort();
        for (const auto& key : keys)
            for (const auto& value : catalog().value(key))
                if (!index.contains(value)) index.insert(value, key);
        // Canonical source keys always take priority over ambiguous translations.
        for (const auto& key : keys) index.insert(key, key);
        return index;
    }();
    return reverse.value(text);
}
inline QString translate(const QString& source)
{
    const auto found = catalog().constFind(canonical(source));
    if (found == catalog().cend()) return {};
    static const auto codes = QStringList{"ru", "en", "de", "fr", "es", "pt_BR", "uk", "zh"};
    int index = codes.indexOf(language);
    if (index < 0) index = codes.indexOf(language.section('_', 0, 0));
    return found.value().at(index < 0 ? 1 : index);
}
inline QString text(const QString& source)
{
    const auto translated = translate(source);
    return translated.isNull() ? source : translated;
}
}
