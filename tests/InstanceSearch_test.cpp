// SPDX-License-Identifier: GPL-3.0-only
#include <QStandardItemModel>
#include <QtTest>
#include "ui/instanceview/InstanceProxyModel.h"
#include "ui/instanceview/InstanceView.h"

class InstanceSearchTest : public QObject {
    Q_OBJECT
private slots:
    void literalNameAndGroupSearch()
    {
        QStandardItemModel source;
        auto fabric = new QStandardItem(QStringLiteral("Fabric [1.21]"));
        fabric->setData(QStringLiteral("Survival"), InstanceViewRoles::GroupRole);
        auto vanilla = new QStandardItem(QStringLiteral("Vanilla"));
        vanilla->setData(QStringLiteral("Creative"), InstanceViewRoles::GroupRole);
        source.appendRow(fabric);
        source.appendRow(vanilla);
        InstanceProxyModel proxy;
        proxy.setSourceModel(&source);
        QCOMPARE(proxy.rowCount(), 2);
        proxy.setSearchText(QStringLiteral(" fAbRiC "));
        QCOMPARE(proxy.rowCount(), 1);
        QCOMPARE(proxy.index(0, 0).data().toString(), QStringLiteral("Fabric [1.21]"));
        proxy.setSearchText(QStringLiteral("SURVIVAL"));
        QCOMPARE(proxy.rowCount(), 1);
        proxy.setSearchText(QStringLiteral("[1.21]"));
        QCOMPARE(proxy.rowCount(), 1);
        proxy.setSearchText(QStringLiteral(".*"));
        QCOMPARE(proxy.rowCount(), 0);
        proxy.setSearchText(QStringLiteral("   "));
        QCOMPARE(proxy.rowCount(), 2);
    }
    void unicodeSearch()
    {
        QStandardItemModel source;
        source.appendRow(new QStandardItem(QString::fromUtf8("Мой мир")));
        InstanceProxyModel proxy;
        proxy.setSourceModel(&source);
        proxy.setSearchText(QString::fromUtf8("МИР"));
        QCOMPARE(proxy.rowCount(), 1);
    }
    void reactsToSourceChanges()
    {
        QStandardItemModel source;
        InstanceProxyModel proxy;
        proxy.setSourceModel(&source);
        proxy.setSearchText(QStringLiteral("world"));
        source.appendRow(new QStandardItem(QStringLiteral("New world")));
        QCOMPARE(proxy.rowCount(), 1);
        source.item(0)->setText(QStringLiteral("Other"));
        QCOMPARE(proxy.rowCount(), 0);
        source.clear();
        QCOMPARE(proxy.rowCount(), 0);
        proxy.setSearchText(QString());
        QCOMPARE(proxy.rowCount(), 0);
    }
};
QTEST_GUILESS_MAIN(InstanceSearchTest)
#include "InstanceSearch_test.moc"
