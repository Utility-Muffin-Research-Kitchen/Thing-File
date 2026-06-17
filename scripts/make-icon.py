#!/usr/bin/env python3
"""Build the File Explorer pak icon: the gray folder with the Leaf mark inside it
and a soft green glow behind the leaf. Composited from the two source artworks
(folder-source.png + leaf-source.png), centered on a 256x256 square.
Output: res/icon.png.

Regenerate:  python3 scripts/make-icon.py
"""
import os
from PIL import Image, ImageFilter, ImageChops

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FOLDER = os.path.join(HERE, "folder-source.png")
LEAF   = os.path.join(HERE, "leaf-source.png")
OUT    = os.path.join(ROOT, "res", "icon.png")

SS = 512               # supersample, downscaled to 256 at the end (smooth glow)
MARGIN = 28            # folder breathing room (at SS)
LEAF_FRAC = 0.72       # leaf height as a fraction of folder height (fills most of it)
LEAF_CY_FRAC = 0.55    # leaf center, fraction down the folder (sits below the tab)
GLOW_COLOR = (95, 205, 85)
GLOW_BLUR = 34
GLOW_PASSES = 2        # composite the blurred halo N times for intensity


def load(path):
    im = Image.open(path).convert("RGBA")
    return im.crop(im.getbbox())


folder = load(FOLDER)
leaf = load(LEAF)

# Folder: centered, scaled to fit the canvas.
fit = SS - 2 * MARGIN
s = fit / max(folder.width, folder.height)
fw, fh = round(folder.width * s), round(folder.height * s)
folder = folder.resize((fw, fh), Image.LANCZOS)
fx, fy = (SS - fw) // 2, (SS - fh) // 2

canvas = Image.new("RGBA", (SS, SS), (0, 0, 0, 0))
canvas.alpha_composite(folder, (fx, fy))

# Leaf: scaled to a fraction of the folder height, centered in the folder body.
lh = round(fh * LEAF_FRAC)
ls = lh / leaf.height
lw = round(leaf.width * ls)
leaf = leaf.resize((lw, lh), Image.LANCZOS)
lcx = fx + fw // 2
lcy = fy + round(fh * LEAF_CY_FRAC)
lx, ly = lcx - lw // 2, lcy - lh // 2

# Green glow: the leaf silhouette filled green, blurred, then SCREEN-blended onto
# the folder so it adds light (a luminous halo) instead of darkening the gray.
glow = Image.new("RGB", (SS, SS), (0, 0, 0))
glow.paste(Image.new("RGB", (lw, lh), GLOW_COLOR), (lx, ly), leaf.split()[3])
glow = glow.filter(ImageFilter.GaussianBlur(GLOW_BLUR))
base = canvas.convert("RGB")
for _ in range(GLOW_PASSES):
    base = ImageChops.screen(base, glow)
canvas = Image.merge("RGBA", (*base.split(), canvas.getchannel("A")))

# Leaf on top.
canvas.alpha_composite(leaf, (lx, ly))

# Crop tight to the folder (the glow is contained within the folder's alpha), then
# downscale so the longest side is 256 — the folder fills the icon, no square margin.
icon = canvas.crop(canvas.getbbox())
sc = 256 / max(icon.width, icon.height)
icon = icon.resize((round(icon.width * sc), round(icon.height * sc)), Image.LANCZOS)
os.makedirs(os.path.dirname(OUT), exist_ok=True)
icon.save(OUT)
print("wrote", OUT, icon.size)
