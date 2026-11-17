// SPDX-License-Identifier: GPL-3.0-only
#include "translations/LiveTranslation.h"
#include "ZeroTierDialog.h"
#include "DesktopServices.h"
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

ZeroTierDialog::ZeroTierDialog(QWidget* parent) : QDialog(parent), m_network(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("Эмулятор локальной сети · ZeroTier"));
    resize(820, 540);
    m_network->setProxy(QNetworkProxy::NoProxy);
    auto layout = new QVBoxLayout(this);
    auto description = new QLabel(tr("Подключитесь с друзьями к одной сети ZeroTier. Владелец закрытой сети должен разрешить доступ каждому участнику."), this);
    description->setWordWrap(true);
    layout->addWidget(description);
    m_status = new QLabel(tr("Проверка службы ZeroTier…"), this);
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::PlainText);
    layout->addWidget(m_status);
    auto row = new QHBoxLayout;
    m_id = new QLineEdit(this);
    m_id->setPlaceholderText(tr("Network ID — 16 шестнадцатеричных символов"));
    m_id->setAccessibleName(tr("Идентификатор сети ZeroTier"));
    m_id->setMaxLength(16);
    m_id->setObjectName(QStringLiteral("zeroTierNetworkId"));
    row->addWidget(m_id, 1);
    m_join = new QPushButton(tr("Подключиться"), this);
    m_join->setObjectName(QStringLiteral("zeroTierJoin"));
    row->addWidget(m_join);
    layout->addLayout(row);
    m_table = new QTableWidget(0, 4, this);
    m_table->setObjectName(QStringLiteral("zeroTierNetworks"));
    m_table->setHorizontalHeaderLabels({tr("Network ID"), tr("Название"), tr("Состояние"), tr("IP-адреса")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_table, 1);
    auto actions = new QHBoxLayout;
    m_refresh = new QPushButton(tr("Обновить"), this);
    m_copy = new QPushButton(tr("Копировать IP"), this);
    m_leave = new QPushButton(tr("Отключиться от сети"), this);
    actions->addWidget(m_refresh);
    m_copy->setObjectName(QStringLiteral("zeroTierCopyIp"));
    actions->addWidget(m_copy);
    actions->addWidget(m_leave);
    layout->addLayout(actions);
    auto refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, [this] { if (!m_busy) refresh(); });
    refreshTimer->start(15000);
    auto note = new QLabel(tr("В Minecraft откройте мир для сети и сообщите друзьям IP ZeroTier и порт игры. Автоматическое обнаружение LAN-мира не гарантируется. Подключайтесь только к доверенным сетям."), this);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto help = new QHBoxLayout;
    auto install = new QPushButton(tr("Установить ZeroTier…"), this);
    auto central = new QPushButton(tr("Управление сетью…"), this);
    auto token = new QPushButton(tr("Выбрать файл доступа…"), this);
    help->addWidget(install);
    help->addWidget(central);
    help->addWidget(token);
    layout->addLayout(help);
    auto close = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(close, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(close);
    connect(install, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, tr("Установка ZeroTier One"),
            tr("Открыть официальный сайт ZeroTier для установки? Может потребоваться разрешение администратора. Установка необязательна; без неё эта функция недоступна.")) == QMessageBox::Yes)
            DesktopServices::openUrl(QUrl("https://www.zerotier.com/download/"));
    });
    connect(central, &QPushButton::clicked, this, [] { DesktopServices::openUrl(QUrl("https://central.zerotier.com/")); });
    connect(token, &QPushButton::clicked, this, &ZeroTierDialog::selectToken);
    connect(m_refresh, &QPushButton::clicked, this, &ZeroTierDialog::refresh);
    connect(m_id, &QLineEdit::textChanged, this, [this] { updateButtons(); });
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] { updateButtons(); });
    connect(m_copy, &QPushButton::clicked, this, [this] {
        const auto item = m_table->item(m_table->currentRow(), 3);
        if (!item) return;
        const auto addresses = item->text().split(", ", Qt::SkipEmptyParts);
        QString selected = addresses.value(0);
        if (addresses.size() > 1) {
            bool ok = false;
            selected = QInputDialog::getItem(this, tr("Копировать IP"), tr("Выберите адрес:"), addresses, 0, false, &ok);
            if (!ok) return;
        }
        QApplication::clipboard()->setText(selected.section('/', 0, 0));
    });
    connect(m_join, &QPushButton::clicked, this, [this] {
        const QString id = m_id->text().trimmed().toLower();
        if (!QRegularExpression("^[0-9a-f]{16}$").match(id).hasMatch()) return;
        if (QMessageBox::question(this, tr("Подключение к сети"),
            tr("Подключиться к сети %1? Участники сети смогут обращаться к вашему компьютеру в пределах правил брандмауэра. Маршруты по умолчанию, глобальные маршруты и DNS ZeroTier будут отключены.").arg(id)) != QMessageBox::Yes) return;
        request("/network/" + id, "POST", QJsonDocument(QJsonObject{
            {"allowManaged", true}, {"allowGlobal", false}, {"allowDefault", false}, {"allowDNS", false}}),
            [this](QJsonDocument) { refresh(); });
    });
    connect(m_leave, &QPushButton::clicked, this, [this] {
        const auto id = selectedNetwork();
        if (id.isEmpty()) return;
        if (QMessageBox::question(this, tr("Отключение"), tr("Отключиться от сети %1? Текущие игровые соединения через неё будут прерваны.").arg(id)) != QMessageBox::Yes) return;
        request("/network/" + id, "DELETE", {}, [this](QJsonDocument) { refresh(); });
    });
    updateButtons();
    new LiveTranslation(this);
    QTimer::singleShot(0, this, [this] { refresh(); });
}

QString ZeroTierDialog::tokenPath() const
{
    if (!m_tokenOverride.isEmpty()) return m_tokenOverride;
#ifdef Q_OS_WIN
    return QDir(qEnvironmentVariable("ProgramData", "C:/ProgramData")).filePath("ZeroTier/One/authtoken.secret");
#elif defined(Q_OS_MACOS)
    return "/Library/Application Support/ZeroTier/One/authtoken.secret";
#else
    return "/var/lib/zerotier-one/authtoken.secret";
#endif
}

void ZeroTierDialog::selectToken()
{
    QMessageBox::information(this, tr("Доступ к локальной службе"),
        tr("Выберите authtoken.secret установленной службы ZeroTier. Этот секрет даёт управление локальной службой: не отправляйте его другим людям и не меняйте права системного файла на общедоступные. Лаунчер не сохраняет секрет в настройках и не отправляет его в интернет."));
    const auto file = QFileDialog::getOpenFileName(this, tr("Файл доступа ZeroTier"), tokenPath(), "ZeroTier token (authtoken.secret)");
    if (file.isEmpty()) return;
    m_tokenOverride = file;
    refresh();
}

void ZeroTierDialog::request(const QString& path, const QByteArray& method,
                            const QJsonDocument& body, std::function<void(QJsonDocument)> success)
{
    if (m_busy) return;
    QFile token(tokenPath());
    if (!token.open(QIODevice::ReadOnly)) {
        showError(tr("Нет доступа к ZeroTier. Установите ZeroTier One и запустите его службу. Если он уже установлен, выберите доступный вам файл authtoken.secret. Системные права доступа могут требовать отдельной настройки; не запускайте весь лаунчер от администратора."));
        return;
    }
    const auto rawSecret = token.read(4097);
    auto secret = rawSecret.trimmed();
    if (secret.isEmpty() || rawSecret.size() > 4096 || secret.contains('\r') || secret.contains('\n')) {
        showError(tr("Некорректный файл доступа ZeroTier."));
        return;
    }
    QNetworkRequest req(QUrl("http://127.0.0.1:9993" + path));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(5000);
    req.setRawHeader("X-ZT1-Auth", secret);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    secret.fill('\0');
    m_busy = true;
    updateButtons();
    auto reply = m_network->sendCustomRequest(req, method, body.isNull() ? QByteArray() : body.toJson(QJsonDocument::Compact));
    reply->setReadBufferSize(1024 * 1024 + 1);
    connect(reply, &QNetworkReply::readyRead, this, [reply] {
        if (reply->bytesAvailable() > 1024 * 1024) {
            reply->setProperty("elysiumOversized", true);
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, success] {
        m_busy = false;
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool ok = reply->error() == QNetworkReply::NoError && code >= 200 && code < 300;
        const auto data = reply->read(1024 * 1024);
        reply->deleteLater();
        updateButtons();
        if (reply->property("elysiumOversized").toBool()) {
            showError(tr("Ответ службы ZeroTier слишком большой."));
            return;
        }
        if (!ok) {
            showError(code == 401 || code == 403
                ? tr("Служба ZeroTier отклонила файл доступа. Выберите authtoken.secret этой службы.")
                : tr("Служба ZeroTier недоступна или запрос отклонён (HTTP %1). Проверьте установку и состояние службы, затем нажмите «Обновить».").arg(code));
            return;
        }
        QJsonParseError error;
        const auto json = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError && !data.isEmpty()) {
            showError(tr("Некорректный ответ локальной службы ZeroTier."));
            return;
        }
        success(json);
    });
}

void ZeroTierDialog::refresh()
{
    request("/status", "GET", {}, [this](QJsonDocument status) {
        if (!status.isObject()) { showError(tr("Некорректный статус службы ZeroTier.")); return; }
        const auto obj = status.object();
        m_status->setText(obj.value("online").toBool() ? tr("ZeroTier: в сети") : tr("ZeroTier: служба запущена, нет связи с сетью"));
        request("/network", "GET", {}, [this](QJsonDocument doc) {
            if (!doc.isArray()) { showError(tr("Не удалось прочитать список сетей ZeroTier.")); return; }
            const auto networks = doc.array();
            const auto previousNetwork = selectedNetwork();
            m_table->setRowCount(0);
            for (const auto& entry : networks) {
                const auto net = entry.toObject();
                const auto id = net.value("id").toString();
                if (!QRegularExpression("^[0-9a-fA-F]{16}$").match(id).hasMatch()) continue;
                QStringList ips;
                for (const auto& ip : net.value("assignedAddresses").toArray()) ips << ip.toString();
                QString state = net.value("status").toString();
                if (state == "OK") state = tr("Подключено");
                else if (state == "ACCESS_DENIED") state = tr("Ожидает разрешения владельца / доступ запрещён");
                else if (state == "REQUESTING_CONFIGURATION") state = tr("Получение настроек");
                else if (state == "NOT_FOUND") state = tr("Сеть не найдена");
                else if (state == "PORT_ERROR") state = tr("Ошибка виртуального адаптера");
                else if (state == "CLIENT_TOO_OLD") state = tr("Требуется обновить ZeroTier One");
                const int row = m_table->rowCount();
                m_table->insertRow(row);
                const QStringList values{id, net.value("name").toString(), state, ips.join(", ")};
                for (int col = 0; col < values.size(); ++col) m_table->setItem(row, col, new QTableWidgetItem(values[col]));
                if (id == previousNetwork) m_table->selectRow(row);
            }
            if (m_table->currentRow() < 0 && m_table->rowCount()) m_table->selectRow(0);
            updateButtons();
        });
    });
}

QString ZeroTierDialog::selectedNetwork() const
{
    const auto item = m_table->item(m_table->currentRow(), 0);
    return item ? item->text() : QString();
}

void ZeroTierDialog::updateButtons()
{
    m_join->setEnabled(!m_busy && QRegularExpression("^[0-9a-fA-F]{16}$").match(m_id->text().trimmed()).hasMatch());
    m_leave->setEnabled(!m_busy && !selectedNetwork().isEmpty());
    m_refresh->setEnabled(!m_busy);
    const auto ip = m_table->item(m_table->currentRow(), 3);
    m_copy->setEnabled(!m_busy && ip && !ip->text().isEmpty());
}


void ZeroTierDialog::showError(const QString& message)
{
    m_table->setRowCount(0);
    m_status->setText(message);
    updateButtons();
}
