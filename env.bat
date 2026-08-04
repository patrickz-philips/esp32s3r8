@echo off
setlocal
set "PROJECT_DIR=%~dp0"

copy /Y "%PROJECT_DIR%.vscode\settings_win.json" "%PROJECT_DIR%.vscode\settings.json" >nul
if errorlevel 1 (
  echo Failed to apply Windows VS Code settings.
  exit /b 1
)

echo Applied Windows VS Code settings.