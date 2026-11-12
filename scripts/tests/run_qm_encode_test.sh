#!/usr/bin/env bash
# OOT D.E.: builds and runs the host test of QM_Encode with the function taken from the game source. Needs gcc/cc and
# Python 3.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/soh/src/overlays/misc/ovl_kaleido_scope/questmenu/questmenu_gfx.c"
DATA="$ROOT/soh/src/overlays/misc/ovl_kaleido_scope/questmenu/questmenu_data.c"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
# From the QM_EMIT1 macro to the #undef after QM_Encode.
awk '/^#define QM_EMIT1/{on=1} on{print} /^#undef QM_EMIT2/{exit}' "$SRC" > "$OUT/qm_encode_extracted.inc"
grep -q "s32 QM_Encode" "$OUT/qm_encode_extracted.inc" || { echo "QM_Encode not found in $SRC"; exit 1; }
# Every C string literal of the quest tables (names, descriptions, objectives), one per line.
python3 - "$DATA" > "$OUT/texts.txt" <<'PY'
import re, sys
src = open(sys.argv[1], encoding="utf-8").read()
for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', src):
    t = m.group(1)
    if t and not t.startswith(("textures/", "__OTR__")):
        print(t.encode().decode("unicode_escape").encode("latin-1").decode("utf-8"))
PY
cc -std=c99 -Wall -Wextra -Wno-unused-label -fsanitize=address,undefined -g -I"$OUT" \
    "$ROOT/scripts/tests/qm_encode_test.c" -o "$OUT/qm_encode_test"
"$OUT/qm_encode_test" "$OUT/texts.txt"
