# Windows release status — Heartflame Forge 1.3.0

## Verified in this working environment

- The source tree was configured and compiled on Linux for the independent C++ core and CLI tools.
- The exact public source ZIP was extracted into a fresh directory, configured, and built with GUI disabled because this environment lacks Qt development files.
- Release CTest from that extracted ZIP: **16/16 passed**.
- `cmake --install` from the extracted ZIP succeeded, and a separate consumer project configured, built, and ran using `find_package(HeartflameForge 1.3 CONFIG REQUIRED)`.
- UndefinedBehaviorSanitizer CTest: **16/16 passed**.
- The public Markdown files and their relative links were checked; the source archive is built without the local `private/` directory or game asset dumps.
- Strict GUI configuration was intentionally tested. It fails with an explicit Qt dependency error in this environment, because Qt Widgets C++ development headers, CMake package files and `moc` are not installed here.

## Not yet verified here

- The Windows Qt GUI was **not compiled or launched on Windows** in this environment.
- The Inno Setup script has not been compiled by `ISCC.exe` here.
- The Windows portable ZIP, installer EXE, Authenticode-signature check on the downloaded Microsoft runtime, and SHA-256 manifest have not been produced here.
- No GitHub Actions run has occurred from this source snapshot. The workflows are configuration source, not evidence of a completed CI run.

## How a Windows package is generated

The `.github/workflows/windows.yml` workflow installs Qt 6 for MSVC 2022 x64, installs the native dependencies with vcpkg, requires the GUI target, builds Release, runs CTest and a build-tree `HeartflameForge.exe --smoke-test` offscreen, stages the installation with `windeployqt`, then smoke-tests the deployed portable folder without inheriting Qt development paths. It includes license notices and the Microsoft VC++ Redistributable only after checking its Authenticode signature, and produces a portable ZIP, Inno Setup installer, and SHA-256 checksums. A matching `v1.3.0` tag is configured to publish the generated files to GitHub Releases after the Windows job succeeds.

To run this, commit/push the source to a GitHub repository with Actions enabled, inspect **Actions → Heartflame Forge — Windows**, and do not create a release tag until the build and smoke tests are green. For an eventual next version, update `VERSION`, `CMakeLists.txt`, README/install-guide names, changelog and installer metadata together, then use a matching `vX.Y.Z` tag.

## Security and privacy notes

- Release binaries are not code-signed with a Heartflame Forge publisher certificate in this source snapshot.
- The Windows workflow verifies the signature of Microsoft's VC++ Redistributable installer; that does not sign Heartflame Forge itself.
- The public source ZIP deliberately omits `private/`, local game archives, working build trees, and credentials.
- Native extension DLLs are not sandboxed; only load extensions whose origin and code you trust.
