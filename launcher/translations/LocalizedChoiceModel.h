// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "CustomLanguage.h"
#include <QAbstractListModel>
#include <QComboBox>
#include <QEvent>
#include <QSignalBlocker>

// A key-binding selector has ~90 choices. Sharing QStringList storage and
// resolving labels on demand avoids thousands of QStandardItems at Settings open.
class LocalizedChoiceModel final : public QAbstractListModel {
    QStringList m_labels;
public:
    LocalizedChoiceModel(const QStringList& labels, QObject* parent) : QAbstractListModel(parent), m_labels(labels) {}
    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : m_labels.size(); }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_labels.size()) return {};
        if (role == Qt::DisplayRole || role == Qt::EditRole) return CustomLanguage::text(m_labels.at(index.row()));
        return {};
    }
    void retranslate()
    {
        if (!m_labels.isEmpty()) emit dataChanged(index(0), index(m_labels.size() - 1), {Qt::DisplayRole});
    }
};
class LocalizedChoiceBox final : public QComboBox {
public:
    explicit LocalizedChoiceBox(const QStringList& labels, QWidget* parent = nullptr) : QComboBox(parent)
    {
        setModel(new LocalizedChoiceModel(labels, this));
    }
protected:
    void changeEvent(QEvent* event) override
    {
        QComboBox::changeEvent(event);
        if (event->type() == QEvent::LanguageChange) {
            QSignalBlocker block(this);
            static_cast<LocalizedChoiceModel*>(model())->retranslate();
            update();
        }
    }
};
