@echo off
setlocal
cd /d "%~dp0"
if not exist "%~dp0VC_redist.x64.exe" (
  echo The Microsoft Visual C++ Runtime installer is not bundled here.
  echo Get the x64 installer from Microsoft's official page:
  echo https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist
  pause
  exit /b 2
)
start /wait "Microsoft Visual C++ Redistributable" "%~dp0VC_redist.x64.exe"
set RESULT=%ERRORLEVEL%
if not "%RESULT%"=="0" echo Runtime installer returned code %RESULT%.
if not "%RESULT%"=="0" pause
exit /b %RESULT%
