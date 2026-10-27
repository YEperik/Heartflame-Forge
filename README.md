<div align="center">
  <img src="assets/heartflame-forge.svg" alt="Heartflame Forge logo" width="132" />

  # Heartflame Forge

  **A modular desktop workshop for game-asset inspection and modding research.**

  *Inspect carefully. Edit conservatively. Keep the originals safe.*

  ![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
  ![Qt 6](https://img.shields.io/badge/GUI-Qt%206-41CD52?logo=qt&logoColor=white)
  ![Windows](https://img.shields.io/badge/Target-Windows%2010%2F11%20x64-0078D4?logo=windows11&logoColor=white)
  ![License](https://img.shields.io/badge/License-MIT-2E8B57.svg)

  **Version 1.3.0 · Windows-first · Local asset processing**

  [Bahasa Indonesia](README.id.md) · [English](README.md)
</div>

---

## Overview

**Heartflame Forge** is a C++17/Qt Widgets workbench for exploring game-asset containers and building format-specific modding tools. The application is organized into separate workspaces, with a C++ core that can be used independently of the GUI.

The project favors conservative handling: unsupported layouts are reported instead of guessed, edited files are written to new destinations, and the source archive is kept unchanged during the tested replacement flows.

> **Important:** This is an evolving research workbench, not a universal game-file editor. A file being recognized by its extension or header does not mean its contents are fully decoded, editable, or safe to rebuild.

## Windows downloads

The project contains a Windows x64 release workflow that builds the Qt desktop app, runs tests, packages Qt runtime files, creates a portable ZIP and an installer, and generates SHA-256 checksums.

**This source package does not include a prebuilt Windows `.exe`.** A Windows binary becomes available only after this source is pushed to a GitHub repository and the Windows workflow completes successfully. On a repository page, open **Actions → Heartflame Forge — Windows** to run or inspect the workflow. For a tagged release such as `v1.3.0`, the workflow is configured to publish its artifacts as a GitHub Release.

| Package | Intended use |
| --- | --- |
| `HeartflameForge-Setup-x64-v1.3.0.exe` | Guided installation, Start Menu shortcut, and uninstall entry. The installer can install the Microsoft Visual C++ runtime prerequisite. |
| `HeartflameForge-Portable-x64-v1.3.0.zip` | Extract to a folder you control and run without an application install step. Keep the entire folder together. |
| `SHA256SUMS.txt` | Integrity checks for files from the same release. |

### Install and start (Windows)

**Installer (recommended)**

1. Download the installer and `SHA256SUMS.txt` from the same trusted release page.
2. Verify the installer hash against `SHA256SUMS.txt` before running it.
3. Run `HeartflameForge-Setup-x64-v1.3.0.exe`. Windows may ask for permission because the installer writes to Program Files and may install the Microsoft runtime.
4. Start **Heartflame Forge** from the Start Menu. Uninstall it through **Settings → Apps → Installed apps**.

**Portable ZIP**

1. Verify the ZIP hash using `SHA256SUMS.txt`.
2. Extract the complete ZIP to a new folder, for example `%LOCALAPPDATA%\Programs\Heartflame Forge Portable`.
3. Keep `HeartflameForge.exe`, Qt DLLs, `platforms`, `plugins`, and helper tools in their original relative locations.
4. Double-click `RUN-HEARTFLAME-FORGE.bat` or `HeartflameForge.exe`.
5. If Windows reports a missing `VCRUNTIME` or `MSVCP` DLL, run `INSTALL-VC-RUNTIME.bat`, finish the Microsoft installer, and start the app again.

**Security note:** release binaries are currently not signed with a project publisher certificate. Do not bypass an unexpected Windows security warning. Verify the checksum and only use builds from a source you trust.

## Feature map and current limits

| Workspace | Available now | Still under research |
| --- | --- | --- |
| **Texture Workshop** | Inspect TIM2 metadata; preview supported images; import/export PNG; save replacement to a new file/ZIP for validated profiles. | PSM=0 image layout and other TIM2 variants; swizzle/CLUT order and runtime appearance need further proof. |
| **Archive Browser** | List ZIP entries, filter paths, inspect metadata, and pass supported TIM2 files to Texture Workshop. | Containers other than the ZIP paths currently implemented. |
| **Binary Inspector** | Read-only header/signature report, bounded hex view, and string search. | Full schemas for unknown container formats. |
| **Model Inspector** | Read-only `.mdl`/`SL01` marker and bounded-range profiling. | Mesh topology, vertex/index buffers, UVs, materials, transforms, and skeleton. |
| **Model Export — GLB** | Optional Assimp-based export for formats Assimp can import. | The game's custom `.mdl` geometry is **not** automatically understood by Assimp; a game-specific decoder is still required. |
| **Animation Inspector** | Header/container reconnaissance. | Skeleton tracks, keyframes, timing, interpolation, and animation writer. |
| **Audio Inspector** | WAV/RIFF metadata and SDPK/ADSC signature inspection. | Proprietary audio decoding, playback, loop metadata, and replacement. |
| **Video / Stream Inspector** | PSS-like header/signature inspection. | In-app stream demux, timestamps, playback/editing pipeline. |
| **Text & Script Inspector** | Read-only ASCII and limited UTF-16LE ASCII-subset string scanning; CLI TSV reports. | Game control codes, Shift-JIS, pointer tables, compression, bytecode, and rebuild logic. |
| **Extensions** | Qt dynamic plugin interface and a sample extension. | Third-party plugin trust, compatibility policy, and a signed plugin ecosystem. |

### Known TIM2 profile limitation

The sample corpus audited during development contains 61 TIM2 files. Sixty match the currently validated replacement profile; `tv_mahou_016.tm2` reports PSM=0 and is intentionally excluded from pixel decoding/replacement until its layout is demonstrated. This is a result for that sample set, not a statement about every TIM2 file or every game.

## Build from source on Windows

### Requirements

- Windows 10/11 x64.
- Visual Studio 2022 with **Desktop development with C++** and Windows SDK.
- CMake 3.21+, Git, and PowerShell.
- Qt 6 Widgets for MSVC 2022 x64.
- vcpkg dependencies: libarchive, libpng, and Assimp.

The reproducible Windows build uses the custom `x64-windows-static-md` triplet in `triplets/`. This triplet builds third-party libraries statically while using the dynamic MSVC runtime expected by the Qt MSVC binaries.

Open **Developer PowerShell for VS 2022** at the repository root and set the real locations of Qt and vcpkg:

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"
$env:QT_ROOT_DIR = "C:\Qt\6.8.3\msvc2022_64"

& "$env:VCPKG_ROOT\vcpkg.exe" install `
  --triplet x64-windows-static-md `
  "--overlay-triplets=$PWD\triplets"

cmake -S . -B build-win `
  -G "Visual Studio 17 2022" -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md `
  "-DVCPKG_OVERLAY_TRIPLETS=$PWD\triplets" `
  "-DVCPKG_INSTALLED_DIR=$PWD\vcpkg_installed" `
  "-DCMAKE_PREFIX_PATH=$env:QT_ROOT_DIR" `
  -DPS2ASSET_BUILD_GUI=ON `
  -DPS2ASSET_REQUIRE_GUI=ON `
  -DPS2ASSET_BUILD_SAMPLE_PLUGIN=ON

cmake --build build-win --config Release --parallel 2
$env:QT_QPA_PLATFORM = "offscreen"
ctest --test-dir build-win -C Release --output-on-failure
& .\build-win\Release\HeartflameForge.exe --smoke-test
```

The GUI smoke test must exit with code `0`. It checks startup/layout, navigation/page count consistency, the read-only model report, the initially disabled ZIP-save action, and discovery of the sample extension.

Launch the desktop app interactively with:

```powershell
& .\build-win\Release\HeartflameForge.exe
```

The official Windows workflow additionally runs `windeployqt`, includes dependency notices, verifies the Microsoft-signed VC++ runtime download, and produces the portable ZIP, Inno Setup installer, and SHA-256 checksums. See [`docs/WINDOWS_INSTALL_AND_BUILD.md`](docs/WINDOWS_INSTALL_AND_BUILD.md) for troubleshooting and local packaging details.

## Build and test on Linux

A headless core/CLI build does not prove the GUI compiles. Install Qt Widgets development headers and set `PS2ASSET_REQUIRE_GUI=ON` when validating a GUI build.

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DPS2ASSET_BUILD_GUI=OFF
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

For the full GUI smoke test on a Qt-enabled Linux machine:

```bash
cmake -S . -B build-gui \
  -DCMAKE_BUILD_TYPE=Release \
  -DPS2ASSET_BUILD_GUI=ON \
  -DPS2ASSET_REQUIRE_GUI=ON
cmake --build build-gui --parallel 2
QT_QPA_PLATFORM=offscreen ctest --test-dir build-gui --output-on-failure
QT_QPA_PLATFORM=offscreen ./build-gui/HeartflameForge --smoke-test
```

## Privacy, modder safety, and safe file handling

- **Local-first:** the current application does not implement accounts, cloud sync, telemetry, or automatic update checks. Asset inspection and replacement are performed locally. Build/release workflows do download development dependencies on the build runner.
- **Keep originals:** use a copy of your own files, back up the source archive, and write edits to new output paths. Never rely on an unsupported profile being safe to rewrite.
- **Minimize public traces:** do not post original game asset dumps, save data, private test archives, usernames, absolute home paths, local drive labels, or unredacted logs/screenshots in issues or discussions.
- **Native plugins are code:** a plugin DLL can execute arbitrary code in your user account. Use only extensions you have reviewed and trust; the sample extension is a development example, not a sandbox.
- **Verify releases:** compare SHA-256 with the checksum file from the same trusted release. A checksum verifies file integrity, not the publisher's identity.
- **Private notes stay private:** put personal test notes in a local `private/` directory. That directory is git-ignored and intentionally excluded from release packaging. Copy the [blank modder privacy checklist](docs/MODDER_PRIVATE_CHECKLIST_TEMPLATE.md) into `private/`; fill in only the private copy. Never put credentials, game dumps, or personal data into public Markdown.
- **Respect ownership and rules:** use only files you are allowed to inspect/mod, retain your original backups, and check the applicable game/platform terms before distributing modified assets.

Read [`docs/PRIVACY_AND_MODDER_SAFETY.md`](docs/PRIVACY_AND_MODDER_SAFETY.md) and [`SECURITY.md`](SECURITY.md) before sharing a reproduction or third-party extension.

## Project layout

```text
assets/                       Brand SVG/PNG/ICO and Qt resource collection
src/gui/                      Qt desktop shell and modular workspaces
src/gui/modules/              One source module per asset feature
src/audio, src/model,         Format-specific core logic
src/text, src/texture,
src/archive, src/image/
include/ps2asset/             Public C++ core/plugin headers
plugins/                      Extension developer notes
packaging/windows/            Windows portable scripts and Inno Setup source
triplets/                     Reproducible vcpkg triplet
cmake/                        CMake config, exported package and Windows resource
.github/workflows/            Linux and Windows CI/release pipelines
docs/                         Architecture, format research, and user guides
tests/                        Synthetic fixtures and regression tests
```

## Documentation

- [Windows install/build guide](docs/WINDOWS_INSTALL_AND_BUILD.md)
- [Windows release build status and verification limits](docs/WINDOWS_RELEASE_STATUS_v1.3.0.md)
- [Privacy and modder safety](docs/PRIVACY_AND_MODDER_SAFETY.md)
- [Blank private modder checklist template](docs/MODDER_PRIVATE_CHECKLIST_TEMPLATE.md)
- [Security reporting policy](SECURITY.md)
- [Module architecture](docs/MODULE_ARCHITECTURE_v1.1.0.md)
- [Format research backlog](docs/FORMAT_RESEARCH_BACKLOG_v1.1.0.md)
- [Extension API](docs/EXTENSION_API.md)
- [Third-party GUI/library review](docs/THIRD_PARTY_GUI_AND_LIBRARIES.md)
- [Third-party notices and licensing](THIRD_PARTY_NOTICES.md)

## Contributing

Keep changes scoped by module, add a regression test for every parser or file-writing bug, and document unverified format assumptions. Do not commit proprietary game assets, private notes, personal identifiers, credentials, build output, or generated release binaries. For a pull request, include the build configuration and test result without attaching files you do not have permission to redistribute.

## License

The original Heartflame Forge project source is distributed under the MIT License; see [`LICENSE`](LICENSE). Third-party libraries and the bundled Microsoft runtime remain subject to their own license terms and notices in [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) and `THIRD_PARTY_LICENSES/` in a packaged Windows release.
