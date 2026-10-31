# Arena overhaul — builds the authored arena levels (/Game/Arenas/Maps/L_Arena_<Name>).
#   GOTA_ARENAS=Quarry,Ashfall,... GOTA_QUIT_AFTER_BUILD=1 \
#   UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript=<abs>/Tools/GOTA_BuildArenaLevels.py -unattended
#
# Each recipe is deterministic (fixed seeds): rebuilding produces the same level, collision and navmesh.
# Coordinates in cm; the fighting floor is centred on the origin, top at Z = 0.
import importlib
import math
import os
import random
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import GOTA_ArenaKit as K  # noqa: E402

importlib.reload(K)
V, R = K.V, K.R
LOG = []


def log(msg):
    LOG.append(msg)
    unreal.log("[GOTA arena] " + msg)


GEO = "/Game/Geometry/"
MS = "/Game/Megascans/3D_Assets/"
PR = "/Game/PhotoR_Backgrounds/Meshes/360d_Horizon/"


def horizon(set_name, scale, z=0.0, material=None):
    """PhotoR 360° horizon (four quarter meshes around the origin) — distant layered scenery."""
    pieces = {
        "Canyon_a": ["PR_canyon_a_D", "PR_canyon_a_L", "PR_canyon_a_R", "PR_canyon_a_U"],
        "Canyon_b": ["PR_canyon_b_D", "PR_canyon_b_L", "PR_canyon_b_R", "PR_canyon_b_U"],
        "Desert": ["PR_desert_D", "PR_desert_L", "PR_desert_R", "PR_desert_U"],
        "Afghan_Mountain_a": ["PR_afgan_mountain_a_D", "PR_afgan_mountain_a_L", "PR_afgan_mountain_a_R", "PR_afgan_mountain_a_U"],
        "Afghan_Mountain_b": ["PR_afgan_mountain_b_D", "PR_afgan_mountain_b_L", "PR_afgan_mountain_b_R", "PR_afgan_mountain_b_U"],
    }[set_name]
    for p in pieces:
        K.mesh(PR + set_name + "/" + p + "." + p, V(0, 0, z), R(), (scale, scale, scale), material, "NoCollision",
               folder="Dressing/Horizon", cast_shadow=False)


def gameplay_floor(radius, floor_mat, curb_mat, apron_mat, apron_radius, boundary_radius, starts_radius=950.0):
    """Layer 1: stable walkable floor (top Z=0), curb step, apron, boundary, starts, horde spawns."""
    K.cyl(V(0, 0, -100), radius, 100, floor_mat, collision="BlockAll", label="Floor_Fighting", folder="Gameplay/Floor")
    K.cyl(V(0, 0, -140), apron_radius, 115, apron_mat, collision="BlockAll", label="Floor_Apron", folder="Gameplay/Floor")
    # Low curb: a 18 cm lip the characters step over (below the 45 cm step height).
    K.ring_of_boxes(radius + 40, 48, (2 * math.pi * (radius + 40) / 48 * 1.04, 80, 36), -7, curb_mat,
                    folder="Gameplay/Floor", collision="BlockAll", label="Curb")
    K.invisible_ring(boundary_radius, 900)
    K.player_starts([
        (0, V(-starts_radius, 0, 120), 0), (1, V(starts_radius, 0, 120), 180),
        (2, V(0, -starts_radius, 120), 90), (3, V(0, starts_radius, 120), -90)])
    K.horde_spawns(boundary_radius - 250, 6)


# ============================================================================ TITAN'S QUARRY
def build_quarry():
    rng = random.Random(1701)
    K.open_fresh_level("/Game/Arenas/Maps/L_Arena_TitansQuarry")

    # ---------------- Layer 1 — gameplay
    gameplay_floor(1750, "Paving", "QuarryBlock", "QuarryGravel", 9000, 2350)
    # Central seal: a sunken ring of darker cut stone with the old surveyor's energy ring (flush, readable).
    K.cyl(V(0, 0, -99), 520, 100.5, "QuarryBlock", collision="NoCollision", label="CentreSeal", folder="Gameplay/Floor")
    K.ring_of_boxes(540, 72, (48, 5, 3), 0.4, "GoldEnergy", folder="Dressing/CentreSeal", label="SealGlow", skip=lambda d: int(d / 5) % 3 == 0)
    K.flagstone_floor(1745, 545, 4, ["Paving", "CrackedRockVT", "Paving"], rng)

    # ---------------- Layer 2 — architecture & landscape
    # Ring of colossal columns just outside the boundary (frames combat, never blocks it).
    for i in range(12):
        deg = i * 30 + 15
        if 255 < deg < 285:
            continue  # gate axis stays open
        p = K.polar(2750, deg, -25)
        K.column(p, 130, 1700 + rng.uniform(-150, 250), "QuarryStone", capital="QuarryBlock", broken=(i % 3 == 1), rng=rng)

    # Terraced quarry benches: man-made excavation steps rising outward (gap to the south for the gate).
    bench_open = lambda d: 240 < (d % 360) < 300
    for i in range(5):
        r = 4300 + i * 900
        top = 700 + i * 950
        n = 30 + i * 6
        K.ring_of_boxes(r, n, (2 * math.pi * r / n * 1.06, 900, top + 140), top / 2.0 - 140, "QuarryStone",
                        folder="Dressing/QuarryBenches", skip=bench_open, jitter=0.0)
        # Cut faces: quarry cliff meshes pressed into each bench face to break the regularity.
        for j in range(10 + i * 2):
            deg = rng.uniform(0, 360)
            if bench_open(deg):
                continue
            p = K.polar(r - 470, deg, top - 140 - rng.uniform(250, 600))
            src = rng.choice([GEO + "SM_Quarry_Cliff_L_01.SM_Quarry_Cliff_L_01", GEO + "SM_Quarry_Cliff_L_02.SM_Quarry_Cliff_L_02",
                              GEO + "SM_Quarry_Cliff_L_03.SM_Quarry_Cliff_L_03", GEO + "SM_Quarry_Cliff_M_04.SM_Quarry_Cliff_M_04",
                              MS + "Quarry_Cliff_08/SM_Quarry_Cliff_08.SM_Quarry_Cliff_08", MS + "Quarry_Cliff_10/SM_Quarry_Cliff_10.SM_Quarry_Cliff_10"])
            s = rng.uniform(0.9, 1.6)
            K.mesh(src, p, R(0, deg + 90 + rng.uniform(-12, 12), 0), (s, s, s), folder="Dressing/QuarryCliffs")
    # Bench lips: rubble and spires along the first bench.
    K.scatter([GEO + "SM_Quarry_RockPile_S_01.SM_Quarry_RockPile_S_01", GEO + "SM_Quarry_RockPile_S_02.SM_Quarry_RockPile_S_02",
               GEO + "SM_Quarry_RockPile_S_03.SM_Quarry_RockPile_S_03", GEO + "SM_Quarry_RockPile_S_04.SM_Quarry_RockPile_S_04",
               GEO + "SM_Quarry_Ridge_S_01.SM_Quarry_Ridge_S_01", GEO + "SM_Quarry_Ridge_S_02.SM_Quarry_Ridge_S_02"],
              V(0, 0, 0), 3500, 3900, 34, rng, z=-25, scale=(1.0, 2.2), avoid=lambda p: bench_open(math.degrees(math.atan2(p.y, p.x))))
    K.scatter([GEO + "SM_Quarry_Spire_L_01.SM_Quarry_Spire_L_01", GEO + "SM_Quarry_Spire_L_02.SM_Quarry_Spire_L_02"],
              V(0, 0, 0), 3300, 3800, 6, rng, z=-25, scale=(1.4, 2.4), avoid=lambda p: bench_open(math.degrees(math.atan2(p.y, p.x))))
    # Quarried blocks abandoned on the apron: evidence of the last excavation.
    for k in range(5):
        base = K.polar(rng.uniform(3000, 3400), rng.choice([20, 65, 110, 160, 200, 320, 345]) + rng.uniform(-8, 8), -25)
        for b in range(rng.randint(3, 6)):
            w = rng.uniform(220, 360)
            K.box(V(base.x + rng.uniform(-260, 260), base.y + rng.uniform(-260, 260), base.z + w * 0.3 + (b // 3) * w * 0.6),
                  (w, w * 0.65, w * 0.6), "QuarryBlock", rot=R(0, rng.uniform(0, 90), rng.uniform(-3, 3)), folder="Dressing/CutBlocks")

    # The Titan: colossal legs of a broken statue rising from the north bench (the quarry's namesake).
    titan = V(0, 6200, 700)
    K.box(V(titan.x, titan.y, titan.z + 300), (3600, 2400, 600), "QuarryBlock", folder="Dressing/Titan")
    K.box(V(titan.x, titan.y, titan.z + 720), (3000, 2000, 240), "QuarryBlock", folder="Dressing/Titan")
    # The broken colossus: lower half of a robed titan, snapped at the waist (Pass 2 sculpted mesh).
    K.arch_mesh("ColossusBroken", V(titan.x, titan.y, titan.z + 870), 180, (6.6, 6.6, 7.2), "QuarryBlock", folder="Dressing/Titan")
    # Fallen head of the Titan, half-buried at the foot of the bench.
    K.sphere(V(-2600, 4300, 300), 900, "QuarryBlock", scale=(1.0, 0.9, 1.15), label="Titan_Head")
    K.box(V(-2600, 3550, 520), (700, 260, 520), "QuarryBlock", rot=R(0, 8, -6), folder="Dressing/Titan")  # brow/nose block
    # Colossal hand in the western apron, fingers broken.
    hand = V(-3600, -1600, -25)
    K.box(V(hand.x, hand.y, hand.z + 280), (1400, 900, 560), "QuarryBlock", rot=R(0, 30, 4), folder="Dressing/Titan")
    for f in range(4):
        p = K.polar(900 + f * 30, 30 + f * 14 - 20, hand.z + 220, hand.x, hand.y)
        K.cyl(V(p.x, p.y, p.z - 140), 120, 700 - f * 70, "QuarryBlock", rot=R(70, 30 + f * 14 - 20, 0), folder="Dressing/Titan")

    # Mining gantry over the east benches: lattice towers, truss, chains and a suspended monolith.
    iron, rust = "Iron", "RustIron"
    t1, t2 = V(5600, -2600, 700), V(5600, 2600, 700)
    for t in (t1, t2):
        K.lattice_tower(t, 700, 5200, rust, levels=7)
    K.arch_mesh("Truss", V(t1.x - 200, 0, 5700), 90, (1.0, 1.5, 1.2), iron, folder="Dressing/Machinery")
    # Boom reaching over the quarry floor.
    K.beam(V(t1.x - 500, 0, 5900), V(2600, 0, 4700), 220, iron)
    K.box(V(2600, 0, 4600), (300, 400, 280), iron, folder="Dressing/Machinery")
    K.cyl(V(2600, 0, 4400), 110, 200, rust, rot=R(90, 0, 0), folder="Dressing/Machinery")
    for dy in (-140, 140):
        K.chain(V(2600, dy, 4380), V(2600, dy * 0.6, 2750), iron, link=70, sag=0.0)
    K.box(V(2600, 0, 2300), (420, 420, 900), "QuarryBlock", rot=R(0, 12, 0), folder="Dressing/Machinery", label="SuspendedMonolith")
    # Suspended work platforms hanging off the truss.
    for y in (-1300, 1300):
        K.box(V(5000, y, 3600), (900, 700, 70), "QuarryBlock", folder="Dressing/Machinery")
        for cx, cy in ((-420, -320), (420, -320), (-420, 320), (420, 320)):
            K.chain(V(5000 + cx, y + cy, 5880), V(5000 + cx, y + cy, 3640), iron, link=80, sag=0.0)
    # Heavy chains slung between the gantry and the north bench.
    K.chain(V(t2.x, t2.y, 5400), V(1800, 5400, 4600), iron, link=110, sag=0.12)
    K.chain(V(t2.x, t2.y, 5000), V(3400, 5600, 3800), iron, link=110, sag=0.1)

    # Broken bridge across the western cut, high above the arena.
    for k in range(7):
        x0 = -8200 + k * 620
        if k == 5:
            continue  # the collapsed span
        K.box(V(x0, -600, 3900), (600, 700, 140), "QuarryBlock", rot=R(0 if k < 5 else -9, 0, 0), folder="Dressing/Bridge")
        K.box(V(x0, -940, 4040), (600, 40, 140), "QuarryStone", folder="Dressing/Bridge")
        K.box(V(x0, -260, 4040), (600, 40, 140), "QuarryStone", folder="Dressing/Bridge")
    K.arch(V(-6900, -600, 700), 0, 800, 2900, 600, 360, "QuarryBlock", folder="Dressing/Bridge")
    K.box(V(-4600, -600, 3600), (300, 650, 600), "QuarryBlock", rot=R(-20, 0, 8), folder="Dressing/Bridge")  # fallen span piece

    # Monumental entry gate on the south axis (the vista through it shows the quarry road and refinery).
    K.gate(V(0, -3700, -25), 90, 1700, 3400, "QuarryBlock", trim="QuarryStone", energy="GoldEnergy")
    K.stairs(V(0, -5400, -25), 90, 1700, 4, 6, 420, "QuarryBlock")
    # Processional road beyond the gate: flanking obelisks (UPA survey pylons).
    for k in range(4):
        for s in (-1, 1):
            p = V(s * 1350, -5600 - k * 1400, -25)
            K.arch_mesh("Obelisk", p, 0, (0.85, 0.85, 0.7), "QuarryBlock", folder="Dressing/Road")
            K.box(V(p.x - s * 132, p.y, p.z + 1100), (6, 120, 900), "UPGEnergy", folder="Dressing/Road")

    # Distant refinery silhouettes (layered background, south).
    for k, (x, y, h, r) in enumerate([(-6000, -26000, 9000, 700), (-2500, -30000, 12000, 900), (3000, -27000, 8000, 600),
                                      (7500, -32000, 14000, 1100), (12000, -25000, 7000, 500)]):
        K.cyl(V(x, y, -200), r, h, "RustIron", folder="Dressing/Refinery", cast_shadow=False)
        K.cyl(V(x, y, h - 200), r * 1.25, 300, "Iron", folder="Dressing/Refinery", cast_shadow=False)
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Smoke_A.P_Smoke_A", V(x, y, h + 200), 18.0, "RefinerySmoke%d" % k)
        K.point_light(V(x, y, h + 150), (1.0, 0.45, 0.15), 200000, 6000, "RefineryFlare%d" % k)
    horizon("Canyon_a", 14.0, z=-1500)

    # ---------------- Layer 3 — atmosphere
    K.lighting(sun_pitch=-41, sun_yaw=128, sun_color=(1.0, 0.84, 0.64), sun_lux=10.0, fog_density=0.018, fog_falloff=0.12,
               fog_color=(0.55, 0.42, 0.3), fog_start=2500, vol_scatter=1.6, sky_intensity=2.4, fog_albedo=(255, 222, 190),
               fog_extinction=0.35)
    K.post_process(exposure_bias=0.75, min_ev=-0.5, max_ev=2.5, bloom=0.5, saturation=1.05, contrast=1.06,
                   gain=(1.03, 1.0, 0.95), vignette=0.38, grain=0.06, temp=5900)
    dust = "/Game/StarterContent/Particles/P_Ambient_Dust.P_Ambient_Dust"
    for i in range(10):
        p = K.polar(1200 + (i % 3) * 900, i * 36 + 10, 260)
        K.fx(dust, p, 2.5, "Dust_%02d" % i)
    K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Smoke_F.P_Smoke_F", V(4700, 0, 900), 6.0, "GantryDust")
    K.ambient("/Game/StarterContent/Audio/Starter_Wind05.Starter_Wind05", V(0, 0, 400), 6000, 12000, 0.55, "Amb_Wind")
    K.ambient("/Game/StarterContent/Audio/Starter_Wind06.Starter_Wind06", V(0, 5600, 2500), 3000, 9000, 0.4, "Amb_WindHigh")

    # Composition cameras (also used by the screenshot script).
    K.camera_point("Spawn", V(-1550, 0, 300), -8, 0)
    K.camera_point("Gate", V(0, -4200, 420), -2, 90)
    K.camera_point("Centre", V(0, -500, 220), 10, 90)
    K.camera_point("Elevated", V(1700, -1900, 1250), -26, 132)
    K.camera_point("Establishing", V(-4200, -4600, 8200), -42, 47)
    return K.finalize("TitansQuarry", V(0, 0, 0), V(5200, 5200, 1400), "/Game/Arenas/Maps/L_Arena_TitansQuarry")


# ============================================================================ ASHFALL CRUCIBLE
def build_ashfall():
    rng = random.Random(2203)
    K.open_fresh_level("/Game/Arenas/Maps/L_Arena_AshfallCrucible")
    # Layer 1 — an obsidian island above a lava lake (the lava is unreachable: decorative only).
    gameplay_floor(1750, "Basalt", "Obsidian", "Basalt", 2520, 2350)
    K.cyl(V(0, 0, -99), 560, 100.5, "Obsidian", label="CentreSeal", folder="Gameplay/Floor")
    for c in range(7):
        base = K.polar(rng.uniform(700, 1500), rng.uniform(0, 360), 0.35)
        yaw = rng.uniform(0, 360)
        for seg in range(rng.randint(3, 5)):
            L = rng.uniform(60, 160)
            yaw += rng.uniform(-50, 50)
            K.box(base, (L, rng.uniform(3, 6), 2), "LavaCrack", rot=R(0, yaw, 0), folder="Dressing/LavaCracks")
            base = V(base.x + math.cos(math.radians(yaw)) * L * 0.9, base.y + math.sin(math.radians(yaw)) * L * 0.9, 0.35)
    # Island skirt: basalt cliffs dropping to the lava.
    skirt = [GEO + "SM_Lava_Cliff_L_0%d.SM_Lava_Cliff_L_0%d" % (k, k) for k in (1, 2, 3, 4)]
    for i in range(22):
        deg = i * (360 / 22.0)
        p = K.polar(2620, deg, -480)
        K.mesh(rng.choice(skirt), p, R(0, deg + 90, 0), (1.1, 1.3, 1.25), folder="Dressing/IslandSkirt")
    K.cyl(V(0, 0, -700), 16000, 280, "Lava", label="LavaLake", folder="Dressing/Lava", cast_shadow=False)

    # Layer 2 — the Crucible: obsidian fortress ring rising out of the lava.
    gate_open = lambda d: 80 < (d % 360) < 100
    for i in range(32):
        deg = i * (360 / 32.0) + 360 / 64.0
        if gate_open(deg):
            continue
        K.arch_mesh("Wall", K.polar(5600, deg, -450), deg + 90, (2 * math.pi * 5600 / 32 * 1.04 / 1000.0, 1.05, 2.0), "Obsidian",
                    folder="Dressing/Fortress")
    for i in range(8):
        deg = i * 45 + 22.5
        p = K.polar(5650, deg, -450)
        K.arch_mesh("TowerSquare", p, deg, (1.12, 1.12, 1.45), "Obsidian", folder="Dressing/Towers")
        K.box(K.polar(5130, deg, 3200), (6, 160, 1900), "LavaCrack", rot=R(0, deg + 90, 0), folder="Dressing/Towers")
    K.gate(K.polar(5450, 90, -450), 90, 2000, 4600, "Obsidian", trim="Basalt", energy="LavaCrack")

    # Molten falls pouring from the battlements into the lake.
    for k, deg in enumerate([30, 150, 210, 300, 335]):
        p = K.polar(5250, deg, 1100)
        K.box(p, (90, 520, 3400), "Lava", rot=R(0, deg + 90, 0), folder="Dressing/MoltenFalls", cast_shadow=False)
        foot = K.polar(5150, deg, -420)
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Smoke_D.P_Smoke_D", foot, 5.0, "FallSteam%d" % k)
        K.point_light(K.polar(4900, deg, 400), (1.0, 0.32, 0.06), 12000, 4000, "FallGlow%d" % k)

    # Iron bridges from the island to the fortress (visual; the boundary keeps fighters on the island).
    for deg in (0, 180):
        a, b = K.polar(2600, deg, -40), K.polar(5300, deg, -40)
        K.beam(a, b, 1, "Iron")  # spine marker (thin)
        mid = K.polar(3950, deg, -60)
        K.box(mid, (2700, 520, 60), "Iron", rot=R(0, deg, 0), folder="Dressing/Bridges")
        for s in (-1, 1):
            off = K.polar(270, deg + 90 * s, 0)
            K.box(V(mid.x + off.x, mid.y + off.y, 60), (2700, 30, 180), "RustIron", rot=R(0, deg, 0), folder="Dressing/Bridges")
            for k in range(5):
                pk = K.polar(2700 + k * 600, deg, 0)
                K.box(V(pk.x + off.x, pk.y + off.y, -300), (80, 80, 520), "RustIron", folder="Dressing/Bridges")
        tower = K.polar(5650, deg + 22.5, 3800)
        K.chain(K.polar(3600, deg, 160), tower, "Iron", link=90, sag=0.08)

    # Braziers (ceremonial fire) on the apron rim, outside the boundary.
    for i in range(8):
        deg = i * 45
        p = K.polar(2460, deg, -25)
        K.arch_mesh("Brazier", p, i * 45, (1.0, 1.0, 1.0), "Iron", folder="Dressing/Braziers")
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Fire/P_Fire_Small.P_Fire_Small", V(p.x, p.y, p.z + 290), 0.8, "Brazier%d" % i)
        K.point_light(V(p.x, p.y, p.z + 420), (1.0, 0.45, 0.15), 2200, 1200, "BrazierLight%d" % i)

    # Volcanic outcrops and broken obelisks rising from the lava between island and fortress.
    spires = [MS + "IcelandicLavaSpire/SM_IcelandicLavaSpire_01.SM_IcelandicLavaSpire_01",
              GEO + "SM_Lava_RockMound_S_01.SM_Lava_RockMound_S_01", GEO + "SM_Lava_RockMound_S_04.SM_Lava_RockMound_S_04",
              GEO + "SM_Lava_TreeMound_M_04.SM_Lava_TreeMound_M_04"]
    K.scatter(spires, V(0, 0, 0), 3200, 4900, 26, rng, z=-480, scale=(1.4, 3.2), folder="Dressing/LavaOutcrops",
              avoid=lambda p: abs(p.y) < 500 or gate_open(math.degrees(math.atan2(p.y, p.x))))
    for k in range(6):
        deg = k * 60 + 30
        p = K.polar(4100, deg, -480)
        K.mesh(GEO + "SM_Lava_RockMound_S_02.SM_Lava_RockMound_S_02", p, R(0, deg, 0), (2.2, 2.2, 2.6), folder="Dressing/Obelisks")
        tilt = 0 if k % 2 == 0 else rng.uniform(8, 18)
        K.arch_mesh("Obelisk", V(p.x, p.y, p.z + 250), deg, (1.0, 1.0, 1.0 if tilt == 0 else 0.6), "Obsidian", folder="Dressing/Obelisks", pitch=tilt)
        K.box(V(p.x, p.y, p.z + 1500), (8, 140, 2000 if tilt == 0 else 1100), "LavaCrack", rot=R(tilt, deg, 0), folder="Dressing/Obelisks")

    # The Crucible itself: a colossal iron cauldron slung between two towers, pouring molten metal.
    c = K.polar(4300, 125, 3000)
    K.arch_mesh("Crucible", V(c.x, c.y, c.z), 35, (1.0, 1.0, 1.0), "RustIron", folder="Dressing/Crucible")
    K.cyl(V(c.x, c.y, c.z + 820), 640, 40, "Lava", folder="Dressing/Crucible", cast_shadow=False)
    for tdeg in (112.5, 157.5):
        K.chain(K.polar(5650, tdeg, 4700), V(c.x, c.y, c.z + 920), "Iron", link=110, sag=0.05)
    pour = K.polar(4300, 118, 1100)
    K.box(pour, (70, 160, 3800), "Lava", rot=R(0, 125, 0), folder="Dressing/Crucible", cast_shadow=False)
    K.point_light(V(c.x, c.y, c.z + 1200), (1.0, 0.4, 0.1), 12000, 3500, "CrucibleGlow")

    # Distant volcanoes.
    for k, (deg, dist, sc) in enumerate([(70, 52000, 3.0), (140, 60000, 3.6), (250, 48000, 2.6), (320, 58000, 3.2)]):
        p = K.polar(dist, deg, -6000)
        K.mesh("/Game/Iceland_Environment/Static_Meshes/SM_Mountain_01.SM_Mountain_01", p, R(0, deg, 0), (sc, sc, sc * 1.25),
               "Basalt", folder="Dressing/Volcanoes", cast_shadow=False)
        top = K.polar(dist, deg, 40000 * sc * 1.25 - 6500)
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Smoke_A.P_Smoke_A", top, 60.0, "VolcanoPlume%d" % k)
        K.point_light(top, (1.0, 0.25, 0.05), 400000, 30000, "VolcanoGlow%d" % k)

    # Layer 3 — atmosphere: ash-choked sun, red underlighting from the lava, embers.
    K.lighting(sun_pitch=-48, sun_yaw=200, sun_color=(1.0, 0.68, 0.5), sun_lux=4.5, sky_tint=(0.85, 0.8, 0.78),
               fog_density=0.022, fog_falloff=0.1, fog_color=(0.2, 0.07, 0.03), fog_start=2500, vol_scatter=1.0,
               sky_intensity=0.9, fog_albedo=(200, 150, 130), fog_extinction=0.6)
    K.post_process(exposure_bias=-0.6, min_ev=-1.0, max_ev=3.0, bloom=0.4, saturation=1.02, contrast=1.1,
                   gain=(1.08, 0.96, 0.88), vignette=0.42, grain=0.08, temp=5200)
    for i in range(10):
        K.point_light(K.polar(3300, i * 36, -250), (1.0, 0.3, 0.05), 6000, 3200, "LavaUnderlight%d" % i)
    for i in range(4):
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Sparks/P_Embers_A.P_Embers_A", K.polar(3400, i * 90 + 45, 200), 0.8, "Embers%d" % i)
    for i in range(3):
        K.fx("/Game/StarterContent/Particles/P_Ambient_Dust.P_Ambient_Dust", K.polar(2000, i * 120 + 20, 400), 1.5, "Ash%d" % i)
    for i in range(4):
        K.ambient(K.amb("Lava"), K.polar(3500, i * 90 + 45, -100), 2500, 6000, 0.7, "Amb_Lava%d" % i)
    K.ambient(K.amb("Wind_Storm"), V(0, 0, 600), 6000, 12000, 0.3, "Amb_Wind")
    K.ambient("/Game/StarterContent/Audio/Fire01_Cue.Fire01_Cue", K.polar(2460, 0, 200), 300, 1800, 0.6, "Amb_Brazier")

    K.camera_point("Spawn", V(-1550, 0, 300), -8, 0)
    K.camera_point("Gate", V(0, 2100, 500), -6, -90)
    K.camera_point("Centre", V(0, -600, 220), 12, 90)
    K.camera_point("Elevated", V(2600, -2600, 1500), -24, 135)
    K.camera_point("Establishing", V(-4200, -4600, 8200), -42, 47)
    return K.finalize("AshfallCrucible", V(0, 0, 0), V(5200, 5200, 1400), "/Game/Arenas/Maps/L_Arena_AshfallCrucible")


# ============================================================================ FROSTBOUND CITADEL
def build_frostbound():
    rng = random.Random(3109)
    K.open_fresh_level("/Game/Arenas/Maps/L_Arena_FrostboundCitadel")
    # Layer 1 — the citadel's inner court. Snow is a surface finish only: same collision / friction.
    gameplay_floor(1750, "FrostStone", "FrostStone", "Snow", 9000, 2350)
    K.cyl(V(0, 0, -99), 520, 100.5, "Ice", label="CentreSeal", folder="Gameplay/Floor")
    K.flagstone_floor(1745, 545, 4, ["RockyVT", "FrostStone", "Snow"], rng)
    K.ring_of_boxes(540, 72, (48, 5, 3), 0.4, "UPGEnergy", folder="Dressing/CentreSeal", skip=lambda d: int(d / 5) % 3 == 0)

    # Layer 2 — citadel walls with snow caps, round towers with snow roofs, south gate.
    gate_open = lambda d: 255 < (d % 360) < 285
    for i in range(36):
        deg = i * 10 + 5
        if gate_open(deg):
            continue
        K.arch_mesh("Wall", K.polar(3700, deg, -25), deg + 90, (2 * math.pi * 3700 / 36 * 1.06 / 1000.0, 0.85, 0.95), "FrostStone",
                    folder="Dressing/Walls")
        if i % 2 == 1:
            K.box(K.polar(3390, deg, 450), (260, 220, 950), "FrostStone", rot=R(-14, deg + 180, 0), folder="Dressing/Buttresses")
            K.box(K.polar(3330, deg, 960), (280, 240, 40), "Snow", rot=R(-14, deg + 180, 0), folder="Dressing/Buttresses")
    for i in range(8):
        deg = i * 45
        if gate_open(deg):
            continue
        p = K.polar(3750, deg, -25)
        K.arch_mesh("TowerRound", p, deg, (1.04, 1.04, 0.95), "FrostStone", folder="Dressing/Towers")
        K.box(K.polar(3220, deg, 1900), (6, 70, 900), "UPGEnergy", rot=R(0, deg + 90, 0), folder="Dressing/Towers")
    K.gate(K.polar(3700, 270, -25), 90, 1800, 3400, "FrostStone", trim="Snow", energy="UPGEnergy")

    # Frozen statues of the citadel's wardens, inside the walls (outside the fighting boundary).
    for k in range(6):
        deg = k * 60 + 30
        p = K.polar(3050, deg, -25)
        face = deg + 180
        K.box(V(p.x, p.y, p.z + 200), (560, 560, 400), "FrostStone", rot=R(0, face, 0), folder="Dressing/Statues")
        K.arch_mesh("Colossus", V(p.x, p.y, p.z + 430), face - 90, (1.75, 1.75, 1.75), "FrostStone", folder="Dressing/Statues")
        K.box(V(p.x, p.y, p.z + 1300), (40, 120, 1700), "Ice", rot=R(0, face, 0), folder="Dressing/Statues")

    # Glacier beyond the walls: jagged ice cliffs, frozen falls and snow-buried rock.
    for i in range(46):
        deg = rng.uniform(0, 360)
        if gate_open(deg) and rng.random() < 0.7:
            continue
        r = rng.uniform(6200, 10500)
        h = rng.uniform(2500, 7500)
        p = K.polar(r, deg, h / 2 - 600)
        K.box(p, (rng.uniform(1400, 3200), rng.uniform(1000, 2400), h), "Ice" if rng.random() < 0.55 else "FrostStone",
              rot=R(rng.uniform(-8, 8), rng.uniform(0, 360), rng.uniform(-8, 8)), folder="Dressing/Glacier")
    for deg in (20, 110, 160, 340):
        p = K.polar(6100, deg, 2000)
        K.box(p, (120, 900, 5200), "Ice", rot=R(0, deg + 90, 0), folder="Dressing/FrozenFalls")
        K.box(K.polar(6000, deg, -300), (700, 1400, 600), "Ice", rot=R(0, deg + 90, 12), folder="Dressing/FrozenFalls")
    trees = ["/Game/WinterForestSet/Meshes/SM_WinterTree_%s.SM_WinterTree_%s" % (c, c) for c in "ABCDEF"]
    K.scatter(trees, V(0, 0, 0), 4300, 5900, 34, rng, z=-25, scale=(0.45, 0.8), folder="Dressing/Pines",
              avoid=lambda p: gate_open(math.degrees(math.atan2(p.y, p.x))))
    piles = ["/Game/WinterForestSet/Meshes/SM_SnowPile_%s.SM_SnowPile_%s" % (c, c) for c in "ABC"]
    K.scatter(piles, V(0, 0, 0), 3950, 4300, 30, rng, z=-25, scale=(1.5, 3.0), folder="Dressing/SnowPiles")
    K.scatter(piles, V(0, 0, 0), 3150, 3420, 26, rng, z=-25, scale=(1.2, 2.4), folder="Dressing/SnowPiles",
              avoid=lambda p: gate_open(math.degrees(math.atan2(p.y, p.x))))
    rocks = ["/Game/WinterForestSet/Meshes/SM_WinterRock_%s.SM_WinterRock_%s" % (c, c) for c in "ABCDEFG"]
    K.scatter(rocks, V(0, 0, 0), 2500, 3000, 18, rng, z=-25, scale=(1.0, 2.2), folder="Dressing/Rocks")
    # Broken bridge between the north towers and the glacier.
    for k in range(6):
        if k == 3:
            continue
        p = K.polar(4500 + k * 650, 90, 3600 - (k * 40))
        K.box(p, (640, 700, 150), "FrostStone", rot=R(0, 90, 0), folder="Dressing/Bridge")
        K.box(V(p.x, p.y, p.z + 90), (640, 760, 40), "Snow", rot=R(0, 90, 0), folder="Dressing/Bridge")
    K.arch(K.polar(5800, 90, -25), 90, 900, 3300, 700, 420, "FrostStone", folder="Dressing/Bridge")

    # Mountain vista.
    for k, (deg, dist, sc) in enumerate([(40, 55000, 2.2), (100, 62000, 2.8), (170, 50000, 2.0), (230, 58000, 2.6), (300, 52000, 2.4)]):
        p = K.polar(dist, deg, -5000)
        K.mesh("/Game/Iceland_Environment/Static_Meshes/SM_Iceland_Mountain_02.SM_Iceland_Mountain_02", p, R(0, deg * 2, 0),
               (sc, sc, sc), "Snow", folder="Dressing/Mountains", cast_shadow=False)
    K.mesh("/Game/WinterForestSet/Meshes/SM_SnowMountain_A.SM_SnowMountain_A", K.polar(16000, 60, -800), R(0, 30, 0), (3.5, 3.5, 3.5), folder="Dressing/Mountains")
    K.mesh("/Game/WinterForestSet/Meshes/SM_SnowMountain_A.SM_SnowMountain_A", K.polar(18000, 200, -800), R(0, 140, 0), (4.2, 4.2, 3.8), folder="Dressing/Mountains")

    # Layer 3 — cold blue daylight, restrained bloom so characters stay readable on snow.
    K.lighting(sun_pitch=-30, sun_yaw=55, sun_color=(0.86, 0.92, 1.0), sun_lux=8.0, sky_tint=(0.8, 0.88, 1.0),
               fog_density=0.03, fog_falloff=0.11, fog_color=(0.5, 0.6, 0.75), fog_start=2200, vol_scatter=1.0,
               sky_intensity=1.5, fog_albedo=(235, 242, 255), fog_extinction=0.5)
    K.post_process(exposure_bias=-0.35, min_ev=-0.5, max_ev=2.5, bloom=0.35, saturation=0.92, contrast=1.05,
                   gain=(0.96, 1.0, 1.06), vignette=0.3, grain=0.05, temp=7300)
    for i in range(12):
        K.fx("/Game/StarterContent/Particles/P_Ambient_Dust.P_Ambient_Dust", K.polar(800 + (i % 4) * 700, i * 30, 420), 1.5, "IceMotes%d" % i)
    K.ambient(K.amb("Wind_Frost"), V(0, 0, 500), 6000, 12000, 0.55, "Amb_Wind")
    K.ambient(K.amb("IceCreak"), K.polar(6000, 120, 500), 3000, 9000, 0.6, "Amb_IceCreak")
    K.ambient(K.amb("IceCreak"), K.polar(6000, 300, 500), 3000, 9000, 0.5, "Amb_IceCreak2")

    K.camera_point("Spawn", V(-1550, 0, 300), -8, 0)
    K.camera_point("Gate", V(0, -2200, 380), 4, 90)
    K.camera_point("Centre", V(0, -600, 220), 12, 90)
    K.camera_point("Elevated", V(2600, -2600, 1500), -24, 135)
    K.camera_point("Establishing", V(-4200, -4600, 8200), -42, 47)
    return K.finalize("FrostboundCitadel", V(0, 0, 0), V(5200, 5200, 1400), "/Game/Arenas/Maps/L_Arena_FrostboundCitadel")


def msa(folder, name):
    return MS + folder + "/" + name + "." + name


WATER_LAKE = "/Game/StarterContent/Materials/M_Water_Lake.M_Water_Lake"
WATER_OCEAN = "/Game/StarterContent/Materials/M_Water_Ocean.M_Water_Ocean"


# ============================================================================ VERDANT SANCTUARY
def build_verdant():
    rng = random.Random(4401)
    K.open_fresh_level("/Game/Arenas/Maps/L_Arena_VerdantSanctuary")
    # Layer 1 — the old ceremonial floor, reclaimed by moss at the edges. Sightlines kept clear.
    gameplay_floor(1750, "TempleStone", "MossStone", "ForestGround", 9000, 2350)
    K.cyl(V(0, 0, -99), 520, 100.5, "MossStone", label="CentreSeal", folder="Gameplay/Floor")
    K.flagstone_floor(1745, 545, 4, ["TempleStone", "TempleStone", "MossRockVT"], rng)
    K.ring_of_boxes(540, 72, (48, 5, 3), 0.4, "GoldEnergy", folder="Dressing/CentreSeal", skip=lambda d: int(d / 5) % 2 == 0)

    # Layer 2 — overgrown colonnade, the north temple, roots, vines, waterfall, forest.
    for i in range(12):
        deg = i * 30 + 15
        if 255 < deg < 285:
            continue
        p = K.polar(2750, deg, -25)
        K.column(p, 120, 1500 + rng.uniform(-100, 200), "MossRockVT" if i % 3 == 0 else "TempleStone", capital="MossStone", broken=(i % 4 == 2), rng=rng)
        if i % 2 == 0:
            for v in range(3):
                vp = K.polar(2750 + rng.uniform(-90, 90), deg + rng.uniform(-2, 2), 900)
                K.cyl(V(vp.x, vp.y, 700 + rng.uniform(0, 300)), 9, rng.uniform(500, 900), "MossStone", folder="Dressing/Vines", cast_shadow=False)
    # North temple: stepped platform, inner sanctum colonnade, broken pediment.
    t = V(0, 5200, -25)
    for k in range(5):
        K.box(V(t.x, t.y + k * 120, t.z + 90 * (k + 1) / 2.0), (4200 - k * 380, 2600 - k * 240, 90 * (k + 1)), "TempleStone", folder="Dressing/Temple")
    K.stairs(V(0, 3600, -25), 90, 1600, 5, 90, 160, "TempleStone")
    for k in range(6):
        x = -1500 + k * 600
        K.column(V(x, 4300, 425), 95, 1500, "TempleStone", capital="MossStone", broken=(k == 4), rng=rng)
    K.box(V(0, 4300, 2150), (3800, 420, 260), "TempleStone", folder="Dressing/Temple")
    K.box(V(-800, 4300, 2420), (2200, 380, 280), "TempleStone", rot=R(0, 0, 9), folder="Dressing/Temple")
    K.box(V(0, 5600, 1500), (2800, 1400, 2200), "TempleStone", folder="Dressing/Temple")
    K.box(V(0, 4930, 1300), (900, 120, 1500), "MossStone", folder="Dressing/Temple")
    # Guardian statues flanking the temple stairs.
    for s in (-1, 1):
        p = V(s * 1300, 3500, -25)
        K.box(V(p.x, p.y, p.z + 300), (640, 640, 600), "TempleStone", folder="Dressing/Statues")
        K.arch_mesh("Colossus", V(p.x, p.y, p.z + 630), 180, (2.0, 2.0, 2.0), "MossStone", folder="Dressing/Statues")
    # Waterfall over a mossy cliff in the north-east, feeding a stream around the ring.
    cliff = K.polar(6200, 40, -25)
    K.mesh(msa("Massive_Nordic_Coastal_Cliff_veqnci0db", "S_Massive_Nordic_Coastal_Cliff_veqnci0db_high_Var1"), cliff, R(0, 220, 0), (1.6, 1.6, 1.6), "MossStone", folder="Dressing/Waterfall")
    fall = K.polar(5450, 40, 2400)
    K.box(fall, (60, 900, 5000), WATER_LAKE, rot=R(0, 130, 0), folder="Dressing/Waterfall", cast_shadow=False)
    pool = K.polar(5000, 40, -18)
    K.cyl(pool, 1500, 10, WATER_LAKE, folder="Dressing/Waterfall", cast_shadow=False)
    for k in range(3):
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Water/P_Water_Leak_A.P_Water_Leak_A", K.polar(5420, 40 + (k - 1) * 3, 4800), 4.0, "FallWater%d" % k)
    K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Steam_A.P_Steam_A", K.polar(5100, 40, 100), 5.0, "FallMist")
    for k in range(40):
        a = 40 + k * 7.0
        p = K.polar(3200 + 260 * math.sin(k * 0.5), a, -17)
        K.box(p, (560, 320, 6), WATER_LAKE, rot=R(0, a + 90, 0), folder="Dressing/Stream", cast_shadow=False)
    # Giant roots arching over the outer ring (outside the boundary).
    for k in range(7):
        deg = k * 51 + 10
        a = K.polar(4200, deg, -40)
        b2 = K.polar(2700, deg + 12, -40)
        mid = V((a.x + b2.x) / 2, (a.y + b2.y) / 2, 380)
        K.beam(a, mid, 170, "ForestGround", folder="Dressing/Roots")
        K.beam(mid, b2, 130, "ForestGround", folder="Dressing/Roots")
    # Forest: huge beeches and maples framing the sanctuary; saplings and ferns at the edges.
    big = ["/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_0%d.SM_EuropeanBeech_Forest_0%d" % (k, k) for k in (1, 2, 3, 4)]
    big += ["/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_0%d.SM_NorwayMaple_Forest_0%d" % (k, k) for k in (1, 2, 3, 4)]
    temple_zone = lambda p: abs(p.x) < 2600 and p.y > 3000
    K.scatter(big, V(0, 0, 0), 3900, 8500, 40, rng, z=-25, scale=(0.9, 1.25), folder="Dressing/Forest", avoid=temple_zone)
    mid_trees = ["/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_0%d.SM_EuropeanBeech_Forest_0%d" % (k, k) for k in (5, 6, 7, 8)]
    K.scatter(mid_trees, V(0, 0, 0), 3000, 3800, 16, rng, z=-25, scale=(0.8, 1.1), folder="Dressing/Forest", avoid=temple_zone)
    PL = "/Game/Megascans/3D_Plants/"
    plants = [PL + "Fern_00/SM_Fern_0%d.SM_Fern_0%d" % (k, k) for k in (1, 2, 3, 4, 5)] + [PL + "Basket_Grass_01/SM_Basket_Grass_0%d.SM_Basket_Grass_0%d" % (k, k) for k in (1, 2, 4, 5)]
    K.scatter(plants, V(0, 0, 0), 2450, 3500, 160, rng, z=-25, scale=(1.4, 2.6), folder="Dressing/Undergrowth")
    K.scatter(["/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Sapling_0%d.SM_EuropeanBeech_Sapling_0%d" % (k, k) for k in (1, 2, 3, 4)],
              V(0, 0, 0), 2500, 3300, 26, rng, z=-25, scale=(1.0, 1.6), folder="Dressing/Undergrowth")
    horizon("Afghan_Mountain_b", 12.0, z=-2500)

    # Layer 3 — green-gold light through the canopy, mist, pollen, fireflies, birdsong.
    K.lighting(sun_pitch=-52, sun_yaw=105, sun_color=(1.0, 0.93, 0.76), sun_lux=9.0, sky_tint=(0.85, 1.0, 0.85),
               fog_density=0.028, fog_falloff=0.14, fog_color=(0.42, 0.52, 0.38), fog_start=1500, vol_scatter=2.6,
               sky_intensity=1.8, fog_albedo=(220, 240, 205), fog_extinction=0.55)
    K.post_process(exposure_bias=0.5, min_ev=-0.5, max_ev=2.5, bloom=0.55, saturation=1.06, contrast=1.05,
                   gain=(0.98, 1.03, 0.94), vignette=0.36, grain=0.05, temp=6100)
    for i in range(8):
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Environment/P_Fireflies.P_Fireflies", K.polar(2700 + (i % 2) * 500, i * 45 + 20, 200), 2.0, "Fireflies%d" % i)
    for i in range(8):
        K.fx("/Game/StarterContent/Particles/P_Ambient_Dust.P_Ambient_Dust", K.polar(900 + (i % 3) * 700, i * 45, 350), 1.5, "Pollen%d" % i)
    K.ambient("/Game/StarterContent/Audio/Starter_Birds01.Starter_Birds01", K.polar(4500, 200, 1500), 4000, 9000, 0.5, "Amb_Birds")
    K.ambient(K.amb("ForestLeaves"), V(0, 0, 600), 5000, 10000, 0.45, "Amb_Leaves")
    K.ambient(K.amb("Waterfall"), K.polar(5300, 40, 600), 1800, 7000, 0.75, "Amb_Waterfall")

    K.camera_point("Spawn", V(-1550, 0, 300), -8, 0)
    K.camera_point("Temple", V(0, 1900, 420), 6, 90)
    K.camera_point("Centre", V(0, -600, 220), 12, 90)
    K.camera_point("Elevated", V(1700, -1900, 1250), -26, 132)
    K.camera_point("Establishing", V(-4200, -4600, 8200), -42, 47)
    return K.finalize("VerdantSanctuary", V(0, 0, 0), V(5200, 5200, 1400), "/Game/Arenas/Maps/L_Arena_VerdantSanctuary")


# ============================================================================ SUNSCAR EXPANSE
def build_sunscar():
    rng = random.Random(5501)
    K.open_fresh_level("/Game/Arenas/Maps/L_Arena_SunscarExpanse")
    # Layer 1 — a sun-dial plaza of the lost civilisation; visual sand drifts never change collision.
    gameplay_floor(1750, "Paving", "Sandstone", "Sand", 9000, 2350)
    K.cyl(V(0, 0, -99), 520, 100.5, "Sandstone", label="CentreSeal", folder="Gameplay/Floor")
    K.flagstone_floor(1745, 545, 4, ["Paving", "Sandstone", "Paving"], rng)
    for i in range(12):
        deg = i * 30
        mid = K.polar(1150, deg, 4.5)
        K.box(mid, (1100, 5, 2), "GoldEnergy", rot=R(0, deg, 0), folder="Dressing/SunDial")

    # Layer 2 — colonnade half-swallowed by sand, gate with seated colossi, eroded fortifications,
    # buried machinery of an older age, sandstone cliffs and hoodoos.
    for i in range(14):
        deg = i * (360 / 14.0) + 6
        if 75 < deg < 105:
            continue
        sink = rng.uniform(0, 500) if i % 3 == 0 else 0
        p = K.polar(2780, deg, -25 - sink)
        K.column(p, 125, 1600 + rng.uniform(-200, 200), "Sandstone", capital="Paving", broken=(i % 3 == 1), rng=rng)
    gate = V(0, 3900, -25)
    K.gate(gate, -90, 1900, 3800, "Sandstone", trim="Paving", energy="GoldEnergy")
    for s in (-1, 1):
        c = V(s * 2600, 4300, -25)
        K.box(V(c.x, c.y, c.z + 120), (2000, 2200, 240), "Sandstone", folder="Dressing/Colossi")
        K.arch_mesh("SeatedColossus", V(c.x, c.y, c.z + 240), 0, (2.6, 2.6, 2.6), "Sandstone", folder="Dressing/Colossi")
    for i in range(30):
        deg = i * 12 + 6
        if 70 < deg < 110:
            continue
        h = rng.uniform(600, 1900)
        K.arch_mesh("WallRuin" if i % 3 else "Wall", K.polar(4700, deg, -80), deg + 90,
                    (2 * math.pi * 4700 / 30 * 0.98 / 1000.0, 0.9, h / 1100.0), "Sandstone", folder="Dressing/Fortifications")
    dunes = [GEO + "Custom/SM_Sand_01.SM_Sand_01", GEO + "Custom/SM_Sand_02.SM_Sand_02"]
    K.scatter(dunes, V(0, 0, 0), 3600, 7600, 30, rng, z=-60, scale=(3.0, 6.5), folder="Dressing/Dunes", material="Sand",
              avoid=lambda p: abs(p.x) < 1500 and p.y > 2800)
    # Buried machinery: a colossal ring-engine half sunk in the western dunes, faint energy still alive.
    e = V(-6400, -1200, -900)
    for k in range(18):
        a = k * 20
        pa = V(e.x, e.y + math.cos(math.radians(a)) * 2600, e.z + math.sin(math.radians(a)) * 2600)
        K.box(pa, (900, 1000, 500), "UPGAlloy", rot=R(0, 90, a + 90), folder="Dressing/BuriedEngine")
    K.box(V(e.x, e.y, e.z + 2600 + 280), (300, 820, 60), "UPGEnergy", rot=R(0, 90, 0), folder="Dressing/BuriedEngine")
    K.cyl(V(e.x - 1200, e.y + 1800, -200), 380, 2200, "UPGAlloy", rot=R(0, 0, 28), folder="Dressing/BuriedEngine")
    # Sandstone cliffs, buttes and hoodoos — layered mid/background.
    cliffs = [msa("Massive_Canyon_Sandstone_Cliff_venuafk", "S_Massive_Canyon_Sandstone_Cliff_venuafk_high_Var1"),
              msa("Massive_Canyon_Sandstone_Cliff_vjqhcapga", "S_Massive_Canyon_Sandstone_Cliff_vjqhcapga_high_Var1"),
              msa("Massive_Canyon_Sandstone_Cliff_vjrmfb1ab", "S_Massive_Canyon_Sandstone_Cliff_vjrmfb1ab_high_Var1"),
              msa("Huge_Canyon_Sandstone_Butte_venvbgp", "S_Huge_Canyon_Sandstone_Butte_venvbgp_high_Var1")]
    hoodoos = [msa("Huge_Canyon_Sandstone_Hoodoo_vepnfcr", "S_Huge_Canyon_Sandstone_Hoodoo_vepnfcr_high_Var1"),
               msa("Massive_Canyon_Sandstone_Hoodoo_vh3oagiga", "S_Massive_Canyon_Sandstone_Hoodoo_vh3oagiga_high_Var1"),
               msa("Massive_Canyon_Sandstone_Hoodoo_vjqiaa2qx", "S_Massive_Canyon_Sandstone_Hoodoo_vjqiaa2qx_high_Var1")]
    K.scatter(cliffs, V(0, 0, 0), 8500, 14000, 16, rng, z=-200, scale=(1.6, 3.0), folder="Dressing/Cliffs", yaw_free=False)
    K.scatter(hoodoos, V(0, 0, 0), 5600, 8200, 12, rng, z=-120, scale=(0.8, 1.4), folder="Dressing/Hoodoos",
              avoid=lambda p: abs(p.x) < 3500 and p.y > 3000)
    for k in range(10):
        p = K.polar(rng.uniform(16000, 26000), rng.uniform(0, 360), -200)
        h = rng.uniform(2500, 5500)
        K.box(V(p.x, p.y, h / 2), (500, 500, h), "Sandstone", folder="Dressing/DistantRuins", cast_shadow=False)
    K.mesh(MS + "Dead_Tree_01/SM_Dead_Tree_01.SM_Dead_Tree_01", K.polar(3300, 200, -25), R(0, 40, 0), (1.6, 1.6, 1.6), folder="Dressing/Vegetation")
    K.mesh(MS + "Dead_Tree_01/SM_Dead_Tree_01.SM_Dead_Tree_01", K.polar(3500, 330, -25), R(0, 160, 0), (1.3, 1.3, 1.3), folder="Dressing/Vegetation")
    horizon("Desert", 16.0, z=-1200)

    # Layer 3 — intense desert sun, warm dust (light enough to keep defensive timing readable).
    K.lighting(sun_pitch=-38, sun_yaw=235, sun_color=(1.0, 0.88, 0.7), sun_lux=11.0, sky_tint=(1.0, 0.95, 0.85),
               fog_density=0.014, fog_falloff=0.1, fog_color=(0.78, 0.62, 0.44), fog_start=3000, vol_scatter=1.2,
               sky_intensity=1.6, fog_albedo=(255, 236, 205), fog_extinction=0.3)
    K.post_process(exposure_bias=0.2, min_ev=-0.5, max_ev=3.0, bloom=0.55, saturation=1.04, contrast=1.08,
                   gain=(1.05, 1.0, 0.92), vignette=0.32, grain=0.06, temp=5600)
    for i in range(8):
        K.fx("/Game/StarterContent/Particles/P_Ambient_Dust.P_Ambient_Dust", K.polar(1200 + (i % 3) * 800, i * 45, 250), 1.5, "Dust%d" % i)
    for i in range(5):
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Smoke_C.P_Smoke_C", K.polar(4500, i * 72 + 10, 200), 8.0, "SandDrift%d" % i)
    K.ambient(K.amb("Wind_Desert"), V(0, 0, 500), 6000, 12000, 0.55, "Amb_Wind")
    K.ambient(K.amb("SandDrift"), K.polar(4600, 180, 300), 2500, 8000, 0.5, "Amb_Sand")

    K.camera_point("Spawn", V(-1550, 0, 300), -8, 0)
    K.camera_point("Gate", V(0, -1500, 260), 8, 90)
    K.camera_point("Centre", V(0, -600, 220), 12, 90)
    K.camera_point("Elevated", V(1700, -1900, 1250), -26, 132)
    K.camera_point("Establishing", V(-4200, -4600, 8200), -42, 47)
    return K.finalize("SunscarExpanse", V(0, 0, 0), V(5200, 5200, 1400), "/Game/Arenas/Maps/L_Arena_SunscarExpanse")


# ============================================================================ STORMBREAK COAST
def build_stormbreak():
    rng = random.Random(6601)
    K.open_fresh_level("/Game/Arenas/Maps/L_Arena_StormbreakCoast")
    # Layer 1 — fortress platform on a sea stack. Wet look is material-only (same friction).
    gameplay_floor(1750, "CoastStone", "WetRock", "WetRock", 3150, 2350)
    K.cyl(V(0, 0, -99), 520, 100.5, "WetRock", label="CentreSeal", folder="Gameplay/Floor")
    K.flagstone_floor(1745, 545, 4, ["RockyVT", "WetRock", "CoastStone"], rng)
    K.ring_of_boxes(540, 72, (48, 5, 3), 0.4, "UPGEnergy", folder="Dressing/CentreSeal", skip=lambda d: int(d / 5) % 3 == 0)

    # Layer 2 — battlements, broken towers, the UPA beacon, the great bridge, sea stacks and harbour.
    for i in range(30):
        deg = i * 12 + 6
        if 80 < deg < 100:
            continue
        K.arch_mesh("Wall", K.polar(3000, deg, -30), deg + 90, (2 * math.pi * 3000 / 30 * 1.04 / 1000.0, 0.55, 0.38), "CoastStone",
                    folder="Dressing/Battlements")
    for k, deg in enumerate((30, 150, 210, 330)):
        p = K.polar(3050, deg, -25)
        h = 2600 if k % 2 == 0 else 1300
        K.arch_mesh("TowerRound", p, deg, (1.04, 1.04, h / 3340.0), "CoastStone", folder="Dressing/Towers")
        if k % 2 == 1:
            for j in range(5):
                q = K.polar(rng.uniform(300, 700), rng.uniform(0, 360), -25, p.x, p.y)
                K.box(V(q.x, q.y, q.z + 100), (rng.uniform(150, 300), rng.uniform(120, 260), rng.uniform(100, 220)), "CoastStone",
                      rot=R(rng.uniform(-20, 20), rng.uniform(0, 360), rng.uniform(-20, 20)), folder="Dressing/Towers")

    beacon = K.polar(3100, 270, -25)
    K.arch_mesh("TowerRound", beacon, 0, (0.86, 0.86, 1.56), "CoastStone", folder="Dressing/Beacon")
    K.arch_mesh("Lantern", V(beacon.x, beacon.y, beacon.z + 5200), 0, (0.7, 0.7, 0.7), "UPGAlloy", folder="Dressing/Beacon")
    K.cyl(V(beacon.x, beacon.y, beacon.z + 5290), 230, 300, "UPGEnergy", folder="Dressing/Beacon", cast_shadow=False)
    K.point_light(V(beacon.x, beacon.y, beacon.z + 5900), (0.55, 0.75, 1.0), 15000, 5000, "BeaconLight")
    # The great bridge north to the mainland, on tall arches rising from the sea.
    for k in range(10):
        y = 3300 + k * 900
        K.box(V(0, y, -80), (1100, 920, 160), "CoastStone", folder="Dressing/Bridge")
        for s in (-1, 1):
            K.box(V(s * 520, y, 40), (80, 920, 160), "CoastStone", folder="Dressing/Bridge")
        if k % 2 == 1:
            K.arch(V(0, y, -4200), 90, 700, 4000, 1000, 340, "WetRock", folder="Dressing/Bridge")
    # Sea stack cliffs under the platform and around it.
    coast = [msa("Massive_Nordic_Coastal_Cliff_veqnci0db", "S_Massive_Nordic_Coastal_Cliff_veqnci0db_high_Var1"),
             msa("Massive_Nordic_Coastal_Cliff_vdssailfa", "S_Massive_Nordic_Coastal_Cliff_vdssailfa_high"),
             msa("Massive_Nordic_Coastal_Cliff_ve3hahfcb", "S_Massive_Nordic_Coastal_Cliff_ve3hahfcb_high_Var1"),
             msa("Massive_Nordic_Coastal_Cliff_vfmjadufa", "S_Massive_Nordic_Coastal_Cliff_vfmjadufa_high_Var1")]
    for i in range(14):
        deg = i * (360 / 14.0)
        p = K.polar(3500, deg, -4300)
        K.mesh(rng.choice(coast), p, R(0, deg + 180, 0), (2.4, 2.4, 1.15), folder="Dressing/SeaStack")
    for k in range(6):
        c = K.polar(rng.uniform(9000, 15000), rng.uniform(0, 360), -4300)
        if c.y > 2500 and abs(c.x) < 3000:
            continue
        sc = rng.uniform(2.5, 4.0)
        K.mesh(rng.choice(coast), c, R(0, rng.uniform(0, 360), 0), (sc, sc, 1.3), folder="Dressing/OuterStacks")
        if k % 2 == 0:
            K.cyl(V(c.x, c.y, -4300 + 3700), 380, 1400, "CoastStone", folder="Dressing/OuterStacks")
    K.mesh(msa("Massive_Nordic_Coastal_Cliff_vefkehbva", "S_Massive_Nordic_Coastal_Cliff_vefkehbva_high"), V(0, 14500, -4300), R(0, 0, 0), (6.0, 6.0, 4.3), folder="Dressing/Mainland")
    K.box(V(0, 16000, -2200), (16000, 6000, 4600), "WetRock", folder="Dressing/Mainland", cast_shadow=False)
    # Harbour and distant ships.
    for k in range(5):
        K.box(V(-6000 + k * 900, -7000, -4180), (400, 3000, 120), "CoastStone", folder="Dressing/Harbour")
    for k, (x, y, yaw) in enumerate([(-22000, -30000, 20), (18000, -36000, -30), (32000, -15000, 80)]):
        K.box(V(x, y, -4050), (6000, 1300, 700), "Iron", rot=R(0, yaw, 0), folder="Dressing/Ships", cast_shadow=False)
        K.box(V(x, y, -3500), (2400, 900, 700), "UPGAlloy", rot=R(0, yaw, 0), folder="Dressing/Ships", cast_shadow=False)
        K.cyl(V(x, y, -3150), 60, 2600, "Iron", folder="Dressing/Ships", cast_shadow=False)
    K.box(V(0, 0, -4300), (240000, 240000, 40), WATER_OCEAN, label="Ocean", folder="Dressing/Ocean", cast_shadow=False)

    # Layer 3 — storm light, dark clouds, spray, rain and cosmetic lightning.
    K.lighting(sun_pitch=-24, sun_yaw=300, sun_color=(0.72, 0.8, 0.92), sun_lux=3.5, sky_tint=(0.75, 0.82, 0.92),
               fog_density=0.04, fog_falloff=0.06, fog_color=(0.32, 0.37, 0.44), fog_start=2000, vol_scatter=0.8,
               sky_intensity=1.4, fog_albedo=(200, 210, 225), fog_extinction=0.7)
    K.post_process(exposure_bias=0.6, min_ev=-1.0, max_ev=2.0, bloom=0.45, saturation=0.82, contrast=1.12,
                   gain=(0.94, 0.98, 1.05), vignette=0.42, grain=0.08, temp=7400)
    flash = K.EAS.spawn_actor_from_class(unreal.GOTALightningFlash, V(0, 0, 3000), R(-40, 0, 0))
    flash.set_editor_property("thunder", K.asset(K.AUDIO_ROOT + "/A_GOTA_SFX_Thunder.A_GOTA_SFX_Thunder"))
    K._finish(flash, "LightningFlash", "Atmosphere")
    for i in range(10):
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Steam_A.P_Steam_A", K.polar(3400, i * 36, -3900), 12.0, "SeaSpray%d" % i)
    for i in range(4):
        K.ambient(K.amb("Surf"), K.polar(3600, i * 90 + 45, -2500), 3000, 9000, 0.8, "Amb_Surf%d" % i)
    K.ambient(K.amb("Rain"), V(0, 0, 600), 6000, 12000, 0.4, "Amb_Rain")
    K.ambient(K.amb("Wind_Storm"), V(0, 0, 800), 6000, 12000, 0.45, "Amb_Wind")

    K.camera_point("Spawn", V(-1550, 0, 300), -8, 0)
    K.camera_point("Bridge", V(0, 7000, 700), -6, -90)
    K.camera_point("Centre", V(0, -600, 220), 12, 90)
    K.camera_point("Elevated", V(1700, -1900, 1250), -26, 132)
    K.camera_point("Establishing", V(-4200, -4600, 8200), -42, 47)
    return K.finalize("StormbreakCoast", V(0, 0, 0), V(5200, 5200, 1400), "/Game/Arenas/Maps/L_Arena_StormbreakCoast")


# ============================================================================ SHATTERED DOMINION
def build_dominion():
    rng = random.Random(7707)
    K.open_fresh_level("/Game/Arenas/Maps/L_Arena_ShatteredDominion")
    # Layer 1 — the largest floor (up to four fighters): radius 2200, boundary 2750.
    gameplay_floor(2200, "Paving", "UPGAlloy", "QuarryGravel", 3300, 2750, starts_radius=1250)
    K.cyl(V(0, 0, -99), 700, 100.5, "UPGAlloy", label="CentreSeal", folder="Gameplay/Floor")
    K.flagstone_floor(2195, 745, 5, ["Paving", "QuarryBlock", "Paving", "TempleStone"], rng)
    K.ring_of_boxes(720, 96, (46, 6, 3), 0.4, "UPGEnergy", folder="Dressing/CentreSeal", skip=lambda d: int(d / 3.75) % 4 == 0)
    for i in range(6):
        K.box(K.polar(1480, i * 60 + 30, 4.5), (1500, 14, 3), "GoldEnergy", rot=R(0, i * 60 + 30, 0), folder="Dressing/CentreSeal")

    # Six regions around the centre, one per 60° sector (centre angle = i*60 + 30).
    REG = [("Quarry", 30), ("Volcanic", 90), ("Frozen", 150), ("Forest", 210), ("Desert", 270), ("Coastal", 330)]
    ground = {"Quarry": "QuarryGravel", "Volcanic": "Basalt", "Frozen": "Snow", "Forest": "ForestGround", "Desert": "Sand", "Coastal": "WetRock"}
    stone = {"Quarry": "QuarryStone", "Volcanic": "Obsidian", "Frozen": "FrostStone", "Forest": "TempleStone", "Desert": "Sandstone", "Coastal": "CoastStone"}

    def in_sector(deg, centre, half=30.0):
        d = (deg - centre + 180.0) % 360.0 - 180.0
        return abs(d) <= half

    # Region ground plates (wedges approximated by rotated slabs), lowered so the central platform reads as raised.
    for name, c in REG:
        for k in range(6):
            r = 4200 + k * 1800
            for j in range(5):
                deg = c - 24 + j * 12
                K.box(K.polar(r, deg, -160), (1900, 2 * math.pi * r / 30 * 1.3, 240), ground[name], rot=R(0, deg, 0),
                      folder="Dressing/Regions/" + name, cast_shadow=False)
    # Transitional rock spines along each sector edge (charred rock / eroded canyon / wet stone blends).
    edge_rocks = [GEO + "SM_Quarry_RockPile_S_01.SM_Quarry_RockPile_S_01", GEO + "SM_Lava_RockMound_S_01.SM_Lava_RockMound_S_01",
                  GEO + "SM_Quarry_Ridge_S_02.SM_Quarry_Ridge_S_02", "/Game/WinterForestSet/Meshes/SM_WinterRock_C.SM_WinterRock_C"]
    for i in range(6):
        edge = i * 60
        for k in range(10):
            p = K.polar(3800 + k * 1100 + rng.uniform(-200, 200), edge + rng.uniform(-4, 4), -60)
            s = rng.uniform(1.6, 3.2)
            K.mesh(rng.choice(edge_rocks), p, R(0, rng.uniform(0, 360), 0), (s, s, s), folder="Dressing/Transitions")

    # Bridges from the platform to every region, in that region's stone.
    for name, c in REG:
        for k in range(5):
            p = K.polar(3500 + k * 700, c, -120)
            K.box(p, (720, 900, 140), stone[name], rot=R(0, c, 0), folder="Dressing/Bridges/" + name)
            for s in (-1, 1):
                q = K.polar(470, c + 90 * s, 0)
                K.box(V(p.x + q.x, p.y + q.y, 20), (720, 50, 160), stone[name], rot=R(0, c, 0), folder="Dressing/Bridges/" + name)

    # Monument towers on the six sector edges: ancient stone base fused with UPG alloy and energy.
    for i in range(6):
        deg = i * 60
        p = K.polar(3500, deg, -25)
        K.arch_mesh("Monument", p, deg, (0.9, 0.9, 0.95), "QuarryBlock", folder="Dressing/Monuments")
        K.arch_mesh("MonumentUpper", V(p.x, p.y, p.z + 3200), deg, (0.9, 0.9, 0.9), "UPGAlloy", folder="Dressing/Monuments")
        inward = K.polar(3500 - 460, deg, -25)
        K.box(V(inward.x, inward.y, inward.z + 3900), (8, 300, 2600), "UPGEnergy", rot=R(0, deg, 0), folder="Dressing/Monuments")
        K.box(V(p.x, p.y, p.z + 3240), (1300, 1300, 80), "GoldEnergy", rot=R(0, deg, 0), folder="Dressing/Monuments")
    # The Sovereign Ring: a colossal floating halo over the arena, with a light column — the central landmark.
    K.arch_mesh("SovereignRing", V(0, 0, 7200), 0, (1.0, 1.0, 1.0), "UPGAlloy", folder="Dressing/SovereignRing")
    K.arch_mesh("SovereignRingGlow", V(0, 0, 7200), 0, (1.0, 1.0, 1.0), "UPGEnergy", folder="Dressing/SovereignRing", cast_shadow=False)
    K.cyl(V(0, 0, 7400), 90, 60000, "UPGEnergy", folder="Dressing/SovereignRing", cast_shadow=False)
    K.point_light(V(0, 0, 6800), (0.6, 0.78, 1.0), 20000, 6000, "RingLight")

    # --- Quarry region (0–60°): terraces and spires.
    for k in range(3):
        r = 7200 + k * 1400
        for j in range(7):
            deg = 6 + j * 8
            K.box(K.polar(r, deg, 600 + k * 900), (1500, 2 * math.pi * r / 45, 1500 + k * 1800), "QuarryStone", rot=R(0, deg + 90, 0), folder="Dressing/Regions/Quarry")
    K.scatter([GEO + "SM_Quarry_Spire_L_01.SM_Quarry_Spire_L_01", GEO + "SM_Quarry_Spire_L_02.SM_Quarry_Spire_L_02",
               GEO + "SM_Quarry_Cliff_L_02.SM_Quarry_Cliff_L_02"], V(0, 0, 0), 4600, 6800, 10, rng, z=-60, scale=(1.4, 2.4),
              arc=(5, 55), folder="Dressing/Regions/Quarry")
    # --- Volcanic (60–120°): lava field, obsidian spires, glow.
    K.box(K.polar(8500, 90, -240), (9000, 6500, 40), "Lava", rot=R(0, 90, 0), folder="Dressing/Regions/Volcanic", cast_shadow=False)
    K.scatter([MS + "IcelandicLavaSpire/SM_IcelandicLavaSpire_01.SM_IcelandicLavaSpire_01", GEO + "SM_Lava_Cliff_L_04.SM_Lava_Cliff_L_04",
               GEO + "SM_Lava_RockMound_S_03.SM_Lava_RockMound_S_03"], V(0, 0, 0), 5000, 11000, 18, rng, z=-240, scale=(1.5, 3.0),
              arc=(64, 116), folder="Dressing/Regions/Volcanic")
    for k in range(4):
        K.point_light(K.polar(6000 + k * 1500, 90, 200), (1.0, 0.3, 0.06), 80000, 5000, "LavaGlow%d" % k)
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Sparks/P_Embers_A.P_Embers_A", K.polar(6000 + k * 1200, 85 + k * 3, 300), 0.8, "DomEmbers%d" % k)
    # --- Frozen (120–180°): ice cliffs, snow-capped stone, pines.
    for k in range(12):
        deg = rng.uniform(124, 176)
        h = rng.uniform(2000, 6000)
        K.box(K.polar(rng.uniform(7000, 11000), deg, h / 2 - 300), (rng.uniform(1200, 2600), rng.uniform(900, 2000), h), "Ice",
              rot=R(rng.uniform(-6, 6), rng.uniform(0, 360), rng.uniform(-6, 6)), folder="Dressing/Regions/Frozen")
    K.scatter(["/Game/WinterForestSet/Meshes/SM_WinterTree_%s.SM_WinterTree_%s" % (c, c) for c in "ABCD"], V(0, 0, 0), 4600, 7000, 14, rng,
              z=-60, scale=(0.45, 0.75), arc=(126, 174), folder="Dressing/Regions/Frozen")
    # --- Forest (180–240°): beeches and maples, mossy ruins.
    K.scatter(["/Game/EuropeanBeech/Geometry/SimpleWind/SM_EuropeanBeech_Forest_0%d.SM_EuropeanBeech_Forest_0%d" % (k, k) for k in (1, 2, 3)]
              + ["/Game/NorwayMaple/Geometry/SimpleWind/SM_NorwayMaple_Forest_0%d.SM_NorwayMaple_Forest_0%d" % (k, k) for k in (1, 2)],
              V(0, 0, 0), 4800, 11000, 22, rng, z=-60, scale=(0.8, 1.15), arc=(184, 236), folder="Dressing/Regions/Forest")
    for k in range(5):
        p = K.polar(rng.uniform(4200, 5600), rng.uniform(186, 234), -60)
        K.column(p, 110, rng.uniform(600, 1400), "MossStone", broken=True, rng=rng, folder="Dressing/Regions/Forest")
    # --- Desert (240–300°): dunes, hoodoos, half-buried columns.
    K.scatter([GEO + "Custom/SM_Sand_01.SM_Sand_01", GEO + "Custom/SM_Sand_02.SM_Sand_02"], V(0, 0, 0), 4500, 11000, 14, rng, z=-120,
              scale=(3.0, 6.0), arc=(244, 296), material="Sand", folder="Dressing/Regions/Desert")
    K.scatter([msa("Massive_Canyon_Sandstone_Hoodoo_vh3oagiga", "S_Massive_Canyon_Sandstone_Hoodoo_vh3oagiga_high_Var1"),
               msa("Huge_Canyon_Sandstone_Hoodoo_vepnfcr", "S_Huge_Canyon_Sandstone_Hoodoo_vepnfcr_high_Var1")],
              V(0, 0, 0), 6500, 11000, 7, rng, z=-120, scale=(0.9, 1.5), arc=(246, 294), folder="Dressing/Regions/Desert")
    for k in range(5):
        p = K.polar(rng.uniform(4200, 5800), rng.uniform(248, 292), -160 - rng.uniform(0, 300))
        K.column(p, 120, rng.uniform(900, 1600), "Sandstone", broken=(k % 2 == 0), rng=rng, folder="Dressing/Regions/Desert")
    # --- Coastal (300–360°): cliffs falling to the sea, wet stone.
    K.box(K.polar(16000, 330, -2600), (30000, 26000, 40), WATER_OCEAN, rot=R(0, 330, 0), folder="Dressing/Regions/Coastal", cast_shadow=False)
    coast = [msa("Massive_Nordic_Coastal_Cliff_ve3hahfcb", "S_Massive_Nordic_Coastal_Cliff_ve3hahfcb_high_Var1"),
             msa("Massive_Nordic_Coastal_Cliff_vfmjadufa", "S_Massive_Nordic_Coastal_Cliff_vfmjadufa_high_Var1")]
    for k in range(8):
        deg = 304 + k * 7
        K.mesh(rng.choice(coast), K.polar(9000 + (k % 2) * 900, deg, -2600), R(0, deg + 180, 0), (2.0, 2.0, 1.9), folder="Dressing/Regions/Coastal")
    for i in range(4):
        K.fx("/Game/Realistic_Starter_VFX_Pack_Vol2/Particles/Smoke/P_Steam_A.P_Steam_A", K.polar(9500, 305 + i * 15, -2300), 10.0, "DomSpray%d" % i)
    horizon("Afghan_Mountain_a", 14.0, z=-3000)

    # Layer 3 — one coherent sun with regional colour from local lights, fog and audio zones.
    K.lighting(sun_pitch=-40, sun_yaw=150, sun_color=(1.0, 0.9, 0.78), sun_lux=9.0, fog_density=0.02, fog_falloff=0.1,
               fog_color=(0.5, 0.48, 0.45), fog_start=3000, vol_scatter=1.4, sky_intensity=1.8, fog_albedo=(240, 235, 228), fog_extinction=0.4)
    K.post_process(exposure_bias=0.6, min_ev=-0.5, max_ev=2.5, bloom=0.55, saturation=1.04, contrast=1.06, gain=(1.0, 1.0, 1.0),
                   vignette=0.35, grain=0.05, temp=6300)
    for i in range(8):
        K.fx("/Game/StarterContent/Particles/P_Ambient_Dust.P_Ambient_Dust", K.polar(1300 + (i % 3) * 800, i * 45, 300), 1.5, "Dust%d" % i)
    K.ambient(K.amb("Wind_Quarry"), V(0, 0, 600), 4000, 9000, 0.4, "Amb_Centre")
    zone = {"Quarry": "Quarry_Machinery", "Volcanic": "Lava", "Frozen": "Wind_Frost", "Forest": "ForestLeaves", "Desert": "Wind_Desert", "Coastal": "Surf"}
    for name, c in REG:
        K.ambient(K.amb(zone[name]), K.polar(6500, c, 300), 3000, 6500, 0.65, "Amb_" + name)

    K.camera_point("Spawn", V(-2000, 0, 320), -8, 0)
    K.camera_point("Ring", V(0, -1800, 260), 18, 90)
    K.camera_point("Centre", V(0, -700, 240), 8, 90)
    K.camera_point("Elevated", V(2200, -2400, 1600), -26, 132)
    K.camera_point("Establishing", V(-6000, -6500, 11000), -40, 47)
    return K.finalize("ShatteredDominion", V(0, 0, 0), V(6200, 6200, 1400), "/Game/Arenas/Maps/L_Arena_ShatteredDominion")


# ============================================================================ DEV: architecture showroom (not cooked)
ARCH = "/Game/Arenas/Shared/Architecture/SM_GOTA_Arch_"


def arch(name):
    return ARCH + name + "." + "SM_GOTA_Arch_" + name


def build_showroom():
    K.open_fresh_level("/Game/Arenas/Dev/L_ArchShowroom")
    K.box(V(0, 0, -50), (60000, 20000, 100), "Paving", collision="BlockAll", folder="Gameplay/Floor")
    row = [("Column", "QuarryStone", 1.0), ("ColumnBroken", "QuarryStone", 1.0), ("ColumnDrum", "QuarryStone", 1.0),
           ("TowerRound", "FrostStone", 0.8), ("TowerSquare", "Obsidian", 0.7), ("Wall", "CoastStone", 1.0),
           ("WallRuin", "CoastStone", 1.0), ("Gate", "Sandstone", 0.7), ("Obelisk", "Obsidian", 0.8),
           ("Colossus", "QuarryBlock", 2.0), ("ColossusBroken", "QuarryBlock", 2.0), ("SeatedColossus", "Sandstone", 2.0),
           ("Brazier", "Iron", 2.0), ("Crucible", "RustIron", 0.6), ("Monument", "QuarryBlock", 0.6),
           ("MonumentUpper", "UPGAlloy", 0.6), ("GantryTower", "RustIron", 0.5), ("Lantern", "UPGAlloy", 0.8),
           ("BridgeArch", "WetRock", 0.5)]
    x = -24000
    for name, m, s in row:
        K.mesh(arch(name), V(x, 0, 0), R(0, -90, 0), (s, s, s), m, folder="Dressing/Showroom", label=name)
        x += 2600
    K.mesh(arch("SovereignRing"), V(0, 9000, 2500), R(70, 0, 0), (0.6, 0.6, 0.6), "UPGAlloy", folder="Dressing/Showroom")
    K.lighting(sun_pitch=-35, sun_yaw=210, sun_color=(1.0, 0.92, 0.8), sun_lux=10.0, fog_density=0.005, clouds=False)
    K.post_process(exposure_bias=0.4)
    return K.finalize("Showroom", V(0, 0, 0), V(2000, 2000, 400), "/Game/Arenas/Dev/L_ArchShowroom")


ARENAS = {"Showroom": build_showroom, "Dominion": build_dominion, "Quarry": build_quarry, "Ashfall": build_ashfall, "Frostbound": build_frostbound,
          "Verdant": build_verdant, "Sunscar": build_sunscar, "Stormbreak": build_stormbreak}


def main():
    K.import_ambience(os.path.join(os.path.dirname(os.path.abspath(__file__)), "ArenaAudioSource"))
    wanted = [a.strip() for a in os.environ.get("GOTA_ARENAS", "Quarry").split(",") if a.strip()]
    for name in wanted:
        fn = ARENAS.get(name)
        if not fn:
            log("unknown arena " + name)
            continue
        nav_ok, saved = fn()
        log("%s: %d actors, nav=%s, saved=%s, missing assets=%s" % (name, K.STATS["actors"], nav_ok, saved, sorted(set(K.STATS["missing"]))))


try:
    main()
    RESULT = "PASS"
except Exception as exc:  # noqa
    import traceback
    log("ERROR " + str(exc))
    log(traceback.format_exc())
    RESULT = "FAIL"
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(proj, "Saved", "GOTA_BuildArenaLevels.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(LOG) + "\nRESULT: " + RESULT + "\n")
if os.environ.get("GOTA_QUIT_AFTER_BUILD") == "1":
    unreal.SystemLibrary.quit_editor()
