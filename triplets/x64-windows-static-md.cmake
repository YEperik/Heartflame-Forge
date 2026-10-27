# Static third-party libraries with the dynamic MSVC runtime used by Qt's
# official MSVC binaries. This avoids shipping a second copy of every vcpkg DLL
# while keeping CRT settings compatible with the Qt toolchain.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
