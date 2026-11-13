#!/usr/bin/env python3
"""OOT D.E.: cuts the art of Rafael's reference image (docs/interface-de/referencia.png) into the high resolution
textures of the Definitive Edition interface (soh/assets/custom/ootde/ui/).

Each piece is enlarged 4x with the Real-ESRGAN x4plus network (scripts/ootde_esrgan_np.py: numpy only, no torch;
the background gets a second pass and is then reduced to 2880 px, so it stays sharp on 1440p screens), then given a
transparent background where it needs one (the buttons and frames were drawn over a dark blue background, the logo over the
leaves of the tree). The button labels are removed: the buttons are rebuilt from their two ends and a slice of their
middle, and the game writes its own text on them.

    pip install opencv-python-headless pillow numpy
    curl -LO https://github.com/xinntao/Real-ESRGAN/releases/download/v0.1.0/RealESRGAN_x4plus.pth
    python3 scripts/ootde_ui_art.py RealESRGAN_x4plus.pth
    python3 scripts/otr_add_custom.py
"""
import os
import sys

import cv2

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ootde_esrgan_np as esrgan  # noqa: E402
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = os.path.join(ROOT, "docs", "interface-de", "referencia.png")
OUT = os.path.join(ROOT, "soh", "assets", "custom", "ootde", "ui")


def save(category, name, rgba):
    folder = os.path.join(OUT, category)
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, f"{name}.rgba32.png")
    Image.fromarray(rgba, "RGBA").save(path, optimize=True)
    print("wrote", os.path.relpath(path, ROOT), rgba.shape[1], "x", rgba.shape[0])


def upscale(sr, img, box, passes=1):
    """Crop of the reference (BGR, as OpenCV reads it) enlarged 4x per pass; returns RGB 0..255 float."""
    x0, y0, x1, y1 = box
    rgb = cv2.cvtColor(np.ascontiguousarray(img[y0:y1, x0:x1]), cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
    for _ in range(passes):
        rgb = esrgan.upscale(sr, rgb)
    return rgb * 255.0


def resize(rgb, w, h):
    return cv2.resize(rgb, (w, h), interpolation=cv2.INTER_AREA)


def fill_inside(mask):
    """Pixels enclosed by the mask (the inside of a frame) become part of it."""
    h, w = mask.shape
    flood = np.where(mask, 0, 255).astype(np.uint8)
    pad = np.zeros((h + 2, w + 2), np.uint8)
    cv2.floodFill(flood, pad, (0, 0), 128)
    return mask | (flood == 255)


# Color of the dark blue background behind the buttons and frames of the reference (sampled around them).
PANEL_BG = np.array([8.0, 13.0, 28.0])


def feather_edges(alpha, width):
    h, w = alpha.shape
    ramp_x = np.clip(np.minimum(np.arange(w), np.arange(w)[::-1]) / width, 0, 1)
    ramp_y = np.clip(np.minimum(np.arange(h), np.arange(h)[::-1]) / width, 0, 1)
    return alpha * ramp_y[:, None] * ramp_x[None, :]


def matte_over_background(rgb, bg=PANEL_BG):
    """Alpha for art drawn over the plain dark background: distance to the background color, solid inside the
    shape. The color is un-mixed from the background where it is translucent (the blue glow)."""
    dist = np.sqrt(((rgb - bg) ** 2).sum(axis=2))
    alpha = np.clip((dist - 22.0) / 70.0, 0, 1)
    inside = fill_inside(alpha > 0.6)
    inside = cv2.erode(inside.astype(np.uint8), np.ones((5, 5), np.uint8)).astype(bool)
    alpha = np.maximum(alpha, inside.astype(np.float32))
    alpha = feather_edges(alpha, 24)
    a = np.maximum(alpha, 1e-3)[:, :, None]
    color = np.clip((rgb - bg * (1 - a)) / a, 0, 255)
    return np.dstack([color, alpha * 255]).astype(np.uint8)


def matte_logo(rgb):
    """The logo sits on dark leaves: shown through a soft rounded window, with the bright parts (letters, shield,
    gold) kept solid up to the edges so nothing of the logo is cut."""
    h, w, _ = rgb.shape
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    nx = np.abs(x - (w - 1) / 2) / ((w - 1) / 2)
    ny = np.abs(y - (h - 1) / 2) / ((h - 1) / 2)
    r = (nx ** 6 + ny ** 6) ** (1 / 6)
    window = np.clip((1.0 - r) / 0.22, 0, 1) ** 1.5
    hsv = cv2.cvtColor(rgb.astype(np.uint8), cv2.COLOR_RGB2HSV).astype(np.float32)
    s, v = hsv[:, :, 1] / 255, hsv[:, :, 2] / 255
    hue = hsv[:, :, 0] * 2
    keep = (((hue < 25) | (hue > 330)) & (s > 0.45) & (v > 0.35)) | ((v > 0.75) & (s < 0.35)) | \
           ((hue > 25) & (hue < 60) & (s > 0.35) & (v > 0.6)) | ((hue > 190) & (hue < 260) & (s > 0.4) & (v > 0.3))
    keep = cv2.GaussianBlur(cv2.dilate(keep.astype(np.uint8), np.ones((7, 7), np.uint8)).astype(np.float32), (0, 0), 3)
    alpha = np.clip(np.maximum(window, keep * feather_edges(np.ones((h, w), np.float32), 6)), 0, 1)
    return np.dstack([rgb, alpha * 255]).astype(np.uint8)


def three_slice(rgba, cap_l, cap_r, mid_x, mid_w=24):
    """Button without its label: left end, a clean slice of the middle, right end."""
    mid = rgba[:, mid_x:mid_x + 2]
    mid = np.repeat(mid[:, :1], mid_w, axis=1)
    return np.concatenate([rgba[:, :cap_l], mid, rgba[:, -cap_r:]], axis=1)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    state = esrgan.load_pth(sys.argv[1])
    sr = state.get("params_ema", state.get("params", state))
    ref = cv2.imread(REF)
    if ref is None:
        sys.exit(f"missing {REF}")

    # Background: Hyrule at dusk, the castle, Link with the Hylian Shield and Navi (top banner of the reference).
    bg = upscale(sr, ref, (780, 0, 1190, 178), passes=2)  # 6560 x 2848
    bg = resize(bg, 2880, 1250)
    save("backgrounds", "gDETitleBg", np.dstack([bg, np.full(bg.shape[:2], 255)]).astype(np.uint8))

    # Logo with "DEFINITIVE EDITION" (top left of the reference).
    logo = upscale(sr, ref, (104, 0, 590, 168))
    # 1944 x 672: kept at the size the network gives (the menu draws it about 700 px wide, the splash about 1400).
    save("decor", "gDELogo", matte_logo(logo))

    # Buttons (normal, selected, pressed) and the frame of the "Elementos visuais" panel.
    for name, box, caps in (
        ("gDEButtonNormal", (1192, 662, 1358, 696), (100, 100)),
        ("gDEButtonSelected", (1188, 697, 1364, 731), (108, 108)),
        ("gDEButtonPressed", (1192, 733, 1358, 767), (100, 100)),
    ):
        up = upscale(sr, ref, box)
        rgba = matte_over_background(up)
        save("buttons", name, three_slice(rgba, caps[0], caps[1], caps[0] + 6))
    frame = upscale(sr, ref, (1372, 704, 1522, 762))
    save("frames", "gDEFrame", matte_over_background(frame))


if __name__ == "__main__":
    main()
