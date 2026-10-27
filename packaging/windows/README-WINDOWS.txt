HEARTFLAME FORGE 1.3.0 - WINDOWS x64
====================================

FIRST RUN (PORTABLE ZIP)
1. Extract the entire ZIP into a new folder, for example:
   %LOCALAPPDATA%\Programs\Heartflame Forge Portable
2. Keep all files and subfolders together. Do not move HeartflameForge.exe
   away from its platforms, imageformats, plugins, and helper files.
3. Double-click RUN-HEARTFLAME-FORGE.bat or HeartflameForge.exe.
4. If Windows reports a missing VCRUNTIME/MSVCP DLL, run
   INSTALL-VC-RUNTIME.bat. That opens the Microsoft-signed Visual C++ runtime
   installer included in this package. Restart Heartflame Forge afterwards.

INSTALLER
Use HeartflameForge-Setup-x64-v1.3.0.exe for a Start Menu shortcut and managed
uninstall entry. The installer may ask for Windows permission to install the
Microsoft runtime prerequisite.

SAFETY AND PRIVACY
- Work on a copy of your own game files and keep the originals backed up.
- The app does not need a login or cloud account to inspect local files.
- Native extension DLLs are executable code. Load only extensions you trust.
- This build is not code-signed by a project publisher. Verify the SHA256 file
  from the same official release and do not bypass an unexpected security alert.
- Do not publish game asset dumps, save files, personal paths, usernames, or
  private modding notes in issues, screenshots, logs, or public repositories.

KNOWN LIMITS
The program is still a workbench: support is format-specific. It does not yet
fully decode game-specific MDL geometry/animation or proprietary audio streams.
A format being listed by the inspector does not mean it is editable or rebuilt.
Unsupported texture profiles are reported instead of being forced through an
unverified decoder. Save edited assets to new paths and test outputs separately.

See README.md and docs/PRIVACY_AND_MODDER_SAFETY.md for details.
