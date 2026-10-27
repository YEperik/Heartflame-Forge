#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${HEARTFLAME_BUILD_DIR:-${PS2ASSET_BUILD_DIR:-${ROOT}/build-qt-gui}}"
JOBS="${PS2ASSET_BUILD_JOBS:-2}"
RUN_AFTER_SMOKE=0
if [[ "${1:-}" == "--run" ]]; then
  RUN_AFTER_SMOKE=1
elif [[ $# -gt 0 ]]; then
  echo "Usage: $0 [--run]" >&2
  exit 64
fi

if ! command -v cmake >/dev/null 2>&1; then
  echo "ERROR: CMake is required." >&2; exit 10
fi
if ! command -v c++ >/dev/null 2>&1; then
  echo "ERROR: a C++ compiler is required." >&2; exit 11
fi

# Strict mode prevents CMake from silently building only the CLI when Qt is absent.
cmake -S "${ROOT}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DPS2ASSET_BUILD_GUI=ON \
  -DPS2ASSET_REQUIRE_GUI=ON \
  -DPS2ASSET_BUILD_SAMPLE_PLUGIN=ON \
  -DBUILD_TESTING=ON
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

if [[ ! -x "${BUILD_DIR}/HeartflameForge" ]]; then
  echo "ERROR: Heartflame Forge GUI target was not produced even though strict configuration succeeded." >&2
  exit 12
fi
QT_QPA_PLATFORM=offscreen "${BUILD_DIR}/HeartflameForge" --smoke-test

echo "GUI build and offscreen startup smoke test passed."
if [[ "${RUN_AFTER_SMOKE}" -eq 1 ]]; then
  if [[ -z "${DISPLAY:-}" && -z "${WAYLAND_DISPLAY:-}" ]]; then
    echo "ERROR: no interactive display is available. Run this option from a desktop session; the smoke test already ran offscreen." >&2
    exit 13
  fi
  exec "${BUILD_DIR}/HeartflameForge"
fi
