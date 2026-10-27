# Changelog

Significant changes to Heartflame Forge are documented here. Older reports in `docs/` are historical development records and may use the former project name.

## 1.3.0 — Heartflame Forge / Windows-first GUI packaging

- Rebrand user-facing GUI, application metadata, diagnostics, documentation, and binary name to **Heartflame Forge**.
- Add Heartflame Forge SVG/PNG branding and a Windows `.ico` application resource.
- Rename the public CMake package to `HeartflameForge` and expose `HeartflameForge::core` / `HeartflameForge::png`; retain deprecated build-tree aliases for source migration.
- Add a Windows x64 workflow using Qt 6.8.3, MSVC 2022, and a custom static-vcpkg/dynamic-CRT triplet.
- Add Windows portable ZIP and Inno Setup installer packaging, Qt deployment, a second smoke test against the deployed portable layout, third-party license copying, Microsoft Authenticode verification of the VC++ runtime installer, and SHA-256 checksums.
- Fix multi-config plugin output layout so the sample plugin is placed at `Release/plugins` beside `HeartflameForge.exe` for the Windows smoke test.
- Install the optional GLB helper beside the GUI so the Model Export page can locate it after installation.
- Add separate public security/privacy documentation and git-ignore local-only `private/` notes, game assets, build output, and credentials.
- Rewrite the public README with Windows installation, source-build, module capability, privacy, security, and known-limit sections.
- Add a Bahasa Indonesia README, explicit local-vs-CI Windows validation status, and update GitHub Actions to current major releases for the Node 24 runner generation.

## 1.2.1 — Parser and packaging audit (historical)

- Correctly report unsupported TIM2 profiles as `PARTIAL` and refuse to decode the PSM=0 sample until its layout is understood.
- Tighten WAV metadata and CLI numeric parsing; add malformed-input regression tests.
- Use no-clobber file output transactions and test output-creation race conditions.
- Install exported CMake targets and verify a downstream package consumer.
- Audit the supplied corpus with 60 supported TIM2 round-trips and one unsupported profile explicitly skipped.

Previous release notes are retained under `docs/DEVELOPMENT_REPORT_*.md` as historical records.
