#!/usr/bin/env sh
set -eu

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cp "$PROJECT_DIR/.vscode/settings_mac.json" "$PROJECT_DIR/.vscode/settings.json"

IDF_PATH="/Users/patrickz/esp/esp-idf"
export IDF_PATH

printf '%s\n' "Applied macOS VS Code settings."
printf '%s\n' "IDF_PATH=$IDF_PATH"
printf '%s\n' "Run 'source ./env.sh' (or '. ./env.sh') to export IDF_PATH into your shell."
