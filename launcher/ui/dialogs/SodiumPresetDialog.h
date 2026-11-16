// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QUrl>
class QTableWidget;
class QPushButton;
class QLabel;
class QProgressDialog;
class QNetworkAccessManager;

class SodiumPresetDialog : public QDialog {
public:
    explicit SodiumPresetDialog(QWidget* parent = nullptr);
    QJsonObject manifest() const { return m_manifest; }
    QString instanceName() const { return m_name; }
private:
    QByteArray fetchBytes(const QUrl& url, qint64 limit);
    QJsonDocument fetch(const QString& path);
    bool canPrepare() const;
    void reload();
    void prepare();
    QNetworkAccessManager* m_network;
    QTableWidget* m_table;
    QPushButton* m_install;
    QPushButton* m_reload;
    QLabel* m_status;
    QProgressDialog* m_progress = nullptr;
    QMap<QString, QJsonDocument> m_cache;
    qint64 m_cacheBytes = 0;
    QMap<QString, QJsonArray> m_projects;
    QJsonObject m_manifest;
    QString m_name;
};
