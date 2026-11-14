// SPDX-License-Identifier: GPL-3.0-only
#include "ZeroTierDiscovery.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QNetworkProxy>
#include <QRegularExpression>
#include <QUuid>

namespace {
constexpr quint16 DiscoveryPort = 46931;
constexpr qint64 ExpiryMs = 12000;
const QHostAddress MulticastGroup("239.255.46.93");
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
    m_socket.setProxy(QNetworkProxy::NoProxy);
    if (!m_socket.bind(QHostAddress::AnyIPv4, DiscoveryPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
        m_error = tr("Не удалось открыть UDP-порт 46931 для поиска хостов.");
    connect(&m_socket, &QUdpSocket::readyRead, this, [this] { receive(); });
    connect(&m_timer, &QTimer::timeout, this, [this] {
        const auto now = QDateTime::currentMSecsSinceEpoch();
        for (auto it = m_hosts.begin(); it != m_hosts.end();) {
            if (now - it->seen > ExpiryMs) it = m_hosts.erase(it);
            else ++it;
        }
        for (auto it = m_subscribers.begin(); it != m_subscribers.end();) {
            if (now - it->second > 60000) it = m_subscribers.erase(it);
            else ++it;
        }
        query();
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
    for (const auto& link : m_links) {
        bool retained = false;
        for (const auto& next : links) if (sameLink(link, next)) retained = true;
        if (!retained) m_socket.leaveMulticastGroup(MulticastGroup, QNetworkInterface::interfaceFromIndex(link.interfaceIndex));
    }
    for (const auto& link : links) {
        bool existed = false;
        for (const auto& old : m_links) if (sameLink(link, old)) existed = true;
        if (!existed && link.interfaceIndex)
            m_socket.joinMulticastGroup(MulticastGroup, QNetworkInterface::interfaceFromIndex(link.interfaceIndex));
    }
    m_links = links;
    for (auto it = m_probes.begin(); it != m_probes.end();) {
        bool retained = false;
        for (const auto& link : links) if (link.network == it.key()) retained = true;
        if (!retained) it = m_probes.erase(it); else ++it;
    }
    query();
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
    m_subscribers.clear();
}

bool ZeroTierDiscovery::send(const Link& link, const QHostAddress& destination, const QByteArray& payload)
{
    const auto iface = QNetworkInterface::interfaceFromIndex(link.interfaceIndex);
    bool present = false;
    for (const auto& entry : iface.addressEntries()) if (entry.ip() == link.address) present = true;
    if (!present || !(iface.flags() & QNetworkInterface::IsUp)) return false;
    if (destination.isMulticast()) m_socket.setMulticastInterface(iface);
    QNetworkDatagram datagram(payload, destination, DiscoveryPort);
    datagram.setSender(link.address);
    datagram.setInterfaceIndex(link.interfaceIndex);
    datagram.setHopLimit(1);
    const bool sent = m_socket.writeDatagram(datagram) == payload.size();
    if (!sent) m_sendError = tr("Ошибка отправки UDP 46931: %1. Проверьте интерфейс ZeroTier и правила брандмауэра.").arg(m_socket.errorString());
    return sent;
}

QByteArray ZeroTierDiscovery::announcement(bool goodbye) const
{
    return QJsonDocument(QJsonObject{{"protocol", "elysium-zt-host/1"}, {"id", m_id},
        {"network", m_hostLink.network}, {"name", m_name}, {"port", int(m_port)}, {"bye", goodbye}}).toJson(QJsonDocument::Compact);
}

void ZeroTierDiscovery::announce(bool goodbye)
{
    if (!hosting()) return;
    const auto prefix = m_hostLink.prefix;
    if (prefix < 1 || prefix > 30) return;
    m_sendError.clear();
    const auto payload = announcement(goodbye);
    const quint32 mask = 0xffffffffu << (32 - prefix);
    const QHostAddress broadcast((m_hostLink.address.toIPv4Address() & mask) | ~mask);
    const bool multicastSent = send(m_hostLink, MulticastGroup, payload);
    const bool broadcastSent = send(m_hostLink, broadcast, payload);
    bool unicastSent = false;
    for (const auto& subscriber : m_subscribers) unicastSent |= send(m_hostLink, subscriber.first, payload);
    if (multicastSent || broadcastSent || unicastSent) m_sendError.clear();
}

void ZeroTierDiscovery::query()
{
    for (const auto& link : m_links) {
        const auto payload = QJsonDocument(QJsonObject{{"protocol", "elysium-zt-query/1"},
            {"network", link.network}, {"id", m_id}}).toJson(QJsonDocument::Compact);
        send(link, MulticastGroup, payload);
        for (const auto& address : m_probes.value(link.network)) send(link, address, payload);
    }
}

bool ZeroTierDiscovery::probe(const QString& network, const QHostAddress& address)
{
    if (address.protocol() != QAbstractSocket::IPv4Protocol || address.isMulticast() || address.isNull()) return false;
    for (const auto& link : m_links) {
        if (link.prefix < 1 || link.prefix > 30 || link.network != network || address == link.address || !address.isInSubnet(link.address, link.prefix)) continue;
        const quint32 mask = 0xffffffffu << (32 - link.prefix);
        if ((address.toIPv4Address() & ~mask) == ~mask || (address.toIPv4Address() & ~mask) == 0) return false;
        auto& probes = m_probes[network];
        if (!probes.contains(address) && probes.size() < 16) probes << address;
        query();
        return true;
    }
    return false;
}

void ZeroTierDiscovery::receive()
{
    // Bound memory and work per event; names and endpoint data are untrusted.
    int budget = 100;
    while (m_socket.hasPendingDatagrams() && budget-- > 0) {
        const auto datagram = m_socket.receiveDatagram(1025);
        if (datagram.data().size() > 1024) continue;
        const auto json = QJsonDocument::fromJson(datagram.data()).object();
        const auto protocol = json.value("protocol").toString();
        if (protocol != "elysium-zt-host/1" && protocol != "elysium-zt-query/1") continue;
        const auto id = json.value("id").toString();
        const auto network = json.value("network").toString();
        if (id == m_id || !QRegularExpression("^[a-fA-F0-9-]{36}$").match(id).hasMatch()) continue;
        bool allowed = false;
        for (const auto& link : m_links) {
            if (link.network == network && datagram.senderAddress().isInSubnet(link.address, link.prefix) &&
                (!datagram.interfaceIndex() || datagram.interfaceIndex() == link.interfaceIndex)) allowed = true;
        }
        if (!allowed) continue;
        if (protocol == "elysium-zt-query/1") {
            if (hosting() && m_hostLink.network == network) {
                const auto key = datagram.senderAddress().toString();
                if (m_subscribers.contains(key) || m_subscribers.size() < 256)
                    m_subscribers[key] = {datagram.senderAddress(), QDateTime::currentMSecsSinceEpoch()};
                send(m_hostLink, datagram.senderAddress(), announcement());
            }
            continue;
        }
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
