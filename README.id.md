<div align="center">
  <img src="assets/heartflame-forge.svg" alt="Logo Heartflame Forge" width="132" />

  # Heartflame Forge

  **Workbench desktop modular untuk inspeksi aset game dan riset modding.**

  *Periksa dengan hati-hati. Edit secara konservatif. Jaga file asli tetap aman.*

  ![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
  ![Qt 6](https://img.shields.io/badge/GUI-Qt%206-41CD52?logo=qt&logoColor=white)
  ![Windows](https://img.shields.io/badge/Target-Windows%2010%2F11%20x64-0078D4?logo=windows11&logoColor=white)
  ![Lisensi](https://img.shields.io/badge/Lisensi-MIT-2E8B57.svg)

  **Versi 1.3.0 · Fokus Windows · Pemrosesan aset secara lokal**

  [English](README.md) · [Bahasa Indonesia](README.id.md)
</div>

---

## Tentang proyek

**Heartflame Forge** adalah aplikasi C++17 dengan GUI Qt Widgets untuk menelusuri container aset game dan mengembangkan tool yang spesifik terhadap format. Setiap fitur memiliki halaman sendiri, sedangkan parser C++ berada di core terpisah agar dapat dipakai oleh GUI, CLI, dan program lain.

Aplikasi ini masih merupakan **workbench riset**, bukan editor universal untuk semua format game. Mengenali ekstensi atau signature tidak berarti isi file sudah dipahami sepenuhnya atau aman untuk ditulis ulang.

## Unduhan Windows

Source saat ini berisi workflow untuk membangun aplikasi Windows x64, menjalankan pengujian, menyiapkan runtime Qt, membuat ZIP portable dan installer, serta menghasilkan checksum SHA-256.

**Paket source ini belum berisi file `.exe` Windows yang sudah dibangun.** File biner baru tersedia setelah source dimasukkan ke repositori GitHub dan workflow Windows berhasil dijalankan. Buka **Actions → Heartflame Forge — Windows** untuk melihat hasil build. Tag seperti `v1.3.0` dikonfigurasi untuk menerbitkan aset Release jika workflow sukses.

| Paket | Kegunaan |
|---|---|
| `HeartflameForge-Setup-x64-v1.3.0.exe` | Installer terpandu, shortcut Start Menu, dan uninstall. Installer dapat memasang prasyarat Microsoft Visual C++ runtime. |
| `HeartflameForge-Portable-x64-v1.3.0.zip` | Aplikasi portable; ekstrak satu paket lengkap ke folder yang dipilih. |
| `SHA256SUMS.txt` | Hash untuk memeriksa integritas file rilis terkait. |

### Memasang dan menjalankan di Windows

**Pilihan A — Installer (disarankan)**

1. Unduh installer dan `SHA256SUMS.txt` dari halaman rilis tepercaya yang sama.
2. Cocokkan SHA-256 installer dengan baris yang tepat di `SHA256SUMS.txt` sebelum menjalankan file.
3. Jalankan `HeartflameForge-Setup-x64-v1.3.0.exe`. Windows mungkin meminta izin karena installer memasang ke Program Files dan dapat memasang Microsoft runtime.
4. Buka **Heartflame Forge** melalui Start Menu. Untuk menghapusnya, buka **Settings → Apps → Installed apps**.

**Pilihan B — ZIP portable**

1. Unduh ZIP portable dan checksum dari rilis yang sama, lalu verifikasi SHA-256.
2. Ekstrak seluruh ZIP ke folder baru, misalnya `%LOCALAPPDATA%\Programs\Heartflame Forge Portable`.
3. Jangan pindahkan `HeartflameForge.exe` terpisah dari DLL Qt, folder `platforms`, `plugins`, dan helper yang disertakan.
4. Jalankan `RUN-HEARTFLAME-FORGE.bat` atau klik dua kali `HeartflameForge.exe`.
5. Jika Windows melaporkan DLL `VCRUNTIME` atau `MSVCP` hilang, jalankan `INSTALL-VC-RUNTIME.bat`, selesaikan installer resmi Microsoft, lalu buka aplikasi lagi.

Rilis saat ini belum ditandatangani dengan sertifikat publisher proyek. Jangan melewati peringatan keamanan Windows yang tidak diharapkan. Hash memeriksa integritas file, bukan membuktikan identitas penerbit.

## Peta fitur dan batasannya

| Workspace | Kemampuan yang tersedia | Yang masih perlu diteliti |
|---|---|---|
| **Texture Workshop** | Inspeksi metadata TIM2; preview profil gambar yang didukung; impor/ekspor PNG; replacement ke file atau ZIP baru untuk profil tervalidasi. | Layout gambar PSM=0 dan varian TIM2 lainnya; swizzle/urutan CLUT dan tampilan di game. |
| **Archive Browser** | Daftar dan filter entry ZIP; inspeksi metadata; membuka TIM2 yang didukung ke Texture Workshop. | Container selain jalur ZIP yang sudah diimplementasikan. |
| **Binary Inspector** | Inspeksi header/signature read-only, tampilan hex terbatas, dan pencarian string. | Skema penuh untuk format container yang belum diketahui. |
| **Model Inspector** | Profil marker `.mdl`/`SL01` dan rentang data secara read-only. | Topologi mesh, vertex/index buffer, UV, material, transform, dan skeleton. |
| **Model Export — GLB** | Ekspor opsional berbasis Assimp untuk format yang dapat diimpor Assimp. | Geometri `.mdl` khusus game belum otomatis dipahami Assimp; decoder khusus masih dibutuhkan. |
| **Animation Inspector** | Rekognisi awal header/container. | Skeleton track, keyframe, timing, interpolasi, dan writer animasi. |
| **Audio Inspector** | Metadata WAV/RIFF dan inspeksi signature SDPK/ADSC. | Decoder audio proprietary, playback, loop metadata, dan replacement. |
| **Video / Stream Inspector** | Inspeksi header/signature PSS-like. | Demux stream di dalam aplikasi, timestamp, playback, dan editing. |
| **Text & Script Inspector** | Pemindaian ASCII read-only dan subset ASCII dari UTF-16LE; laporan TSV via CLI. | Control code, Shift-JIS, pointer table, kompresi, bytecode, dan rebuild. |
| **Extensions** | API plugin Qt dinamis dan extension contoh. | Kebijakan kepercayaan plugin pihak ketiga, kompatibilitas, dan ekosistem plugin bertanda tangan. |

### Batas profil TIM2 yang diketahui

Corpus sampel yang diaudit selama pengembangan berisi 61 file TIM2. Sebanyak 60 cocok dengan profil replacement yang sudah divalidasi; `tv_mahou_016.tm2` melaporkan PSM=0 dan sengaja dikecualikan dari decode/replacement gambar sampai layout-nya dapat dibuktikan. Ini hasil dari corpus sampel itu, bukan klaim mengenai semua file TIM2 atau setiap game.

## Build dari source di Windows

### Prasyarat

- Windows 10/11 x64.
- Visual Studio 2022 dengan workload **Desktop development with C++** dan Windows SDK.
- CMake 3.21+, Git, dan PowerShell.
- Qt 6 Widgets untuk MSVC 2022 x64.
- vcpkg untuk libarchive, libpng, dan Assimp.

Buka **Developer PowerShell for VS 2022** di root repositori. Sesuaikan lokasi Qt dan vcpkg terlebih dahulu:

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
  -DPS2ASSET_BUILD_SAMPLE_PLUGIN=ON `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-win --config Release --parallel 2
$env:QT_QPA_PLATFORM = "offscreen"
ctest --test-dir build-win -C Release --output-on-failure
& .\build-win\Release\HeartflameForge.exe --smoke-test
```

Smoke test harus keluar dengan kode `0`. Tes memeriksa startup window secara offscreen, konsistensi daftar navigasi/halaman, panel laporan model, status awal tombol simpan ZIP, dan pemuatan extension contoh.

Untuk membuka GUI secara interaktif dari Developer PowerShell:

```powershell
& .\build-win\Release\HeartflameForge.exe
```

Script [`scripts/build_and_smoke_gui.ps1`](scripts/build_and_smoke_gui.ps1) menjalankan configure/build/test/smoke secara berurutan. Tambahkan `-Run` untuk membuka aplikasi setelah smoke test berhasil. Workflow Windows resmi juga menyiapkan runtime dengan `windeployqt`, menyalin pemberitahuan lisensi, memverifikasi signature Microsoft VC++ runtime, serta mengemas ZIP portable dan installer. Lihat [panduan build/install Windows](docs/WINDOWS_INSTALL_AND_BUILD.md).

## Privasi dan keamanan modder

- **Lokal-first:** source saat ini tidak mengimplementasikan login, telemetry, cloud sync, atau pemeriksaan update otomatis. Pemrosesan aset dilakukan pada komputer lokal. CI memang mengunduh dependency build pada runner.
- **Simpan file asli:** gunakan salinan kerja dan simpan hasil edit ke lokasi baru. Jangan memaksakan decoder pada profil yang dilaporkan tidak didukung.
- **Kurangi jejak publik:** jangan unggah dump game lengkap, save data, username Windows, drive/path pribadi, atau log/screenshot yang belum disamarkan.
- **Plugin adalah kode native:** DLL extension tidak di-sandbox dan dapat bertindak dengan izin akun pengguna. Jalankan hanya plugin yang sumber serta asalnya dipercaya.
- **Verifikasi rilis:** cocokkan SHA-256 dengan checksum dari rilis tepercaya yang sama. Checksum sendiri bukan bukti identitas publisher.
- **Catatan privat lokal:** simpan mapping file dan eksperimen pribadi pada folder lokal `private/`. Folder ini diabaikan Git dan tidak disertakan pada paket source publik. Salin [template checklist modder](docs/MODDER_PRIVATE_CHECKLIST_TEMPLATE.md) ke folder `private/`, lalu isi hanya salinan lokal itu. Jangan menaruh path pribadi, kredensial, dump game, atau catatan sensitif pada dokumentasi publik.

Baca [Panduan privasi dan keselamatan modder](docs/PRIVACY_AND_MODDER_SAFETY.md) serta [Kebijakan keamanan](SECURITY.md) sebelum mengirim laporan publik.

## Struktur proyek

- `src/` — parser core, CLI, GUI dan halaman per fitur.
- `include/ps2asset/` — header API C++ core dan interface extension.
- `assets/` — logo/icon aplikasi.
- `packaging/windows/` — installer Inno Setup dan launcher portable.
- `.github/workflows/` — build/test CI dan packaging Windows.
- `tests/` — unit test, fixture sintetis, dan tes integrasi.
- `docs/` — arsitektur, format backlog, build/run, privasi, dan laporan historis.
- `private/` — lokal saja; tidak dipublikasikan.

## Dokumentasi

- [Panduan instalasi/build Windows](docs/WINDOWS_INSTALL_AND_BUILD.md)
- [Status build rilis Windows dan batas verifikasi](docs/WINDOWS_RELEASE_STATUS_v1.3.0.md)
- [Arsitektur modul](docs/MODULE_ARCHITECTURE_v1.1.0.md)
- [Backlog riset format](docs/FORMAT_RESEARCH_BACKLOG_v1.1.0.md)
- [Extension API](docs/EXTENSION_API.md)
- [Evaluasi library pihak ketiga](docs/THIRD_PARTY_GUI_AND_LIBRARIES.md)
- [Pemberitahuan lisensi pihak ketiga](THIRD_PARTY_NOTICES.md)
- [Changelog](CHANGELOG.md)

## Lisensi

Source asli Heartflame Forge didistribusikan di bawah lisensi MIT; lihat [`LICENSE`](LICENSE). Library pihak ketiga dan Microsoft runtime tunduk pada ketentuan lisensi masing-masing dan pemberitahuan lisensinya.
