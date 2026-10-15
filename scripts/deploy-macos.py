# SPDX-License-Identifier: LGPL-3.0-only
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Bundle transitive macOS libraries and configure local Qt plugins.

import argparse
import os
from pathlib import Path
import shutil
import subprocess


def macho_files(contents):
    magic = {b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xca\xfe\xba\xbe",
             b"\xbe\xba\xfe\xca", b"\xca\xfe\xba\xbf", b"\xbf\xba\xfe\xca"}
    found = set()
    for path in contents.rglob("*"):
        if not path.is_file():
            continue
        path = path.resolve()
        if not path.is_relative_to(contents):
            raise RuntimeError(f"Bundle symlink points outside the app: {path}")
        if path in found:
            continue
        with path.open("rb") as file:
            if file.read(4) in magic:
                found.add(path)
    return found


def deploy(bundle):
    contents = bundle.resolve() / "Contents"
    frameworks = contents / "Frameworks"
    frameworks.mkdir(exist_ok=True)
    for _ in range(100):
        changed = False
        for binary in macho_files(contents):
            output = subprocess.check_output(["otool", "-L", str(binary)], text=True)
            for line in output.splitlines()[1:]:
                dependency = line.strip().split(" (", 1)[0]
                if not dependency.startswith(("/opt/", "/usr/local/")):
                    continue
                source = Path(dependency)
                if ".framework/" in dependency:
                    prefix, relative = dependency.split(".framework/", 1)
                    source_framework = Path(prefix + ".framework")
                    target_framework = frameworks / source_framework.name
                    if not target_framework.exists():
                        shutil.copytree(source_framework, target_framework, symlinks=True)
                    target = target_framework / relative
                else:
                    target = frameworks / source.name
                    if not target.exists():
                        shutil.copy2(source, target)
                if not target.resolve().is_relative_to(contents):
                    raise RuntimeError(f"Library target points outside the app: {target}")
                replacement = "@loader_path/" + os.path.relpath(target, binary.parent)
                subprocess.run(["install_name_tool", "-change", dependency, replacement,
                                str(binary)], check=True)
                identity = "@rpath/" + str(target.relative_to(frameworks))
                subprocess.run(["install_name_tool", "-id", identity, str(target)], check=True)
                print(f"Bundled {source.name} for {binary.name}")
                changed = True
        if not changed:
            break
    else:
        raise RuntimeError("Library deployment did not converge")
    (contents / "Resources/qt.conf").write_text(
        "[Paths]\nPlugins = PlugIns\nLibraries = Frameworks\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Deploy transitive macOS bundle dependencies.")
    parser.add_argument("bundle", type=Path)
    deploy(parser.parse_args().bundle)
