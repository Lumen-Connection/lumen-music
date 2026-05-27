<div align="center">

# 🔶 Lumen Music

**A lightweight, elegant desktop music player built in C++ with Qt.**

Local library, playlists with cover art, playback queue, listening history, and customizable themes.

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6.11-41CD52?logo=qt&logoColor=white)
![Platform](https://img.shields.io/badge/Windows-x64-0078D6?logo=windows&logoColor=white)
![SQLite](https://img.shields.io/badge/SQLite-local-003B57?logo=sqlite&logoColor=white)

</div>

---

## About

**Lumen Music** is a local audio player focused on a clean, fluid experience.
You import your files, organize them into playlists, like tracks, and control
everything through a dark, minimalist interface. No cloud, no accounts —
everything is stored locally.

## Features

### Library and organization
- **Import music** from local audio files
- **Playlists** — create, rename, and delete
- **Playlist cover** with a color gradient **or an image** (with lightbox zoom)
- **Reorder tracks** within a playlist via **drag and drop** (order is persisted)
- **Liked songs** as their own collection, with a play button
- **Edit** track title and artist, and **delete** tracks
- Clicking a playlist tag jumps straight to it

### Playback
- **Full player**: play/pause, previous/next, shuffle, repeat, volume, and mute
- **Spotify-style queue**: add to queue, view, and remove; queued tracks play
  before the current context resumes
- **Context-aware playback** — each playlist, the liked collection, and the
  library play within themselves (next/previous respects where you started)
- **Recently played** — history based on what was actually played

### Look and feel
- **Themes**: Lumen (default), Hot Vinyl, Ocean, Forest, Night Purple, and Modern Gray
- Dark, minimalist, and responsive interface

## Tech stack

- **Language:** C++17
- **Framework:** Qt 6 (Widgets, Multimedia, SQL)
- **Build:** CMake + Ninja + MinGW
- **Storage:** SQLite (local database)

## Building the project

### Prerequisites
- [Qt 6.11](https://www.qt.io/download) with the **MinGW 64-bit** kit and the
  **Multimedia** and **SQL** modules

### Option A — Qt Creator (easiest)
1. Open `CMakeLists.txt` in Qt Creator
2. Select the **Desktop Qt 6.x MinGW 64-bit** kit
3. Click **Run**

### Option B — Command line (PowerShell)
The project uses **CMake** with **presets** (`CMakePresets.json`). Adjust the
Qt/MinGW paths inside the presets to match your installation if they differ.

```powershell
# Put Qt's CMake/Ninja/MinGW on the PATH (adjust for your version):
$env:Path = "D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;D:\Qt\Tools\mingw1310_64\bin;D:\Qt\6.11.0\mingw_64\bin;" + $env:Path

cmake --preset release          # configure
cmake --build --preset release  # build
```

> **Paths with accents/spaces:** the MinGW tools (moc/windres) don't handle
> build directories containing non-ASCII characters well (e.g. "Área de
> Trabalho", the Portuguese name for Desktop). That's why the presets put the
> build under `%LOCALAPPDATA%\LumenMusic-build\<preset>` (an ASCII path) rather
> than inside the repository. The `LumenMusic.exe` binary lands there.

To build without presets, point the build directory at a path without accents:

```powershell
cmake -S . -B "$env:LOCALAPPDATA\LumenMusic-build\release" -G Ninja `
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="D:/Qt/6.11.0/mingw_64"
cmake --build "$env:LOCALAPPDATA\LumenMusic-build\release"
```

## Building a distributable package

Packaging is automatic: CMake's `install` step invokes `windeployqt`, so a
`cmake --install` or a `cpack` produces a folder/zip that runs on a PC
**without Qt installed**.

### Self-contained folder (`cmake --install`)

```powershell
$build = "$env:LOCALAPPDATA\LumenMusic-build\release"
cmake --install $build --prefix dist\LumenMusic
```

Produces `dist\LumenMusic\` with `LumenMusic.exe`, `yt-dlp.exe`, the Qt DLLs,
and the plugins. Test it by running `dist\LumenMusic\LumenMusic.exe` and playing
a track (this exercises the multimedia plugins and the SQLite driver).

### Release ZIP (`cpack`)

```powershell
$build = "$env:LOCALAPPDATA\LumenMusic-build\release"
cpack --config "$build\CPackConfig.cmake" -G ZIP -B dist
```

Produces `dist\LumenMusic-v<version>-win64.zip`. The version comes from a single
source of truth — `project(... VERSION ...)` in `CMakeLists.txt` — which is also
embedded into the `.exe` (VERSIONINFO) and read automatically by the Inno Setup
installer.

### Installer (Inno Setup, optional)

With the `dist\LumenMusic` folder ready, compile `installer\lumen-music.iss` with
`ISCC.exe`. The version is read straight from the `.exe`, so there's no version
number to maintain by hand.

## License

Built by [Lumen Connection](https://lumenconnection.com.br), distributed under the
[AGPL-3.0](LICENSE) license.

---

<div align="center">
Made with C++ and Qt • <strong>🄯 Lumen Connection</strong>
</div>
