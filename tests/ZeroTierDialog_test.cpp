// SPDX-License-Identifier: GPL-3.0-only
#include <QtTest>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include "ui/dialogs/ZeroTierDialog.h"

class ZeroTierDialogTest : public QObject {
    Q_OBJECT
private slots:
    void networkIdValidation()
    {
        // No event loop is entered: the deferred API request never starts.
        ZeroTierDialog dialog;
        auto id = dialog.findChild<QLineEdit*>("zeroTierNetworkId");
        auto join = dialog.findChild<QPushButton*>("zeroTierJoin");
        QVERIFY(id);
        QVERIFY(join);
        QVERIFY(!join->isEnabled());
        id->setText("123");
        QVERIFY(!join->isEnabled());
        id->setText("0123456789abcdeg");
        QVERIFY(!join->isEnabled());
        id->setText("0123456789ABCDEF");
        QVERIFY(join->isEnabled());
        id->clear();
        QVERIFY(!join->isEnabled());
    }
    void noHostControlsOrStaleAddresses()
    {
        ZeroTierDialog dialog;
        auto table = dialog.findChild<QTableWidget*>("zeroTierNetworks");
        auto copy = dialog.findChild<QPushButton*>("zeroTierCopyIp");
        QVERIFY(table);
        QCOMPARE(table->rowCount(), 0);
        QVERIFY(copy);
        QVERIFY(!copy->isEnabled());
        QCOMPARE(dialog.findChildren<QTableWidget*>().size(), 1);
        for (const auto button : dialog.findChildren<QPushButton*>()) {
            QVERIFY(!button->text().contains("хост", Qt::CaseInsensitive));
        }
    }
};
QTEST_MAIN(ZeroTierDialogTest)
#include "ZeroTierDialog_test.moc"
