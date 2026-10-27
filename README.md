# Heartflame-Forge 

**Version 0.8.0 — batch replacement for multiple TIM2 textures into a single output ZIP**

Heartflame-Forge is a C++17/CMake application designed for the inspection and modding of PlayStation 2 assets. The core parser is intentionally independent of Qt, allowing it to be utilized by desktop UIs, CLIs, automated tests, or other tools.

## Changes in v0.8.0

- The `replace_zip_entries_bytes()` API replaces multiple regular entries within a single ZIP repacking process.
- The `replace-zip-entries` CLI reads a TSV manifest (`entry-path<TAB>file-tm2`) and validates all inputs as TIM2 files before generating the output.
- Texture Workshop bundles all edited TIM2 documents—originating from the same source archive—into a single output ZIP file.
- Duplicate target paths are rejected; each target must appear exactly once and must be a regular file. Default payload limits are 32 MiB per entry and 256 MiB in total.
- If any entry is missing or an operation fails, the destination ZIP file is not left in a partial or corrupted state. The source archive remains untouched.

## Changes in v0.7.0
- Added an initial GUI workflow, **Save changes to a new ZIP…**, to replace a single TIM2 entry within an output ZIP file.
- Any document opened from a ZIP file retains the path to its source archive and its original entry name.
- TIM2 files are re-validated prior to writing; the original output is discarded, and the target entry is verified after repacking. The source ZIP file is not overwritten.
- As of v0.8.0, this initial functionality has been expanded to support batch replacement, allowing all textures edited from the same session archive to be saved into a single output ZIP file.

## Changes in v0.5.0

- Added an optional `ps2asset_png` library, based on libpng, for encoding/decoding RGBA8 PNG files (with a 64-megapixel limit and a 512 MiB file size limit).
- The `ps2asset-texture export-png` CLI command exports textures as transparent PNGs; `replace-png` imports a PNG and saves the resulting TIM2 to a new file. The replacement command prevents overwriting existing destination files.
- `ps2asset-texture-validate --png-roundtrip <input.tm2|archive.zip>` performs a full round-trip check: decode → PNG encode/decode → replace → re-parse → decode (all in memory).
- PNG integration testing on 61 original textures yielded a 61/61 success rate; the original input files remained unchanged.
- Added tests for PNG encoding/decoding, handling of malformed PNGs, and TIM2 replacement using PNG-derived data.

## Changes in v0.4.0

- Added `ps2asset-texture-validate`, a read-only validation tool for individual TIM2 files or ZIP archives containing `.tm2` files.
- The validator replaces textures using synthetic patterns in memory, ensuring the file size remains constant, verifying that bytes outside the target payload are unaltered, and subsequently re-parsing and decoding the result. The input files are not modified. - Added a self-test validator to CTest so that the pipeline replacement can be automatically tested without requiring game assets to be distributed as fixtures.
- Using the original samples available during development, the validator passed for 61 out of 61 textures. These results confirm the integrity of the round-trip process within the parser’s representation, rather than the compatibility of the game or emulator runtime.

## Workspace GUI

- **Overview** — status of currently opened assets, module shortcuts, and notes on supported formats.
- **Texture Workshop** — open individual `.tm2` files or `.zip` archives; features include an image list, transparency preview, metadata display, PNG export, importing/replacing from standard image files, saving edited `.tm2` files, and a **"Save all modified textures to a new ZIP..."** action to package all modified assets from the original archive.
- **Archive Browser** — a read-only index of ZIP entry metadata, including a table of paths, types, and sizes; supports path filtering and allows passing `.tm2` entries to the Texture Workshop. Edits made to documents originating from a ZIP can be packaged into a new output ZIP via the Texture menu.
- **Binary Inspector** — provides read-only inspection of signatures/headers, details on recognized formats, a hex dump of the first 8 KiB, and ASCII string searching within loaded files (up to 64 MiB).
- **3D Models & Animation**, **Audio & Video**, and **Scripts & Text** — workspaces within the UI shell reserved for future implementation. These pages are explicitly marked as not yet having active format parsers/decoders.
- **Diagnostics / Activity Log** — displays asset operations, extension loading results, and relevant errors. This panel can be accessed via the View menu.
- **Extension Modules** — dynamic Qt plugins can add new workspace pages without modifying the main shell. A sample plugin is built by default, provided that Qt Widgets are available.

## Texture Module: Current Limitations

Importing/replacing is permitted only for validated TIM2 profiles: 8-bit indexed, 256-color RGBA32 CLUT, single mip level, 48-byte image header, and a fixed-size buffer. The dimensions of imported images can be adjusted upon confirmation. The palette/index plane is replaced without altering the container size. Edited output **does not automatically overwrite the source file**. For ZIP sources, users can either store standalone `.tm2` files or create a new ZIP archive that replaces all original entries from the TIM2 documents edited during the session. This repackaging preserves entry paths and metadata—to the extent supported by `libarchive`—though the compressed byte streams within the archive will not necessarily be identical to the original source.

An audit of the user-provided sample archive identified 61 `.tm2` files; all 61 were successfully parsed/decoded and met the criteria for replacement. These findings apply only to the specific samples examined, not to all TIM2 files or all games.

## Build prerequisites

- CMake 3.16+
- C++17-compliant compiler (GCC, Clang, or MSVC)
- Qt 6 Widgets or Qt 5.12+ Widgets (for the GUI)
- `libarchive` development package (optional during configuration; required for ZIP functionality in the GUI)
- `libpng` development package (optional; used for CLI PNG commands and the PNG round-trip validator—the GUI utilizes Qt’s built-in codecs instead)

Example packages for Ubuntu/Debian:
```sh
sudo apt-get update
sudo apt-get install --yes build-essential cmake qt6-base-dev libarchive-dev libpng-dev
```

For Windows, install Visual Studio 2022 with the C++ Desktop workload, CMake, Qt 6 Widgets (via the Qt installer or Qt VS Tools), and a version of libarchive compatible with your toolchain. For macOS, install the compiler from Xcode Command Line Tools, along with CMake, Qt 6, and libarchive via a package manager.

If the Qt development files are unavailable, the core, CLI, and test targets can still be built, while the GUI target will be skipped (with a corresponding message from CMake). To include the GUI and sample plugins, the Qt development package is required.

### Linux/macOS/Windows via CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Headless build (without GUI):

```sh
cmake -S . -B build-headless -DPS2ASSET_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless --config Release
ctest --test-dir build-headless --output-on-failure
```

CMake Presets (CMake 3.21+):

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release
```

If Qt Widgets are available, the `ps2asset-workbench` desktop executable will be created. When sample plugins are enabled (the default setting), the extension libraries are placed in `build/plugins/`, alongside the executable.

### GUI startup smoke test

In an environment with a full Qt installation:

```sh
# Linux without a display server
QT_QPA_PLATFORM=offscreen ./build/ps2asset-workbench --smoke-test
```

The smoke test verifies the consistency of the navigation/page stack and ensures that sample plugins load correctly (if that option is enabled). ### CLI

```sh
# General header report
./build/ps2asset-inspect path/to/file.tm2

# TIM2 metadata and import/replace support
./build/ps2asset-texture info path/to/texture.tm2

# Export diagnostics to PPM or transparent PNG
./build/ps2asset-texture export path/to/texture.tm2 0 output.ppm
./build/ps2asset-texture export-png path/to/texture.tm2 0 output.png

# Replace a texture from a PNG and write a new TIM2 file (the destination file must not already exist)
./build/ps2asset-texture replace-png path/to/texture.tm2 0 replacement.png replaced.tm2
# Round-trip synthetic and PNG validators; the entire test runs in memory
./build/ps2asset-texture-validate path/to/texture.tm2
./build/ps2asset-texture-validate path/to/assets.zip
./build/ps2asset-texture-validate --png-roundtrip path/to/texture.tm2
./build/ps2asset-texture-validate --png-roundtrip path/to/assets.zip
./build/ps2asset-texture-validate --self-test
```

## Project structure

```text
include/ps2asset/ API core (inspection, archive, TIM2)
include/ps2asset/workbench/ public Qt plugin interface v1.0
src/inspect.cpp signature/header inspection
src/texture/tim2.cpp TIM2 parse/decode/replace
src/image/png.cpp optional libpng RGBA8 codec
src/archive/zip_reader.cpp ZIP entry listing and TIM2 extraction via libarchive
src/gui/main_window.* desktop shell and workspaces
src/gui/plugin_manager.* dynamic module discovery/lifetime
src/gui/plugins/sample_plugin.cpp sample plugin for CI/smoke test
src/main.cpp CLI header inspection
src/texture_cli.cpp CLI texture diagnostics
tools/validate_textures.cpp end-to-end TIM2 replacement validator
plugins/README.md extension authoring notes
tests/ core and ZIP metadata unit tests
.github/workflows/ci.yml Qt GUI build, plugin build, test, smoke test
```

## API Extensions

Public interface: `include/ps2asset/workbench/plugin_interface.hpp`.

- IID: `org.ps2asset.workbench.IWorkbenchPlugin/1.0`
- Implement `id()`, `displayName()`, `description()`, and `createWidget(QWidget*)`.
- Export the implementation with Qt's `Q_PLUGIN_METADATA` and `Q_INTERFACES`.
- Drop compatible plugin library into `plugins/` beside the executable or the Qt application-data `plugins/` folder.
- Each plugin ID must be unique. Invalid or incompatible extensions are skipped and logged; they must not stop application startup.

Plugin code is native code and should be installed only from trusted sources. Public API compatibility is versioned through the IID; incompatible interface changes require a new IID.

## Safety and known limitations

1. Compatibility of generated TIM2 output in the game or emulator has not yet been confirmed.
2. Import rejects TIM2 profiles outside the current indexed-8 + RGBA32 CLUT profile rather than guessing.
3. TIM2 CLUT/texture swizzle variations outside the tested sample are not implemented.
4. GUI ZIP replacement is wired into the source but needs compile/runtime verification on a host with Qt development headers.
5. GUI/plugin compilation and PNG interaction tests remain pending on a host with Qt development headers.
6. Model, audio, video, and script pages are UI scaffolding only; no full proprietary decoder/writer is claimed.
7. Binary Inspector is read-only and limits a loaded file to 64 MiB; its hex preview shows 8 KiB, while ASCII search scans the loaded bytes.

## Tests and CI

Headless tests cover header/signature inspection, TIM2 decode/replacement, ZIP listing, and a synthetic end-to-end replacement → reparse → decode round-trip. The `ps2asset-texture-validate` utility was also run against the supplied sample ZIP during development: 61/61 TIM2 files passed the in-memory replacement + reparse/decode validation, with no failures and no input files written. With Qt installed, CI also builds the GUI and sample plugin and runs `--smoke-test` in Qt offscreen mode. The current local environment lacks Qt development headers, so GUI/plugin build and GUI dialogs remain pending Qt-enabled CI/desktop validation. The independent libpng encoder/decoder, CLI PNG export/replacement, and PNG replacement/reparse round-trip passed locally (61/61 real sample textures).

## ZIP replacement (v0.6.0+)

With libarchive enabled, replace one entry or batch-replace several entries while keeping the source archive untouched. The output ZIP must not already exist.

```bash
# One entry
ps2asset-texture replace-zip-entry input.zip path/inside/archive.tm2 replacement.tm2 output.zip

# Multiple entries from a tab-separated manifest
ps2asset-texture replace-zip-entries input.zip replacements.tsv output.zip
```

Each `replacements.tsv` row contains the exact ZIP entry path, one literal tab character, then a TIM2 replacement file path. Relative replacement paths are resolved against the manifest directory. Blank lines and comment lines starting with `#` are ignored. For example, the two columns below must be separated by an actual tab:

```text
dataimg/effect/a.tm2<TAB>edited_a.tm2
dataimg/effect/b.tm2<TAB>edited_b.tm2
```

Each replacement is parsed as a TIM2 file before the ZIP writer executes. Default limits are 32 MiB per entry and 256 MiB for the total size of all replacements. Target paths must be unique, and each target must correspond to exactly one regular file within the source archive. Unmodified entries are processed via `libarchive`; metadata is preserved wherever the library exposes it, though the ZIP-compressed byte stream is rebuilt and will not necessarily be byte-for-byte identical to the original. Always maintain a backup and test the result within the target game or emulator.

Version 0.8.0 introduces a GUI action, **Save all modified textures to a new ZIP…**, within the Texture Workshop. All edited TIM2 files originating from the same source archive are validated and packaged into a single output ZIP file. Existing target paths are rejected, and all target entries undergo verification. The build process and GUI dialogs currently require validation on a host system equipped with Qt development headers.
