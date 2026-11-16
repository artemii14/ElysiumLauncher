// SPDX-License-Identifier: GPL-3.0-only
#include "SodiumPresetDialog.h"
#include "Application.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "tasks/Task.h"
#include "modplatform/modrinth/SodiumPreset.h"
#include "modplatform/modrinth/SodiumRuntime.h"
#include <QCryptographicHash>
#include <QCheckBox>
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
    auto info = new QLabel(tr("Sodium + Iris + Zoomify (или Ok Zoomer). Fabric API обязателен для Fabric. Mod Menu и Sodium Extra добавляются по возможности. Уже установленные сборки не изменяются."), this);
    info->setWordWrap(true);
    layout->addWidget(info);
    m_dynamicLights = new QCheckBox(tr("Добавить LambDynamicLights при совместимости"), this);
    m_dynamicLights->setObjectName(QStringLiteral("sodiumDynamicLights"));
    m_dynamicLights->setChecked(true);
    layout->addWidget(m_dynamicLights);
    m_shaders = new QCheckBox(tr("Установить Complementary Unbound и Reimagined для Iris"), this);
    m_shaders->setObjectName(QStringLiteral("sodiumComplementaryShaders"));
    m_shaders->setChecked(true);
    m_shaders->setToolTip(tr("ZIP-файлы будут помещены в shaderpacks. Выберите один пакет в настройках Iris внутри игры."));
    layout->addWidget(m_shaders);
    m_status = new QLabel(tr("Загрузка актуального каталога Modrinth…"), this);
    m_status->setWordWrap(true);
    m_status->setTextFormat(Qt::PlainText);
    layout->addWidget(m_status);
    m_table = new QTableWidget(0, 4, this);
    m_table->setObjectName(QStringLiteral("sodiumPresetTable"));
    m_table->setHorizontalHeaderLabels({tr("Minecraft"), tr("Загрузчик"), tr("Мод на зум"), tr("Доступность")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_table);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_reload = buttons->addButton(tr("Обновить каталог"), QDialogButtonBox::ActionRole);
    m_install = buttons->addButton(tr("Подготовить установку…"), QDialogButtonBox::ActionRole);
    m_install->setObjectName(QStringLiteral("sodiumPresetInstall"));
    m_install->setEnabled(false);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_reload, &QPushButton::clicked, this, &SodiumPresetDialog::reload);
    connect(m_install, &QPushButton::clicked, this, &SodiumPresetDialog::prepare);
    const auto refreshInstall = [this] { m_install->setEnabled(!m_progress && canPrepare()); };
    connect(m_table, &QTableWidget::itemSelectionChanged, this, refreshInstall);
    connect(m_table, &QTableWidget::currentCellChanged, this, refreshInstall);
    connect(m_table, &QTableWidget::itemChanged, this, refreshInstall);
    connect(m_table->model(), &QAbstractItemModel::rowsRemoved, this, refreshInstall);
    connect(m_table->model(), &QAbstractItemModel::modelReset, this, refreshInstall);
    layout->addWidget(buttons);
    QTimer::singleShot(0, this, [this] { reload(); });
}

QJsonDocument SodiumPresetDialog::fetch(const QString& path)
{
    if (m_progress && m_progress->wasCanceled()) fail(tr("Операция отменена."));
    if (m_cache.contains(path)) return m_cache.value(path);
    if (m_cache.size() >= 256) fail(tr("Слишком много зависимостей в ответе Modrinth."));
    if (m_progress) m_progress->setLabelText(tr("Проверка Modrinth: %1").arg(path));
    const auto bytes = fetchBytes(QUrl("https://api.modrinth.com/v2/" + path), 24 * 1024 * 1024);
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError) fail(tr("Некорректный ответ Modrinth."));
    if (m_cacheBytes + bytes.size() > 96 * 1024 * 1024) fail(tr("Каталог Modrinth слишком большой. Попробуйте повторить загрузку позже."));
    m_cacheBytes += bytes.size();
    m_cache[path] = json;
    return json;
}

QByteArray SodiumPresetDialog::fetchBytes(const QUrl& url, qint64 limit)
{
    if (m_progress && m_progress->wasCanceled()) fail(tr("Операция отменена."));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "ElysiumLauncher/Sodium-Presets (github.com/artemii14/ubiquitous-happiness)");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    auto reply = m_network->get(request);
    const qint64 Limit = limit;
    reply->setReadBufferSize(Limit + 1);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::readyRead, reply, [reply, Limit] { if (reply->bytesAvailable() > Limit) reply->abort(); });
    QTimer deadline;
    deadline.setSingleShot(true);
    connect(&deadline, &QTimer::timeout, reply, &QNetworkReply::abort);
    deadline.start(30000);
    const auto cancel = connect(m_progress, &QProgressDialog::canceled, reply, &QNetworkReply::abort);
    if (!reply->isFinished()) loop.exec();
    disconnect(cancel);
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool ok = reply->error() == QNetworkReply::NoError && status == 200;
    const auto bytes = reply->read(Limit + 1);
    const auto networkError = reply->errorString();
    reply->deleteLater();
    if (m_progress && m_progress->wasCanceled()) fail(tr("Операция отменена."));
    if (!ok || bytes.size() > Limit) fail(tr("Ошибка загрузки %1%2 (HTTP %3): %4").arg(url.host(), url.path(), QString::number(status), networkError));
    return bytes;
}

bool SodiumPresetDialog::canPrepare() const
{
    const auto row = m_table->currentRow();
    const auto status = row >= 0 && row < m_table->rowCount() ? m_table->item(row, 3) : nullptr;
    return status && status->data(Qt::UserRole).toBool();
}

void SodiumPresetDialog::reload()
{
    if (m_progress) return;
    QProgressDialog progress(tr("Проверка каталога…"), tr("Отмена"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    m_progress = &progress;
    m_reload->setEnabled(false);
    m_table->setEnabled(false);
    m_install->setEnabled(false);
    m_table->setRowCount(0);
    m_cache.clear();
    m_cacheBytes = 0;
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
                if (!exists("sodium")) continue;
                QStringList missing;
                for (const auto& project : QStringList{"iris"}) if (!exists(project)) missing << project;
                if (loader == "fabric" && !exists("fabric-api")) missing << "fabric-api";
                const auto zoom = exists("zoomify") ? QString("zoomify") : exists("ok-zoomer") ? QString("ok-zoomer") : QString();
                if (zoom.isEmpty()) missing << "Zoomify / Ok Zoomer";
                const int row = m_table->rowCount();
                m_table->insertRow(row);
                m_table->setItem(row, 0, new QTableWidgetItem(game));
                m_table->setItem(row, 1, new QTableWidgetItem(loader));
                m_table->setItem(row, 2, new QTableWidgetItem(zoom == "zoomify" ? QString("Zoomify") : QString("Ok Zoomer")));
                auto availability = new QTableWidgetItem(missing.isEmpty() ? tr("Проверить совместимость") : tr("Нет: %1").arg(missing.join(", ")));
                availability->setData(Qt::UserRole, missing.isEmpty());
                m_table->setItem(row, 3, availability);
            }
        }
        m_status->setText(tr("Вариантов в каталоге: %1. Недоступные строки указывают недостающие обязательные моды. Перед установкой проверяются JAR-зависимости. Моды могут быть beta/alpha; игровой запуск каждого набора не проверен.").arg(m_table->rowCount()));
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
    if (m_progress) return;
    const auto row = m_table->currentRow();
    if (!canPrepare()) return;
    const auto game = m_table->item(row, 0)->text();
    const auto loader = m_table->item(row, 1)->text();
    const bool addLights = m_dynamicLights->isChecked();
    const bool addShaders = m_shaders->isChecked();
    QProgressDialog progress(tr("Подбор зависимостей…"), tr("Отмена"), 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    m_progress = &progress;
    m_install->setEnabled(false);
    m_reload->setEnabled(false);
    m_table->setEnabled(false);
    // Recheck at installation time even when the catalog window stayed open.
    m_cache.clear();
    m_cacheBytes = 0;
    m_projects.clear();
    m_manifest = {};
    try {
        auto versions = [this](const QString& project) {
            if (!QRegularExpression("^[A-Za-z0-9_-]+$").match(project).hasMatch()) fail(tr("Некорректный ID проекта."));
            if (m_projects.contains(project)) return m_projects.value(project);
            const auto doc = fetch("project/" + project + "/version");
            if (!doc.isArray()) fail(tr("Некорректные зависимости Modrinth."));
            const auto sorted = newestFirst(doc.array());
            m_projects[project] = sorted;
            if (!sorted.isEmpty()) {
                const auto canonical = sorted.first().toObject()["project_id"].toString();
                if (!canonical.isEmpty()) m_projects[canonical] = sorted;
            }
            return sorted;
        };
        auto version = [this](const QString& id) {
            if (!QRegularExpression("^[A-Za-z0-9]+$").match(id).hasMatch()) fail(tr("Некорректный ID версии."));
            return fetch("version/" + id).object();
        };
        QMap<QString, QList<RuntimeMod>> runtimeCache;
        QMap<QString, QString> invalidMetadata;
        qint64 jarBytes = 0;
        QString lastConflict;
        auto compatible = [&](const Resolver::Selection& selection) {
            QList<RuntimeMod> mods;
            for (const auto& candidate : selection) {
                const auto id = candidate["id"].toString();
                if (invalidMetadata.contains(id)) { lastConflict = invalidMetadata.value(id); return false; }
                if (!runtimeCache.contains(id)) {
                    if (runtimeCache.size() + invalidMetadata.size() >= 96) fail(tr("Слишком много вариантов JAR. Уточните версию Minecraft."));
                    const auto file = primaryFile(candidate);
                    const auto declaredSize = file["size"].toDouble();
                    constexpr qint64 JarLimit = 64 * 1024 * 1024;
                    if (declaredSize > JarLimit || jarBytes >= 256 * 1024 * 1024) fail(tr("Превышен лимит проверки JAR."));
                    progress.setLabelText(tr("Проверка правил внутри JAR: %1").arg(file["filename"].toString()));
                    const auto bytes = fetchBytes(QUrl(file["url"].toString()), qMin(JarLimit, 256 * 1024 * 1024 - jarBytes));
                    jarBytes += bytes.size();
                    if (QCryptographicHash::hash(bytes, QCryptographicHash::Sha512).toHex() != file["hashes"].toObject()["sha512"].toString().toLatin1().toLower())
                        fail(tr("SHA-512 JAR не совпадает: %1").arg(file["filename"].toString()));
                    QString error;
                    const auto metadata = readRuntimeMods(bytes, loader, error);
                    if (!error.isEmpty()) {
                        lastConflict = tr("Не удалось проверить %1: %2").arg(file["filename"].toString(), error);
                        invalidMetadata[id] = lastConflict;
                        return false;
                    }
                    runtimeCache[id] = metadata;
                }
                mods.append(runtimeCache.value(id));
            }
            const auto error = runtimeConflict(mods, {{"minecraft", game}});
            if (!error.isEmpty()) lastConflict = error;
            return error.isEmpty();
        };
        // Prefer newest Iris first, then the newest Sodium it actually supports.
        Resolver resolver(game, loader, versions, version, compatible);
        Resolver::Selection selected;
        const auto roots = coreProjects(loader);
        auto withZoom = roots;
        withZoom << "zoomify";
        QString zoomTitle = "Zoomify";
        if (!resolver.resolve(withZoom, selected)) {
            selected.clear();
            withZoom = roots;
            withZoom << "ok-zoomer";
            zoomTitle = "Ok Zoomer";
            if (!resolver.resolve(withZoom, selected)) fail(tr("Нет совместимого полного набора. %1").arg(lastConflict));
        }
        m_table->item(row, 2)->setText(zoomTitle);
        resolver.addOptional("sodium-extra", selected);
        resolver.addOptional("modmenu", selected);
        QStringList extrasSummary;
        if (addLights) {
            if (resolver.addOptional("lambdynamiclights", selected))
                extrasSummary << tr("LambDynamicLights: добавлен");
            else extrasSummary << tr("LambDynamicLights: нет совместимой версии, пропущен");
        }
        const auto uid = loader == "fabric" ? QString("net.fabricmc.fabric-loader") : QString("net.neoforged");
        auto loaders = APPLICATION->metadataIndex()->get(uid);
        progress.setLabelText(tr("Проверка версии загрузчика…"));
        if (!loaders->isLoaded()) {
            auto loadTask = loaders->getLoadTask();
            if (!loadTask) fail(tr("Не удалось начать загрузку метаданных загрузчика."));
            QEventLoop loop;
            QTimer deadline;
            deadline.setSingleShot(true);
            connect(loadTask.get(), &Task::finished, &loop, &QEventLoop::quit);
            connect(&progress, &QProgressDialog::canceled, &loop, &QEventLoop::quit);
            connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
            deadline.start(30000);
            loadTask->start();
            if (!loadTask->isFinished() && !progress.wasCanceled()) loop.exec();
            if (progress.wasCanceled() || !loadTask->isFinished()) {
                loadTask->abort();
                fail(progress.wasCanceled() ? tr("Операция отменена.") : tr("Истекло время ожидания метаданных загрузчика."));
            }
            if (!loadTask->wasSuccessful()) fail(tr("Не удалось загрузить метаданные загрузчика. Повторите попытку позже."));
        }
        if (progress.wasCanceled()) fail(tr("Операция отменена."));
        auto chosen = loaders->getLatestForParent("net.minecraft", game);
        if (!chosen && loader == "fabric") {
            for (const auto& candidate : loaders->versions())
                if (!chosen || candidate->toComparableVersion() > chosen->toComparableVersion()) chosen = candidate;
        }
        if (!chosen) fail(tr("Не найдена версия загрузчика для Minecraft %1.").arg(game));
        QList<RuntimeMod> finalMods;
        for (const auto& mod : selected) finalMods.append(runtimeCache.value(mod["id"].toString()));
        const auto loaderConflict = runtimeConflict(finalMods, {{"minecraft", game},
            {loader == "fabric" ? QString("fabricloader") : QString("neoforge"), chosen->descriptor()}});
        if (!loaderConflict.isEmpty()) fail(tr("Ограничения загрузчика: %1").arg(loaderConflict));
        QJsonArray files;
        QSet<QString> names;
        QStringList summary;
        for (const auto& mod : selected) {
            const auto file = primaryFile(mod);
            const auto filename = file["filename"].toString();
            if (file.isEmpty() || names.contains(filename.toCaseFolded())) fail(tr("Некорректные или совпадающие имена JAR-файлов."));
            names.insert(filename.toCaseFolded());
            files.append(packFile(file, "mods"));
            summary << mod["name"].toString() + " " + mod["version_number"].toString() + " [" + mod["version_type"].toString() + "]";
        }
        if (addShaders) {
            for (const auto& slug : shaderProjects()) {
                // These are shader projects, not Fabric/NeoForge mods. Filter by Iris + MC.
                const auto project = fetch("project/" + slug).object();
                if (project["project_type"].toString() != "shader" || project["slug"].toString() != slug)
                    fail(tr("Некорректный проект шейдеров: %1").arg(slug));
                const auto title = project["title"].toString(slug);
                QJsonObject chosenShader, chosenFile;
                for (const auto& value : newestFirst(versions(slug))) {
                    const auto candidate = value.toObject();
                    if (candidate["project_id"].toString() != project["id"].toString()) continue;
                    const auto file = shaderFile(candidate, game);
                    if (file.isEmpty()) continue;
                    bool compatibleDependencies = true;
                    for (const auto& dependency : candidate["dependencies"].toArray()) {
                        const auto dep = dependency.toObject();
                        const auto type = dep["dependency_type"].toString();
                        if (type != "required" && type != "incompatible") continue;
                        auto id = dep["project_id"].toString();
                        const auto pin = dep["version_id"].toString();
                        if (id.isEmpty() && !pin.isEmpty()) id = version(pin)["project_id"].toString();
                        const bool present = selected.contains(id) && (pin.isEmpty() || selected[id]["id"].toString() == pin);
                        if ((type == "required" && !present) || (type == "incompatible" && present)) compatibleDependencies = false;
                    }
                    if (!compatibleDependencies) continue;
                    chosenShader = candidate; chosenFile = file; break;
                }
                if (chosenFile.isEmpty()) {
                    extrasSummary << tr("%1: нет совместимого ZIP для Minecraft %2 / Iris, пропущен").arg(title, game);
                    continue;
                }
                const auto path = QString("shaderpacks/") + chosenFile["filename"].toString();
                if (names.contains(path.toCaseFolded())) fail(tr("Совпадающие имена shaderpacks."));
                names.insert(path.toCaseFolded());
                files.append(packFile(chosenFile, "shaderpacks"));
                extrasSummary << title + " " + chosenShader["version_number"].toString() + tr(" → shaderpacks (выбрать в Iris)");
            }
        }
        summary.append(extrasSummary);
        m_name = QString("Sodium %1 · %2").arg(game, loader == "fabric" ? "Fabric" : "NeoForge");
        m_manifest = QJsonObject{{"formatVersion", 1}, {"game", "minecraft"}, {"name", m_name},
            {"versionId", QString::number(QDateTime::currentSecsSinceEpoch())}, {"files", files},
            {"dependencies", QJsonObject{{"minecraft", game}, {loader == "fabric" ? "fabric-loader" : "neoforge", chosen->descriptor()}}}};
        progress.hide();
        if (QMessageBox::question(this, tr("Создать отдельную сборку?"),
            tr("%1\nЗагрузчик: %2\n\n%3\n\nМоды и выбранные шейдеры загружаются с Modrinth с проверкой SHA-512. Шейдеры не включаются автоматически. Уже установленные сборки не изменяются. Совместимость по метаданным не гарантирует отсутствие ошибок в игре.").arg(m_name, chosen->descriptor(), summary.join('\n'))) == QMessageBox::Yes)
            accept();
        else m_manifest = {};
    } catch (const std::exception& e) {
        m_manifest = {};
        m_status->setText(QString::fromUtf8(e.what()));
    }
    m_progress = nullptr;
    m_table->setEnabled(true);
    m_reload->setEnabled(true);
    m_install->setEnabled(canPrepare());
}
