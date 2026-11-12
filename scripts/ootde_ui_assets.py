#!/usr/bin/env python3
"""OOT D.E.: generates the textures of the Definitive Edition interface (soh/assets/custom/ootde/ui/).

Every texture is drawn here from code (noise, gradients and shapes), so they can be rebuilt or tweaked at any time:

    python3 scripts/ootde_ui_assets.py
    python3 scripts/otr_add_custom.py      # then add them to the soh.otr of the Android app

All of them are white or grey RGBA32 images: the interface tints them with the colors of the theme when it draws them,
so one texture serves several colors. Needs Pillow and numpy.
"""
import os

import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "soh", "assets", "custom", "ootde", "ui")

RNG = np.random.default_rng(1998)


def save(category, name, rgba):
    folder = os.path.join(OUT, category)
    os.makedirs(folder, exist_ok=True)
    img = Image.fromarray(np.clip(rgba * 255.0 + 0.5, 0, 255).astype(np.uint8), "RGBA")
    path = os.path.join(folder, f"{name}.rgba32.png")
    img.save(path, optimize=True)
    print("wrote", os.path.relpath(path, ROOT), img.size)


def value_noise(size, cells):
    """Smooth tileable value noise in [0, 1]."""
    grid = RNG.random((cells, cells))
    t = np.linspace(0, cells, size, endpoint=False)
    i0 = np.floor(t).astype(int)
    f = t - i0
    f = f * f * (3 - 2 * f)
    i1 = (i0 + 1) % cells
    a = grid[np.ix_(i0, i0)]
    b = grid[np.ix_(i0, i1)]
    c = grid[np.ix_(i1, i0)]
    d = grid[np.ix_(i1, i1)]
    fx = f[None, :]
    fy = f[:, None]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def fbm(size, base, octaves, gain=0.5):
    total = np.zeros((size, size))
    amp = 1.0
    norm = 0.0
    cells = base
    for _ in range(octaves):
        total += value_noise(size, cells) * amp
        norm += amp
        amp *= gain
        cells *= 2
    return total / norm


def stone(size=256):
    """Dark slate: layered noise, soft veins and a little grain. Grey; tinted blue/graphite when drawn."""
    n = fbm(size, 4, 6)
    veins = np.abs(fbm(size, 3, 5) - 0.5)
    veins = np.clip(1.0 - veins * 9.0, 0, 1) ** 3
    grain = RNG.random((size, size))
    v = 0.62 + (n - 0.5) * 0.55 - veins * 0.22 + (grain - 0.5) * 0.06
    v = np.clip(v, 0, 1)
    rgba = np.dstack([v, v, v * 1.03, np.ones_like(v)])
    return np.clip(rgba, 0, 1)


def glow(size=128):
    """Soft round glow (white, alpha falls off smoothly to zero at the edge)."""
    y, x = np.mgrid[0:size, 0:size]
    c = (size - 1) / 2
    r = np.sqrt((x - c) ** 2 + (y - c) ** 2) / c
    a = np.clip(1 - r, 0, 1) ** 2.4
    a += np.exp(-(r ** 2) / 0.02) * 0.35
    a = np.clip(a, 0, 1)
    one = np.ones_like(a)
    return np.dstack([one, one, one, a])


def spark(size=64):
    """Four-point star twinkle."""
    y, x = np.mgrid[0:size, 0:size]
    c = (size - 1) / 2
    dx = np.abs(x - c) / c
    dy = np.abs(y - c) / c
    ray_h = np.exp(-dy * 26) * np.clip(1 - dx, 0, 1) ** 1.5
    ray_v = np.exp(-dx * 26) * np.clip(1 - dy, 0, 1) ** 1.5
    r = np.sqrt(dx ** 2 + dy ** 2)
    core = np.exp(-(r ** 2) / 0.012)
    halo = np.clip(1 - r, 0, 1) ** 3 * 0.35
    a = np.clip(ray_h + ray_v + core + halo, 0, 1)
    one = np.ones_like(a)
    return np.dstack([one, one, one, a])


def navi_wing(w=96, h=64):
    """One fairy wing, pointing right from its root on the left edge: translucent, brighter rim and veins."""
    y, x = np.mgrid[0:h, 0:w].astype(float)
    u = x / (w - 1)
    v = (y - (h - 1) / 2) / ((h - 1) / 2)
    # Teardrop: widest at 60% of the length.
    half = np.sin(np.clip(u, 0, 1) * np.pi) ** 0.8 * (0.35 + 0.65 * np.clip(u / 0.6, 0, 1))
    half = np.maximum(half, 1e-3)
    d = np.abs(v) / half
    inside = np.clip((1 - d) * 6, 0, 1) * (u > 0.02)
    rim = np.exp(-((1 - d) ** 2) / 0.02) * inside
    veins = np.zeros_like(u)
    for k in (-0.55, 0.0, 0.55):
        veins += np.exp(-((v / half - k * u) ** 2) / 0.004) * np.clip(u * 1.4, 0, 1)
    a = inside * (0.28 + 0.45 * rim + 0.35 * np.clip(veins, 0, 1)) * np.clip(1.15 - u * 0.4, 0, 1)
    a = np.clip(a, 0, 1)
    one = np.ones_like(a)
    return np.dstack([one, one, one, a])


def main():
    save("textures", "gDEStone", stone())
    save("effects", "gDEGlow", glow())
    save("effects", "gDESpark", spark())
    save("decor", "gDENaviWing", navi_wing())


if __name__ == "__main__":
    main()
