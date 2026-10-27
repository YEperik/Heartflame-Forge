# Third-party repository evaluation (reviewed 2026-10-27)

These are candidate dependencies, not vendored source. Inspect the exact release, transitive dependencies, security notes, and license before adding any repository to the build or shipping its binaries.

## GUI choices

### Keep the existing Qt Widgets shell for now

The app already has Qt Widgets pages, dialogs, a dockable diagnostics panel, and a dynamic Qt plugin API. Staying with Qt is the lowest-risk route for v1.1.0; new feature pages are separated into individual source files and the byte-processing core remains GUI-independent. Qt availability remains optional in CMake so the CLI/core can build without it.

### Dear ImGui — useful for a dedicated inspector/viewport, not a forced rewrite

- [ocornut/imgui](https://github.com/ocornut/imgui): official Dear ImGui project, MIT license, C++ immediate-mode GUI.
- [Dear ImGui docking branch](https://github.com/ocornut/imgui/wiki/Docking): docking is available on the maintained `docking` branch; it supports dockspace layouts suitable for a scene viewport, hierarchy, inspector, and timeline.
- [pthom/imgui_bundle](https://github.com/pthom/imgui_bundle): integrated C++/Python ecosystem with Dear ImGui, ImPlot/ImPlot3D, ImmVision, ImGuizmo, node-editor, ImFileDialog and other extensions. Its CMake options allow submodules to be disabled. Attractive for a mostly-new ImGui application, but broader than we need for the current Qt shell.
- [aiekick/ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog): MIT-licensed file dialog component specifically for Dear ImGui.
- [Nelarius/imnodes](https://github.com/Nelarius/imnodes): small node graph editor for Dear ImGui; potentially useful for future material graphs or animation state graphs, not needed for file parsing.
- [goossens/ImGuiColorTextEdit](https://github.com/goossens/ImGuiColorTextEdit): text-editor widget with syntax coloring; candidate for the script module after script syntax is known. Verify current license and activity at the selected revision before bundling.

**Recommendation:** do not mix Qt and ImGui in the main shell yet. If an interactive 3D viewport becomes the next bottleneck, prototype it in a separate branch using the ImGui docking branch or imgui_bundle and choose one UI backend before production. Keep core APIs independent so both frontends could consume them.

## Format libraries

- [assimp/assimp](https://github.com/assimp/assimp): import library and format exporters including glTF 2.0 binary (`glb2`). Useful to export an already-decoded scene. It cannot infer the game's custom vertex/index/scene layout from `SL01` markers; a game-specific decoder still has to create a correct scene first. This build detects Assimp optionally and omits the GLB CLI if the development package is absent.
- [mackron/miniaudio](https://github.com/mackron/miniaudio): single-source audio playback/capture library with WAV/FLAC/MP3 decoding and WAV encoding. Candidate for preview after we identify how game resources turn into decoded PCM. Its upstream says the choice of license is public domain or MIT No Attribution; validate the release's license file when integrating.
- [FFmpeg](https://github.com/FFmpeg/FFmpeg) and its [license guidance](https://ffmpeg.org/legal.html): mature audio/video demux/decode/encode tools and libraries. Candidate for standard codecs and preview once container extraction is understood. The default is LGPL-based with some optional GPL components; codec/build configuration and redistribution obligations need review before linking or bundling. FFmpeg is not a substitute for understanding a proprietary PS2 packet/container layout.
- [btzy/nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended): cross-platform native file dialogs with C/C++ interfaces and zlib license. It is relevant only if a future pure-ImGui frontend needs native dialogs; the current Qt shell already provides dialogs.

## Integration policy

1. Use optional CMake feature switches and do not make core parsers depend on multimedia or GUI libraries.
2. Prefer subprocess/CLI boundaries for large multimedia tools until legal, packaging, and crash-isolation decisions are finalized.
3. Record dependency version/tag, license, build options, and shipped binary origin.
4. Do not enable conversion/edit operations for a format just because a generic library accepts the extension; verify a real file and round-trip invariants first.
