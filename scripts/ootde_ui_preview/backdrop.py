#!/usr/bin/env python3
"""Stand-in backdrop for the interface preview: a dusk sky over a field, castle and mountains, drawn from code.

In the game the menu is drawn over the title screen running live (Link riding through Hyrule Field); this picture
only gives the preview something similar behind the menu. It is not shipped with the game.
"""
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

W, H = int(sys.argv[2]), int(sys.argv[3])
rng = np.random.default_rng(7)

y = np.linspace(0, 1, H)[:, None]
x = np.linspace(0, 1, W)[None, :]
top = np.array([24, 26, 70])
mid = np.array([148, 82, 120])
low = np.array([246, 168, 96])
img = np.zeros((H, W, 3))
for c in range(3):
    tc = np.clip(y / 0.62, 0, 1)
    col = np.where(tc < 0.55, top[c] + (mid[c] - top[c]) * (tc / 0.55), mid[c] + (low[c] - mid[c]) * ((tc - 0.55) / 0.45))
    img[:, :, c] = col
# Sun glow low on the right.
sx, sy = 0.68, 0.58
d = np.sqrt(((x - sx) * W / H) ** 2 + (y - sy) ** 2)
img += (np.exp(-d * 5.0)[:, :, None] * np.array([255, 190, 120])[None, None, :] * 0.55)
# Stars.
for _ in range(260):
    px, py = rng.integers(0, W), rng.integers(0, int(H * 0.35))
    b = rng.uniform(120, 255)
    img[py, px] = np.maximum(img[py, px], [b, b, b * 1.05])
im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
draw = ImageDraw.Draw(im, "RGBA")


def ridge(base, amp, color, seed, rough=6):
    r = np.random.default_rng(seed)
    pts = []
    n = 60
    vals = r.uniform(-1, 1, n)
    for k in range(rough):
        vals = (vals + np.roll(vals, 1) + np.roll(vals, -1)) / 3
    for i in range(n):
        pts.append((i * W / (n - 1), H * (base - amp * vals[i] * 3)))
    pts += [(W, H), (0, H)]
    draw.polygon(pts, fill=color)


ridge(0.60, 0.05, (92, 70, 110, 255), 1)
# Volcano on the right (Death Mountain-like).
draw.polygon([(W * 0.78, H * 0.62), (W * 0.88, H * 0.34), (W * 0.92, H * 0.34), (W * 1.02, H * 0.62)],
             fill=(84, 58, 88, 255))
# Castle silhouette.
cx, cy = W * 0.56, H * 0.60
col = (58, 44, 78, 255)
draw.rectangle([cx - W * 0.05, cy - H * 0.07, cx + W * 0.05, cy], fill=col)
for dx, hh in ((-0.045, 0.14), (-0.02, 0.19), (0.0, 0.25), (0.022, 0.17), (0.045, 0.13)):
    tx = cx + W * dx
    draw.rectangle([tx - W * 0.007, cy - H * hh, tx + W * 0.007, cy], fill=col)
    draw.polygon([(tx - W * 0.010, cy - H * hh), (tx + W * 0.010, cy - H * hh), (tx, cy - H * (hh + 0.06))], fill=col)
ridge(0.66, 0.035, (52, 70, 60, 255), 2)
ridge(0.76, 0.03, (40, 66, 44, 255), 3)
ridge(0.88, 0.025, (26, 44, 30, 255), 4)
im = im.filter(ImageFilter.GaussianBlur(1.2))
# Mist.
fog = Image.new("RGBA", (W, H), (0, 0, 0, 0))
fd = ImageDraw.Draw(fog)
for i in range(8):
    yy = H * (0.6 + i * 0.03)
    fd.ellipse([-W * 0.2 + i * 90, yy - H * 0.02, W * 1.2, yy + H * 0.03], fill=(255, 200, 170, 14))
im = Image.alpha_composite(im.convert("RGBA"), fog.filter(ImageFilter.GaussianBlur(30)))
im.save(sys.argv[1])
