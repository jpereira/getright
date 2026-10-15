# SPDX-License-Identifier: LGPL-3.0-only
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Generate the website package list from a published GitHub release.

import json
import os
from pathlib import Path
import urllib.error
import urllib.request


repository = "jpereira/getright"
headers = {"Accept": "application/vnd.github+json", "User-Agent": "GetRight-Port"}
if os.environ.get("GH_TOKEN"):
    headers["Authorization"] = "Bearer " + os.environ["GH_TOKEN"]
request = urllib.request.Request(
    f"https://api.github.com/repos/{repository}/releases/latest", headers=headers)
try:
    with urllib.request.urlopen(request, timeout=30) as response:
        release = json.load(response)
except urllib.error.HTTPError as error:
    if error.code != 404:
        raise
    release = {}

manifest = {
    "version": release.get("tag_name"),
    "url": release.get("html_url", f"https://github.com/{repository}/releases"),
    "published": release.get("published_at"),
    "assets": [
        {"name": asset["name"], "url": asset["browser_download_url"], "size": asset["size"]}
        for asset in release.get("assets", [])
        if asset["name"].endswith((".dmg", ".zip", ".tar.gz", ".txt"))
    ],
}
Path("site/downloads.json").write_text(json.dumps(manifest, indent=2) + "\n")
