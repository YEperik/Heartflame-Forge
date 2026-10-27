[CmdletBinding()]
param(
    [switch]$Run,
    [string]$BuildDirectory = "build-win"
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw 'CMake is required.' }
if (-not $env:QT_ROOT_DIR -or -not (Test-Path (Join-Path $env:QT_ROOT_DIR 'lib\cmake\Qt6'))) {
    throw 'Set QT_ROOT_DIR to the Qt 6 MSVC x64 root (the folder containing lib\cmake\Qt6).'
}
if (-not $env:VCPKG_ROOT -or -not (Test-Path (Join-Path $env:VCPKG_ROOT 'scripts\buildsystems\vcpkg.cmake'))) {
    throw 'Set VCPKG_ROOT to a vcpkg checkout.'
}

$build = Join-Path $root $BuildDirectory
$toolchain = Join-Path $env:VCPKG_ROOT 'scripts\buildsystems\vcpkg.cmake'
$configureArgs = @(
    '-S', '.', '-B', $build,
    '-G', 'Visual Studio 17 2022', '-A', 'x64',
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    '-DVCPKG_TARGET_TRIPLET=x64-windows-static-md',
    "-DVCPKG_OVERLAY_TRIPLETS=$root\triplets",
    "-DVCPKG_INSTALLED_DIR=$root\vcpkg_installed",
    "-DCMAKE_PREFIX_PATH=$env:QT_ROOT_DIR",
    '-DPS2ASSET_BUILD_GUI=ON',
    '-DPS2ASSET_REQUIRE_GUI=ON',
    '-DPS2ASSET_BUILD_SAMPLE_PLUGIN=ON',
    '-DBUILD_TESTING=ON'
)
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& cmake --build $build --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$env:QT_QPA_PLATFORM = 'offscreen'
& ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$exe = Join-Path $build 'Release\HeartflameForge.exe'
if (-not (Test-Path $exe)) { throw "GUI executable is missing: $exe" }
& $exe --smoke-test
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host 'Heartflame Forge GUI build and startup smoke test passed.' -ForegroundColor Green
if ($Run) {
    Remove-Item Env:\QT_QPA_PLATFORM -ErrorAction SilentlyContinue
    Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
}
