# Initial Reverse Engineering Report — `dataimg.zip`

**Status:** Inventory, signature inspection, and initial header validation complete. Asset decoders and writers/rebuilders have not yet been developed.

## 1. Input and Integrity

| Item | Inspection Result |
|---|---|
| Input | `dataimg.zip` |
| ZIP Size | 234,323,151 bytes |
| Non-directory files | 1,300 |
| Directory entries | 53 |
| Total data size within ZIP | 322,660,954 bytes |
| ZIP integrity test | Passed; `zipfile.testzip()` returned `None` |
| Structure | Contains a `dataimg/` directory with assets and a `dataimg/Z.FAT` file |

The contents appear to be a **game asset/data package** rather than a complete PS2 disc image. No `SYSTEM.CNF` or game boot executables (such as `SLUS_*.ELF` or `SLES_*.ELF`) were found within the ZIP; consequently, the executable version, boot path, and runtime behavior cannot be verified from this file alone.

### Title Identification

The video file `dataimg/movie/009_title.pss` reveals the title: **Innocent Life: A Futuristic Harvest Moon**. The font file named `ilps2_eng_18.fnt` also aligns with this identification. The title is identified with high confidence based on visual assets; however, the release region and executable revision remain unconfirmed.

Probing `009_title.pss` with `ffprobe` identified an MPEG-2 video stream with a resolution of 512×448, a frame rate of `30000/1001` (≈29.97 fps), and a duration of 50.05 seconds. This represents a metadata check of the video file, not a comprehensive validation of all cutscenes.

## 2. Inventory by File Extension
| Extension | Count | Initial Notes |
|---|---:|---|
| `.map` | 748 | All have the `SL01` signature |
| `.atb` | 126 | Map/scene data; structure not yet decoded |
| `.scp` | 108 | All have the `SL01` signature; likely related to scripts/events based on path and name; not yet decoded |
| `.mdl` | 85 | 76 have `SL01`; the other 9 use a different header |
| `.tm2` | 61 | All have the `TIM2` signature; picture count in header = 1 |
| `.bin` | 37 | Mixed; 7 have the `SL01` signature, others have various numeric headers |
| `.gim` | 33 | All begin with `MIG.00.1PSP` |
| `.sdl` | 29 | All begin with `ADSC` |
| `.sd` | 21 | All begin with `SDPK` |
| `.irx` | 18 | All begin with an ELF magic number |
| `.dat` | 11 | Mixed; 1 has the `SL01` signature, 7 have `MN01`, others differ |
| `.pss` | 11 | All begin with the MPEG-PS pack start code `00 00 01 BA` |
| `.fnt` | 4 | All have the `SL01` signature |
| `.psi` | 2 | Structure not yet identified |
| `.gis` | 2 | Text begins with `.GIS 1.00` |
| `.msg` | 1 | Starts with a table of little-endian values; structure not yet identified |
| `.dir` | 1 | A list of filenames in text format, one per line |
| `.img` | 1 | `ioprp310.img`, contains the `ROMDIR` marker at offset 0x10 |
| `.fat` | 1 | `Z.FAT`, explained in section 4 |

The figure above summarizes all 1,300 non-directory files. The inventory CSV stores the size, the initial 16 bytes, the signature classification, and the results of the `SL01` check for each file.

## 3. Validated byte findings

### `SL01` Header

Cross-sample evidence reveals the following consistent pattern across 944 files with the `SL01` magic number:

| Offset | Size | Evidence / Value | Conclusion | Confidence |
|---|---:|---|---|---|
| `0x00` | 4 | ASCII `SL01` | File family signature | High |
| `0x04` | 4 | E.g., `08 39 00 00` in `bulb1.map` → LE `0x00003908` | Meaning yet to be determined; should not be assumed to be the decompressed size or a checksum | Unknown |
| `0x08` | 4 | `C6 14 00 00` in `bulb1.map` → LE `0x14C6` = 5,318 | Value equals `file_size - 12` | High; verified across 944/944 files |
| `0x0C` | variable | Payload/file-type-specific structure begins here | Subsequent bytes vary by type; should not be treated as a universal field beyond the 12-byte header boundary | Medium |

Concrete example: `dataimg/common/art/bulb1.map`

- File size: 5,330 bytes.
- Bytes `0x00..0x0B`: `53 4C 30 31 08 39 00 00 C6 14 00 00`.
- Little-endian value at `0x08`: `0x14C6` = 5,318 = 5,330 − 12.
- The byte at `0x0C` is `0B`; an ASCII `FHDR` tag begins at offset `0x0D`, followed by the bytes `98 00 00 00`. This confirms that a tagged structure exists within the file, though the size and relationships of all records have not yet been fully mapped.

Some of the four-byte tags visible in the `.map` sample include `FHDR`, `FMAP`, `FOPT`, `FETC`, `FATB`, `FNAM`, `FMDL`, `MDHD`, `MDMT`, `MDOB`, `MDCC`, `MDST`, and `FTEX`. The specific meaning and byte boundaries for each tag still need to be verified for each record type.

### Other signatures

| Family | Evidence | Status |
|---|---|---|
| TIM2 | 61/61 `.tm2` files start with `TIM2`; u16 LE values ​​at offset `0x04` = 4 and offset `0x06` = 1 in examined samples | Initial signature/field identified; image decoder not yet implemented |
| GIM | 33/33 `.gim` files start with `MIG.00.1PSP` | Signature identified; not yet exported to PNG |
| PSS | 11/11 `.pss` files start with `00 00 01 BA` | Identified as MPEG program stream; one title file successfully probed |
| IRX | 18/18 `.irx` files start with `7F 45 4C 46` | ELF; sample `libsd.irx` is labeled as ELF32/little-endian with `e_machine=0x0008` |
| SDL | 29/29 `.sdl` files start with `ADSC`, followed by an `SShd` tag in the samples | Signature observed; codec/chunk layout not yet validated |
| SD | 21/21 `.sd` files start with `SDPK` | Signature observed; internal pack structure not yet parsed |
| GIS | 2/2 `.gis` files start with the text `.GIS 1.00` | Text header identified |

The signature only identifies the initial wrapper/family; it does not automatically confirm all internal fields or guarantee that the payload can be read by a generic decoder.

## 4. `Z.FAT`: consistent structure, semantics yet to be verified

- File size: 15,408 bytes.
- Initial 12 bytes: `00 00 00 00 03 05 00 00 00 00 00 00`. - Little-endian word at offset `0x04` = `0x503` = 1,283.
- The arithmetic aligns perfectly: `12 + 1,283 × 12 = 15,408`; thus, a 12-byte header followed by 1,283 records of 12 bytes each is a very strong structural hypothesis.
- The first record at `0x0C`: `47 6C 0A E0 1F 90 07 00 40 24 00 00`, representing three little-endian (LE) words: `0xE00A6C47`, `0x0007901F`, and `0x00002440`.

The record fields have not yet been named, nor have they been used to reconstruct anything. While part of the third word matches the file size as listed in the directory, a size match alone does not confirm the meaning of the field. Furthermore, the number of FAT records differs from the number of loose files present in the ZIP archive; a one-to-one mapping has not yet been established.

## 5. Included Implementation

The `ps2asset-inspect` project is a read-only C++17 inspection tool developed based on the evidence above. It reads a maximum of 64 bytes per file, identifies the initial signature, and verifies the `SL01` invariant without modifying any files. There are no decoders, serializers, or writers/reimporters included or claimed to be complete.

### Build and Testing

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Results in this environment:

- Compiler: GCC 14.2.0.
- CMake: 3.31.6.
- C++ build successful.
- Unit tests: 1/1 passed.
- ZIP integrity check: passed.
- CLI tested with sample `.map`, `.tm2`, `.irx`, and `.pss` files; all four types were correctly identified.

## 6. Pending tasks / risks

1. The meaning of the `SL01` field at offset `0x04` and the payload structure remain incomplete; no decompressor has been developed yet, despite evidence gathered across multiple samples.
2. `Z.FAT` records have not yet been definitively mapped to specific names/paths or payloads.
3. `.atb` files, non-`SL01` `.mdl` variants, `.scp`, `.msg`, `.psi`, `.sd`, and `.sdl` files have not yet been parsed down to the record/codec level.
4. It has not been verified whether textures embedded within `.map` files are complete TIM2 data structures or records that require further processing.
5. No writer or mechanism for injecting modifications into the game exists; the compatibility of modified files has not been tested.
6. As the boot executable and the complete disc image are not included in the ZIP, runtime/emulator testing cannot be performed using only these inputs.

## 7. Next technical priorities

1. Compare multiple small `.map` files containing the same asset type to reconstruct record/tag lengths and identify payload boundaries.
2. Determine whether `SL01` is a tagged resource structure, a compression wrapper, or a combination of both, using verifiable decoders or references; do not rely on guesswork for the `0x04` field.
3. Validate the `Z.FAT` table by cross-referencing names, words, sizes, and record sequences across all available assets.
4. Develop a read-only parser for the format that is easiest to verify (e.g., standalone TIM2 files), including test fixtures.
5. Once read/round-trip functionality is confirmed, proceed to design a writer for the target assets and verify the output within the game.
