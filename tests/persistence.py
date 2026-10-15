# SPDX-License-Identifier: LGPL-3.0-only
# Copyright (C) 2026 Jorge Pereira
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Verify settings and transfer history persistence.

"""Exercise JSON persistence through the compiled application, with isolated data."""

import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest


APP = Path(os.environ.get(
    "GETRIGHT_APP_EXECUTABLE",
    Path(__file__).resolve().parents[1] / "build/getright-port.app/Contents/MacOS/getright-port",
))


class PersistenceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="getright-persistence-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)

    def run_app(self, *options):
        subprocess.run(
            [str(APP), "--screenshot", str(self.directory / "window.png"), *options],
            env={**os.environ, "QT_QPA_PLATFORM": "offscreen",
                 "GETRIGHT_PORT_DATA_DIR": str(self.directory)},
            check=True, capture_output=True, timeout=20,
        )

    def read(self, name):
        return json.loads((self.directory / name).read_text())

    def test_empty_install_and_settings(self):
        self.run_app("--theme", "dark", "--look-and-feel", "windowsxp")
        self.assertEqual(self.read("history.json")["transfers"], [])
        settings = self.read("settings.json")
        self.assertEqual(settings["theme"], "dark")
        self.assertEqual(settings["lookAndFeel"], "windowsxp")
        self.assertTrue(settings["windowGeometry"])
        self.run_app()
        restored = self.read("settings.json")
        self.assertTrue(restored.pop("windowGeometry"))
        settings.pop("windowGeometry")
        self.assertEqual(restored, settings)
        self.assertEqual(self.read("history.json")["transfers"], [])

    def test_history_survives_restart_and_tracks_local_partial(self):
        partial = self.directory / "partial.bin"
        partial.write_bytes(b"partial")
        transfers = [
            {"id": "partial", "name": partial.name, "path": str(partial),
             "url": "https://example.invalid/file", "direction": "download",
             "state": "Downloading", "bytesReceived": "999", "totalBytes": "1000",
             "addedAt": "2026-10-14T12:00:00.000Z", "elapsedSeconds": "12"},
            {"id": "finished", "name": "finished.bin", "path": "/missing/finished.bin",
             "url": "https://example.invalid/finished", "direction": "download",
             "state": "Complete", "bytesReceived": "42", "totalBytes": "42",
             "completedAt": "2026-10-14T13:00:00.000Z"},
        ]
        (self.directory / "history.json").write_text(json.dumps({"transfers": transfers}))
        self.run_app()
        saved = self.read("history.json")["transfers"]
        self.assertEqual(len(saved), 2)
        self.assertEqual(saved[0]["state"], "Paused")
        self.assertEqual(saved[0]["bytesReceived"], str(partial.stat().st_size))
        for key in ("id", "url", "path", "addedAt", "elapsedSeconds"):
            self.assertEqual(saved[0][key], transfers[0][key])
        self.assertEqual(saved[1]["state"], "Complete")
        self.assertEqual(saved[1]["completedAt"], transfers[1]["completedAt"])
        self.run_app()
        self.assertEqual(self.read("history.json")["transfers"], saved)
        partial.unlink()
        self.run_app()
        self.assertEqual(self.read("history.json")["transfers"][0]["bytesReceived"], "0")

    def test_pending_restart_preserves_file_and_zero_offset(self):
        destination = self.directory / "download.bin"
        destination.write_bytes(b"completed download")
        transfer = {"id": "restart", "name": destination.name, "path": str(destination),
                    "url": "https://example.invalid/download", "direction": "download",
                    "state": "Queued", "restartRequested": True,
                    "bytesReceived": "0", "totalBytes": "18"}
        (self.directory / "history.json").write_text(
            json.dumps({"transfers": [transfer]}))
        for _ in range(2):
            self.run_app()
            saved = self.read("history.json")["transfers"][0]
            self.assertTrue(saved["restartRequested"])
            self.assertEqual(saved["bytesReceived"], "0")
            self.assertEqual(destination.read_bytes(), b"completed download")

    def test_unreadable_json_is_preserved(self):
        for name in ("history.json", "settings.json"):
            (self.directory / name).write_text("{broken")
        self.run_app()
        for name in ("history.json", "settings.json"):
            self.assertEqual((self.directory / name).read_text(), "{broken")


if __name__ == "__main__":
    unittest.main()
