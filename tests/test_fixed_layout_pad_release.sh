#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TASK_TMP="$(mktemp -d)"
trap 'rm -rf "$TASK_TMP"' EXIT
python3 - "$ROOT" "$TASK_TMP" <<'PY'
from pathlib import Path
import sys
s=(Path(sys.argv[1])/'source/reader/fixed_layout_reader_input.cpp').read_text()
p=Path(sys.argv[2])
p.joinpath('pad_helpers.inc').write_text(s[s.index('namespace {'):s.index('namespace fixed_layout_input {')])
p.joinpath('pad_block.inc').write_text(s[s.index('  // Circle Pad / C-Stick'):s.index('  if (!status_dirty &&')])
PY
"${CXX:-c++}" -std=c++11 -I"$TASK_TMP" "$ROOT/tests/test_fixed_layout_pad_release.cpp" -o "$TASK_TMP/test"
"$TASK_TMP/test"
