# Windows vcpkg triplet

`x64-windows-static-md.cmake` builds the third-party vcpkg libraries statically
while retaining the dynamic MSVC runtime expected by the official Qt MSVC
binaries. This reduces third-party DLLs in the portable package. Qt itself is
still dynamically deployed using `windeployqt`; do not replace the Qt DLLs with
unverified copies.
