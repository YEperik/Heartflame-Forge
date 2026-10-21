# PS2 Asset Workbench

**Versi 0.4.0 — validasi round-trip tekstur nyata, desktop GUI modular, Texture Workshop, dan extension API**

PS2 Asset Workbench adalah aplikasi C++17/CMake untuk inspeksi dan modding aset PlayStation 2. Core parser sengaja tidak bergantung pada Qt, sehingga dapat digunakan oleh desktop UI, CLI, tes otomatis, atau tool lain.

## Perubahan v0.4.0

- Menambahkan `ps2asset-texture-validate`, alat validasi read-only untuk TIM2 tunggal atau ZIP yang berisi file `.tm2`.
- Validator mengganti tekstur menggunakan pola sintetis di memori, memastikan ukuran file tetap, memverifikasi byte di luar payload target tidak berubah, lalu mem-parse dan mendekode hasil kembali. Input file tidak ditulis.
- Menambahkan validator self-test ke CTest agar pipeline replacement diuji otomatis tanpa mendistribusikan aset game sebagai fixture.
- Pada sampel asli yang tersedia saat pengembangan, validator lulus untuk 61 dari 61 tekstur. Hasil ini membuktikan round-trip pada representasi yang ditangani parser, bukan kompatibilitas runtime game/emulator.

## Workspace GUI

- **Ikhtisar** — status aset yang sedang dibuka, pintasan modul, dan catatan dukungan format.
- **Texture Workshop** — buka `.tm2` tunggal atau `.zip`; daftar gambar, pratinjau transparansi, metadata, ekspor PNG, impor/ganti dari gambar umum, dan penyimpanan `.tm2` hasil edit sebagai file baru.
- **Archive Browser** — indeks metadata entri ZIP secara read-only, tabel path/tipe/ukuran, filter path, dan alur meneruskan entri `.tm2` ke Texture Workshop.
- **Binary Inspector** — inspeksi signature/header read-only, detail format yang sudah dikenali, hex dump hingga 8 KiB awal, dan pencarian string ASCII pada file yang dimuat hingga 64 MiB.
- **3D Model & Animasi**, **Audio & Video**, serta **Script & Teks** — workspace di shell UI untuk tahapan selanjutnya. Halaman tersebut secara eksplisit ditandai belum mengaktifkan parser/decoder format.
- **Diagnostics / Activity Log** — menampilkan operasi asset, hasil pemuatan extension, dan kesalahan yang relevan. Panel dapat dibuka dari menu Tampilan.
- **Extension modules** — plugin Qt dinamis dapat menambahkan halaman workspace tanpa mengubah shell utama. Sample plugin dibangun secara default ketika Qt Widgets tersedia.

## Modul tekstur: batas kemampuan saat ini

Import/replacement hanya diizinkan pada profil TIM2 yang sudah divalidasi: indexed 8-bit, CLUT RGBA32 256 warna, satu mip level, header gambar 48 byte, dan buffer berukuran tetap. Dimensi gambar yang diimpor dapat disesuaikan setelah konfirmasi. Palette/index plane diganti tanpa merombak ukuran container. Output hasil edit **tidak menimpa sumber secara otomatis**. Bila sumber berasal dari ZIP, hasil disimpan sebagai `.tm2` mandiri; penulisan ulang ZIP belum diimplementasikan.

Untuk arsip sampel pengguna, hasil audit sebelumnya menemukan 61 file `.tm2`; seluruh 61 file itu berhasil diparse/didekode dan sesuai profil replacement. Temuan ini hanya berlaku untuk sampel yang diperiksa, bukan semua TIM2 atau semua game.

## Build prerequisites

- CMake 3.16+
- Compiler C++17 (GCC, Clang, atau MSVC)
- Qt 6 Widgets atau Qt 5.12+ Widgets untuk GUI
- `libarchive` development package untuk dukungan ZIP (opsional saat konfigurasi, diperlukan untuk ZIP pada GUI)

Contoh paket untuk Ubuntu/Debian:

```sh
sudo apt-get update
sudo apt-get install --yes build-essential cmake qt6-base-dev libarchive-dev
```

Untuk Windows, pasang Visual Studio 2022 dengan workload C++ Desktop, CMake, Qt 6 Widgets (installer Qt atau Qt VS Tools), dan libarchive yang kompatibel dengan toolchain. Untuk macOS, pasang compiler dari Xcode Command Line Tools, CMake, Qt 6, dan libarchive melalui package manager.

Jika Qt development files tidak tersedia, target core/CLI/test tetap dapat dibangun, sedangkan target GUI akan dilewati dengan pesan CMake. Untuk mendapatkan GUI + sample plugin, Qt development package diperlukan.

### Linux/macOS/Windows via CMake

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Headless build (tanpa GUI):

```sh
cmake -S . -B build-headless -DPS2ASSET_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-headless --config Release
ctest --test-dir build-headless --output-on-failure
```

Preset CMake (CMake 3.21+):

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Jika Qt Widgets tersedia, executable desktop `ps2asset-workbench` dibuat. Saat sample plugin diaktifkan (default), library extension ditempatkan ke `build/plugins/` di samping executable.

### GUI startup smoke test

Di lingkungan dengan Qt lengkap:

```sh
# Linux tanpa display server
QT_QPA_PLATFORM=offscreen ./build/ps2asset-workbench --smoke-test
```

Smoke test memverifikasi konsistensi navigasi/stack halaman dan keberhasilan memuat sample plugin bila opsi tersebut aktif.

### CLI

```sh
# Laporan header umum
./build/ps2asset-inspect path/to/file.tm2

# Metadata TIM2 dan dukungan import/replace
./build/ps2asset-texture info path/to/texture.tm2

# Ekspor diagnostik ke PPM (GUI mengekspor PNG)
./build/ps2asset-texture export path/to/texture.tm2 0 output.ppm

# Validasi alur ganti → parse ulang → decode ulang tanpa menulis file
./build/ps2asset-texture-validate path/to/texture.tm2
./build/ps2asset-texture-validate path/to/assets.zip

# Self-test sintetis (juga didaftarkan ke CTest)
./build/ps2asset-texture-validate --self-test
```

## Struktur proyek

```text
include/ps2asset/                    API core (inspection, archive, TIM2)
include/ps2asset/workbench/          public Qt plugin interface v1.0
src/inspect.cpp                      signature/header inspection
src/texture/tim2.cpp                TIM2 parse/decode/replace
src/archive/zip_reader.cpp           ZIP entry listing and TIM2 extraction via libarchive
src/gui/main_window.*                desktop shell and workspaces
src/gui/plugin_manager.*             dynamic module discovery/lifetime
src/gui/plugins/sample_plugin.cpp     sample plugin for CI/smoke test
src/main.cpp                         CLI header inspection
src/texture_cli.cpp                  CLI texture diagnostics
tools/validate_textures.cpp           end-to-end TIM2 replacement validator
plugins/README.md                    extension authoring notes
tests/                               core and ZIP metadata unit tests
.github/workflows/ci.yml             Qt GUI build, plugin build, test, smoke test
```

## Extension API

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
4. GUI does not rewrite ZIP archives.
5. GUI/plugin compilation and PNG interaction tests remain pending on a host with Qt development headers.
6. Model, audio, video, and script pages are UI scaffolding only; no full proprietary decoder/writer is claimed.
7. Binary Inspector is read-only and limits a loaded file to 64 MiB; its hex preview shows 8 KiB, while ASCII search scans the loaded bytes.

## Tests and CI

Headless tests cover header/signature inspection, TIM2 decode/replacement, ZIP listing, and a synthetic end-to-end replacement → reparse → decode round-trip. The `ps2asset-texture-validate` utility was also run against the supplied sample ZIP during development: 61/61 TIM2 files passed the in-memory replacement + reparse/decode validation, with no failures and no input files written. With Qt installed, CI also builds the GUI and sample plugin and runs `--smoke-test` in Qt offscreen mode. The current local environment lacks Qt development headers and cannot resolve `deb.debian.org`, so GUI/plugin build and PNG codec interactions remain pending Qt-enabled CI/desktop validation.
