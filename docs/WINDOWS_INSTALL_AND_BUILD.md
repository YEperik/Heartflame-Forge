# Windows installation and build

This is the Windows-first guide for **Heartflame Forge 1.3.0**. The app is built as a 64-bit Qt Widgets desktop application using MSVC 2022 and Qt 6. The C++ core and command-line tools remain separate from the GUI.

## Choose an installation method

### Option A — Setup installer (recommended)

After the Windows workflow has produced a release, download `HeartflameForge-Setup-x64-v1.3.0.exe` from the project's Releases page. Verify the SHA-256 value against `SHA256SUMS.txt` from that same release before installing.

1. Run the installer from the release package. The installer uses Windows UAC because it installs under Program Files and may install Microsoft's Visual C++ runtime prerequisite.
2. Follow the prompts, then start **Heartflame Forge** from the Start Menu.
3. Uninstall from **Settings → Apps → Installed apps**.

The current build is not code-signed by a project publisher. Windows may show an unknown-publisher notice; do not bypass an unexpected security warning. Only obtain release files from a repository you trust.

### Option B — Portable ZIP

1. Download `HeartflameForge-Portable-x64-v1.3.0.zip` from the same release.
2. Verify its SHA-256 using `SHA256SUMS.txt`.
3. Extract the entire archive to a new directory, such as `%LOCALAPPDATA%\Programs\Heartflame Forge Portable`.
4. Keep the executable, `platforms`, Qt libraries, `plugins`, and helper tools together.
5. Run `RUN-HEARTFLAME-FORGE.bat` or double-click `HeartflameForge.exe`.
6. If Windows reports a missing `VCRUNTIME` or `MSVCP` DLL, run `INSTALL-VC-RUNTIME.bat` and launch the app again.

Do not copy only the EXE to another folder. Qt's Windows platform plugin (`platforms\qwindows.dll`) and any trusted extension must remain in their expected subdirectories.

## Build from source in PowerShell

### Requirements

- Windows 10/11 x64.
- Visual Studio 2022 with **Desktop development with C++** and Windows SDK.
- CMake 3.21 or newer, Git, and PowerShell.
- Qt 6 Widgets for `win64_msvc2022_64`. The workflow uses Qt 6.8.3 from `install-qt-action`.
- vcpkg for `libarchive`, `libpng`, and `assimp`.

Install Qt through the official Qt installer or let the project's GitHub Actions workflow install the pinned build used for releases. For vcpkg, set `$env:VCPKG_ROOT` to its checkout (the Visual Studio image often provides `VCPKG_INSTALLATION_ROOT`).

```powershell
# Run from the repository root, in a Developer PowerShell for VS 2022.
$env:VCPKG_ROOT = "C:\vcpkg"  # change this to your actual vcpkg path
$env:QT_ROOT_DIR = "C:\Qt\6.8.3\msvc2022_64" # change to your Qt MSVC x64 installation

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
  -DPS2ASSET_BUILD_SAMPLE_PLUGIN=ON `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-win --config Release --parallel 2
$env:QT_QPA_PLATFORM = "offscreen"
ctest --test-dir build-win -C Release --output-on-failure
& .\build-win\Release\HeartflameForge.exe --smoke-test
```

The smoke test must exit with code `0`. It checks that the window can be shown offscreen, navigation and page-stack counts agree, the model inspection panel exists, the save-to-new-ZIP action is initially disabled, and the sample extension loads.

To launch the interactive desktop application from a Developer PowerShell:

```powershell
& .\build-win\Release\HeartflameForge.exe
```

## Produce a local portable package

After a successful Release build:

```powershell
$stage = Join-Path $PWD "dist\HeartflameForge"
cmake --install build-win --config Release --prefix $stage
& "$env:QT_ROOT_DIR\bin\windeployqt.exe" --release --no-compiler-runtime --dir $stage "$stage\HeartflameForge.exe"
```

The official Windows workflow completes the remaining steps: it validates the Microsoft runtime download signature, includes dependency notices, builds the portable ZIP, produces the Inno Setup installer, and emits SHA-256 checksums.

## Troubleshooting

- **Qt not found:** verify `$env:QT_ROOT_DIR` points to the MSVC 2022 x64 Qt root and includes `lib\cmake\Qt6`.
- **`vcpkg` package not found:** confirm `vcpkg install` succeeded and the custom triplet path is supplied to both vcpkg and CMake.
- **Platform plugin error:** do not remove `platforms\qwindows.dll`; re-extract the full package instead of copying individual files.
- **GLB action unavailable:** Assimp is optional. When included, the helper executable must be beside `HeartflameForge.exe`; `.mdl` from this game still needs a dedicated geometry decoder.
- **Unsupported texture profile:** retain the original file and export edits to a new file. An unsupported status means the tool has intentionally not guessed the pixel layout.

See [the public README](../README.md), [privacy and modder safety](PRIVACY_AND_MODDER_SAFETY.md), and [security policy](../SECURITY.md).
