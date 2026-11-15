// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QDialog>
#include <QJsonDocument>
#include <functional>

class QNetworkAccessManager;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// Talks only to the loopback ZeroTier service. Never to Central with a local token.
class ZeroTierDialog : public QDialog {
public:
    explicit ZeroTierDialog(QWidget* parent = nullptr);
private:
    void refresh();
    void request(const QString& path, const QByteArray& method,
                 const QJsonDocument& body, std::function<void(QJsonDocument)> success);
    void updateButtons();
    void selectToken();
    void showError(const QString& message);
    QString selectedNetwork() const;
    QString tokenPath() const;
    QNetworkAccessManager* m_network;
    QLabel* m_status;
    QLineEdit* m_id;
    QTableWidget* m_table;
    QPushButton* m_join;
    QPushButton* m_leave;
    QPushButton* m_refresh;
    QPushButton* m_copy;
    QString m_tokenOverride;
    bool m_busy = false;
};
