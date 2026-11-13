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
                ET.fromstring((INFO / name).read_text())
        resource = ET.fromstring((INFO / "elysiumlauncher.qrc.in").read_text())
        for file in resource.iter("file"):
            self.assertTrue((INFO / file.text.replace("@Launcher_AppID@", APP_ID)).exists())
        main = (ROOT / "launcher/main.cpp").read_text()
        self.assertIn("Q_INIT_RESOURCE(elysiumlauncher);", main)
        self.assertNotIn("Q_INIT_RESOURCE(elyprismlauncher);", main)

    def test_apple_catalog_assets(self):
        catalog = INFO / "ElysiumLauncher.icon"
        data = json.loads((catalog / "icon.json").read_text())
        for group in data["groups"]:
            for layer in group["layers"]:
                asset = catalog / "Assets" / layer["image-name"]
                self.assertTrue(asset.is_file())
                ET.fromstring(asset.read_text())

    def test_default_identity_and_updater(self):
        cmake = (ROOT / "CMakeLists.txt").read_text()
        self.assertRegex(cmake, r'set\(Launcher_APP_BINARY_NAME "elysiumlauncher"')
        self.assertRegex(cmake, r'set\(Launcher_UPDATER_GITHUB_REPO ""')
        info = (INFO / "CMakeLists.txt").read_text()
        for value in ("ElysiumLauncher", APP_ID, "ELYSIUMLAUNCHER"):
            self.assertIn(value, info)
        application = (ROOT / "launcher/Application.cpp").read_text()
        self.assertIn('registerSetting("ApplicationTheme", QStringLiteral("elysium"))', application)
        manager = (ROOT / "launcher/ui/themes/ThemeManager.cpp").read_text()
        self.assertIn("addTheme(std::make_unique<ElysiumTheme>())", manager)

    def test_main_layout_is_vertical(self):
        ui = ET.parse(ROOT / "launcher/ui/MainWindow.ui")
        layout = ui.find('.//layout[@name="horizontalLayout"]')
        self.assertEqual(layout.attrib["class"], "QVBoxLayout")

    def test_theme_is_in_cmake_and_qss_balances(self):
        self.assertIn("ui/themes/ElysiumTheme.h", (ROOT / "launcher/CMakeLists.txt").read_text())
        theme = (ROOT / "launcher/ui/themes/ElysiumTheme.h").read_text()
        qss = re.search(r'R"QSS\((.*?)\)QSS"', theme, re.S).group(1)
        self.assertEqual(qss.count("{"), qss.count("}"))
        self.assertGreater(qss.count("{"), 10)


if __name__ == "__main__":
    unittest.main(verbosity=2)
