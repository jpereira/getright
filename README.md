# GetRight Port

<img src="resources/getright-logo.png" alt="Original GetRight logo" width="144">

The original logo comes from the [GetRight press page](https://getright.com/press/).

A study port of the GetRight 6.5 download manager to a cross-platform codebase.
The interface uses Qt Widgets and preserves the classic Download Status layout:
filters on the left, a task list, a toolbar, detailed progress, and menus.

## Features

- Download filters by status, folder, and common file types.
- Persistent history by day, last week, and last month.
- HTTP/HTTPS download queue, one file at a time.
- Local file uploads to HTTP/HTTPS destinations that accept `PUT`.
- Start, stop, resume, remove, and monitor transfers.
- Pause and resume downloads while preserving partial files.
- Right-click a transfer for Start, Pause, Stop, Remove, Download Again, Copy URL, and Move To.
- Move downloads to another folder, preserving partial files and resuming active transfers.
- Double-click a transfer to open a window with its file, address, destination,
  progress, speed, elapsed time, and estimated time remaining.
- Details panel with URL, local destination, size, percentage, speed, and timing.
- Window title with combined progress, total active speed, and active/total counts for
  downloads and uploads. Progress includes all transfers, regardless of the selected filter;
  it shows "size unknown" while any unfinished transfer has an unknown size.
- System, Light, and Dark themes saved in user preferences.
- Native macOS appearance or styles inspired by Windows 11 and Windows XP.
- File, Downloads, View, and Help menus with keyboard shortcuts.

## Data, history, and settings

The application stores its data in `~/.getright/`:

- `settings.json`: theme, appearance, window geometry, and last download folder.
- `history.json`: queue and history, including ID, name, URL, destination, transfer type,
  status, bytes received, total size, added and completion dates, and active duration.

JSON files use atomic writes. History is saved every two seconds when there are changes
and when the application exits. Active transfers are restored as paused; use **Start**
to resume them. The resume position comes from the actual size of the partial file at
the destination; keep that file to continue downloading. The `bytesReceived` counter
records local bytes for incomplete downloads.

An installation without history starts with an empty list. To use another data directory,
set `GETRIGHT_PORT_DATA_DIR`; both JSON files are stored there.

Resuming a download requires the server to support HTTP Range requests.
Stopped HTTP PUT uploads restart from the beginning. Authentication, FTP, BitTorrent,
segmented downloads, and browser integration are not implemented.

## Requirements

- macOS 15 or later on Apple Silicon for release packages.
- Xcode Command Line Tools.
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
python3 tests/persistence.py
```

## Create the installer

```sh
make osx-dmg
```

This command copies the required Qt frameworks and plugins into the bundle and creates
`build/getright-port-macos-arm64.dmg`. The DMG includes dynamic Qt dependencies;
project source code and license notices are available in this repository.

## Interface and themes

Use `View > Theme` to choose **System**, **Light**, or **Dark**, and
`View > Look & Feel` to select macOS, Windows 11, or Windows XP. Both preferences persist
between sessions. On macOS, the Windows styles are visual approximations built with
Qt Fusion controls. The native macOS style follows the selected application color scheme;
the Windows styles use matching palettes and button colors.

**Add** prefills its URL field with a valid HTTP/HTTPS address from the clipboard.
Review or edit it before confirming.

Use **Pause** to suspend a download and **Start** to resume it. Double-click a row to open
the transfer progress window. Right-click a row to control the transfer or copy its URL.
**Move To** selects another destination folder and moves any existing partial or completed
file there. An active download pauses during the move and resumes afterward. Existing
destination files are preserved; choose another folder if the name is already in use.
If the move fails, an active download stays paused at its original destination.
**Download Again** queues a completed download from the beginning, replacing the existing
file when the new transfer starts. The restart request persists between sessions.

**Upload** asks for a local file and an HTTP/HTTPS destination;
select the task and press **Start** to send it using `PUT`.

The executable accepts `--screenshot path.png` to save the current interface.
Add `--progress-window` to capture the progress window for the first transfer in the list.
The `make screenshots` target generates a local screenshot when needed.

## Background transfers and menu bar icon

Closing the main window hides it and keeps transfers running in the background.
The original GetRight logo remains in the macOS menu bar. Click it with the left or middle
mouse button to hide a visible window or restore a hidden or minimized window and bring
it forward. Hover over the icon to see the combined download and upload speeds.

Right-click the icon for **Open**, **Pause All**, **Resume All**, and **Close**.
**Open** always displays the main window. **Pause All** pauses active and queued transfers,
keeping partial downloads. **Resume All** schedules paused and queued downloads and uploads,
one at a time. Stopped and failed transfers require **Start**. **Close** quits the application.

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
