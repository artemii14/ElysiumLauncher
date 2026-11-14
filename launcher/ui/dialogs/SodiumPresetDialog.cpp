// SPDX-License-Identifier: GPL-3.0-only
#include "SodiumPresetDialog.h"
#include "Application.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "modplatform/modrinth/SodiumPreset.h"
#include <QDialogButtonBox>
#include <QDateTime>
#include "Version.h"
#include <QEventLoop>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressDialog>
#include <QPushButton>
#include <QSet>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <stdexcept>

using namespace Elysium::Sodium;
namespace {
[[noreturn]] void fail(const QString& message) { throw std::runtime_error(message.toStdString()); }
}

SodiumPresetDialog::SodiumPresetDialog(QWidget* parent) : QDialog(parent), m_network(new QNetworkAccessManager(this))
{
    setWindowTitle(tr("Установить Sodium-сборку"));
    resize(780, 560);
    auto layout = new QVBoxLayout(this);
    auto info = new QLabel(tr("Sodium + Iris + LambDynamicLights + Zoomify (или Ok Zoomer). Fabric API обязателен для Fabric. Mod Menu и Sodium Extra добавляются по возможности. Уже установленные сборки не изменяются."), this);
    info->setWordWrap(true);
    layout->addWidget(info);
    m_status = new QLabel(tr("Загрузка актуального каталога Modrinth…"), this);
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::PlainText);
    layout->addWidget(m_status);
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({tr("Minecraft"), tr("Загрузчик"), tr("Приближение")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_reload = buttons->addButton(tr("Обновить каталог"), QDialogButtonBox::ActionRole);
    m_install = buttons->addButton(tr("Подготовить установку…"), QDialogButtonBox::ActionRole);
    m_install->setEnabled(false);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_reload, &QPushButton::clicked, this, &SodiumPresetDialog::reload);
    connect(m_install, &QPushButton::clicked, this, &SodiumPresetDialog::prepare);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this] { m_install->setEnabled(m_table->currentRow() >= 0); });
    layout->addWidget(buttons);
    QTimer::singleShot(0, this, [this] { reload(); });
}

QJsonDocument SodiumPresetDialog::fetch(const QString& path)
{
    if (m_progress && m_progress->wasCanceled()) fail(tr("Операция отменена."));
    if (m_cache.contains(path)) return m_cache.value(path);
    if (m_cache.size() >= 256) fail(tr("Слишком много зависимостей в ответе Modrinth."));
    if (m_progress) m_progress->setLabelText(tr("Проверка Modrinth: %1").arg(path));
    QNetworkRequest request(QUrl("https://api.modrinth.com/v2/" + path));
    request.setRawHeader("User-Agent", "ElysiumLauncher/Sodium-Presets (github.com/artemii14/ubiquitous-happiness)");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    auto reply = m_network->get(request);
    constexpr qsizetype Limit = 24 * 1024 * 1024;
    reply->setReadBufferSize(Limit + 1);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::readyRead, reply, [reply] { if (reply->bytesAvailable() > Limit) reply->abort(); });
    const auto cancel = connect(m_progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
    if (!reply->isFinished()) loop.exec();
    disconnect(cancel);
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool ok = reply->error() == QNetworkReply::NoError && status == 200;
    const auto bytes = reply->read(Limit + 1);
    reply->deleteLater();
    if (m_progress && m_progress->wasCanceled()) fail(tr("Операция отменена."));
    if (!ok || bytes.size() > Limit) fail(tr("Не удалось получить данные Modrinth (HTTP %1). Проверьте сеть или повторите позже.").arg(status));
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError) fail(tr("Некорректный ответ Modrinth."));
    m_cache[path] = json;
    return json;
}

void SodiumPresetDialog::reload()
{
    QProgressDialog progress(tr("Проверка каталога…"), tr("Отмена"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    m_progress = &progress;
    m_reload->setEnabled(false);
    m_table->setEnabled(false);
    m_install->setEnabled(false);
    m_table->setRowCount(0);
    m_cache.clear();
    m_projects.clear();
    try {
        QSet<QString> releases;
        const auto tags = fetch("tag/game_version");
        if (!tags.isArray()) fail(tr("Modrinth вернул неверный список Minecraft."));
        for (const auto& value : tags.array()) {
            const auto item = value.toObject();
            const auto game = item["version"].toString();
            if (item["version_type"] == "release" && stableMinecraft(game)) releases.insert(game);
        }
        for (const auto& slug : projects()) {
            const auto doc = fetch("project/" + slug + "/version");
            if (!doc.isArray()) fail(tr("Нет данных о версиях %1.").arg(slug));
            m_projects[slug] = newestFirst(doc.array());
        }
        QStringList games = releases.values();
        std::sort(games.begin(), games.end(), [](const QString& a, const QString& b) { return ::Version(a) > ::Version(b); });
        for (const auto& game : games) {
            for (const QString& loader : {QString("fabric"), QString("neoforge")}) {
                auto exists = [this, game, loader](const QString& project) {
                    for (const auto& value : m_projects.value(project)) if (supports(value.toObject(), game, loader)) return true;
                    return false;
                };
                if (!exists("sodium") || !exists("iris") || !exists("lambdynamiclights") ||
                    (loader == "fabric" && !exists("fabric-api"))) continue;
                const auto zoom = exists("zoomify") ? QString("zoomify") : exists("ok-zoomer") ? QString("ok-zoomer") : QString();
                if (zoom.isEmpty()) continue;
                const int row = m_table->rowCount();
                m_table->insertRow(row);
                m_table->setItem(row, 0, new QTableWidgetItem(game));
                m_table->setItem(row, 1, new QTableWidgetItem(loader));
                m_table->setItem(row, 2, new QTableWidgetItem(zoom));
            }
        }
        m_status->setText(tr("Доступно вариантов: %1. Перед установкой проверяются зависимости. Файлы модов могут быть beta/alpha; каталог не означает проверку запуска каждого набора.").arg(m_table->rowCount()));
    } catch (const std::exception& e) {
        m_table->setRowCount(0);
        m_status->setText(QString::fromUtf8(e.what()));
    }
    m_progress = nullptr;
    m_reload->setEnabled(true);
    m_table->setEnabled(true);
    if (m_table->rowCount()) m_table->selectRow(0);
}

void SodiumPresetDialog::prepare()
{
    const auto row = m_table->currentRow();
    if (row < 0) return;
    const auto game = m_table->item(row, 0)->text();
    const auto loader = m_table->item(row, 1)->text();
    QProgressDialog progress(tr("Подбор зависимостей…"), tr("Отмена"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    m_progress = &progress;
    m_install->setEnabled(false);
    m_reload->setEnabled(false);
    m_table->setEnabled(false);
    try {
        auto versions = [this](const QString& project) {
            if (!QRegularExpression("^[A-Za-z0-9_-]+$").match(project).hasMatch()) fail(tr("Некорректный ID проекта."));
            if (m_projects.contains(project)) return m_projects.value(project);
            const auto doc = fetch("project/" + project + "/version");
            if (!doc.isArray()) fail(tr("Некорректные зависимости Modrinth."));
            m_projects[project] = newestFirst(doc.array());
            return m_projects.value(project);
        };
        auto version = [this](const QString& id) {
            if (!QRegularExpression("^[A-Za-z0-9]+$").match(id).hasMatch()) fail(tr("Некорректный ID версии."));
            return fetch("version/" + id).object();
        };
        Resolver resolver(game, loader, versions, version);
        Resolver::Selection selected;
        QStringList roots{"sodium", "iris", "lambdynamiclights"};
        if (loader == "fabric") roots << "fabric-api";
        auto withZoom = roots;
        withZoom << "zoomify";
        if (!resolver.resolve(withZoom, selected)) {
            selected.clear();
            withZoom = roots;
            withZoom << "ok-zoomer";
            if (!resolver.resolve(withZoom, selected)) fail(tr("Нет полного набора модов с разрешимыми зависимостями для этой версии."));
        }
        resolver.addOptional("sodium-extra", selected);
        resolver.addOptional("modmenu", selected);
        const auto uid = loader == "fabric" ? QString("net.fabricmc.fabric-loader") : QString("net.neoforged");
        auto loaders = APPLICATION->metadataIndex()->get(uid);
        progress.setLabelText(tr("Проверка версии загрузчика…"));
        loaders->waitToLoad();
        if (progress.wasCanceled()) fail(tr("Операция отменена."));
        auto chosen = loaders->getLatestForParent("net.minecraft", game);
        if (!chosen && loader == "fabric") {
            for (const auto& candidate : loaders->versions())
                if (!chosen || candidate->toComparableVersion() > chosen->toComparableVersion()) chosen = candidate;
        }
        if (!chosen) fail(tr("Не найдена версия загрузчика для Minecraft %1.").arg(game));
        QJsonArray files;
        QSet<QString> names;
        QStringList summary;
        for (const auto& mod : selected) {
            const auto file = primaryFile(mod);
            const auto filename = file["filename"].toString();
            if (file.isEmpty() || names.contains(filename.toCaseFolded())) fail(tr("Некорректные или совпадающие имена JAR-файлов."));
            names.insert(filename.toCaseFolded());
            files.append(QJsonObject{{"path", "mods/" + filename}, {"hashes", file["hashes"]},
                {"downloads", QJsonArray{file["url"]}}, {"fileSize", file["size"]},
                {"env", QJsonObject{{"client", "required"}, {"server", "unsupported"}}}});
            summary << mod["name"].toString() + " [" + mod["version_type"].toString() + "]";
        }
        m_name = QString("Sodium %1 · %2").arg(game, loader == "fabric" ? "Fabric" : "NeoForge");
        m_manifest = QJsonObject{{"formatVersion", 1}, {"game", "minecraft"}, {"name", m_name},
            {"versionId", QString::number(QDateTime::currentSecsSinceEpoch())}, {"files", files},
            {"dependencies", QJsonObject{{"minecraft", game}, {loader == "fabric" ? "fabric-loader" : "neoforge", chosen->descriptor()}}}};
        progress.hide();
        if (QMessageBox::question(this, tr("Создать отдельную сборку?"),
            tr("%1\nЗагрузчик: %2\n\n%3\n\nМоды загружаются с Modrinth с проверкой SHA-512. Уже установленные сборки не изменяются. Совместимость по метаданным не гарантирует отсутствие ошибок в игре.").arg(m_name, chosen->descriptor(), summary.join('\n'))) == QMessageBox::Yes)
            accept();
        else m_manifest = {};
    } catch (const std::exception& e) {
        m_manifest = {};
        m_status->setText(QString::fromUtf8(e.what()));
    }
    m_progress = nullptr;
    m_table->setEnabled(true);
    m_reload->setEnabled(true);
    m_install->setEnabled(m_table->currentRow() >= 0);
}
