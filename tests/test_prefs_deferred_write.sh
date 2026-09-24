#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TASK_TMP="$(mktemp -d)"
trap 'rm -rf "$TASK_TMP"' EXIT
for signature in 'void Prefs::RequestWrite()' 'bool Prefs::FlushPendingWrite('; do
  python3 "$ROOT/tests/extract_test_function.py" "$ROOT/source/settings/prefs.cpp" "$signature" >> "$TASK_TMP/prefs_deferred.inc"
done
"${CXX:-c++}" -std=c++11 -I"$TASK_TMP" "$ROOT/tests/test_prefs_deferred_write.cpp" -o "$TASK_TMP/test"
"$TASK_TMP/test"
