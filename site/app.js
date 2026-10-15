// SPDX-License-Identifier: LGPL-3.0-only
// Author: Jorge Pereira <jpereiran@gmail.com>
// Date: 2026-10-14
// Description: Screenshot gallery and published release package list.

const views = {
  light: {
    file: "port-light.png",
    alt: "GetRight Port in the native macOS light appearance during a live download",
    caption: "Native macOS, light theme. Click the image to see it full size.",
    counter: "01 / 03",
  },
  dark: {
    file: "port-dark.png",
    alt: "GetRight Port in the native macOS dark appearance during a live download",
    caption: "Native macOS, dark theme. The same queue, a different appearance.",
    counter: "02 / 03",
  },
  progress: {
    file: "port-progress.png",
    alt: "A live GetRight Port download progress window with speed and remaining time",
    caption: "Double-click a transfer for its file, progress, speed, and timing.",
    counter: "03 / 03",
  },
};

for (const button of document.querySelectorAll("[data-view]")) {
  button.addEventListener("click", () => {
    const view = views[button.dataset.view];
    for (const tab of document.querySelectorAll("[data-view]")) {
      tab.classList.toggle("selected", tab === button);
      tab.setAttribute("aria-pressed", String(tab === button));
    }
    const image = document.querySelector("#gallery-image");
    image.src = `assets/${view.file}`;
    image.alt = view.alt;
    document.querySelector("#gallery-link").href = image.src;
    document.querySelector("#gallery-caption").textContent = view.caption;
    document.querySelector("#gallery-counter").textContent = view.counter;
  });
}

function packageInfo(name) {
  if (name.endsWith(".dmg")) return ["macOS installer", "DMG"];
  if (name.endsWith(".zip")) return ["macOS application", "ZIP"];
  if (name.endsWith(".tar.gz")) return ["Source code", "TAR.GZ"];
  return ["SHA-256 checksums", "TXT"];
}

function formatSize(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

function releaseFallback(message) {
  const body = document.querySelector("#packages");
  body.replaceChildren();
  const row = body.insertRow();
  const cell = row.insertCell();
  cell.colSpan = 4;
  cell.textContent = message;
  const status = document.querySelector("#download-status");
  status.replaceChildren();
  const link = document.createElement("a");
  link.href = "https://github.com/jpereira/getright/releases";
  link.textContent = "Browse releases on GitHub ↗";
  status.append(link);
}

async function loadPackages() {
  try {
    const response = await fetch("downloads.json");
    if (!response.ok) throw new Error("Release list unavailable");
    const release = await response.json();
    const assets = (release.assets || []).filter((asset) =>
      asset.url.startsWith("https://github.com/jpereira/getright/releases/download/") &&
      /\.(dmg|zip|tar\.gz|txt)$/.test(asset.name));
    if (!release.version || !assets.length) {
      releaseFallback("Packaged releases are being prepared. Source code is available on GitHub.");
      return;
    }
    document.querySelector("#release-version").textContent = release.version;
    const body = document.querySelector("#packages");
    body.replaceChildren();
    for (const asset of assets) {
      const [label, format] = packageInfo(asset.name);
      const row = body.insertRow();
      row.insertCell().textContent = label;
      row.insertCell().textContent = format;
      row.insertCell().textContent = formatSize(asset.size);
      const link = document.createElement("a");
      link.href = asset.url;
      link.textContent = "Download ↓";
      link.setAttribute("aria-label", `Download ${asset.name}`);
      link.title = asset.name;
      row.insertCell().append(link);
    }
    document.querySelector("#download-status").textContent =
      `${release.version} · Published packages from GitHub Releases. Source and checksums included.`;
  } catch {
    releaseFallback("The package list could not be loaded. Downloads are available on GitHub.");
  }
}

loadPackages();
