// SPDX-License-Identifier: GPL-3.0-only
#include "ZeroTierDialog.h"
#include "DesktopServices.h"
#include "Application.h"
#include "settings/SettingsObject.h"
#include "net/ZeroTierDiscovery.h"
#include <QInputDialog>
#include <QNetworkInterface>
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
    resize(820, 740);
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
    row->addWidget(m_id, 1);
    m_join = new QPushButton(tr("Подключиться"), this);
    row->addWidget(m_join);
    layout->addLayout(row);
    m_table = new QTableWidget(0, 4, this);
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
    actions->addWidget(m_copy);
    actions->addWidget(m_leave);
    layout->addLayout(actions);
    auto hostActions = new QHBoxLayout;
    m_host = new QPushButton(tr("Стать хостом…"), this);
    m_stopHost = new QPushButton(tr("Перестать быть хостом"), this);
    auto rename = new QPushButton(tr("Изменить имя…"), this);
    hostActions->addWidget(m_host);
    hostActions->addWidget(m_stopHost);
    hostActions->addWidget(rename);
    auto findHost = new QPushButton(tr("Найти по ZT IP…"), this);
    hostActions->addWidget(findHost);
    connect(findHost, &QPushButton::clicked, this, [this] {
        if (selectedNetwork().isEmpty()) {
            QMessageBox::information(this, tr("Поиск хоста"), tr("Сначала выберите подключённую сеть и нажмите «Обновить»."));
            return;
        }
        bool ok = false;
        const auto ip = QInputDialog::getText(this, tr("Прямой поиск хоста"),
            tr("Введите ZT IPv4 хоста. Этот способ не требует broadcast/multicast, но на обоих компьютерах должен быть разрешён UDP 46931."),
            QLineEdit::Normal, {}, &ok).trimmed();
        if (!ok) return;
        if (!ZeroTierDiscovery::instance()->probe(selectedNetwork(), QHostAddress(ip)))
            QMessageBox::warning(this, tr("Поиск хоста"), tr("Нужен IPv4 другого участника в подсети выбранной сети ZeroTier."));
        else QMessageBox::information(this, tr("Поиск хоста"), tr("Запрос отправлен. Если хост включён и UDP 46931 разрешён, адрес появится в списке. Ping проверяет ICMP, а не этот UDP-порт."));
    });
    layout->addLayout(hostActions);
    m_hostStatus = new QLabel(this);
    m_hostStatus->setWordWrap(true);
    m_hostStatus->setTextFormat(Qt::PlainText);
    layout->addWidget(m_hostStatus);
    m_hosts = new QTableWidget(0, 2, this);
    m_hosts->setHorizontalHeaderLabels({tr("Хосты в выбранной сети"), tr("IP:порт")});
    m_hosts->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_hosts->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_hosts->setSelectionMode(QAbstractItemView::SingleSelection);
    m_hosts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_hosts, 1);
    auto copyHost = new QPushButton(tr("Копировать адрес хоста"), this);
    copyHost->setEnabled(false);
    layout->addWidget(copyHost);
    connect(m_hosts, &QTableWidget::itemSelectionChanged, this, [this, copyHost] {
        copyHost->setEnabled(m_hosts->currentRow() >= 0);
    });
    connect(copyHost, &QPushButton::clicked, this, [this] {
        if (const auto item = m_hosts->item(m_hosts->currentRow(), 1))
            QApplication::clipboard()->setText(item->text());
    });
    connect(m_host, &QPushButton::clicked, this, &ZeroTierDialog::startHosting);
    connect(m_stopHost, &QPushButton::clicked, this, [this] {
        ZeroTierDiscovery::instance()->stopHost();
        updateHosts();
    });
    connect(rename, &QPushButton::clicked, this, [this] { askName(); updateHosts(); });
    auto discoveryTimer = new QTimer(this);
    connect(discoveryTimer, &QTimer::timeout, this, &ZeroTierDialog::updateHosts);
    discoveryTimer->start(2000);
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
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] { updateButtons(); updateHosts(); });
    connect(m_copy, &QPushButton::clicked, this, [this] {
        const auto item = m_table->item(m_table->currentRow(), 3);
        if (!item) return;
        // First assigned address, without the CIDR prefix length.
        QApplication::clipboard()->setText(item->text().section(", ", 0, 0).section('/', 0, 0));
    });
    connect(m_join, &QPushButton::clicked, this, [this] {
        const QString id = m_id->text().trimmed().toLower();
        if (!QRegularExpression("^[0-9a-f]{16}$").match(id).hasMatch()) return;
        if (QMessageBox::question(this, tr("Подключение к сети"),
            tr("Подключиться к сети %1? Участники сети смогут обращаться к вашему компьютеру в пределах правил брандмауэра. Маршруты по умолчанию, глобальные маршруты и DNS ZeroTier будут отключены.").arg(id)) != QMessageBox::Yes) return;
        request("/network/" + id, "POST", QJsonDocument(QJsonObject{
            {"allowManaged", true}, {"allowGlobal", false}, {"allowDefault", false}, {"allowDNS", false}}),
            [this](QJsonDocument) { askName(); refresh(); });
    });
    connect(m_leave, &QPushButton::clicked, this, [this] {
        const auto id = selectedNetwork();
        if (id.isEmpty()) return;
        if (QMessageBox::question(this, tr("Отключение"), tr("Отключиться от сети %1? Текущие игровые соединения через неё будут прерваны.").arg(id)) != QMessageBox::Yes) return;
        request("/network/" + id, "DELETE", {}, [this, id](QJsonDocument) { ZeroTierDiscovery::instance()->removeNetwork(id); refresh(); });
    });
    updateButtons();
    updateHosts();
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
        ZeroTierDiscovery::instance()->setLinks({});
        m_table->setRowCount(0);
        m_status->setText(tr("Нет доступа к ZeroTier. Установите ZeroTier One и запустите его службу. Если он уже установлен, выберите доступный вам файл authtoken.secret. Системные права доступа могут требовать отдельной настройки; не запускайте весь лаунчер от администратора."));
        updateButtons();
        return;
    }
    auto secret = token.read(4097).trimmed();
    if (secret.isEmpty() || secret.size() > 4096 || secret.contains('\r') || secret.contains('\n')) {
        m_status->setText(tr("Некорректный файл доступа ZeroTier."));
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
    connect(reply, &QNetworkReply::finished, this, [this, reply, success] {
        m_busy = false;
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool ok = reply->error() == QNetworkReply::NoError && code >= 200 && code < 300;
        const auto data = reply->read(1024 * 1024);
        reply->deleteLater();
        updateButtons();
        if (!ok) {
            ZeroTierDiscovery::instance()->setLinks({});
            m_status->setText(code == 401 || code == 403
                ? tr("Служба ZeroTier отклонила файл доступа. Выберите authtoken.secret этой службы.")
                : tr("Служба ZeroTier недоступна или запрос отклонён (HTTP %1). Проверьте установку и состояние службы, затем нажмите «Обновить».").arg(code));
            return;
        }
        QJsonParseError error;
        const auto json = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError && !data.isEmpty()) {
            m_status->setText(tr("Некорректный ответ локальной службы ZeroTier."));
            return;
        }
        success(json);
    });
}

void ZeroTierDialog::refresh()
{
    request("/status", "GET", {}, [this](QJsonDocument status) {
        const auto obj = status.object();
        m_status->setText(obj.value("online").toBool() ? tr("ZeroTier: в сети") : tr("ZeroTier: служба запущена, нет связи с сетью"));
        request("/network", "GET", {}, [this](QJsonDocument doc) {
            if (!doc.isArray()) { m_status->setText(tr("Не удалось прочитать список сетей ZeroTier.")); return; }
            const auto networks = doc.array();
            const auto previousNetwork = selectedNetwork();
            QList<ZeroTierDiscovery::Link> links;
            m_table->setRowCount(0);
            for (const auto& entry : networks) {
                const auto net = entry.toObject();
                const auto id = net.value("id").toString();
                if (!QRegularExpression("^[0-9a-fA-F]{16}$").match(id).hasMatch()) continue;
                QStringList ips;
                for (const auto& ip : net.value("assignedAddresses").toArray()) ips << ip.toString();
                if (net.value("status").toString() == "OK") {
                    for (const auto& assigned : net.value("assignedAddresses").toArray()) {
                        const auto cidr = QHostAddress::parseSubnet(assigned.toString());
                        const QHostAddress address(assigned.toString().section('/', 0, 0));
                        if (address.protocol() != QAbstractSocket::IPv4Protocol || cidr.second < 1 || cidr.second > 30) continue;
                        for (const auto& iface : QNetworkInterface::allInterfaces()) {
                            if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
                            for (const auto& entry : iface.addressEntries()) {
                                if (entry.ip() == address) links << ZeroTierDiscovery::Link{id, address, cidr.second, uint(iface.index())};
                            }
                        }
                    }
                }
                QString state = net.value("status").toString();
                if (state == "OK") state = tr("Подключено");
                else if (state == "ACCESS_DENIED") state = tr("Ожидает разрешения владельца / доступ запрещён");
                else if (state == "REQUESTING_CONFIGURATION") state = tr("Получение настроек");
                const int row = m_table->rowCount();
                m_table->insertRow(row);
                const QStringList values{id, net.value("name").toString(), state, ips.join(", ")};
                for (int col = 0; col < values.size(); ++col) m_table->setItem(row, col, new QTableWidgetItem(values[col]));
                if (id == previousNetwork) m_table->selectRow(row);
            }
            ZeroTierDiscovery::instance()->setLinks(links);
            if (m_table->currentRow() < 0 && m_table->rowCount()) m_table->selectRow(0);
            updateButtons();
            updateHosts();
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
    m_host->setEnabled(!m_busy && ip && !ip->text().isEmpty());
    m_stopHost->setEnabled(ZeroTierDiscovery::instance()->hosting());
}

bool ZeroTierDialog::askName()
{
    bool ok = false;
    const auto name = QInputDialog::getText(this, tr("Имя в сети"),
        tr("Как вас будут видеть другие пользователи лаунчера? (до 48 символов)"),
        QLineEdit::Normal, APPLICATION->settings()->get("ZeroTierDisplayName").toString(), &ok).trimmed();
    if (!ok) return false;
    if (name.isEmpty() || name.size() > 48 || name.contains(QRegularExpression("[\\x00-\\x1f\\x7f]"))) {
        QMessageBox::warning(this, tr("Имя в сети"), tr("Введите имя от 1 до 48 символов без управляющих символов."));
        return false;
    }
    APPLICATION->settings()->set("ZeroTierDisplayName", name);
    ZeroTierDiscovery::instance()->setName(name);
    return true;
}

void ZeroTierDialog::startHosting()
{
    const auto network = selectedNetwork();
    const auto ips = m_table->item(m_table->currentRow(), 3);
    if (network.isEmpty() || !ips) return;
    auto name = APPLICATION->settings()->get("ZeroTierDisplayName").toString();
    if (name.isEmpty()) {
        if (!askName()) return;
        name = APPLICATION->settings()->get("ZeroTierDisplayName").toString();
    }
    QString suggested;
    const auto addresses = ips->text().split(", ");
    for (const auto& ip : addresses) {
        const auto plain = ip.section('/', 0, 0);
        if (QHostAddress(plain).protocol() == QAbstractSocket::IPv4Protocol) { suggested = plain; break; }
    }
    bool ok = false;
    const auto ip = QInputDialog::getText(this, tr("Стать хостом"),
        tr("Ваш IPv4 в выбранной сети ZeroTier (подставлен автоматически):"), QLineEdit::Normal, suggested, &ok).trimmed();
    if (!ok) return;
    const int port = QInputDialog::getInt(this, tr("Порт Minecraft"),
        tr("Откройте мир для сети и введите порт, показанный Minecraft. Лаунчер не запускает сервер и не открывает порт в брандмауэре."),
        25565, 1, 65535, 1, &ok);
    if (!ok) return;
    if (QMessageBox::question(this, tr("Публикация адреса"),
        tr("Показать ваше имя и %1:%2 участникам этой сети? Объявления не подтверждают личность хоста. Закрытие этого окна не остановит публикацию: используйте «Перестать быть хостом» или закройте лаунчер.").arg(ip).arg(port)) != QMessageBox::Yes) return;
    if (!ZeroTierDiscovery::instance()->startHost(network, name, QHostAddress(ip), quint16(port))) {
        QMessageBox::warning(this, tr("Не удалось стать хостом"),
            tr("Нужен назначенный локальный IPv4 выбранной подключённой сети ZeroTier и доступный UDP-порт 46931. Нажмите «Обновить» и проверьте выбранный IP."));
    }
    updateHosts();
}

void ZeroTierDialog::updateHosts()
{
    auto service = ZeroTierDiscovery::instance();
    QString selectedEndpoint;
    if (const auto item = m_hosts->item(m_hosts->currentRow(), 1)) selectedEndpoint = item->text();
    const auto hosts = service->hosts(selectedNetwork());
    m_hosts->setRowCount(0);
    for (const auto& host : hosts) {
        const auto row = m_hosts->rowCount();
        m_hosts->insertRow(row);
        m_hosts->setItem(row, 0, new QTableWidgetItem(host.name));
        m_hosts->setItem(row, 1, new QTableWidgetItem(host.endpoint));
        if (host.endpoint == selectedEndpoint) m_hosts->selectRow(row);
    }
    m_hostStatus->setText(!service->error().isEmpty() ? service->error() :
        service->hosting() ? tr("Вы публикуете адрес в сети %1. Другим нужен открытый эмулятор сети ElysiumLauncher и доступный UDP-порт 46931.").arg(service->hostNetwork()) :
        tr("Выберите сеть, чтобы увидеть хостов. Поиск использует multicast и broadcast. Если список пуст, нажмите «Найти по ZT IP…». Адреса не проверяют личность участников."));
    m_stopHost->setEnabled(service->hosting());
}
