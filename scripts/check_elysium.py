#!/usr/bin/env python3
"""Dependency-free packaging smoke checks, not a substitute for a Qt build."""
from pathlib import Path
import json
import re
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
INFO = ROOT / "program_info"
APP_ID = "io.github.artemii14.ElysiumLauncher"


class ElysiumPackagingTests(unittest.TestCase):
    def test_branding_inputs_exist(self):
        for name in (
            f"{APP_ID}.svg", f"{APP_ID}_256.png", f"{APP_ID}.desktop.in",
            f"{APP_ID}.metainfo.xml.in", f"{APP_ID}.mime.xml",
            "elysiumlauncher.ico", "elysiumlauncher.icns", "elysiumlauncher.qrc.in",
            "elysiumlauncher.rc.in", "elysiumlauncher.manifest.in", "elysiumlauncher.6.scd.in",
        ):
            with self.subTest(name=name):
                self.assertTrue((INFO / name).is_file(), name)
                self.assertGreater((INFO / name).stat().st_size, 0)

    def test_xml_and_resource_references(self):
        for name in (f"{APP_ID}.svg", f"{APP_ID}.metainfo.xml.in", f"{APP_ID}.mime.xml",
                     "elysiumlauncher.manifest.in", "elysiumlauncher.qrc.in"):
            with self.subTest(name=name):
                ET.fromstring((INFO / name).read_text(encoding="utf-8"))
        resource = ET.fromstring((INFO / "elysiumlauncher.qrc.in").read_text(encoding="utf-8"))
        for file in resource.iter("file"):
            self.assertTrue((INFO / file.text.replace("@Launcher_AppID@", APP_ID)).exists())
        main = (ROOT / "launcher/main.cpp").read_text(encoding="utf-8")
        self.assertIn("Q_INIT_RESOURCE(elysiumlauncher);", main)
        self.assertNotIn("Q_INIT_RESOURCE(elyprismlauncher);", main)

    def test_apple_catalog_assets(self):
        catalog = INFO / "ElysiumLauncher.icon"
        data = json.loads((catalog / "icon.json").read_text(encoding="utf-8"))
        for group in data["groups"]:
            for layer in group["layers"]:
                asset = catalog / "Assets" / layer["image-name"]
                self.assertTrue(asset.is_file())
                ET.fromstring(asset.read_text(encoding="utf-8"))

    def test_default_identity_and_updater(self):
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertRegex(cmake, r'set\(Launcher_APP_BINARY_NAME "elysiumlauncher"')
        self.assertRegex(cmake, r'set\(Launcher_UPDATER_GITHUB_REPO ""')
        info = (INFO / "CMakeLists.txt").read_text(encoding="utf-8")
        for value in ("ElysiumLauncher", APP_ID, "ELYSIUMLAUNCHER"):
            self.assertIn(value, info)
        application = (ROOT / "launcher/Application.cpp").read_text(encoding="utf-8")
        self.assertIn('registerSetting("ApplicationTheme", QStringLiteral("elysium"))', application)
        manager = (ROOT / "launcher/ui/themes/ThemeManager.cpp").read_text(encoding="utf-8")
        self.assertIn("addTheme(std::make_unique<ElysiumTheme>())", manager)

    def test_main_layout_is_vertical(self):
        ui = ET.parse(ROOT / "launcher/ui/MainWindow.ui")
        layout = ui.find('.//layout[@name="horizontalLayout"]')
        self.assertEqual(layout.attrib["class"], "QVBoxLayout")

    def test_dual_mode_sources_are_wired(self):
        cmake = (ROOT / "launcher/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn("ui/MainWindowModes.cpp", cmake)
        self.assertIn("minecraft/OptiFineProfile.h", cmake)
        header = (ROOT / "launcher/ui/MainWindow.h").read_text(encoding="utf-8")
        implementation = (ROOT / "launcher/ui/MainWindowModes.cpp").read_text(encoding="utf-8")
        main = (ROOT / "launcher/ui/MainWindow.cpp").read_text(encoding="utf-8")
        for method in ("setupLaunchModes", "applyLaunchMode", "updateCompactControls",
                       "createQuickVanilla", "importOptiFineProfile", "interfaceWindowState"):
            self.assertIn(method, header)
            self.assertIn("MainWindow::" + method, implementation)
        self.assertIn("setupLaunchModes();", main)
        self.assertIn('registerSetting("ElysiumInterfaceMode"', (ROOT / "launcher/Application.cpp").read_text(encoding="utf-8"))
        self.assertNotIn("saveState().toBase64()", main)

    def test_navigation_replaces_top_toolbar(self):
        source = (ROOT / "launcher/ui/MainWindowModes.cpp").read_text(encoding="utf-8")
        self.assertIn('toolbar->toggleViewAction()->setVisible(false)', source)
        self.assertIn('removeToolBar(toolbar)', source)
        self.assertLess(source.index('workspace->addWidget(collection, 1)'), source.index('workspace->addWidget(railScroll)'))
        main = (ROOT / 'launcher/ui/MainWindow.cpp').read_text(encoding='utf-8')
        self.assertNotIn('Your next adventure starts here.', main)
        app = (ROOT / 'launcher/Application.cpp').read_text(encoding='utf-8')
        self.assertNotIn('m_mainWindow->restoreState(', app)
        self.assertNotIn('ui->mainToolBar->setVisible(m_advancedMainToolbarVisible)', source)
        self.assertNotIn('Компоненты / Java', source)
        self.assertNotIn('railPage(', source)
        for action in ('Accounts', 'Folders', 'Help'):
            self.assertIn('railMenu(ui->action' + action + 'Button', source)
        self.assertIn('railAction(ui->actionEditInstance', source)
        self.assertIn('navigationIcon', source)
        self.assertIn('new QScrollArea', source)

    def test_sidebar_selected_pack_identity(self):
        source = (ROOT / "launcher/ui/MainWindowModes.cpp").read_text(encoding="utf-8")
        self.assertIn('navigation->addWidget(m_sidebarInstanceIcon)', source)
        self.assertIn('navigation->addWidget(m_sidebarInstanceName)', source)
        self.assertIn('getIcon(instance->iconKey())', source)
        self.assertIn('m_sidebarInstanceName->setText(instance->name())', source)
        self.assertIn('m_sidebarInstanceName->setTextFormat(Qt::PlainText)', source)
        self.assertIn('m_sidebarInstanceIcon->setIcon(icon)', source)
        self.assertIn('&MainWindow::on_actionChangeInstIcon_triggered', source)

    def test_interface_mode_in_settings(self):
        source = (ROOT / "launcher/ui/MainWindowModes.cpp").read_text(encoding="utf-8")
        self.assertNotIn("m_modernModeButton", source)
        self.assertNotIn("elysiumModeBar", source)
        self.assertNotIn("Готово к запуску через обычную проверку", source)
        ui = (ROOT / "launcher/ui/pages/global/LauncherPage.ui").read_text(encoding="utf-8")
        self.assertIn('name="interfaceModeCombo"', ui)
        self.assertIn('>Modern</string>', ui)
        self.assertIn('>Legacy</string>', ui)
        settings = (ROOT / "launcher/ui/pages/global/LauncherPage.cpp").read_text(encoding="utf-8")
        self.assertIn('ui->interfaceModeCombo->currentIndex() == 1 ? "classic" : "modern"', settings)

    def test_midnight_icon_pack(self):
        import re
        pack = ROOT / "launcher/resources/midnight"
        assets = {p.stem for p in (pack / "scalable").glob("*.svg")}
        self.assertGreaterEqual(len(assets), 70)
        self.assertIn("Q_INIT_RESOURCE(midnight);", (ROOT / "launcher/main.cpp").read_text(encoding="utf-8"))
        appearance = (ROOT / "launcher/ui/widgets/AppearanceWidget.cpp").read_text(encoding="utf-8")
        self.assertIn('theme->path() + "/scalable/settings.svg"', appearance)
        self.assertIn('theme->id() == "midnight"', appearance)
        self.assertIn('settings->set("IconTheme", newIconTheme)', appearance)
        self.assertIn("resources/midnight/midnight.qrc", (ROOT / "launcher/CMakeLists.txt").read_text(encoding="utf-8"))
        self.assertIn('"midnight"', (ROOT / "launcher/ui/themes/ThemeManager.h").read_text(encoding="utf-8"))
        self.assertIn('registerSetting("IconTheme", QStringLiteral("midnight"))', (ROOT / "launcher/Application.cpp").read_text(encoding="utf-8"))
        brands = {"atlauncher", "atlauncher-placeholder", "discord", "flame", "ftb_logo", "matrix", "modrinth", "reddit-alien", "technic"}
        referenced = set()
        for path in (ROOT / "launcher").rglob("*"):
            if path.suffix in (".cpp", ".h", ".ui"):
                text = path.read_text(encoding="utf-8")
                referenced.update(re.findall(r'fromTheme\("([^" :]+)"', text))
                referenced.update(re.findall(r'<iconset theme="([^"]+)"', text))
        self.assertFalse(referenced - assets - brands, referenced - assets - brands)
        for svg in (pack / "scalable").glob("*.svg"):
            root = ET.parse(svg).getroot()
            self.assertEqual(root.attrib["viewBox"], "0 0 24 24")
            self.assertNotIn("<image", svg.read_text(encoding="utf-8"))

    def test_zerotier_local_control_boundaries(self):
        source = (ROOT / "launcher/ui/dialogs/ZeroTierDialog.cpp").read_text(encoding="utf-8")
        self.assertIn('http://127.0.0.1:9993', source)
        self.assertIn('QNetworkProxy::NoProxy', source)
        self.assertIn('QNetworkRequest::ManualRedirectPolicy', source)
        self.assertIn('req.setTransferTimeout(5000)', source)
        self.assertIn('{"allowDefault", false}', source)
        self.assertIn('{"allowGlobal", false}', source)
        self.assertIn('{"allowDNS", false}', source)
        self.assertNotIn('QProcess', source)
        self.assertNotIn('qDebug', source)
        self.assertNotIn('qInfo', source)
        self.assertIn('ui/dialogs/ZeroTierDialog.cpp', (ROOT / "launcher/CMakeLists.txt").read_text(encoding="utf-8"))

    def test_zerotier_host_feature_removed(self):
        self.assertFalse((ROOT / "launcher/net/ZeroTierDiscovery.cpp").exists())
        source = (ROOT / "launcher/ui/dialogs/ZeroTierDialog.cpp").read_text(encoding="utf-8")
        for retired in ('ZeroTierDiscovery', 'Стать хостом', '46931', 'askName', 'updateHosts'):
            self.assertNotIn(retired, source)
        self.assertIn('showError(', source)
        self.assertIn('QInputDialog::getItem', source)
        self.assertIn('reply->setReadBufferSize', source)
        cmake = (ROOT / "launcher/CMakeLists.txt").read_text(encoding="utf-8")
        self.assertNotIn('ZeroTierDiscovery', cmake)

    def test_sodium_presets_are_create_only(self):
        source = (ROOT / "launcher/ui/dialogs/SodiumPresetDialog.cpp").read_text(encoding="utf-8")
        self.assertIn('loader == "fabric" && !exists("fabric-api")', source)
        self.assertIn('resolver.addOptional("sodium-extra", selected)', source)
        self.assertIn('resolver.addOptional("modmenu", selected)', source)
        self.assertIn('withZoom << "ok-zoomer"', source)
        importer = (ROOT / "launcher/modplatform/modrinth/ModrinthInstanceCreationTask.cpp").read_text(encoding="utf-8")
        self.assertIn('if (!m_generatedManifest.isEmpty()) return false;', importer)
        self.assertIn('QSaveFile manifest', importer)
        self.assertIn('TEST_NAME SodiumPreset', (ROOT / "tests/CMakeLists.txt").read_text(encoding="utf-8"))

    def test_optifine_test_is_registered(self):
        self.assertTrue((ROOT / "tests/OptiFineProfile_test.cpp").is_file())
        self.assertIn("TEST_NAME OptiFineProfile", (ROOT / "tests/CMakeLists.txt").read_text(encoding="utf-8"))
        implementation = (ROOT / "launcher/ui/MainWindowModes.cpp").read_text(encoding="utf-8")
        self.assertIn("Elysium::planOptiFineImport", implementation)
        self.assertIn("VanillaCreationTask", implementation)
        self.assertNotIn("QProcess::startDetached", implementation)

    def test_theme_is_in_cmake_and_qss_balances(self):
        self.assertIn("ui/themes/ElysiumTheme.h", (ROOT / "launcher/CMakeLists.txt").read_text(encoding="utf-8"))
        theme = (ROOT / "launcher/ui/themes/ElysiumTheme.h").read_text(encoding="utf-8")
        qss = re.search(r'R"QSS\((.*?)\)QSS"', theme, re.S).group(1)
        self.assertEqual(qss.count("{"), qss.count("}"))
        self.assertGreater(qss.count("{"), 10)


if __name__ == "__main__":
    unittest.main(verbosity=2)
