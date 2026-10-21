# PS2 Asset Workbench

**Versi 0.2.0 — fondasi GUI C++17/CMake dan modul tekstur TIM2**

PS2 Asset Workbench adalah proyek modular untuk inspeksi dan modding aset game PS2. Pekerjaan pertama difokuskan pada tekstur TIM2 di arsip yang disediakan pengguna. Proyek memisahkan parser format dari GUI agar modul model, audio, script, dan format proprietary dapat ditambahkan tanpa mengikatnya ke antarmuka.

## Fitur modul tekstur saat ini

- Buka file `.tm2` secara langsung.
- Jelajahi semua entri `.tm2` dalam `.zip` dengan libarchive (jika tersedia saat build).
- Tampilkan daftar gambar beserta dimensi, tipe, ukuran data, CLUT/palette, PSM, dan status dukungan.
- Pratinjau tekstur dengan alpha di atas latar checkerboard.
- Ekspor tekstur yang didukung menjadi PNG.
- Import gambar PNG/BMP/JPEG/TIFF, dengan konfirmasi resize bila dimensi berbeda.
- Ganti indeks pixel dan palette dalam memori, lalu simpan seluruh file TIM2 menjadi file `.tm2` baru secara atomik menggunakan `QSaveFile`.
- CLI untuk inspeksi header dan diagnostik TIM2; unit test untuk parser/decode/replace.

Aplikasi **tidak menimpa berkas sumber secara otomatis**. Bila tekstur dibuka dari ZIP, hasil yang disimpan adalah file `.tm2` mandiri; proyek versi ini belum membangun ulang arsip ZIP asli.

## Profil format yang sudah diuji

Pada arsip sampel yang diaudit, ada 61 file `.tm2`. Seluruh 61 file berhasil diparse, didekode, dan memiliki profil yang mendukung penggantian:

- TIM2 version: `4`, format type: `0`
- Resolusi: `128 × 64`
- Image type: indexed 8-bit (`image_type = 5`, PSMT8)
- CLUT: 256 warna RGBA32 (`clut_type = 3`, 1024 byte)
- Header gambar: 48 byte
- Data indeks gambar: 8192 byte
- Satu level mipmap

Hasil ini berlaku untuk 61 tekstur dalam arsip sampel yang diberikan, bukan klaim bahwa semua file TIM2 atau semua game PS2 memakai profil tersebut. Import saat ini sengaja menolak profil yang tidak sesuai daripada menulis data menggunakan asumsi yang belum terbukti.

## Dependensi

- CMake 3.16 atau lebih baru
- Compiler C++17 (GCC, Clang, atau MSVC)
- Qt 6 Widgets atau Qt 5.12+ Widgets untuk aplikasi desktop
- libarchive development package untuk penjelajahan ZIP (opsional, tetapi diperlukan jika ingin membuka `.zip` dari GUI)

Jika Qt development files tidak terpasang, konfigurasi CMake masih dapat membangun core, CLI, dan test; target GUI akan dilewati dan CMake menampilkan peringatan. Instal paket pengembangan Qt, lalu konfigurasi ulang.

### Build di Linux/macOS

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Untuk hanya membangun core/CLI/test di lingkungan tanpa Qt:

```sh
cmake -S . -B build -DPS2ASSET_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Jika Qt tersedia, executable GUI bernama `ps2asset-workbench` akan dibuat di folder build. Pada Windows, target dapat dibangun melalui CMake + Visual Studio generator dan Qt 5/6 yang kompatibel dengan compiler yang dipilih.

### CLI

```sh
# Laporan header umum
./build/ps2asset-inspect path/to/file.tm2

# Metadata TIM2 dan dukungan import/replace
./build/ps2asset-texture info path/to/texture.tm2

# Ekspor diagnostik ke PPM (GUI mengekspor PNG)
./build/ps2asset-texture export path/to/texture.tm2 0 output.ppm
```

## Struktur proyek

```text
include/ps2asset/       API inti (header inspection, TIM2, archive)
src/inspect.cpp         inspeksi signature/header
src/texture/tim2.cpp    parser, decoder, palette quantizer, replacement
src/archive/            pembacaan `.tm2` dari ZIP via libarchive
src/gui/                aplikasi desktop Qt Widgets
src/main.cpp            CLI inspeksi
src/texture_cli.cpp     CLI tekstur
 tests/                 unit test tanpa aset komersial
 docs/                  catatan arsitektur dan batas format
 tools/                 skrip audit archive
```

## Batas dan pekerjaan berikutnya

1. Validasi lewat game/emulator belum dilakukan; output hasil edit tidak boleh dianggap kompatibel sebelum diuji.
2. Import dibatasi pada TIM2 indexed 8-bit dengan CLUT RGBA32 256 warna, satu mip level, header 48 byte, dan dimensi yang sama setelah resize di GUI.
3. Dukungan CLUT/texture swizzle di luar sampel belum diimplementasikan.
4. GUI tidak menulis ulang ZIP; hasil edit dari arsip perlu disimpan sebagai `.tm2` mandiri.
5. Format `SL01`, model 3D, audio, script/event, dan format lain tetap modul kerja berikutnya.

## Pengujian yang dilakukan pada sesi ini

- Build core, CLI, dan test dengan CMake/GCC berhasil.
- CTest: **2/2 tes lulus** (`header-inspection`, `tim2-decode-and-replace`).
- Integrasi terhadap arsip sampel: **61 entri TIM2, 61 gambar, 61 mendukung replace, 61 berhasil decode, 0 gagal decode**.
- GUI Qt belum dapat dikompilasi di lingkungan pembuatan ini karena hanya runtime Qt terpasang; header/development package Qt tidak tersedia dan pembaruan package repository timed out. Source GUI sudah disertakan, tetapi perlu diverifikasi build pada lingkungan yang memasang Qt Widgets.
