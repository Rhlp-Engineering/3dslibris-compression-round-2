#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TASK_TMP="$(mktemp -d)"
trap 'rm -rf "$TASK_TMP"' EXIT
for signature in 'bool EnsureMuPdfDisplayListForPage(' 'bool EnsureCurrentMuPdfPreviewCache('; do
  python3 "$ROOT/tests/extract_test_function.py" "$ROOT/source/formats/mupdf/mupdf_worker.cpp" "$signature" >> "$TASK_TMP/mupdf_preview_under_test.inc"
done
python3 "$ROOT/tests/extract_test_function.py" "$ROOT/source/formats/mupdf/mupdf_viewport.cpp" "static void ResetMuPdfDeferredCachesForSynchronousRender(" >> "$TASK_TMP/mupdf_preview_under_test.inc"
"${CXX:-c++}" -std=c++11 -I"$TASK_TMP" "$ROOT/tests/test_mupdf_preview_reuse.cpp" -o "$TASK_TMP/test"
"$TASK_TMP/test"
