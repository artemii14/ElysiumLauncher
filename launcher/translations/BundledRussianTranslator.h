// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QFile>
#include <QHash>
#include <QTranslator>
#include <QXmlStreamReader>

// Offline fallback. Downloaded catalogs take precedence; no network or writable
// translation directory is needed for the bundled Russian UI.
class BundledRussianTranslator final : public QTranslator {
public:
    explicit BundledRussianTranslator(QObject* parent = nullptr) : QTranslator(parent)
    {
        QFile file(":/translations/ru.ts");
        if (!file.open(QIODevice::ReadOnly)) return;
        QXmlStreamReader xml(&file);
        QString context;
        while (!xml.atEnd()) {
            xml.readNext();
            if (!xml.isStartElement()) continue;
            if (xml.name() == QStringLiteral("context")) context.clear();
            else if (xml.name() == QStringLiteral("name")) context = xml.readElementText();
            else if (xml.name() == QStringLiteral("message")) {
                QString source, comment;
                QStringList translated;
                bool valid = false;
                while (xml.readNextStartElement()) {
                    if (xml.name() == QStringLiteral("source")) source = xml.readElementText();
                    else if (xml.name() == QStringLiteral("comment")) comment = xml.readElementText();
                    else if (xml.name() == QStringLiteral("translation")) {
                        const auto type = xml.attributes().value(QStringLiteral("type"));
                        // Our ElyPrism base still uses some strings removed from current
                        // Prism. Their historical translations remain useful exact matches.
                        valid = type != QStringLiteral("unfinished");
                        QString single;
                        while (!xml.atEnd()) {
                            xml.readNext();
                            if (xml.isEndElement() && xml.name() == QStringLiteral("translation")) break;
                            if (xml.isStartElement() && xml.name() == QStringLiteral("numerusform"))
                                translated << xml.readElementText();
                            else if (xml.isCharacters()) single += xml.text();
                        }
                        if (translated.isEmpty() && !single.isEmpty()) translated << single;
                    } else xml.skipCurrentElement();
                }
                if (valid && !source.isEmpty() && !translated.isEmpty() && !translated.first().isEmpty())
                    m_messages.insert(context + QChar(4) + source + QChar(4) + comment, translated);
            }
        }
        if (xml.hasError()) m_messages.clear();
    }
    bool isEmpty() const override { return m_messages.isEmpty(); }
    QString translate(const char* context, const char* source, const char* disambiguation = nullptr, int n = -1) const override
    {
        auto key = QString::fromUtf8(context) + QChar(4) + QString::fromUtf8(source) + QChar(4);
        auto found = m_messages.constFind(key + QString::fromUtf8(disambiguation));
        if (found == m_messages.cend()) found = m_messages.constFind(key);
        if (found == m_messages.cend()) return {};
        const auto& forms = found.value();
        int form = 0;
        if (n >= 0 && forms.size() > 1) {
            form = n % 10 == 1 && n % 100 != 11 ? 0
                 : n % 10 >= 2 && n % 10 <= 4 && !(n % 100 >= 12 && n % 100 <= 14) ? 1 : 2;
        }
        return forms.at(qMin(form, int(forms.size()) - 1));
    }
private:
    QHash<QString, QStringList> m_messages;
};
