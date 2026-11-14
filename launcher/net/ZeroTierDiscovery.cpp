// SPDX-License-Identifier: GPL-3.0-only
#include "ZeroTierDiscovery.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QUuid>

namespace {
constexpr quint16 DiscoveryPort = 46931;
constexpr qint64 ExpiryMs = 12000;
bool sameLink(const ZeroTierDiscovery::Link& a, const ZeroTierDiscovery::Link& b)
{
    return a.network == b.network && a.address == b.address && a.interfaceIndex == b.interfaceIndex;
}
}

ZeroTierDiscovery* ZeroTierDiscovery::instance()
{
    static auto service = new ZeroTierDiscovery(QCoreApplication::instance());
    return service;
}

ZeroTierDiscovery::ZeroTierDiscovery(QObject* parent) : QObject(parent), m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    if (!m_socket.bind(QHostAddress::AnyIPv4, DiscoveryPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
        m_error = tr("Не удалось открыть UDP-порт 46931 для поиска хостов.");
    connect(&m_socket, &QUdpSocket::readyRead, this, [this] { receive(); });
    connect(&m_timer, &QTimer::timeout, this, [this] {
        const auto now = QDateTime::currentMSecsSinceEpoch();
        for (auto it = m_hosts.begin(); it != m_hosts.end();) {
            if (now - it->seen > ExpiryMs) it = m_hosts.erase(it);
            else ++it;
        }
        announce();
    });
    m_timer.start(3000);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this] { stopHost(); });
}

void ZeroTierDiscovery::setLinks(const QList<Link>& links)
{
    if (hosting()) {
        bool found = false;
        for (const auto& link : links) if (sameLink(link, m_hostLink)) found = true;
        if (!found) stopHost();
    }
    m_links = links;
    for (auto it = m_hosts.begin(); it != m_hosts.end();) {
        bool present = false;
        for (const auto& link : links) if (it->network == link.network) present = true;
        if (!present) it = m_hosts.erase(it);
        else ++it;
    }
}

void ZeroTierDiscovery::removeNetwork(const QString& network)
{
    QList<Link> remaining;
    for (const auto& link : m_links) if (link.network != network) remaining << link;
    setLinks(remaining);
}

bool ZeroTierDiscovery::startHost(const QString& network, const QString& name, const QHostAddress& address, quint16 port)
{
    if (!m_error.isEmpty() || name.trimmed().isEmpty() || !port) return false;
    for (const auto& link : m_links) {
        if (link.network != network || link.address != address) continue;
        stopHost();
        m_hostLink = link;
        m_name = name.trimmed().left(48);
        m_port = port;
        announce();
        return true;
    }
    return false;
}

void ZeroTierDiscovery::setName(const QString& name)
{
    m_name = name.trimmed().left(48);
    announce();
}

void ZeroTierDiscovery::stopHost()
{
    if (hosting()) announce(true);
    m_port = 0;
}

void ZeroTierDiscovery::announce(bool goodbye)
{
    if (!hosting()) return;
    // Check that the OS still has this specific ZeroTier address before sending.
    // A disconnected link must not fall back to a physical/default interface.
    const auto iface = QNetworkInterface::interfaceFromIndex(m_hostLink.interfaceIndex);
    bool present = false;
    for (const auto& entry : iface.addressEntries()) if (entry.ip() == m_hostLink.address) present = true;
    if (!present || !(iface.flags() & QNetworkInterface::IsUp)) return;
    const auto json = QJsonObject{{"protocol", "elysium-zt-host/1"}, {"id", m_id},
        {"network", m_hostLink.network}, {"name", m_name}, {"port", int(m_port)}, {"bye", goodbye}};
    const auto prefix = m_hostLink.prefix;
    if (prefix < 1 || prefix > 30) return;
    const quint32 mask = 0xffffffffu << (32 - prefix);
    const QHostAddress broadcast((m_hostLink.address.toIPv4Address() & mask) | ~mask);
    QNetworkDatagram datagram(QJsonDocument(json).toJson(QJsonDocument::Compact), broadcast, DiscoveryPort);
    datagram.setSender(m_hostLink.address);
    datagram.setInterfaceIndex(m_hostLink.interfaceIndex);
    datagram.setHopLimit(1);
    m_socket.writeDatagram(datagram);
}

void ZeroTierDiscovery::receive()
{
    // Bound memory and work per event; names and endpoint data are untrusted.
    int budget = 100;
    while (m_socket.hasPendingDatagrams() && budget-- > 0) {
        const auto datagram = m_socket.receiveDatagram(1025);
        if (datagram.data().size() > 1024) continue;
        const auto json = QJsonDocument::fromJson(datagram.data()).object();
        if (json.value("protocol").toString() != "elysium-zt-host/1") continue;
        const auto id = json.value("id").toString();
        const auto network = json.value("network").toString();
        if (id == m_id || !QRegularExpression("^[a-fA-F0-9-]{36}$").match(id).hasMatch()) continue;
        bool allowed = false;
        for (const auto& link : m_links) {
            if (link.network == network && datagram.senderAddress().isInSubnet(link.address, link.prefix) &&
                (!datagram.interfaceIndex() || datagram.interfaceIndex() == link.interfaceIndex)) allowed = true;
        }
        if (!allowed) continue;
        // Derive IP from the actual sender, never from a claimed JSON address.
        const auto key = network + "/" + datagram.senderAddress().toString() + "/" + id;
        if (json.value("bye").toBool()) { m_hosts.remove(key); continue; }
        const auto port = json.value("port").toInt();
        auto name = json.value("name").toString().trimmed();
        name.remove(QRegularExpression("[\\x00-\\x1f\\x7f]"));
        if (port < 1 || port > 65535 || name.isEmpty() || name.size() > 48) continue;
        if (!m_hosts.contains(key) && m_hosts.size() >= 256) continue;
        m_hosts[key] = {network, name, datagram.senderAddress().toString() + ":" + QString::number(port), QDateTime::currentMSecsSinceEpoch()};
    }
}

QList<ZeroTierDiscovery::Host> ZeroTierDiscovery::hosts(const QString& network) const
{
    QList<Host> result;
    for (const auto& host : m_hosts) if (host.network == network) result << host;
    if (hosting() && m_hostLink.network == network)
        result << Host{network, m_name + tr(" (вы)"), m_hostLink.address.toString() + ":" + QString::number(m_port), QDateTime::currentMSecsSinceEpoch()};
    return result;
}
