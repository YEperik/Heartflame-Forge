# Format research backlog — v1.1.0

This list separates observed evidence from interpretations. The marker names below are byte strings seen in the current sample set; they are not decoded field semantics yet.

## P0 — model geometry and GLB

**Evidence:** 85 `.mdl` files were previously profiled; `SL01` wrappers and candidate resource ranges are bounded/validated. Some files contain `MDHD`, `MDOB`, `MDMT`, `MDCC`, and `MDST` markers.

**Unknown:** chunk ownership/lengths inside those records, endianness for all fields, vertex stride, position/normal/color/UV packing, index width, primitive topology, coordinate basis, material mapping, and hierarchy.

**Next experiments:** select several small and large samples; diff structures of similar models; identify repeated ranges; look for aligned floats/indices with cross-file consistency; produce candidate vertex dumps only behind an explicit experimental command; compare bounds and topology in a small standalone test viewer.

**Pass criteria:** at least three structurally similar files parse with bounded chunk traversal; all vertex/index references fit validated ranges; reconstructed mesh bounds are plausible; triangle topology is independently checked; malformed offsets fail closed. Only then construct `aiScene` and test GLB output in a second viewer.

## P0 — animation

**Unknown:** clip containers, skeleton/bone identifiers, keyframe time base, quaternion/vector packing, interpolation modes, looping, and relation between `.mdl` and animation resources.

**Next experiments:** inventory all likely animation extensions and embedded markers; compare corresponding models/resources; search for repeated time/value records; document candidates by offset and file hashes. Do not name data as position/rotation until a hypothesis is independently confirmed.

**Pass criteria:** one clip can be parsed without out-of-bounds access; timeline monotonicity/loop behavior is tested; bone references resolve; sampled poses can be compared across clips or against an in-game capture.

## P1 — audio (`SDPK`, `ADSC`)

**Evidence:** `inspect_header` recognizes those signatures, and ADSC inspection currently reports the next four-byte tag only.

**Unknown:** container chunk lengths, sample format, codec, packet/frame boundaries, channel mapping, sample rate, loop metadata.

**Next experiments:** inventory resource sizes and tag frequency; test little-endian lengths against exact file bounds; compare near-duplicate audio assets; use external tools only after identifying whether a standardized codec/container is embedded.

**Pass criteria:** exact bounded chunk walk over multiple files, an independently recognized/decodeable payload, and a PCM/WAV export comparison. Until then the Audio Inspector reports metadata only.

## P1 — video/cutscene (`PSS`/`STR` candidates)

**Evidence:** a MPEG program-stream pack-start signature is recognized by generic header inspection. Signature alone does not prove the whole file is PSS or a playable video.

**Unknown:** packet/sector alignment, audio stream IDs, video codec profile, timing, padding and block boundaries.

**Next experiments:** inventory candidate files; measure repeated start-code and sector patterns; validate packet lengths; compare extracts with a known demuxer only after safe extraction and signature checks.

**Pass criteria:** complete packet walk to EOF without overrun, identified elementary streams and timestamps, and reproducible preview using a standard decoder. Keep the custom demux separate from FFmpeg codec integration.

## P1 — text/script

**Evidence:** the new scanner detects printable ASCII and UTF-16LE runs with ASCII-range codepoints and outputs offsets to TSV.

**Unknown:** actual encoding per resource, string table boundaries, relocation/pointer tables, terminators, compression, control codes and bytecode.

**Next experiments:** aggregate string locations by file; compare offsets around repeated strings; test whether table/pointer candidates resolve to valid ranges; preserve raw bytes and original encoding in reports.

**Pass criteria:** deterministic extraction/injection of one verified table with byte-length and pointer updates checked; unknown controls preserved rather than normalized away.

## P2 — texture

**Evidence:** 61 sample TIM2 textures round-tripped to PNG under the validated indexed 8-bit + RGBA32 CLUT profile; ZIP batch replacement works in tests.

**Unknown:** other pixel storage modes, palettes, mip layouts, alpha semantics and uncommon TIM2 variants.

**Next experiments:** classify rejected/unsupported TIM2 headers; gather fixtures per image type; add decoder tests by profile rather than expanding one large decoder function.

## Shared engineering checklist

- Keep source hashes before/after each experiment.
- Record path, size, hash, header bytes, candidate offsets, and tool version.
- Run malformed/truncated/overlap/overflow tests for each parser.
- Do not overwrite source assets; output to a new path and verify it after writing.
- Distinguish `implemented`, `read-only heuristic`, `experimental`, and `blocked on format research` in the GUI and report.
