# Heartflame Forge — Blank Modder Privacy Checklist Template

> **This is a blank public template, not a place for personal notes.** Copy it into your local `private/` directory and fill in only that copy. Never put personal paths, private archive names, credentials, or sensitive notes into this template or public issues.

## Identitas dan lokasi kerja lokal
- Nama panggilan lokal (hindari nama legal bila tidak dibutuhkan):
- Alias folder kerja lokal (hindari absolute path):
- Alias arsip uji privat:
- Target build/versi:

## Privasi dan keamanan sebelum eksperimen
- [ ] Saya bekerja pada salinan file yang memang boleh saya gunakan.
- [ ] File asli dibackup dan tidak dimasukkan ke repositori publik.
- [ ] Direktori output terpisah dari sumber; tool tidak diberi izin menimpa file sumber.
- [ ] Hash SHA-256 sumber dicatat bila perlu reproduksibilitas.
- [ ] Tidak ada password, token API, cookie, private key, signed URL, atau kredensial dalam source, log, screenshot, dan catatan.
- [ ] Screenshot/log disamarkan dari nama pengguna Windows, nama perangkat, drive letter, home path, dan notifikasi pribadi.
- [ ] Dump aset game, save file, dan file proprietary tidak dilampirkan ke issue/release publik tanpa hak redistribusi.
- [ ] Profil format yang tidak didukung ditandai sebagai unsupported/partial; tidak dipaksa lewat decoder spekulatif.
- [ ] Hasil format yang dimodifikasi diuji terpisah, bukan langsung menimpa aset sumber.
- [ ] Extension DLL berasal dari sumber yang dipercaya dan sudah ditinjau; plugin native tidak di-sandbox.

## Catatan eksperimen lokal
- File alias:
- Langkah reproduksi:
- Build preset dan compiler:
- Hasil yang diharapkan:
- Hasil aktual:
- Hash sebelum/sesudah:
- Log privat (path lokal, jangan ikut publik):

## Sebelum membuat laporan publik
- [ ] Hapus nama/path komputer, akun, arsip uji privat, dan detail yang tidak relevan.
- [ ] Lampirkan fixture sintetis atau data redistributable minimal jika memungkinkan.
- [ ] Sertakan versi aplikasi, OS/compiler, perintah reproduksi, expected vs actual, serta status test.
- [ ] Jangan mengklaim format terdukung penuh berdasarkan signature/header saja.
- [ ] Pastikan file lampiran boleh didistribusikan.
