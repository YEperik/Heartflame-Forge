@echo off
setlocal
cd /d "%~dp0"
if not exist "%~dp0HeartflameForge.exe" (
  echo HeartflameForge.exe was not found. Extract the complete package first.
  pause
  exit /b 2
)
start "Heartflame Forge" "%~dp0HeartflameForge.exe"
endlocal
