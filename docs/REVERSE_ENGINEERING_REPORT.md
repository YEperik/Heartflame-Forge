# Laporan Reverse Engineering Awal — `dataimg.zip`

**Status:** inventarisasi, pemeriksaan signature, dan validasi header awal selesai. Decoder aset dan writer/rebuilder belum dibuat.

## 1. Input dan integritas

| Item | Hasil pemeriksaan |
|---|---|
| Input | `dataimg.zip` |
| Ukuran ZIP | 234.323.151 byte |
| File non-direktori | 1.300 |
| Entri direktori | 53 |
| Total ukuran data di dalam ZIP | 322.660.954 byte |
| Uji integritas ZIP | Lulus; `zipfile.testzip()` mengembalikan `None` |
| Susunan yang ada | Direktori `dataimg/` berisi aset dan `dataimg/Z.FAT` |

Isi ini tampak seperti **paket aset/data game**, bukan image disc PS2 lengkap. Di dalam ZIP tidak ditemukan `SYSTEM.CNF` atau executable boot game seperti `SLUS_*.ELF`/`SLES_*.ELF`; karena itu versi executable, boot path, dan perilaku runtime belum bisa diuji dari berkas ini saja.

### Identifikasi judul

Video `dataimg/movie/009_title.pss` memperlihatkan judul **Innocent Life: A Futuristic Harvest Moon**. Nama font `ilps2_eng_18.fnt` juga selaras dengan identifikasi tersebut. Judul mempunyai keyakinan tinggi berdasarkan aset visual; wilayah rilis dan revisi executable belum terkonfirmasi.

Probe terhadap `009_title.pss` dengan `ffprobe` mengenali stream video MPEG-2 berukuran 512×448, frame rate `30000/1001` (≈29,97 fps), dan durasi 50,05 detik. Ini pemeriksaan metadata video, bukan validasi keseluruhan semua cutscene.

## 2. Inventaris menurut ekstensi

| Ekstensi | Jumlah | Catatan awal |
|---|---:|---|
| `.map` | 748 | Semua memiliki signature `SL01` |
| `.atb` | 126 | Data map/scene; struktur belum didekode |
| `.scp` | 108 | Semua memiliki signature `SL01`; kemungkinan terkait script/event berdasarkan jalur dan nama, belum di-decode |
| `.mdl` | 85 | 76 `SL01`; 9 lainnya memakai header berbeda |
| `.tm2` | 61 | Semua signature `TIM2`; picture count pada header = 1 |
| `.bin` | 37 | Campuran; 7 signature `SL01` dan kelompok header numerik lain |
| `.gim` | 33 | Semua mulai dengan `MIG.00.1PSP` |
| `.sdl` | 29 | Semua mulai dengan `ADSC` |
| `.sd` | 21 | Semua mulai dengan `SDPK` |
| `.irx` | 18 | Semua mulai dengan ELF magic |
| `.dat` | 11 | Campuran; 1 signature `SL01`, 7 signature `MN01`, lainnya berbeda |
| `.pss` | 11 | Semua mulai dengan MPEG-PS pack start code `00 00 01 BA` |
| `.fnt` | 4 | Semua signature `SL01` |
| `.psi` | 2 | Struktur belum diidentifikasi |
| `.gis` | 2 | Teks dimulai `.GIS 1.00` |
| `.msg` | 1 | Berawalan tabel nilai little-endian; struktur belum diidentifikasi |
| `.dir` | 1 | Daftar nama file teks berbaris |
| `.img` | 1 | `ioprp310.img`, berisi marker `ROMDIR` pada offset 0x10 |
| `.fat` | 1 | `Z.FAT`, dijelaskan di bagian 4 |

Angka di atas merangkum semua 1.300 file non-direktori. CSV inventaris menyimpan ukuran, 16 byte awal, klasifikasi signature, serta hasil pemeriksaan `SL01` untuk setiap file.

## 3. Temuan byte yang sudah divalidasi

### Header `SL01`

Bukti lintas sampel menunjukkan pola tetap berikut pada 944 file yang bermagic `SL01`:

| Offset | Ukuran | Bukti / nilai | Kesimpulan | Keyakinan |
|---|---:|---|---|---|
| `0x00` | 4 | ASCII `SL01` | Signature keluarga berkas | Tinggi |
| `0x04` | 4 | Misalnya `08 39 00 00` pada `bulb1.map` → LE `0x00003908` | Makna belum ditetapkan; tidak boleh langsung disebut ukuran dekompresi/checksum | Belum diketahui |
| `0x08` | 4 | `C6 14 00 00` pada `bulb1.map` → LE `0x14C6` = 5.318 | Nilainya sama dengan `ukuran_file - 12` | Tinggi; diuji pada 944/944 file |
| `0x0C` | variabel | Payload/struktur khusus tipe file dimulai di sini | Byte berikutnya berbeda antarjenis; tidak aman diperlakukan sebagai field universal selain batas header 12 byte | Sedang |

Contoh konkret `dataimg/common/art/bulb1.map`:

- Ukuran file: 5.330 byte.
- Byte `0x00..0x0B`: `53 4C 30 31 08 39 00 00 C6 14 00 00`.
- Nilai little-endian di `0x08`: `0x14C6` = 5.318 = 5.330 − 12.
- Byte pada `0x0C` adalah `0B`; tag ASCII `FHDR` mulai pada offset `0x0D`, diikuti byte `98 00 00 00`. Ini bukti struktur bertag ada di dalam berkas, tetapi ukuran dan relasi seluruh record belum dipetakan sepenuhnya.

Beberapa tag empat-byte yang terlihat di sampel `.map` adalah `FHDR`, `FMAP`, `FOPT`, `FETC`, `FATB`, `FNAM`, `FMDL`, `MDHD`, `MDMT`, `MDOB`, `MDCC`, `MDST`, dan `FTEX`. Makna serta batas byte masing-masing tag tetap perlu diuji per tipe record.

### Signature lain

| Keluarga | Bukti | Status |
|---|---|---|
| TIM2 | 61/61 `.tm2` mulai `TIM2`; nilai u16 LE offset `0x04` = 4 dan offset `0x06` = 1 pada sampel yang diperiksa | Signature/field awal teridentifikasi; decoder gambar belum dibuat |
| GIM | 33/33 `.gim` mulai `MIG.00.1PSP` | Signature teridentifikasi; belum diekspor ke PNG |
| PSS | 11/11 `.pss` mulai `00 00 01 BA` | Teridentifikasi sebagai MPEG program stream; satu file title berhasil diprobe |
| IRX | 18/18 `.irx` mulai `7F 45 4C 46` | ELF; contoh `libsd.irx` berlabel ELF32/little-endian dengan `e_machine=0x0008` |
| SDL | 29/29 `.sdl` mulai `ADSC`, diikuti tag `SShd` pada sampel | Signature terlihat; codec/chunk layout belum divalidasi |
| SD | 21/21 `.sd` mulai `SDPK` | Signature terlihat; internal pack belum diurai |
| GIS | 2/2 `.gis` mulai teks `.GIS 1.00` | Header teks teridentifikasi |

Signature mengidentifikasi pembungkus/keluarga awal saja; itu tidak otomatis membuktikan semua field internal atau bahwa payload dapat dibaca dengan decoder generik.

## 4. `Z.FAT`: struktur yang konsisten, semantik belum terbukti

- Ukuran file: 15.408 byte.
- 12 byte awal: `00 00 00 00 03 05 00 00 00 00 00 00`.
- Word little-endian pada offset `0x04` = `0x503` = 1.283.
- Aritmetika cocok tepat: `12 + 1.283 × 12 = 15.408`; sehingga header 12 byte diikuti 1.283 record × 12 byte adalah hipotesis struktur yang sangat kuat.
- Record pertama pada `0x0C`: `47 6C 0A E0 1F 90 07 00 40 24 00 00`, sebagai tiga word LE `0xE00A6C47`, `0x0007901F`, `0x00002440`.

Kolom-kolom record belum dinamai atau dipakai untuk membangun ulang apa pun. Sebagian word ketiga cocok dengan ukuran file dalam direktori, tetapi kecocokan ukuran saja tidak membuktikan arti kolom. Jumlah record FAT juga berbeda dari jumlah file longgar yang ada pada ZIP; pemetaan satu-ke-satu belum didapat.

## 5. Implementasi yang disertakan

Proyek `ps2asset-inspect` adalah alat inspeksi read-only C++17 yang dibuat dari bukti di atas. Alat ini membaca paling banyak 64 byte per file, mengenali signature awal, dan memeriksa invariant `SL01` tanpa mengubah berkas. Tidak ada decoder, serializer, atau writer/reimporter yang diklaim selesai.

### Build dan pengujian

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Hasil pada lingkungan ini:

- Compiler: GCC 14.2.0.
- CMake: 3.31.6.
- Build C++ berhasil.
- Unit test: 1/1 lulus.
- Uji integritas ZIP: lulus.
- CLI dicoba pada contoh `.map`, `.tm2`, `.irx`, dan `.pss`; keempat jenis dikenali dengan benar.

## 6. Yang belum dikerjakan / risiko

1. Makna field `SL01` pada `0x04` dan struktur payload belum lengkap; belum ada dekompresor berdasarkan bukti lintas sampel.
2. Record `Z.FAT` belum dipetakan ke nama/path atau payload dengan pasti.
3. `.atb`, bentuk `.mdl` non-`SL01`, `.scp`, `.msg`, `.psi`, `.sd`, dan `.sdl` belum diurai sampai level record/codec.
4. Tekstur dalam file `.map` yang tertanam belum dibuktikan apakah berupa data TIM2 utuh atau record yang perlu diproses lebih dahulu.
5. Tidak ada writer atau injeksi perubahan ke game; kompatibilitas hasil modifikasi belum diuji.
6. Karena executable boot dan image disc lengkap tidak ada di ZIP, pengujian runtime/emulator belum dapat dilakukan hanya dengan input ini.

## 7. Prioritas teknis berikutnya

1. Bandingkan banyak `.map` kecil dengan tipe aset yang sama untuk merekonstruksi panjang record/tag dan menemukan batas payload.
2. Tentukan apakah `SL01` adalah struktur resource bertag, kompresi pembungkus, atau gabungan keduanya menggunakan decoder/referensi yang dapat diuji; jangan menebak field `0x04`.
3. Uji tabel `Z.FAT` dengan pencocokan nama, word, ukuran, dan urutan record pada semua aset yang ada.
4. Bangun parser read-only spesifik format paling mudah dikonfirmasi terlebih dahulu (misalnya TIM2 berdiri sendiri), disertai test fixtures.
5. Setelah read/round-trip terbukti, baru rencanakan writer untuk aset target dan uji hasilnya di game.

