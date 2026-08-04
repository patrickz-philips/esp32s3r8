@echo off
set "PROJECT_DIR=%~dp0"

copy /Y "%PROJECT_DIR%.vscode\settings_win.json" "%PROJECT_DIR%.vscode\settings.json" >nul
if errorlevel 1 (
  echo Failed to apply Windows VS Code settings.
  exit /b 1
)

set "IDF_PATH=C:\Espressif\frameworks\esp-idf-v5.5.4"

echo Applied Windows VS Code settings.
echo IDF_PATH=%IDF_PATH%