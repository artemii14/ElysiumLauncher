#include "ui/widgets/HomeCustomization.h"
// SPDX-License-Identifier: GPL-3.0-only
// Independent native UI, inspired by the workflows of XMCL and Legacy Launcher.
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Application.h"
#include "Exception.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/Component.h"
#include "minecraft/OptiFineProfile.h"
#include "minecraft/PackProfile.h"
#include "minecraft/VanillaInstanceCreationTask.h"
#include "ui/dialogs/VersionSelectDialog.h"
#include "ui/dialogs/ZeroTierDialog.h"
#include "ui/dialogs/SodiumPresetDialog.h"
#include "modplatform/modrinth/ModrinthInstanceCreationTask.h"
#include "ui/instanceview/InstanceProxyModel.h"
#include "ui/instanceview/InstanceView.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QCryptographicHash>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMenu>
#include <QPushButton>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QToolButton>
#include <QToolBar>
#include <QVBoxLayout>

namespace {
// Share the same vector artwork with menus, settings and instance dialogs.
QIcon navigationIcon(const QString& name)
{
    QString glyph = name;
    if (name == "add") glyph = "new";
    else if (name == "play") glyph = "launch";
    else if (name == "account") glyph = "accounts";
    else if (name == "folder") glyph = "viewfolder";
    return QIcon::fromTheme(glyph, QIcon(QStringLiteral(":/icons/midnight/scalable/%1.svg").arg(glyph)));
}
QByteArray fileHash(const QString& path)
{
    QFile file(path);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!file.open(QIODevice::ReadOnly) || !hash.addData(&file))
        return {};
    return hash.result();
}
}

QByteArray MainWindow::interfaceWindowState()
{
    // Legacy toolbars are retired; never persist their restored visibility.
    ui->mainToolBar->hide();
    ui->newsToolBar->hide();
    ui->instanceToolBar->hide();
    return saveState();
}

void MainWindow::setupLaunchModes()
{
    // XMCL-inspired workspace rail; every destination uses a Prism/Elysium action.
    m_modernHome = new QWidget(this);
    auto workspace = new QHBoxLayout(m_modernHome);
    workspace->setContentsMargins(0, 0, 0, 0);
    workspace->setSpacing(0);
    auto rail = new MidnightSurface(m_modernHome);
    rail->setObjectName(QStringLiteral("elysiumRail"));
    rail->setAttribute(Qt::WA_StyledBackground, true);
    rail->setMinimumWidth(200);
    auto navigation = new QVBoxLayout(rail);
    navigation->setContentsMargins(14, 20, 14, 16);
    navigation->setSpacing(6);
    auto section = [rail, navigation](const QString& title) {
        auto label = new QLabel(title, rail);
        label->setObjectName(QStringLiteral("elysiumEyebrow"));
        label->setContentsMargins(8, 12, 0, 6);
        navigation->addWidget(label);
    };
    auto decorate = [](QPushButton* button, const QString& glyph) {
        button->setIcon(navigationIcon(glyph));
        button->setIconSize(QSize(20, 20));
        button->setMinimumHeight(42);
        button->setCursor(Qt::PointingHandCursor);
    };
    auto railAction = [rail, navigation, decorate](QAction* action, const QString& title, const QString& glyph) {
        auto button = new QPushButton(title, rail);
        decorate(button, glyph);
        connect(button, &QPushButton::clicked, action, &QAction::trigger);
        auto sync = [action, button] { button->setEnabled(action->isEnabled()); };
        connect(action, &QAction::changed, button, sync);
        sync();
        navigation->addWidget(button);
    };
    auto railMenu = [rail, navigation](QAction* action, const QString& title, const QString& glyph) {
        auto button = new QToolButton(rail);
        button->setDefaultAction(action);
        button->setText(title);
        button->setIcon(navigationIcon(glyph));
        button->setIconSize(QSize(20, 20));
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setPopupMode(QToolButton::InstantPopup);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setMinimumHeight(42);
        navigation->addWidget(button);
    };
    // Selected pack identity: actual instance artwork, not a launcher glyph.
    m_sidebarInstanceIcon = new QToolButton(rail);
    m_sidebarInstanceIcon->setObjectName(QStringLiteral("elysiumSelectedIcon"));
    m_sidebarInstanceIcon->setIconSize(QSize(80, 80));
    m_sidebarInstanceIcon->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_sidebarInstanceIcon->setCursor(Qt::PointingHandCursor);
    m_sidebarInstanceIcon->setToolTip(tr("Изменить иконку сборки"));
    connect(m_sidebarInstanceIcon, &QToolButton::clicked, this, &MainWindow::on_actionChangeInstIcon_triggered);
    m_sidebarInstanceIcon->setFixedHeight(88);
    m_sidebarInstanceName = new QLabel(rail);
    m_sidebarInstanceName->setObjectName(QStringLiteral("elysiumSelectedName"));
    m_sidebarInstanceName->setTextFormat(Qt::PlainText);
    m_sidebarInstanceName->setAlignment(Qt::AlignCenter);
    m_sidebarInstanceName->setWordWrap(true);
    m_sidebarInstanceName->setContentsMargins(6, 0, 6, 12);
    navigation->addWidget(m_sidebarInstanceIcon);
    navigation->addWidget(m_sidebarInstanceName);
    section(tr("БИБЛИОТЕКА"));
    railAction(ui->actionAddInstance, tr("Создать сборку…"), "add");
    auto quickVanilla = new QPushButton(tr("Установить vanilla…"), rail);
    decorate(quickVanilla, "import");
    connect(quickVanilla, &QPushButton::clicked, this, &MainWindow::createQuickVanilla);
    navigation->addWidget(quickVanilla);
    auto sodium = new QPushButton(tr("Установить Sodium…"), rail);
    decorate(sodium, "quickmods");
    connect(sodium, &QPushButton::clicked, this, &MainWindow::createSodiumPreset);
    navigation->addWidget(sodium);
    section(tr("ВЫБРАННАЯ СБОРКА"));
    railAction(ui->actionLaunchInstance, tr("Играть"), "play");
    railAction(ui->actionEditInstance, tr("Настроить сборку…"), "settings");
    auto importOptiFine = new QPushButton(tr("Импорт OptiFine…"), rail);
    decorate(importOptiFine, "import");
    importOptiFine->setToolTip(tr("Импорт локального профиля OptiFine в выбранную vanilla-сборку"));
    auto syncImport = [this, importOptiFine] {
        importOptiFine->setEnabled(m_selectedInstance && m_selectedInstance->canEdit() && !m_selectedInstance->isRunning());
    };
    connect(ui->actionEditInstance, &QAction::changed, importOptiFine, syncImport);
    syncImport();
    connect(importOptiFine, &QPushButton::clicked, this, &MainWindow::importOptiFineProfile);
    navigation->addWidget(importOptiFine);
    navigation->addStretch();
    section(tr("ЛАУНЧЕР"));
    railMenu(ui->actionAccountsButton, tr("Учётные записи"), "account");
    railMenu(ui->actionFoldersButton, tr("Папки"), "folder");
    railAction(ui->actionSettings, tr("Настройки"), "settings");
    auto lan = new QPushButton(tr("Эмулятор локальной сети"), rail);
    decorate(lan, "proxy");
    lan->setToolTip(tr("Подключение к виртуальной сети через установленный ZeroTier One"));
    connect(lan, &QPushButton::clicked, this, [this] {
        ZeroTierDialog dialog(this);
        dialog.exec();
    });
    navigation->addWidget(lan);
    railMenu(ui->actionHelpButton, tr("Справка"), "help");
    auto railScroll = new QScrollArea(m_modernHome);
    railScroll->setFrameShape(QFrame::NoFrame);
    railScroll->setWidgetResizable(true);
    railScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    railScroll->setFixedWidth(236);
    railScroll->setWidget(rail);
    // Navigation follows the library, on the right (see workspace order below).
    auto collection = new QWidget(m_modernHome);
    auto collectionLayout = new QVBoxLayout(collection);
    collectionLayout->setContentsMargins(0, 0, 0, 0);
    collectionLayout->setSpacing(0);
    for (QWidget* widget : {m_libraryHeader, static_cast<QWidget*>(view), static_cast<QWidget*>(m_libraryEmpty)}) {
        ui->horizontalLayout->removeWidget(widget);
        collectionLayout->addWidget(widget, widget == m_libraryHeader ? 0 : 1);
    }
    workspace->addWidget(collection, 1);
    workspace->addWidget(railScroll);
    ui->horizontalLayout->addWidget(m_modernHome, 1);

    m_compactHome = new MidnightSurface(this);
    m_compactHome->setObjectName(QStringLiteral("elysiumCompactHome"));
    auto layout = new QVBoxLayout(m_compactHome);
    layout->setContentsMargins(40, 28, 40, 32);
    layout->setSpacing(16);
    layout->addStretch();
    auto title = new QLabel(tr("Запуск Minecraft"), m_compactHome);
    title->setObjectName(QStringLiteral("elysiumTitle"));
    title->setWordWrap(true);
    layout->addWidget(title);
    auto description = new QLabel(tr("Выберите сборку для запуска или установите новую версию Minecraft."), m_compactHome);
    description->setWordWrap(true);
    layout->addWidget(description);
    m_compactInstances = new QComboBox(m_compactHome);
    m_compactInstances->setAccessibleName(tr("Готовая сборка Minecraft"));
    m_compactInstances->setPlaceholderText(tr("Выбери сборку…"));
    m_compactInstances->setMinimumHeight(44);
    m_compactInstances->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_compactInstances->setMinimumContentsLength(24);
    m_compactModel = new QSortFilterProxyModel(this);
    m_compactModel->setSourceModel(APPLICATION->instances());
    m_compactModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_compactModel->sort(0);
    m_compactInstances->setModel(m_compactModel);
    layout->addWidget(m_compactInstances);
    m_compactDetails = new QLabel(m_compactHome);
    m_compactDetails->setWordWrap(true);
    m_compactDetails->setObjectName(QStringLiteral("elysiumSubtitle"));
    layout->addWidget(m_compactDetails);

    auto actions = new QHBoxLayout();
    auto addActionButton = [this, actions](QAction* action, const QString& label, const QString& name = {}) {
        auto button = new QPushButton(label, m_compactHome);
        button->setObjectName(name);
        button->setMinimumHeight(44);
        connect(button, &QPushButton::clicked, action, &QAction::trigger);
        auto sync = [button, action] { button->setEnabled(action->isEnabled()); };
        connect(action, &QAction::changed, button, sync);
        sync();
        actions->addWidget(button);
    };
    addActionButton(ui->actionLaunchInstance, tr("Играть"), QStringLiteral("elysiumCreate"));
    addActionButton(ui->actionEditInstance, tr("Настроить сборку"));
    addActionButton(ui->actionKillInstance, tr("Остановить"));
    layout->addLayout(actions);

    auto install = new QHBoxLayout();
    auto vanilla = new QPushButton(tr("+ Vanilla автоматически"), m_compactHome);
    auto optiFine = new QPushButton(tr("Импорт OptiFine…"), m_compactHome);
    connect(vanilla, &QPushButton::clicked, this, &MainWindow::createQuickVanilla);
    connect(optiFine, &QPushButton::clicked, this, &MainWindow::importOptiFineProfile);
    install->addWidget(vanilla);
    install->addWidget(optiFine);
    auto sodiumCompact = new QPushButton(tr("Sodium-сборка…"), m_compactHome);
    connect(sodiumCompact, &QPushButton::clicked, this, &MainWindow::createSodiumPreset);
    install->addWidget(sodiumCompact);
    layout->addLayout(install);
    auto extras = new QHBoxLayout();
    auto create = new QPushButton(tr("Модпаки / Forge / Fabric…"), m_compactHome);
    connect(create, &QPushButton::clicked, ui->actionAddInstance, &QAction::trigger);
    extras->addWidget(create);
    auto accounts = new QToolButton(m_compactHome);
    accounts->setDefaultAction(ui->actionAccountsButton);
    accounts->setPopupMode(QToolButton::InstantPopup);
    accounts->setText(tr("Учётные записи"));
    accounts->setIcon(navigationIcon("account"));
    accounts->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    extras->addWidget(accounts);
    auto launcherMenu = new QToolButton(m_compactHome);
    launcherMenu->setText(tr("Лаунчер"));
    launcherMenu->setIcon(navigationIcon("settings"));
    launcherMenu->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    launcherMenu->setPopupMode(QToolButton::InstantPopup);
    auto menu = new QMenu(launcherMenu);
    menu->addAction(ui->actionSettings);
    auto lanAction = menu->addAction(QIcon::fromTheme("proxy"), tr("Эмулятор локальной сети…"));
    connect(lanAction, &QAction::triggered, this, [this] { ZeroTierDialog dialog(this); dialog.exec(); });
    menu->addMenu(ui->foldersMenu);
    menu->addMenu(ui->helpMenu);
    launcherMenu->setMenu(menu);
    extras->addWidget(launcherMenu);
    layout->addLayout(extras);
    layout->addStretch();
    ui->horizontalLayout->addWidget(m_compactHome, 1);

    connect(m_compactInstances, &QComboBox::activated, this, [this](int row) {
        if (row < 0)
            return;
        // The compact picker must not inherit a hidden library search filter.
        if (auto search = findChild<QLineEdit*>(QStringLiteral("elysiumSearch")))
            search->clear();
        setSelectedInstanceById(m_compactModel->index(row, 0).data(InstanceList::InstanceIDRole).toString());
        updateCompactControls();
    });
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this] { updateCompactControls(); });
    connect(ui->actionLaunchInstance, &QAction::changed, this, &MainWindow::updateCompactControls);
    connect(m_compactModel, &QAbstractItemModel::modelReset, this, &MainWindow::updateCompactControls);
    connect(m_compactModel, &QAbstractItemModel::rowsInserted, this, &MainWindow::updateCompactControls);
    connect(m_compactModel, &QAbstractItemModel::rowsRemoved, this, &MainWindow::updateCompactControls);
    connect(m_compactModel, &QAbstractItemModel::dataChanged, this, &MainWindow::updateCompactControls);
    connect(APPLICATION->icons(), &IconList::iconUpdated, this, [this] { updateCompactControls(); });
    new HomeButtonOrder(rail, QStringLiteral("modern-v1"));
    new HomeButtonOrder(m_compactHome, QStringLiteral("legacy-v1"));
    applyLaunchMode(APPLICATION->settings()->get("ElysiumInterfaceMode").toString() == "classic");
    updateCompactControls();
}

void MainWindow::applyLaunchMode(bool compact)
{
    if (!m_compactHome)
        return;
    if (compact) {
        if (auto search = findChild<QLineEdit*>(QStringLiteral("elysiumSearch")))
            search->clear();
    }
    // Detach retired toolbars as well as hiding them. Old saved Qt layouts must
    // not bring back the top bar, news strip or duplicate instance actions.
    for (QToolBar* toolbar : {static_cast<QToolBar*>(ui->mainToolBar),
                              static_cast<QToolBar*>(ui->newsToolBar),
                              static_cast<QToolBar*>(ui->instanceToolBar)}) {
        removeToolBar(toolbar);
        toolbar->hide();
        toolbar->toggleViewAction()->setVisible(false);
    }
    m_compactMode = compact;
    m_compactHome->setVisible(compact);
    m_modernHome->setVisible(!compact);
    m_libraryHeader->setVisible(!compact);
    view->setVisible(!compact && proxymodel->rowCount() > 0);
    m_libraryEmpty->setVisible(!compact && proxymodel->rowCount() == 0);
    APPLICATION->settings()->set("ElysiumInterfaceMode", compact ? "classic" : "modern");
}

void MainWindow::updateCompactControls()
{
    if (!m_compactInstances)
        return;
    QSignalBlocker blocker(m_compactInstances);
    // Resolve by ID afresh: source resets must never dereference an old instance pointer.
    const auto id = APPLICATION->settings()->get("SelectedInstance").toString();
    const int row = m_compactInstances->findData(id, InstanceList::InstanceIDRole);
    m_compactInstances->setCurrentIndex(row);
    auto instance = APPLICATION->instances()->getInstanceById(id);
    if (m_sidebarInstanceIcon && m_sidebarInstanceName) {
        m_sidebarInstanceIcon->setVisible(instance != nullptr);
        m_sidebarInstanceName->setVisible(instance != nullptr);
        if (instance) {
            const auto icon = APPLICATION->icons()->getIcon(instance->iconKey());
            m_sidebarInstanceIcon->setIcon(icon);
            m_sidebarInstanceIcon->setEnabled(ui->actionChangeInstIcon->isEnabled());
            m_sidebarInstanceIcon->setAccessibleName(tr("Иконка сборки %1").arg(instance->name()));
            m_sidebarInstanceName->setText(instance->name());
            m_sidebarInstanceName->setToolTip(instance->name());
        } else {
            m_sidebarInstanceIcon->setIcon(QIcon());
            m_sidebarInstanceName->clear();
        }
    }
    if (!instance) {
        m_compactDetails->setText(tr("Нет выбранной сборки. Установите Minecraft или добавьте сборку."));
        return;
    }
    auto minecraft = dynamic_cast<MinecraftInstance*>(instance);
    const auto version = minecraft ? minecraft->getPackProfile()->getComponentVersion("net.minecraft") : QString();
    m_compactDetails->setText(tr("Minecraft %1 · %2").arg(version.isEmpty() ? tr("версия не указана") : version,
        instance->isRunning() ? tr("Игра запущена") : tr("Готово к запуску")));
}

void MainWindow::createQuickVanilla()
{
    auto versions = APPLICATION->metadataIndex()->get("net.minecraft");
    VersionSelectDialog dialog(versions.get(), tr("Выбери Minecraft — сборка будет создана автоматически"), this);
    if (dialog.exec() != QDialog::Accepted || !dialog.selectedVersion())
        return;
    const auto version = dialog.selectedVersion();
    const auto name = QStringLiteral("Vanilla %1").arg(version->descriptor());
    // Reuse only an untouched managed-looking vanilla, never a modded instance.
    for (int i = 0; i < APPLICATION->instances()->rowCount(); ++i) {
        auto instance = dynamic_cast<MinecraftInstance*>(APPLICATION->instances()->at(i));
        bool vanillaOnly = instance != nullptr;
        if (instance) {
            auto pack = instance->getPackProfile();
            for (int component = 0; component < pack->rowCount(); ++component) {
                const auto uid = pack->getComponent(static_cast<size_t>(component))->getID();
                if (uid != "net.minecraft" && uid != "org.lwjgl" && uid != "org.lwjgl3")
                    vanillaOnly = false;
            }
        }
        if (vanillaOnly && instance->name() == name &&
            APPLICATION->instances()->getInstanceGroup(instance->id()) == "Elysium Vanilla" &&
            instance->getPackProfile()->getComponentVersion("net.minecraft") == version->descriptor() &&
            !instance->getPackProfile()->getModLoaders().has_value() &&
            !instance->getPackProfile()->getComponent("optifine.OptiFine")) {
            setSelectedInstanceById(instance->id());
            updateCompactControls();
            return;
        }
    }
    auto task = new VanillaCreationTask(version);
    task->setName(name);
    task->setGroup(QStringLiteral("Elysium Vanilla"));
    task->setIcon(QStringLiteral("grass"));
    instanceFromInstanceTask(task);
    updateCompactControls();
}

void MainWindow::importOptiFineProfile()
{
    auto instance = dynamic_cast<MinecraftInstance*>(m_selectedInstance);
    if (!instance || !instance->canEdit() || instance->isRunning()) {
        QMessageBox::information(this, tr("OptiFine"), tr("Выбери остановленную сборку Minecraft."));
        return;
    }
    auto profile = instance->getPackProfile();
    if (profile->getModLoaders().has_value() || profile->getComponent("optifine.OptiFine")) {
        QMessageBox::information(this, tr("OptiFine"), tr("Для этого импорта нужна vanilla без других загрузчиков и без OptiFine.\nДля Forge используй совместимый OptiFine через раздел модов."));
        return;
    }
    if (QMessageBox::question(this, tr("Локальный профиль OptiFine"),
        tr("Сначала установи OptiFine официальным установщиком в отдельную .minecraft.\n"
           "Далее выбери созданный versions/<OptiFine>/<OptiFine>.json и каталог libraries этой .minecraft.\n"
           "Импорт не запускает установщик и ничего не скачивает. Импортированные JAR будут исполняться при запуске игры.\n"
           "Используй только файлы из доверенного источника. Продолжить?")) != QMessageBox::Yes)
        return;
    const auto path = QFileDialog::getOpenFileName(this, tr("Профиль OptiFine из официального установщика"), {}, tr("JSON (*.json)"));
    if (path.isEmpty())
        return;
    QFile input(path);
    QJsonParseError parseError;
    if (input.size() > 1024 * 1024 || !input.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("OptiFine"), tr("Не удалось прочитать профиль (максимум 1 МБ)."));
        return;
    }
    auto document = QJsonDocument::fromJson(input.readAll(), &parseError);
    auto plan = Elysium::planOptiFineImport(document.object(), profile->getComponentVersion("net.minecraft"));
    if (parseError.error != QJsonParseError::NoError || !plan.valid()) {
        QMessageBox::warning(this, tr("OptiFine"), parseError.error != QJsonParseError::NoError ? parseError.errorString() : plan.error);
        return;
    }
    const auto source = QFileDialog::getExistingDirectory(this, tr("Каталог libraries исходной .minecraft"));
    if (source.isEmpty())
        return;
    const auto sourceRoot = QFileInfo(source).canonicalFilePath();
    if (sourceRoot.isEmpty()) {
        QMessageBox::warning(this, tr("OptiFine"), tr("Исходный каталог больше не существует."));
        return;
    }
    QDir destination(instance->getLocalLibraryPath());
    struct Copy { QString source; QString destination; };
    QList<Copy> copies;
    for (const auto& library : plan.libraries) {
        const auto from = QDir(sourceRoot).filePath(library.relativePath);
        const auto canonical = QFileInfo(from).canonicalFilePath();
        const auto relative = QDir(sourceRoot).relativeFilePath(canonical);
        if (canonical.isEmpty() || QDir::isAbsolutePath(relative) || relative == ".." || relative.startsWith("../") ||
            !QFileInfo(canonical).isFile()) {
            QMessageBox::warning(this, tr("OptiFine"), tr("JAR отсутствует или находится вне выбранного каталога:\n%1").arg(library.relativePath));
            return;
        }
        const auto to = destination.filePath(library.fileName);
        if (QFileInfo::exists(to)) {
            const auto hash = fileHash(canonical);
            if (hash.isEmpty() || hash != fileHash(to)) {
                QMessageBox::warning(this, tr("OptiFine"), tr("Конфликт библиотеки. Существующие файлы не перезаписаны:\n%1").arg(to));
                return;
            }
        } else {
            copies.append({canonical, to});
        }
    }
    QTemporaryDir staging;
    if (!staging.isValid() || !destination.mkpath(".")) {
        QMessageBox::warning(this, tr("OptiFine"), tr("Не удалось подготовить каталог импорта."));
        return;
    }
    const auto component = staging.filePath("optifine.OptiFine.json");
    QFile output(component);
    const auto json = QJsonDocument(plan.component).toJson();
    if (!output.open(QIODevice::WriteOnly) || output.write(json) != json.size()) {
        QMessageBox::warning(this, tr("OptiFine"), tr("Не удалось записать компонент."));
        return;
    }
    output.close();
    QStringList created;
    bool success = true;
    for (const auto& copy : copies) {
        if (!QFile::copy(copy.source, copy.destination)) {
            success = false;
            break;
        }
        created.append(copy.destination);
    }
    if (success) {
        try {
            success = profile->installComponents({component});
        } catch (const Exception&) {
            success = false;
        }
    }
    if (!success) {
        for (const auto& file : created)
            QFile::remove(file);
        QMessageBox::warning(this, tr("OptiFine"), tr("Импорт не завершён. Новые JAR удалены; существующие файлы сохранены."));
        return;
    }
    profile->saveNow();
    updateCompactControls();
    QMessageBox::information(this, tr("OptiFine"), tr("Профиль импортирован. Совместимость конкретной версии проверь запуском игры."));
}

void MainWindow::createSodiumPreset()
{
    SodiumPresetDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted || dialog.manifest().isEmpty()) return;
    auto task = new ModrinthCreationTask({}, APPLICATION->settings(), this, {});
    task->setGeneratedManifest(dialog.manifest());
    task->setName(dialog.instanceName());
    task->setGroup(QStringLiteral("Elysium Sodium"));
    task->setIcon(QStringLiteral("grass"));
    instanceFromInstanceTask(task);
}
