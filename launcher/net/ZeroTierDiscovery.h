// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <QObject>
#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QString>
#include <QTimer>
#include <QUdpSocket>

// Best-effort discovery, not identity/authentication. Restricted to addresses
// of connected ZeroTier networks supplied by the authenticated local API.
class ZeroTierDiscovery : public QObject {
public:
    struct Link {
        QString network;
        QHostAddress address;
        int prefix = 0;
        uint interfaceIndex = 0;
    };
    struct Host {
        QString network;
        QString name;
        QString endpoint;
        qint64 seen = 0;
    };
    static ZeroTierDiscovery* instance();
    void setLinks(const QList<Link>& links);
    bool startHost(const QString& network, const QString& name, const QHostAddress& address, quint16 port);
    void stopHost();
    void setName(const QString& name);
    QList<Host> hosts(const QString& network) const;
    bool hosting() const { return m_port != 0; }
    QString hostNetwork() const { return m_hostLink.network; }
    QString error() const { return m_error; }
    void removeNetwork(const QString& network);
private:
    explicit ZeroTierDiscovery(QObject* parent);
    void announce(bool goodbye = false);
    void receive();
    QUdpSocket m_socket;
    QTimer m_timer;
    QList<Link> m_links;
    QMap<QString, Host> m_hosts;
    Link m_hostLink;
    QString m_name;
    QString m_id;
    QString m_error;
    quint16 m_port = 0;
};
