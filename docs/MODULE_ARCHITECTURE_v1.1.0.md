# Modular Workbench Architecture — v1.1.0

## Design goals

1. Binary parsing and validation live in `ps2asset_core` and must not include Qt or Dear ImGui.
2. Every GUI workspace has its own source file and page class under `src/gui/modules/`.
3. Each tool declares implemented behavior separately from analysis that remains unknown.
4. Optional dependencies must not prevent the core/CLI from building.
5. Read-only analysis is the default. A write/export action must validate inputs, refuse existing outputs, and keep source files unchanged.

## Layers

- **Core** (`include/ps2asset`, `src/`): reusable parsers. New modules should expose structured result types and deterministic formatters instead of manipulating UI widgets.
- **Feature catalog** (`workbench/module_catalog.hpp`, `src/workbench/module_catalog.cpp`): one source of truth for IDs, labels, current readiness, capabilities, and missing analysis. GUI navigation reads its display name/status from the catalog.
- **GUI shell** (`src/gui/main_window.*`): menu, navigation, status and diagnostics. It links feature pages but does not own their parser implementations.
- **Feature pages** (`src/gui/modules/`): separate Audio, Video/Stream, Animation, Text/Script, and GLB Export pages. `FeaturePageBase` shares file picking and safe read-only loading; each feature supplies a distinct inspector.
- **Extensions** (`IWorkbenchPlugin`, Qt module libraries in `plugins/`): third-party or locally developed pages may be loaded without adding them to the main window source. Plugins are trusted native code and should only be installed from known sources.
- **CLI**: automation-friendly counterpart to the GUI. New functionality should have a core API and CLI test before the UI depends on it.

## Module ownership and state

| Module ID | Owns | Implemented now | Still missing |
|---|---|---|---|
| `texture` | TIM2, preview, PNG and replacement | supported TIM2 profile and ZIP batch replacement | other TIM2 pixel/CLUT modes |
| `archive` | ZIP listing/repack | safe output ZIP and entry checks | non-ZIP containers, game-specific metadata |
| `binary` | generic byte view | header/signature inspection and hex/text search | meaning of unknown fields |
| `model-inspector` | MDL/SL01 | header, markers, bounded range scan | vertex, indices, UV, material, transform and node graph |
| `model-export-glb` | standard 3D scene export | optional Assimp CLI for formats Assimp imports | custom game `.mdl` decoder -> scene conversion |
| `animation` | container, skeleton, timeline, keyframes | read-only marker/container profile | animation block layout, time base, bone mapping and interpolation |
| `audio` | WAV and game resource signatures | bounded RIFF/WAVE metadata parser | SDPK/ADSC decoder and playback |
| `video` | stream and cutscene | MPEG pack-start signature detection | PSS/STR demux, audio/video packet mapping, codec verification |
| `text` | strings/tables/scripts | bounded ASCII + UTF-16LE ASCII-subset scan and TSV output | encoding detection, table relocation, script grammar and safe rebuild |

## Suggested internal decomposition

When the game-specific formats are understood, split the larger modules into focused subtools:

- **Animation**: Container/Clip Inspector → Skeleton & Bone Map → Track/Keyframe Timeline → Curve/Interpolation Inspector → Export Adapter.
- **Model**: Container/Chunk Inspector → Vertex Layout Candidates → Index/Topology Inspector → UV/Material Inspector → Scene Builder → GLB Exporter.
- **Audio**: Container Inspector → Codec/Frame Inspector → Decoder Adapter → Waveform/Metadata View → WAV Export.
- **Video**: Container/Packet Inspector → PSS/STR Demux → Audio/Video stream map → Preview/Transcode.
- **Text**: String Scanner → Encoding/Locale Table → Pointer/Offset Map → Script/Event Disassembler → Safe Table Rebuilder.
- **Texture**: TIM2 Header/Picture Inspector → Pixel/Palette Decoder → Preview/Palette View → PNG Import/Export → Batch Archive Writer.

Subtools should be added only after the corresponding binary structure can be cross-validated on multiple files and boundary/malformed tests exist.

## GUI decision

The current application remains on **Qt Widgets** for the shell because its plugin interface and working dialogs/pages already use Qt. The new core APIs and separate page classes deliberately avoid tying the parser layer to Qt; this allows an optional Dear ImGui viewport or a future complete GUI backend without rewriting file parsing. A complete dual Qt + ImGui shell is not introduced in this stage because it creates duplicated input/event-loop, rendering, docking, and packaging concerns before the formats have been decoded.
