#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TASK_TMP="$(mktemp -d)"
trap 'rm -rf "$TASK_TMP"' EXIT
python3 "$ROOT/tests/extract_test_function.py" "$ROOT/source/formats/mupdf/mupdf_worker.cpp" "void CancelMuPdfIncrementalRenderState(" >> "$TASK_TMP/mupdf_cancel_under_test.inc"
"${CXX:-c++}" -std=c++11 -I"$TASK_TMP" "$ROOT/tests/test_mupdf_worker_cancel.cpp" -o "$TASK_TMP/test"
"$TASK_TMP/test"
