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
    if (catalog().contains(text)) return text;
    for (auto it = catalog().cbegin(); it != catalog().cend(); ++it)
        if (it.value().contains(text)) return it.key();
    return {};
}
inline QString translate(const QString& source)
{
    const auto found = catalog().constFind(canonical(source));
    if (found == catalog().cend()) return {};
    const auto codes = QStringList{"ru", "en", "de", "fr", "es", "pt_BR", "uk", "zh"};
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
