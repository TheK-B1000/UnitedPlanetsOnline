# Gods of the Arena — hero UI art derivatives from the approved hero cards (offline, NOT run in Unreal).
#   python Tools/GOTA_BuildHeroArt.py
# Input:  Docs/References/Heroes/GOTA_HeroCard_<Hero>.png
# Output: Tools/HeroArtSource/T_GOTA_<Hero>_{Portrait,Thumbnail}.png
#
# Portrait  = the card's single large character illustration (left region). Baked logo and
#             name/quote blocks are inpainted because live UMG text is drawn there.
# Thumbnail = the hero's tile from Ayla's roster carousel (one source => identical framing for all 8),
#             cropped above the baked name/icon (live text is drawn instead).
# Not yet derived: ability icons and weapon art (positions differ per card) — see GOTA_HERO_ROSTER.md.

import os

import cv2
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Docs", "References", "Heroes")
OUT = os.path.join(ROOT, "Tools", "HeroArtSource")
os.makedirs(OUT, exist_ok=True)

HEROES = ["Ayla", "Drakon", "Lysara", "Kairos", "Zephyr", "Varoth", "Nyx", "Orion"]

# Large-art crop (x0, y0, x1, y1) and baked text boxes to remove, per card.
STD = dict(crop=(0, 0, 1110, 698), text=[(18, 14, 262, 232)])
ART = {h: STD for h in HEROES}
ART["Ayla"] = dict(crop=(0, 0, 950, 735), text=[(18, 14, 250, 222), (14, 428, 372, 735)])
ART["Nyx"] = dict(crop=(0, 0, 1110, 690), text=[(18, 14, 262, 232), (16, 388, 384, 672)])

# Roster tiles in Ayla's card (x0, x1); y band above the baked faction icon + name.
TILES = {"Ayla": (84, 236), "Drakon": (259, 405), "Lysara": (433, 580), "Kairos": (607, 755),
         "Zephyr": (782, 929), "Varoth": (956, 1103), "Nyx": (1131, 1278), "Orion": (1305, 1453)}
TILE_Y = (758, 892)


def text_mask(img, box):
    x0, y0, x1, y1 = box
    region = img[y0:y1, x0:x1]
    lum = cv2.cvtColor(region, cv2.COLOR_BGR2GRAY)
    b, g, r = [region[..., i].astype(int) for i in range(3)]
    gold = (r > g) & (g > b) & ((r - b) > 35) & (lum > 95)
    bright = lum > 175
    m = np.zeros(img.shape[:2], np.uint8)
    m[y0:y1, x0:x1] = ((gold | bright) * 255).astype(np.uint8)
    m = cv2.morphologyEx(m, cv2.MORPH_CLOSE, np.ones((9, 9), np.uint8))
    return cv2.dilate(m, np.ones((7, 7), np.uint8))


for hero in HEROES:
    card = cv2.imread(os.path.join(SRC, "GOTA_HeroCard_%s.png" % hero), cv2.IMREAD_COLOR)
    spec = ART[hero]
    mask = np.zeros(card.shape[:2], np.uint8)
    for box in spec["text"]:
        mask |= text_mask(card, box)
    clean = cv2.inpaint(card, mask, 11, cv2.INPAINT_TELEA)
    x0, y0, x1, y1 = spec["crop"]
    portrait = clean[y0:y1, x0:x1]
    # Normalise to 1600 px wide (UI never shows it larger than ~1100 px at 1080p).
    h = int(round(portrait.shape[0] * 1600 / portrait.shape[1]))
    portrait = cv2.resize(portrait, (1600, h), interpolation=cv2.INTER_LANCZOS4)
    cv2.imwrite(os.path.join(OUT, "T_GOTA_%s_Portrait.png" % hero), portrait)

ayla = cv2.imread(os.path.join(SRC, "GOTA_HeroCard_Ayla.png"), cv2.IMREAD_COLOR)
for hero, (x0, x1) in TILES.items():
    tile = ayla[TILE_Y[0]:TILE_Y[1], x0:x1]
    tile = cv2.resize(tile, (tile.shape[1] * 2, tile.shape[0] * 2), interpolation=cv2.INTER_LANCZOS4)
    cv2.imwrite(os.path.join(OUT, "T_GOTA_%s_Thumbnail.png" % hero), tile)

print("Hero art written to", OUT, sorted(os.listdir(OUT)))
