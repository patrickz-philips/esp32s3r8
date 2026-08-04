#!/usr/bin/env sh
set -eu

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cp "$PROJECT_DIR/.vscode/settings_mac.json" "$PROJECT_DIR/.vscode/settings.json"

printf '%s\n' "Applied macOS VS Code settings."