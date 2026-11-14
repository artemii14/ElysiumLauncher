// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include "net/ZeroTierDiscovery.h"

class ZeroTierDiscoveryTest : public QObject {
    Q_OBJECT
private slots:
    void init() { ZeroTierDiscovery::instance()->stopHost(); ZeroTierDiscovery::instance()->setLinks({}); }
    void rejectsUnknownNetwork()
    {
        auto service = ZeroTierDiscovery::instance();
        QVERIFY(!service->startHost("0123456789abcdef", "Player", QHostAddress("192.0.2.2"), 25565));
        QVERIFY(!service->hosting());
    }
    void hostLifecycle()
    {
        auto service = ZeroTierDiscovery::instance();
        if (!service->error().isEmpty()) QSKIP("Discovery socket unavailable on this runner");
        // TEST-NET address with no interface: announce() cannot send packets.
        service->setLinks({{"0123456789abcdef", QHostAddress("192.0.2.2"), 24, 0}});
        QVERIFY(!service->startHost("0123456789abcdef", "", QHostAddress("192.0.2.2"), 25565));
        QVERIFY(!service->startHost("0123456789abcdef", "Player", QHostAddress("192.0.2.3"), 25565));
        QVERIFY(!service->startHost("0123456789abcdef", "Player", QHostAddress("192.0.2.2"), 0));
        QVERIFY(service->startHost("0123456789abcdef", "Player", QHostAddress("192.0.2.2"), 51234));
        QVERIFY(service->hosting());
        QCOMPARE(service->hosts("0123456789abcdef").size(), 1);
        QCOMPARE(service->hosts("0123456789abcdef").first().endpoint, QString("192.0.2.2:51234"));
        QVERIFY(service->hosts("fedcba9876543210").isEmpty());
        service->setName("New Name");
        QVERIFY(service->hosts("0123456789abcdef").first().name.startsWith("New Name"));
        service->removeNetwork("0123456789abcdef");
        QVERIFY(!service->hosting());
        QVERIFY(service->hosts("0123456789abcdef").isEmpty());
    }
    void stopHosting()
    {
        auto service = ZeroTierDiscovery::instance();
        if (!service->error().isEmpty()) QSKIP("Discovery socket unavailable on this runner");
        service->setLinks({{"0123456789abcdef", QHostAddress("192.0.2.2"), 24, 0}});
        QVERIFY(service->startHost("0123456789abcdef", "Player", QHostAddress("192.0.2.2"), 25565));
        service->stopHost();
        QVERIFY(!service->hosting());
        QVERIFY(service->hosts("0123456789abcdef").isEmpty());
    }
};
QTEST_GUILESS_MAIN(ZeroTierDiscoveryTest)
#include "ZeroTierDiscovery_test.moc"
