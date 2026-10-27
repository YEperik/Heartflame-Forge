# Third-party notices

Heartflame Forge uses third-party components. Their own license terms apply independently of the MIT license for the original project source.

| Component | Purpose | License / notice source |
| --- | --- | --- |
| Qt 6 Widgets | Desktop GUI, image handling, plugin host | Qt's per-component license files are copied into `THIRD_PARTY_LICENSES/Qt/` in Windows packages. Qt open-source distributions may use LGPL/GPL terms; review the exact installed module notices and comply with the chosen terms. |
| libarchive | ZIP/archive access | The selected vcpkg port's `copyright`/license file is included under `THIRD_PARTY_LICENSES/vcpkg/`. |
| libpng | PNG CLI codec and tests | The selected vcpkg port's license file is included under `THIRD_PARTY_LICENSES/vcpkg/`. |
| Assimp | Optional GLB/glTF export for importable formats | The selected vcpkg port's license file is included under `THIRD_PARTY_LICENSES/vcpkg/`. Assimp does not decode the game's custom MDL by itself. |
| Microsoft Visual C++ Redistributable (x64) | MSVC runtime prerequisite | Downloaded from Microsoft's official link `https://aka.ms/vc14/vc_redist.x64.exe`; the Windows workflow checks the Authenticode signature before packaging. See Microsoft's current [redistributable guidance](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist). |
| Inno Setup | Windows installer generator | See [Inno Setup's license and project information](https://jrsoftware.org/isinfo.php). It is used as a build-time packaging tool; it is not required to run the application. |

The Windows workflow copies license/copyright files from the exact Qt/vcpkg installation used for that build rather than assuming every transitive dependency has the same license. Review the generated `THIRD_PARTY_LICENSES/` directory before publishing a release. Do not remove Qt licensing notices or redistribute DLLs built from a different Qt license/toolchain without reviewing the consequences.
