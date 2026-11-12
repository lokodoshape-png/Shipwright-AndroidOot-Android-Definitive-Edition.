#!/usr/bin/env bash
# OOT D.E.: checks that "otr_add_custom.py --verify" accepts the committed soh.otr and refuses broken copies of it.
set -uo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OTR="$ROOT/Android/app/src/main/assets/soh.otr"
TOOL="$ROOT/scripts/otr_add_custom.py"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
failures=0
expect() { # expect <0|1> <description> <otr file>
    python3 -I "$TOOL" --verify --otr "$3" > "$OUT/log" 2>&1
    rc=$?
    if { [ "$1" = 0 ] && [ $rc = 0 ]; } || { [ "$1" = 1 ] && [ $rc != 0 ]; }; then
        echo "PASS $2"
    else
        echo "FAIL $2 (exit $rc)"; cat "$OUT/log"; failures=$((failures + 1))
    fi
}
expect 0 "committed soh.otr is valid" "$OTR"
expect 1 "missing file refused" "$OUT/none.otr"
head -c $(( $(stat -c %s "$OTR") / 2 )) "$OTR" > "$OUT/half.otr"
expect 1 "truncated file refused" "$OUT/half.otr"
head -c 4096 /dev/urandom > "$OUT/random.otr"
expect 1 "non-MPQ file refused" "$OUT/random.otr"
# Corrupt the data of the first file in the archive (offset 32 is the start of the file data).
cp "$OTR" "$OUT/corrupt.otr"
printf '\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff' | dd of="$OUT/corrupt.otr" bs=1 seek=40 conv=notrunc status=none
expect 1 "corrupted file data refused" "$OUT/corrupt.otr"
[ $failures = 0 ] && echo "ALL TESTS PASSED" || { echo "$failures TEST(S) FAILED"; exit 1; }
