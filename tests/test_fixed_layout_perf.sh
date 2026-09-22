#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TASK_TMP="$(mktemp -d)"
trap 'rm -rf "$TASK_TMP"' EXIT
"${CXX:-c++}" -std=c++11 -pthread -DDSLIBRIS_DEBUG -DFIXED_PERF_HOST_TEST -I"$ROOT/tests/stubs/perf" -I"$ROOT/include" "$ROOT/tests/test_fixed_layout_perf.cpp" "$ROOT/source/shared/fixed_layout_perf.cpp" -o "$TASK_TMP/test"
"$TASK_TMP/test"
