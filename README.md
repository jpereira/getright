# GetRight Port

<img src="resources/getright-logo.png" alt="Original GetRight logo" width="144">

The original logo comes from the [GetRight press page](https://getright.com/press/).

A study port of the GetRight 6.5 download manager to a cross-platform codebase.
The interface uses Qt Widgets and preserves the classic Download Status layout:
filters on the left, a task list, a toolbar, detailed progress, and menus.

[Project website](https://jpereira.github.io/getright/) ·
[Download packages](https://github.com/jpereira/getright/releases)

![GetRight Port running on macOS with the native light appearance](site/assets/port-light.png)

![A real GetRight download with an advancing progress bar](site/assets/port-download.gif)

## Features

- Download filters by status, folder, and common file types.
- Persistent history by day, last week, and last month.
- HTTP/HTTPS download queue, one file at a time.
- Local file uploads to HTTP/HTTPS destinations that accept `PUT`.
- Start, stop, resume, remove, and monitor transfers.
- Pause and resume downloads while preserving partial files.
- Right-click a transfer for Start, Pause, Stop, Remove, Download Again, Copy URL, and Move To.
- Move downloads to another folder, preserving partial files and resuming active transfers.
- Click a transfer to open a window with its file, address, destination,
  progress, speed, elapsed time, and estimated time remaining.
- Optional details panel with URL, local destination, progress, speed, timing, and saved errors.
- Download completion tips with file and folder actions, displayed near the menu bar icon.
- Window title with combined progress, total active speed, and active/total counts for
  downloads and uploads. Progress includes all transfers, regardless of the selected filter;
  it shows "size unknown" while any unfinished transfer has an unknown size.
- System, Light, and Dark themes saved in user preferences.
- Native macOS appearance or styles inspired by Windows 11 and Windows XP.
- File, Edit, Downloads, View, and Help menus with keyboard shortcuts.

## Data, history, and settings

The application stores its data in `~/.getright/`:

- `settings.json`: theme, appearance, window geometry, last folder, and display preferences.
- `history.json`: queue and history, including ID, name, URL, destination, transfer type,
  status, bytes received, total size, added and completion dates, active duration,
  HTTP validators, and the last error.

JSON files use atomic writes. History is saved every two seconds when there are changes
and when the application exits. Active transfers are restored as paused; use **Start**
to resume them. The resume position comes from the actual size of the partial file at
the destination; keep that file to continue downloading. The `bytesReceived` counter
records local bytes for incomplete downloads.

An installation without history starts with an empty list. To use another data directory,
set `GETRIGHT_PORT_DATA_DIR`; both JSON files are stored there.

A safe resume requires HTTP Range support and a saved strong ETag or Last-Modified validator.
The app sends If-Range and checks the returned byte range. Downloads without a usable validator,
including older history entries, restart after a successful full response. HTTP errors preserve
existing files; invalid ranges fail without appending data. Local write failures mark the task
as failed, and the queue proceeds to the next eligible transfer.

Choosing an existing file in Add requests replacement, rather than treating it as a partial
for that URL. Existing contents remain untouched until a valid download response arrives.

Unreadable JSON is preserved and a recovery message remains visible in the window.
Use **File > Open Data Folder**, quit the app, and keep a backup of the affected file.
Repair it or rename it with a `.broken` suffix, then reopen the app to create a fresh file.
Renaming `history.json` starts an empty queue; downloaded files remain on disk.

Paused and stopped HTTP PUT uploads restart from the beginning. Authentication, FTP, BitTorrent,
segmented downloads, and browser integration are not implemented.

## Installation

Release packages require macOS 15 or later on Apple Silicon. Open the DMG and copy the app
to Applications, or extract the application ZIP. Packages include Qt and its runtime dependencies.
The app is ad-hoc signed and is not Apple-notarized.

## Build requirements

- macOS with Xcode Command Line Tools.
- CMake 3.21 or later and Qt 6.8 or later, with Widgets and Network.
- `macdeployqt` and `hdiutil` to create the self-contained DMG.

With Homebrew:

```sh
brew install cmake qt
```

## Build and run

From the repository root:

```sh
make          # Show help without building
make osx      # Build the arm64 bundle
open build/getright-port.app
```

The executable is at `build/getright-port.app/Contents/MacOS/getright-port`.
To use another build directory or Qt prefix:

```sh
make osx BUILD_DIR=out
make osx QT_PREFIX=/path/to/qt
```

After building, verify persistence with isolated temporary data:

```sh
ctest --test-dir build --output-on-failure
python3 tests/persistence.py
```

The CTest suite exercises a local HTTP server, resume validation, disk write failures,
queue progression, multiple selection, transfer window controls, and completion tips.

## Create the installer

```sh
make osx-dmg
```

This command copies the required Qt frameworks and plugins into the bundle and creates
`build/getright-port-macos-arm64.dmg`. The packaged app is staged separately at
`build/package/getright-port.app`; the development bundle stays linked to the local Qt install.
The DMG includes dynamic Qt dependencies;
project source code and license notices are available in this repository.

## GitHub builds and releases

GitHub Actions builds and tests the macOS Apple Silicon app on pushes to `main` and pull
requests. Build artifacts include the DMG, application ZIP, source archive, and SHA-256
checksums. The bundle includes native macOS styling and HTTPS plugins as well as Qt.

To publish a release, set `VERSION` to the intended `vMAJOR.MINOR.PATCH`, commit the change,
and push a matching tag:

```sh
git tag v1.0.1
git push origin main v1.0.1
```

The tag build publishes packages on GitHub Releases and refreshes the website download
list. Releases are ad-hoc signed and are not Apple-notarized. GitHub Pages publishes the
static project site from `site/`; its package list contains assets from a published release.

## Interface and themes

Use `View > Theme` to choose **System**, **Light**, or **Dark**, and
`View > Look & Feel` to select macOS, Windows 11, or Windows XP. Both preferences persist
between sessions. On macOS, the Windows styles are visual approximations built with
Qt Fusion controls. The native macOS style follows the selected application color scheme;
the Windows styles use matching palettes and button colors.

**Add** prefills its URL field with a valid HTTP/HTTPS address from the clipboard.
Review or edit it before confirming.

Use **Pause** to suspend a download and **Start** to resume it. Start, Pause, Stop, and Remove
apply to all selected rows. Start schedules the selection while another transfer is active.
Click a single row to open the transfer window; modifier-clicks keep multiple selection available.
The compact window provides Options, Pause/Resume, and Cancel. Cancel stops its transfer and
closes the window. Options includes a live speed graph, Copy URL, and Show in Finder.
**View > Show transfer details** enables the main details panel; **Show toolbar labels**
adds labels to the compact icon toolbar. Both choices persist.

Errors remain available in transfer details and history. Completed files offer **Open File**
and **Show in Finder** in the context menu. Right-click a row to control the transfer or copy
its URL.

**Move To** selects another destination folder and moves any existing partial or completed
file there. An active download pauses during the move and resumes afterward. Existing
destination files are preserved; choose another folder if the name is already in use.
If the move fails, an active download stays paused at its original destination.
**Download Again** queues a completed download from the beginning, replacing existing contents
once a successful response is validated. The restart request persists between sessions.

**Upload** asks for a local file and an HTTP/HTTPS destination;
select the task and press **Start** to send it using `PUT`.

The executable accepts `--screenshot path.png` to save the current interface.
Add `--progress-window` to capture the progress window for the first transfer in the list.
Install `ffmpeg` with `brew install ffmpeg`, then run `make screenshots` in a graphical macOS
session to regenerate the five still captures and animated download GIF in `site/assets/`.
The capture tool uses a real source archive, a local HTTP server, and isolated temporary data.
Every captured frame has a one-pixel black outline.

<img src="site/assets/port-progress.png" alt="Compact GetRight download window" width="470">

<img src="site/assets/port-finished.png" alt="Download completion tip" width="320">

## Background transfers and menu bar icon

Closing the main window hides it and keeps transfers running in the background.
The original GetRight logo remains in the macOS menu bar. Click it with the left or middle
mouse button to hide a visible window or restore a hidden or minimized window and bring
it forward. Hover over the icon to see the combined download and upload speeds.

Right-click the icon for **Open**, **Pause All**, **Resume All**, and **Close**.
**Open** always displays the main window. **Pause All** pauses active and queued transfers,
keeping partial downloads. **Resume All** schedules paused and queued downloads and uploads,
one at a time. Stopped and failed transfers require **Start**. **Close** quits the application.

A completed download shows a tip near the menu bar icon without taking focus. Click its link
or icon to open the file, or its folder button to reveal the file in Finder. The tip disappears
after fifteen seconds; right-click it and select **Keep visible** to retain it. Downloads do not
open automatically.

Use **File > Exit** or **Cmd+Q** to quit the application and save its state.
On desktops without a system tray, closing the window exits the application.

## Open source and license

GetRight Port is open source software distributed under the **GNU LGPL version 3**.
The full text is available in [LICENSE.md](LICENSE.md).

Qt Widgets and Qt Network offer LGPL version 3 or GPL licensing, as described in the
Qt documentation. This project links Qt dynamically. When distributing it, retain the
applicable notices and sources and meet the LGPL requirements, including the information
needed to replace or relink the libraries.
See [Qt licensing](https://doc.qt.io/qt-6/licensing.html) and
[Qt LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations).

**Author:** Jorge Pereira <jpereiran@gmail.com>  
**Project:** <https://github.com/jpereira/getright>

## Study and provenance

This study project was developed with help from OpenAI Codex and REA
(Reverse Engineering Assistant). The original executable used as a reference is available
at <http://download.getright.com/getright-download.exe>.
