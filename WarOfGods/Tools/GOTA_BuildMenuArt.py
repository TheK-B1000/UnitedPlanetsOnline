# Gods of the Arena — offline menu art builder (NOT run inside Unreal).
# Produces UI source PNGs from the UPG reference + original generated art.
#
# Requires: Pillow, numpy, opencv-python-headless (any CPython 3.10+).
#   python Tools/GOTA_BuildMenuArt.py <fonts_dir>
# Output: Content source PNGs in Tools/MenuArtSource/ (imported by GOTA_BuildMenuUI.py).
#
# The reference is a flattened concept. Baked text is removed (inpainted) from the
# backdrop clean plate; live UMG widgets are drawn on top at the same positions.

import math
import os
import sys

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF = os.path.join(ROOT, "Docs", "References", "GOTA_UPG_MainMenu_Visual_Reference.png")
OUT = os.path.join(ROOT, "Tools", "MenuArtSource")
FONTS = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "Tools", "MenuFonts")

os.makedirs(OUT, exist_ok=True)

src = cv2.imread(REF, cv2.IMREAD_COLOR)  # BGR, 1672x941
H, W = src.shape[:2]


def lum(img):
    return cv2.cvtColor(img, cv2.COLOR_BGR2GRAY).astype(np.float32)


def box_mask(mask, box, cond):
    x0, y0, x1, y1 = box
    mask[y0:y1, x0:x1] |= cond[y0:y1, x0:x1].astype(np.uint8) * 255


def save_bgr(img, name):
    cv2.imwrite(os.path.join(OUT, name), img)


# ---------------------------------------------------------------- clean plate
L = lum(src)
b, g, r = [src[..., i].astype(np.int32) for i in range(3)]
gold = (r > g) & (g > b) & ((r - b) > 40) & (L > 70)
bright = lambda t: L > t

mask = np.zeros((H, W), np.uint8)
# Region A: insignia + identity text
box_mask(mask, (44, 8, 136, 110), bright(45))
box_mask(mask, (136, 20, 350, 92), bright(80))
# Region C: nav icons, labels, continue frame
box_mask(mask, (54, 152, 110, 680), bright(75))
box_mask(mask, (110, 152, 345, 680), bright(95))
frame = np.zeros((H, W), bool)
frame[142:224, 38:382] = True
frame[159:207, 54:366] = False
box_mask(mask, (36, 140, 384, 226), frame & bright(38))
# Region B: title logotype + subtitle + ornament
title_core = np.zeros((H, W), np.uint8)
box_mask(title_core, (640, 30, 1052, 360), gold | bright(215))
# close letter counters + dark bevel outlines so no ghost survives
title_core = cv2.morphologyEx(title_core, cv2.MORPH_CLOSE, np.ones((23, 23), np.uint8))
title_core = cv2.dilate(title_core, np.ones((11, 11), np.uint8))
mask |= title_core
box_mask(mask, (655, 352, 1048, 386), bright(105))
box_mask(mask, (715, 384, 950, 406), bright(95))
# Region E slogan
box_mask(mask, (1410, 596, 1640, 672), bright(105))
mask = cv2.dilate(mask, np.ones((5, 5), np.uint8), iterations=1)
# Region D + E panels: full rect fill (panels are drawn over these areas)
mask[16:88, 1272:1652] = 255
mask[118:200, 1400:1652] = 255

plate = cv2.inpaint(src, mask, 9, cv2.INPAINT_TELEA)

# Calm the baked "continue" highlight fill: pull it toward neighbour rows' tone.
roi = plate[150:216, 46:374].astype(np.float32)
ref_roi = plate[222:288, 46:374].astype(np.float32)
gain = (ref_roi.mean(axis=(0, 1)) + 1) / (roi.mean(axis=(0, 1)) + 1)
patched = np.clip(roi * gain, 0, 255)
feather = np.ones(roi.shape[:2], np.float32)
feather = cv2.GaussianBlur(np.pad(feather[12:-12, 12:-12], 12), (0, 0), 9)[..., None]
plate[150:216, 46:374] = np.clip(patched * feather + roi * (1 - feather), 0, 255).astype(np.uint8)

# Card band: cards are opaque UMG widgets — blur/darken the band so nothing
# baked can peek out at other aspect ratios.
band = plate[712:H].copy()
band = cv2.GaussianBlur(band, (0, 0), 14)
band = (band.astype(np.float32) * 0.55).astype(np.uint8)
plate[712:H] = band
# soften seam
for i in range(24):
    y = 700 + i
    a = i / 24.0
    blurred = cv2.GaussianBlur(plate[y - 2:y + 3], (0, 0), 6)[2]
    plate[y] = (plate[y] * (1 - a) + blurred * a * 0.8 + plate[y] * a * 0.2).astype(np.uint8)

plate_up = cv2.resize(plate, (1920, int(round(H * 1920 / W))), interpolation=cv2.INTER_LANCZOS4)
save_bgr(plate_up, "T_GOTA_Menu_Backdrop_CleanPlate.png")
save_bgr(cv2.cvtColor(mask, cv2.COLOR_GRAY2BGR), "_debug_cleanplate_mask.png")

# ------------------------------------------------------------------ card art
CARDS = {
    "Story": (51, 728, 354, 899),
    "Arena": (371, 728, 669, 899),
    "Multiplayer": (688, 728, 987, 899),
    "Settings": (1004, 728, 1302, 899),
    "Extras": (1320, 728, 1621, 899),
}
for name, (x0, y0, x1, y1) in CARDS.items():
    card = src[y0 + 3:y1 - 3, x0 + 3:x1 - 3].copy()
    cl = lum(card)
    cm = np.zeros(cl.shape, np.uint8)
    hh = card.shape[0]
    cm[hh - 64:, :] = ((cl[hh - 64:, :] > 85)).astype(np.uint8) * 255
    cm = cv2.dilate(cm, np.ones((5, 5), np.uint8))
    card = cv2.inpaint(card, cm, 7, cv2.INPAINT_TELEA)
    card = cv2.resize(card, (card.shape[1] * 2, card.shape[0] * 2), interpolation=cv2.INTER_LANCZOS4)
    save_bgr(card, f"T_GOTA_Card_{name}.png")

# Portrait + Earth thumbnail
por = src[23:80, 1280:1335]
save_bgr(cv2.resize(por, (por.shape[1] * 4, por.shape[0] * 4), interpolation=cv2.INTER_LANCZOS4), "T_GOTA_Portrait_Warden.png")
earth = src[126:194, 1558:1645]
save_bgr(cv2.resize(earth, (earth.shape[1] * 4, earth.shape[0] * 4), interpolation=cv2.INTER_LANCZOS4), "T_GOTA_Earth_Capital.png")


# ------------------------------------------------------------------ icons
def extract_icon(box, name, lo=60, hi=170, scale=4, white=True):
    x0, y0, x1, y1 = box
    crop = src[y0:y1, x0:x1]
    cl = lum(crop)
    a = np.clip((cl - lo) / (hi - lo), 0, 1)
    a = cv2.resize(a, (crop.shape[1] * scale, crop.shape[0] * scale), interpolation=cv2.INTER_CUBIC)
    a = np.clip(a, 0, 1)
    rgba = np.zeros((a.shape[0], a.shape[1], 4), np.uint8)
    if white:
        rgba[..., :3] = 255
    else:
        up = cv2.resize(crop, (a.shape[1], a.shape[0]), interpolation=cv2.INTER_CUBIC)
        rgba[..., 0:3] = up[..., ::-1]
    rgba[..., 3] = (a * 255).astype(np.uint8)
    Image.fromarray(rgba, "RGBA").save(os.path.join(OUT, name))


NAV_ICONS = {
    "Continue": (58, 160, 104, 206),
    "Story": (60, 236, 102, 282),
    "Arena": (60, 312, 102, 358),
    "Multiplayer": (60, 388, 102, 434),
    "Settings": (60, 468, 102, 514),
    "Extras": (60, 546, 102, 592),
    "Quit": (60, 622, 102, 668),
}
for name, bx in NAV_ICONS.items():
    extract_icon(bx, f"T_GOTA_Icon_{name}.png")
extract_icon((1536, 38, 1574, 64), "T_GOTA_Icon_Friends.png")
extract_icon((1418, 140, 1452, 168), "T_GOTA_Icon_UPGMark.png")


# --------------------------------------------- original generated art (PIL)
def gold_gradient(w, h, stops):
    """stops: list of (t, (r,g,b)) vertical gradient."""
    grad = np.zeros((h, w, 3), np.float32)
    ys = np.linspace(0, 1, h)
    for c in range(3):
        grad[..., c] = np.interp(ys, [s[0] for s in stops], [s[1][c] for s in stops])[:, None]
    return Image.fromarray(grad.astype(np.uint8), "RGB")


GOLD_STOPS = [(0.0, (255, 240, 196)), (0.32, (232, 190, 110)), (0.52, (150, 98, 40)),
              (0.62, (205, 156, 80)), (0.85, (250, 222, 150)), (1.0, (170, 118, 52))]


def metal_fill(mask_img, stops=GOLD_STOPS, outline=6, shadow=14, glow=0):
    """mask_img: L mode coverage. Returns RGBA with bevel-ish gold, dark outline, shadow."""
    w, h = mask_img.size
    fill = gold_gradient(w, h, stops)
    # inner bevel highlight from blurred mask gradient
    m = np.asarray(mask_img).astype(np.float32) / 255
    blur = cv2.GaussianBlur(m, (0, 0), 3)
    gy = cv2.Sobel(blur, cv2.CV_32F, 0, 1, ksize=5)
    gx = cv2.Sobel(blur, cv2.CV_32F, 1, 0, ksize=5)
    light = np.clip(-(gy * 0.8 + gx * 0.4) * 0.25, -0.35, 0.55)
    f = np.asarray(fill).astype(np.float32) / 255
    f = np.clip(f * (1 + light[..., None]), 0, 1)
    fill = Image.fromarray((f * 255).astype(np.uint8), "RGB")

    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    if shadow:
        sh = mask_img.filter(ImageFilter.MaxFilter(outline * 2 + 1 if outline else 1)).filter(ImageFilter.GaussianBlur(shadow))
        out.paste(Image.new("RGBA", (w, h), (0, 0, 0, 200)), (0, 0), sh)
    if glow:
        gl = mask_img.filter(ImageFilter.GaussianBlur(glow))
        out.paste(Image.new("RGBA", (w, h), (255, 200, 120, 90)), (0, 0), gl)
    if outline:
        ol = mask_img.filter(ImageFilter.MaxFilter(outline * 2 + 1))
        out.paste(Image.new("RGBA", (w, h), (34, 20, 8, 255)), (0, 0), ol)
    out.paste(fill.convert("RGBA"), (0, 0), mask_img)
    return out


def font(name, size):
    return ImageFont.truetype(os.path.join(FONTS, name), size)


def text_mask(size, items):
    """items: (text, font, center_x, top_y, tracking)."""
    m = Image.new("L", size, 0)
    d = ImageDraw.Draw(m)
    for text, fnt, cx, ty, track in items:
        widths = [d.textlength(ch, font=fnt) for ch in text]
        total = sum(widths) + track * (len(text) - 1)
        x = cx - total / 2
        for ch, cw in zip(text, widths):
            d.text((x, ty), ch, font=fnt, fill=255)
            x += cw + track
    return m


# Title logotype (original treatment of the game name; subtitle stays live UMG text)
LW, LH = 2048, 1700
cx = LW // 2
logo_mask = Image.new("L", (LW, LH), 0)
d = ImageDraw.Draw(logo_mask)
# halo ring behind the letters
ring = Image.new("L", (LW, LH), 0)
rd = ImageDraw.Draw(ring)
for rad, wdt in ((560, 22), (500, 9)):
    rd.ellipse([cx - rad, 640 - rad, cx + rad, 640 + rad], outline=255, width=wdt)
# cut ring where letters sit
ring_np = np.asarray(ring).copy()
ring_np[470:1500, :] = 0
ring = Image.fromarray(ring_np)
logo_mask = Image.fromarray(np.maximum(np.asarray(logo_mask), np.asarray(ring)))
big = font("Cinzel_700.ttf", 470)
small = font("Cinzel_700.ttf", 150)
letters = text_mask((LW, LH), [
    ("GODS", big, cx, 360, 18),
    ("ARENA", big, cx, 1010, 10),
])
mid = text_mask((LW, LH), [("OF  THE", small, cx, 860, 26)])
logo_mask = Image.fromarray(np.maximum.reduce([np.asarray(logo_mask), np.asarray(letters), np.asarray(mid)]))
# central blade / spire through the title
blade = Image.new("L", (LW, LH), 0)
bd = ImageDraw.Draw(blade)
bd.polygon([(cx, 40), (cx + 34, 520), (cx + 18, 900), (cx, 1520), (cx - 18, 900), (cx - 34, 520)], fill=255)
bd.polygon([(cx - 120, 560), (cx, 600), (cx + 120, 560), (cx, 640)], fill=255)
# carve a thin gap around the blade so it reads in front of the letters
gap = blade.filter(ImageFilter.MaxFilter(17))
lm = np.asarray(logo_mask).astype(np.int32) - np.asarray(gap).astype(np.int32)
logo_mask = Image.fromarray(np.clip(np.maximum(lm, np.asarray(blade)), 0, 255).astype(np.uint8))
# divider rules flanking "OF THE"
rules = Image.new("L", (LW, LH), 0)
rr = ImageDraw.Draw(rules)
for sgn in (-1, 1):
    x_in, x_out = cx + sgn * 390, cx + sgn * 660
    rr.line([(min(x_in, x_out), 960), (max(x_in, x_out), 960)], fill=255, width=6)
    rr.polygon([(x_out, 948), (x_out + sgn * 16, 960), (x_out, 972), (x_out - sgn * 16, 960)], fill=255)
logo_mask = Image.fromarray(np.maximum(np.asarray(logo_mask), np.asarray(rules)))
logo = metal_fill(logo_mask, outline=7, shadow=18, glow=26)
bbox = logo.getbbox()
logo = logo.crop((max(0, bbox[0] - 24), max(0, bbox[1] - 24), min(LW, bbox[2] + 24), min(LH, bbox[3] + 24)))
logo.save(os.path.join(OUT, "T_GOTA_Logo_GodsOfTheArena.png"))


# UPG insignia — original 8-point compass star in double ring
def insignia(size=1024):
    s = size
    c = s / 2
    m = Image.new("L", (s, s), 0)
    dd = ImageDraw.Draw(m)
    for rad, wdt in ((0.40, 0.030), (0.355, 0.012)):
        R = rad * s
        dd.ellipse([c - R, c - R, c + R, c + R], outline=255, width=int(wdt * s))
    # tick marks
    for i in range(32):
        ang = i * math.pi / 16
        r0, r1 = 0.37 * s, (0.395 if i % 4 else 0.41) * s
        dd.line([(c + math.cos(ang) * r0, c + math.sin(ang) * r0), (c + math.cos(ang) * r1, c + math.sin(ang) * r1)], fill=255, width=int(0.006 * s))
    light = Image.new("L", (s, s), 0)
    ld = ImageDraw.Draw(light)
    dark = Image.new("L", (s, s), 0)
    dk = ImageDraw.Draw(dark)

    def point(angle, length, width):
        tip = (c + math.cos(angle) * length, c + math.sin(angle) * length)
        l = (c + math.cos(angle - math.pi / 2) * width, c + math.sin(angle - math.pi / 2) * width)
        r = (c + math.cos(angle + math.pi / 2) * width, c + math.sin(angle + math.pi / 2) * width)
        ld.polygon([(c, c), l, tip], fill=255)
        dk.polygon([(c, c), r, tip], fill=255)

    for i in range(4):
        point(-math.pi / 2 + i * math.pi / 2 + math.pi / 4, 0.30 * s, 0.045 * s)
    for i in range(4):
        point(-math.pi / 2 + i * math.pi / 2, 0.49 * s, 0.07 * s)
    full = Image.fromarray(np.maximum.reduce([np.asarray(m), np.asarray(light), np.asarray(dark)]))
    stops = [(0.0, (246, 214, 140)), (0.5, (196, 144, 62)), (1.0, (238, 200, 120))]
    base = metal_fill(full, stops=stops, outline=0, shadow=10)
    # facet shading
    shade = Image.new("RGBA", (s, s), (120, 70, 18, 95))
    base.paste(shade, (0, 0), dark)
    # bright core
    core = Image.new("L", (s, s), 0)
    ImageDraw.Draw(core).ellipse([c - 0.03 * s, c - 0.03 * s, c + 0.03 * s, c + 0.03 * s], fill=255)
    base.paste(Image.new("RGBA", (s, s), (255, 250, 230, 255)), (0, 0), core.filter(ImageFilter.GaussianBlur(4)))
    return base


insignia().resize((512, 512), Image.LANCZOS).save(os.path.join(OUT, "T_GOTA_UPG_Insignia.png"))

# Simple UI glyphs (original)
def glyph_plus(size=128):
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    dd = ImageDraw.Draw(im)
    t = size // 14
    dd.rectangle([size // 2 - t // 2, size * 0.2, size // 2 + t // 2, size * 0.8], fill=(255, 255, 255, 255))
    dd.rectangle([size * 0.2, size // 2 - t // 2, size * 0.8, size // 2 + t // 2], fill=(255, 255, 255, 255))
    return im


glyph_plus().save(os.path.join(OUT, "T_GOTA_Icon_Plus.png"))


def glyph_diamond(size=64):
    im = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    dd = ImageDraw.Draw(im)
    dd.polygon([(size / 2, 2), (size - 2, size / 2), (size / 2, size - 2), (2, size / 2)], fill=(255, 255, 255, 255))
    return im


glyph_diamond().save(os.path.join(OUT, "T_GOTA_Glyph_Diamond.png"))

# Gradients used by cards / panels (white, tinted in UMG)
def vgrad(w, h, a0, a1, name):
    a = np.linspace(a0, a1, h)[:, None].repeat(w, 1)
    rgba = np.zeros((h, w, 4), np.uint8)
    rgba[..., :3] = 255
    rgba[..., 3] = (a * 255).astype(np.uint8)
    Image.fromarray(rgba, "RGBA").save(os.path.join(OUT, name))


def hgrad(w, h, a0, a1, name):
    a = np.linspace(a0, a1, w)[None, :].repeat(h, 0)
    rgba = np.zeros((h, w, 4), np.uint8)
    rgba[..., :3] = 255
    rgba[..., 3] = (a * 255).astype(np.uint8)
    Image.fromarray(rgba, "RGBA").save(os.path.join(OUT, name))


vgrad(8, 256, 0.0, 1.0, "T_GOTA_Grad_V.png")
hgrad(256, 8, 1.0, 0.0, "T_GOTA_Grad_H.png")
# selection sweep: soft horizontal highlight peaking left
sw = np.linspace(1, 0, 512) ** 1.6
rgba = np.zeros((8, 512, 4), np.uint8)
rgba[..., :3] = 255
rgba[..., 3] = (sw[None, :] * 255).astype(np.uint8)
Image.fromarray(rgba, "RGBA").save(os.path.join(OUT, "T_GOTA_Grad_Selection.png"))

print("Menu art written to", OUT)
for f in sorted(os.listdir(OUT)):
    print("  ", f)
