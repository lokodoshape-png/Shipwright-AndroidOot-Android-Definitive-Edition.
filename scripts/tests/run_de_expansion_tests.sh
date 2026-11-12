#!/usr/bin/env bash
# OOT D.E. Definitive Expansion: builds and runs the host tests of the expansion core (settings, safe file writes,
# save manager) with the same sources the game compiles. Needs g++ (C++17). nlohmann/json is taken from
# NLOHMANN_INCLUDE, /usr/include or downloaded (v3.11.3, the version libultraship fetches).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DE="$ROOT/soh/soh/Enhancements/DefinitiveExpansion"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
INC="${NLOHMANN_INCLUDE:-}"
if [ -z "$INC" ]; then
    if [ -f /usr/include/nlohmann/json.hpp ]; then
        INC=/usr/include
    else
        mkdir -p "$OUT/inc/nlohmann"
        curl -sSfL -o "$OUT/inc/nlohmann/json.hpp" \
            https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp
        INC="$OUT/inc"
    fi
fi
g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g -I"$INC" -I"$DE" \
    "$ROOT/scripts/tests/de_expansion_test.cpp" "$DE/DEStorage.cpp" "$DE/OOTDEConfig.cpp" "$DE/DESaveManager.cpp" \
    -o "$OUT/de_expansion_test" 2>&1
"$OUT/de_expansion_test" "$OUT/work"
