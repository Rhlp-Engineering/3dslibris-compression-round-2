#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TASK_TMP="$(mktemp -d)"
trap 'rm -rf "$TASK_TMP"' EXIT
"${CXX:-c++}" -std=c++11 -I"$ROOT/include" "$ROOT/tests/test_prefs_file_utils.cpp" "$ROOT/source/settings/prefs_file_utils.cpp" -o "$TASK_TMP/test"
"$TASK_TMP/test"
