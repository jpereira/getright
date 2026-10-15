# SPDX-License-Identifier: LGPL-3.0-only
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Verify macOS bundle plugins, metadata, and library independence.

import os
from pathlib import Path
import plistlib
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]
BUNDLE = Path(os.environ.get("GETRIGHT_APP_BUNDLE", ROOT / "build/package/GetRight Port.app"))
CONTENTS = BUNDLE / "Contents"
EXECUTABLE = CONTENTS / "MacOS/GetRight Port"


class BundleTests(unittest.TestCase):
    def test_required_runtime_components(self):
        for plugin in (
            "platforms/libqcocoa.dylib", "platforms/libqoffscreen.dylib",
            "styles/libqmacstyle.dylib", "tls/libqsecuretransportbackend.dylib",
            "tls/libqcertonlybackend.dylib",
        ):
            self.assertTrue((CONTENTS / "PlugIns" / plugin).is_file(), plugin)
        for notice in ("LICENSE.md", "THIRD_PARTY_NOTICES.md", "qt.conf"):
            self.assertTrue((CONTENTS / "Resources" / notice).is_file(), notice)

    def test_dependencies_are_bundled_or_provided_by_macos(self):
        binaries = [EXECUTABLE, *CONTENTS.rglob("*.dylib")]
        frameworks = list((CONTENTS / "Frameworks").glob("*.framework"))
        self.assertGreaterEqual(len(frameworks), 4)
        binaries.extend(framework / "Versions/A" / framework.stem for framework in frameworks)
        for binary in binaries:
            output = subprocess.check_output(["otool", "-L", str(binary)], text=True)
            for line in output.splitlines()[1:]:
                dependency = line.strip().split(" (", 1)[0]
                self.assertFalse(dependency.startswith(("/opt/", "/usr/local/")), dependency)
                if dependency.startswith("@rpath/"):
                    relative = dependency.removeprefix("@rpath/")
                    self.assertTrue((CONTENTS / "Frameworks" / relative).exists(), dependency)
                if dependency.startswith("@loader_path/"):
                    relative = dependency.removeprefix("@loader_path/")
                    self.assertTrue((binary.parent / relative).exists(), dependency)

    def test_release_architecture_and_version(self):
        architecture = subprocess.check_output(["lipo", "-archs", str(EXECUTABLE)], text=True)
        self.assertEqual(architecture.strip(), "arm64")
        with (CONTENTS / "Info.plist").open("rb") as file:
            metadata = plistlib.load(file)
        version = (ROOT / "VERSION").read_text().strip().removeprefix("v")
        self.assertEqual(metadata["CFBundleShortVersionString"], version)
        self.assertEqual(metadata["CFBundleVersion"], version)


if __name__ == "__main__":
    unittest.main()
