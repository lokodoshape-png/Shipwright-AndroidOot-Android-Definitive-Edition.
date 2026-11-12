#!/usr/bin/env bash
# OOT D.E.: runs the computer-side tests of the Android file helpers (SafeFiles.java). Needs a JDK and Python 3.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
javac -encoding UTF-8 -d "$OUT" "$ROOT/Android/app/src/main/java/com/dishii/soh/SafeFiles.java" \
    "$ROOT/scripts/tests/java/SafeFilesTest.java"
# Reference CRC32C of the test's fake 32 MB ROM, computed independently in Python.
java -cp "$OUT" com.dishii.soh.SafeFilesTest --make-rom "$OUT/ref.z64"
REF=$(python3 "$ROOT/scripts/tests/crc32c_reference.py" "$OUT/ref.z64")
java -cp "$OUT" com.dishii.soh.SafeFilesTest "$REF" "$ROOT/Android/app/src/main/assets/soh.otr"
