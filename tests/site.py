# SPDX-License-Identifier: LGPL-3.0-only
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Check local website links, assets, and release manifest integrity.

from html.parser import HTMLParser
import json
from pathlib import Path
import unittest
from urllib.parse import urlsplit


ROOT = Path(__file__).resolve().parents[1] / "site"


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = []
        self.references = []
        self.images = []
        self.language = None

    def handle_starttag(self, tag, attributes):
        attributes = dict(attributes)
        if tag == "html":
            self.language = attributes.get("lang")
        if "id" in attributes:
            self.ids.append(attributes["id"])
        if tag in ("a", "link", "script", "img"):
            value = attributes.get("href", attributes.get("src"))
            if value:
                self.references.append(value)
        if tag == "img":
            self.images.append(attributes)


class WebsiteTests(unittest.TestCase):
    def setUp(self):
        self.page = Page()
        self.page.feed((ROOT / "index.html").read_text())

    def test_local_links_and_section_targets(self):
        self.assertEqual(self.page.language, "en")
        self.assertEqual(len(self.page.ids), len(set(self.page.ids)))
        for reference in self.page.references:
            parts = urlsplit(reference)
            if parts.scheme or parts.netloc:
                continue
            if parts.path:
                self.assertTrue((ROOT / parts.path).is_file(), reference)
            elif parts.fragment:
                self.assertIn(parts.fragment, self.page.ids)

    def test_screenshot_accessibility(self):
        self.assertGreaterEqual(len(self.page.images), 5)
        for image in self.page.images:
            self.assertIn("alt", image)
        for name in ("port-light.png", "port-dark.png", "port-classic.png",
                     "port-progress.png", "port-finished.png", "port-download.gif",
                     "port-window-download.gif"):
            self.assertTrue((ROOT / "assets" / name).is_file(), name)
        readme = (ROOT.parent / "README.md").read_text()
        self.assertLess(readme.index("site/assets/port-window-download.gif"), readme.index("## Features"))

    def test_release_downloads_point_to_published_assets(self):
        manifest = json.loads((ROOT / "downloads.json").read_text())
        self.assertIn("version", manifest)
        self.assertIsInstance(manifest["assets"], list)
        for asset in manifest["assets"]:
            self.assertTrue(asset["url"].startswith(
                "https://github.com/jpereira/getright/releases/download/"))
            self.assertGreater(asset["size"], 0)
            self.assertTrue(asset["name"].endswith((".dmg", ".zip", ".tar.gz", ".txt")))


if __name__ == "__main__":
    unittest.main()
