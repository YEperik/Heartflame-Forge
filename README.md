# PS2 Asset Inspector — exploratory baseline

A small, read-only C++17 utility created from the initial audit of `Innocent Life - A Futuristic Harvest Moon - Special Edition (USA)`.
It inspects at most the first 64 bytes of each file and reports recognizable
signatures plus a verified structural check for the current `SL01` samples. It
does **not** decompress, decode, edit, or rebuild game assets.

## Build

Requirements: CMake 3.16+ and a C++17 compiler.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

## Usage

```sh
# Inspect a single file
./build/ps2asset-inspect path/to/asset.map

# Inspect a folder recursively
./build/ps2asset-inspect path/to/extracted/dataimg

# Summary only
./build/ps2asset-inspect --summary path/to/extracted/dataimg
```

Output is tab-separated. Recognized signatures currently include `SL01`, `TIM2`,
MPEG program-stream pack headers (`00 00 01 BA`), ELF, Sony GIM (`MIG.00.1PSP`),
`SDPK`, `ADSC`, `.GIS`, and a `ROMDIR` marker found within the inspected prefix.

## Evidence boundary

For every `SL01` asset checked in the supplied archive (944 files), the little-endian
32-bit value at offset `0x08` equals `file_size - 12`. This is reported as a
length invariant; the code deliberately leaves the field at `0x04` uninterpreted.
That avoids asserting an unproven decompressed-size or checksum meaning.

The tool handles files/folders, not ZIP directly. Extract the archive first, or use
`tools/audit_archive.py` for an archive inventory without extracting its contents.
