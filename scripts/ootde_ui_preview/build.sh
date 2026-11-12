#!/usr/bin/env bash
# OOT D.E.: builds the interface preview and renders the screens to PNG (needs g++, Python 3 with Pillow and numpy).
#
#   scripts/ootde_ui_preview/build.sh <imgui folder (v1.91.6-docking)> <output folder> [width height]
#
# The game uses Dear ImGui v1.91.6-docking (libultraship); clone it with
#   git clone --depth 1 -b v1.91.6-docking https://github.com/ocornut/imgui.git
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../.." && pwd)"
imgui="$(cd "$1" && pwd)"
out="$(mkdir -p "$2" && cd "$2" && pwd)"
width="${3:-2400}"
height="${4:-1080}"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

mkdir -p "$work/raw/fonts"
cp "$root/soh/assets/custom/fonts/"{Cinzel-Bold,Cinzel-Regular,EBGaramond-Medium}.ttf "$work/raw/fonts/"
python3 "$here/backdrop.py" "$work/backdrop.png" "$width" "$height"
python3 - "$root" "$work" <<'PY'
import glob, os, sys
from PIL import Image
root, work = sys.argv[1], sys.argv[2]
def raw(src, dst):
    im = Image.open(src).convert("RGBA")
    with open(dst, "wb") as f:
        f.write(f"{im.width} {im.height}\n".encode())
        f.write(im.tobytes())
for png in glob.glob(os.path.join(root, "soh/assets/custom/ootde/ui/*/*.rgba32.png")):
    raw(png, os.path.join(work, "raw", os.path.basename(png).split(".")[0] + ".raw"))
raw(os.path.join(work, "backdrop.png"), os.path.join(work, "raw", "background.raw"))
PY

src="$root/soh/soh/Enhancements/DefinitiveUI"
g++ -O2 -std=c++20 -DIMGUI_DEFINE_MATH_OPERATORS -I"$imgui" -I"$src" \
    "$here/preview.cpp" "$src/DeTheme.cpp" "$src/DeTitleMenuView.cpp" \
    "$imgui/imgui.cpp" "$imgui/imgui_draw.cpp" "$imgui/imgui_widgets.cpp" "$imgui/imgui_tables.cpp" \
    -o "$work/preview"
"$work/preview" "$work/raw" "$work" "$width" "$height"
python3 - "$work" "$out" <<'PY'
import glob, os, sys
from PIL import Image
for ppm in glob.glob(os.path.join(sys.argv[1], "*.ppm")):
    dst = os.path.join(sys.argv[2], os.path.basename(ppm)[:-4] + ".png")
    Image.open(ppm).save(dst, optimize=True)
    print("saved", dst)
PY
