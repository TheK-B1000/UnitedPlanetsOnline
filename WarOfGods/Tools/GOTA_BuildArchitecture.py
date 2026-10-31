# Arena Environment Pass 2 — procedural architecture library (Geometry Script, full editor).
#   GOTA_QUIT_AFTER_BUILD=1 UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript=<abs>/Tools/GOTA_BuildArchitecture.py
# Builds Nanite static meshes in /Game/Arenas/Shared/Architecture (SM_GOTA_Arch_*): real profiles, bevels,
# fluting, crenellations, openings and weathering — replacing textured primitives at landmarks.
# Pieces are authored at a reference size (cm) and scaled by the arena recipes. Decorative only
# (no collision); gameplay surfaces are unchanged. Deterministic (fixed noise seeds).
import math
import os

import unreal

ROOT = "/Game/Arenas/Shared/Architecture"
GSP = unreal.GeometryScript_Primitives
GSB = unreal.GeometryScript_MeshBooleans
GSD = unreal.GeometryScript_MeshDeformers
GSM = unreal.GeometryScript_MeshModeling
GSN = unreal.GeometryScript_Normals
GSE = unreal.GeometryScript_MeshEdits
EAL = unreal.EditorAssetLibrary
LOG = []
ONLY = [s.strip() for s in os.environ.get("GOTA_ARCH_ONLY", "").split(",") if s.strip()]


def log(m):
    LOG.append(m)
    unreal.log("[GOTA arch] " + m)


def V(x, y, z):
    return unreal.Vector(float(x), float(y), float(z))


def V2(x, y):
    return unreal.Vector2D(float(x), float(y))


def XF(loc=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
    return unreal.Transform(location=V(*loc), rotation=unreal.Rotator(roll=rot[2], pitch=rot[0], yaw=rot[1]), scale=V(*scale))


def opts():
    return unreal.GeometryScriptPrimitiveOptions()


def new():
    return unreal.DynamicMesh()


# ------------------------------------------------------------------ primitive helpers (all append into `m`)
def box(m, loc, size, rot=(0, 0, 0)):
    """Box with its base centred at loc (Z up)."""
    GSP.append_box(m, opts(), XF(loc, rot), size[0], size[1], size[2], 0, 0, 0, unreal.GeometryScriptPrimitiveOriginMode.BASE)


def cyl(m, loc, radius, height, steps=24, rot=(0, 0, 0)):
    GSP.append_cylinder(m, opts(), XF(loc, rot), radius, height, steps, 0, True, unreal.GeometryScriptPrimitiveOriginMode.BASE)


def revolve(m, loc, profile, steps=40, rot=(0, 0, 0)):
    """profile: list of (radius, z) from bottom to top, revolved around Z."""
    ro = unreal.GeometryScriptRevolveOptions()
    GSP.append_revolve_path(m, opts(), XF(loc, rot), [V2(r, z) for r, z in profile], ro, steps, True)


def subtract(m, tool):
    GSB.apply_mesh_boolean(m, XF(), tool, XF(), unreal.GeometryScriptBooleanOperation.SUBTRACT, unreal.GeometryScriptMeshBooleanOptions())


def union_into(m, other):
    GSB.apply_mesh_boolean(m, XF(), other, XF(), unreal.GeometryScriptBooleanOperation.UNION, unreal.GeometryScriptMeshBooleanOptions())


def plane_cut(m, origin, pitch=0.0, roll=0.0, yaw=0.0):
    """Keeps the part *below* a (tilted) plane through `origin` — broken tops."""
    frame = XF(origin, (pitch, yaw, roll))
    o = unreal.GeometryScriptMeshPlaneCutOptions()
    o.set_editor_property("flip_cut_side", False)
    GSB.apply_mesh_plane_cut(m, frame, o)


def weather(m, magnitude, frequency, seed):
    """Low-amplitude Perlin displacement along normals: erosion, chipped irregular stone."""
    no = unreal.GeometryScriptPerlinNoiseOptions()
    layer = no.get_editor_property("base_layer")
    layer.set_editor_property("magnitude", magnitude)
    layer.set_editor_property("frequency", frequency)
    layer.set_editor_property("random_seed", seed)
    no.set_editor_property("base_layer", layer)
    no.set_editor_property("apply_along_normal", True)
    GSD.apply_perlin_noise_to_mesh2(m, unreal.GeometryScriptMeshSelection(), no)


def bevel(m, dist, subdiv=1):
    bo = unreal.GeometryScriptMeshBevelOptions()
    bo.set_editor_property("bevel_distance", dist)
    bo.set_editor_property("subdivisions", subdiv)
    GSM.apply_mesh_polygroup_bevel(m, bo)


def finish_normals(m, angle=40.0):
    so = unreal.GeometryScriptSplitNormalsOptions()
    so.set_editor_property("split_by_opening_angle", True)
    so.set_editor_property("opening_angle_deg", angle)
    GSN.compute_split_normals(m, so, unreal.GeometryScriptCalculateNormalsOptions())


def save(m, name):
    path = ROOT + "/SM_GOTA_Arch_" + name
    finish_normals(m)
    o = unreal.GeometryScriptCreateNewStaticMeshAssetOptions()
    o.set_editor_property("enable_nanite", True)
    o.set_editor_property("enable_collision", False)
    o.set_editor_property("enable_recompute_normals", False)
    o.set_editor_property("enable_recompute_tangents", True)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    res = unreal.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(m, path, o)
    mesh = res[0] if isinstance(res, (tuple, list)) else res
    if mesh:
        EAL.save_asset(path)
        box3 = mesh.get_bounding_box()
        s = box3.max - box3.min
        log("OK   %s  size=(%.0f, %.0f, %.0f)  tris=%d" % (name, s.x, s.y, s.z, m.get_triangle_count() if hasattr(m, "get_triangle_count") else -1))
    else:
        log("FAIL %s  (%s)" % (name, res))


# ================================================================== detail passes
def densify(m, edge):
    """Uniform remesh to ~edge cm so weathering noise has geometry to move (Nanite keeps it cheap)."""
    try:
        uo = unreal.GeometryScriptUniformRemeshOptions()
        uo.set_editor_property("target_type", unreal.GeometryScriptUniformRemeshTargetType.TARGET_EDGE_LENGTH)
        uo.set_editor_property("target_edge_length", edge)
        unreal.GeometryScript_Remeshing.apply_uniform_remesh(m, unreal.GeometryScriptRemeshOptions(), uo)
    except Exception as exc:  # noqa
        log("densify fallback (%s)" % exc)
        unreal.GeometryScript_MeshSubdivide.apply_uniform_tessellation(m, unreal.GeometryScriptUniformTessellationOptions(), 2)


def erode(m, edge, coarse, fine, seed):
    """Remesh, then two noise octaves: broad erosion plus fine chipping."""
    densify(m, edge)
    weather(m, coarse, 0.004, seed)
    weather(m, fine, 0.03, seed + 1)


def cone(m, loc, r0, r1, h, steps=24, rot=(0, 0, 0)):
    GSP.append_cone(m, opts(), XF(loc, rot), r0, r1, h, steps, 0, True, unreal.GeometryScriptPrimitiveOriginMode.BASE)


def torus(m, loc, major, minor, msteps=48, nsteps=10, rot=(0, 0, 0)):
    GSP.append_torus(m, opts(), XF(loc, rot), unreal.GeometryScriptRevolveOptions(), major, minor, msteps, nsteps,
                     unreal.GeometryScriptPrimitiveOriginMode.CENTER)


def beam(m, a, b, t):
    dx, dy, dz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    L = math.sqrt(dx * dx + dy * dy + dz * dz)
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = math.degrees(math.atan2(dz, math.sqrt(dx * dx + dy * dy)))
    GSP.append_box(m, opts(), XF(((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2), (pitch, yaw, 0)), L, t, t, 0, 0, 0,
                   unreal.GeometryScriptPrimitiveOriginMode.CENTER)


# ================================================================== pieces
def column_profile(r, h):
    """Attic base, entasis shaft, echinus capital: radius r, total height h (without abacus)."""
    b = r * 0.55
    return [(0, 0), (r * 1.32, 0), (r * 1.32, b * 0.35), (r * 1.22, b * 0.5), (r * 1.26, b * 0.62),
            (r * 1.12, b * 0.8), (r * 1.08, b), (r, b * 1.12),
            (r * 1.0, h * 0.33), (r * 0.97, h * 0.6), (r * 0.9, h * 0.9),
            (r * 0.93, h * 0.905), (r * 0.93, h * 0.92), (r * 0.9, h * 0.925),
            (r * 1.05, h * 0.96), (r * 1.22, h), (0, h)]


def build_column(fluted=True, seed=7):
    r, h = 100.0, 1200.0
    m = new()
    revolve(m, (0, 0, 0), column_profile(r, h), steps=48)
    box(m, (0, 0, h), (r * 2.7, r * 2.7, r * 0.42))
    if fluted:
        for i in range(20):
            a = math.radians(i * 18.0)
            tool = new()
            cyl(tool, (math.cos(a) * r, math.sin(a) * r, r * 0.75), r * 0.13, h * 0.8, steps=10)
            subtract(m, tool)
    plinth = new()
    box(plinth, (0, 0, -r * 0.5), (r * 2.9, r * 2.9, r * 0.5))
    union_into(m, plinth)
    erode(m, 15.4, 3.0, 0.9, seed)
    return m


def build_broken_column():
    m = build_column(True, seed=19)
    plane_cut(m, (0, 0, 560), pitch=14.0, roll=-9.0)
    weather(m, 4.0, 0.05, 23)
    return m


def build_drum():
    """Fallen column drum: fluted shaft segment with broken, tilted ends."""
    r = 100.0
    m = new()
    cyl(m, (0, 0, 0), r, 260, steps=48)
    for i in range(20):
        a = math.radians(i * 18.0)
        tool = new()
        cyl(tool, (math.cos(a) * r, math.sin(a) * r, -20), r * 0.13, 300, steps=10)
        subtract(m, tool)
    plane_cut(m, (0, 0, 240), pitch=8.0)
    erode(m, 13.2, 4.0, 1.2, 31)
    return m


def build_round_tower():
    """Round tower: battered plinth, string courses, slit windows, corbelled crenellated parapet (r=500, h=3000)."""
    r, h = 500.0, 3000.0
    m = new()
    revolve(m, (0, 0, 0), [(0, 0), (r * 1.18, 0), (r * 1.12, 240), (r * 1.02, 330), (r, 360), (r, h), (0, h)], steps=64)
    for z in (h * 0.38, h * 0.7):
        torus(m, (0, 0, z), r + 8, 22, 64, 8)
    for i in range(32):
        a = i * (360.0 / 32)
        box(m, (math.cos(math.radians(a)) * (r + 30), math.sin(math.radians(a)) * (r + 30), h - 40), (60, 90, 120), rot=(0, a, 0))
    revolve(m, (0, 0, 0), [(r * 0.86, h + 80), (r * 1.12, h + 80), (r * 1.12, h + 330), (r * 0.86, h + 330)], steps=64)
    cyl(m, (0, 0, h + 60), r * 1.12, 30, steps=64)
    for i in range(14):
        a = math.radians(i * (360.0 / 14) + 12.86)
        t = new()
        box(t, (math.cos(a) * r, math.sin(a) * r, h + 210), (r * 0.6, 120, 200), rot=(0, math.degrees(a) + 90, 0))
        subtract(m, t)
    for lvl, z in enumerate((900, 1700, 2400)):
        for i in range(4):
            a = math.radians(i * 90 + lvl * 45)
            t = new()
            box(t, (math.cos(a) * r, math.sin(a) * r, z), (220, 36, 260), rot=(0, math.degrees(a), 0))
            subtract(m, t)
    erode(m, 35.2, 5.0, 1.5, 11)
    return m


def build_square_tower():
    """Square keep: battered plinth, corner quoins, string course, machicolated cornice, crenels, windows."""
    w, h = 900.0, 3200.0
    m = new()
    box(m, (0, 0, 0), (w + 160, w + 160, 260))
    box(m, (0, 0, 260), (w + 70, w + 70, 70))
    box(m, (0, 0, 0), (w, w, h))
    for k in range(9):
        z = 330 + k * 300
        for sx in (-1, 1):
            for sy in (-1, 1):
                lw = 200 if k % 2 == 0 else 130
                box(m, (sx * (w / 2 - lw / 2 + 12), sy * (w / 2 - 38), z), (lw, 100, 140))
                box(m, (sx * (w / 2 - 38), sy * (w / 2 - lw / 2 + 12), z), (100, lw, 140))
    box(m, (0, 0, h * 0.62), (w + 50, w + 50, 50))
    for side in range(4):
        for i in range(7):
            t = -w / 2 + 80 + i * (w - 160) / 6
            if side == 0:
                p = (w / 2 + 40, t)
            elif side == 1:
                p = (t, w / 2 + 40)
            elif side == 2:
                p = (-w / 2 - 40, t)
            else:
                p = (t, -w / 2 - 40)
            box(m, (p[0], p[1], h - 160), (80, 80, 160))
    box(m, (0, 0, h), (w + 180, w + 180, 90))
    box(m, (0, 0, h + 90), (w + 180, w + 180, 260))
    inner = new()
    box(inner, (0, 0, h + 140), (w + 20, w + 20, 300))
    subtract(m, inner)
    for side in range(4):
        for i in range(4):
            t = -w / 2 + 70 + i * (w + 40) / 3.5
            cut = new()
            if side == 0:
                box(cut, (w / 2 + 90, t, h + 200), (220, 110, 200))
            elif side == 1:
                box(cut, (t, w / 2 + 90, h + 200), (110, 220, 200))
            elif side == 2:
                box(cut, (-w / 2 - 90, t, h + 200), (220, 110, 200))
            else:
                box(cut, (t, -w / 2 - 90, h + 200), (110, 220, 200))
            subtract(m, cut)
    for side in range(4):
        a = math.radians(side * 90)
        for z in (1100, 2200):
            cx, cy = math.cos(a) * w / 2, math.sin(a) * w / 2
            cut = new()
            box(cut, (cx, cy, z), (220 if side % 2 == 0 else 150, 150 if side % 2 == 0 else 220, 300))
            subtract(m, cut)
    erode(m, 39.6, 5.0, 1.6, 41)
    return m


def build_wall(ruined=False):
    """Curtain wall segment (1000 long, 500 thick, 1400 high): batter, string course, merlons, drain slits."""
    L, T, H = 1000.0, 500.0, 1400.0
    m = new()
    box(m, (0, 0, 0), (L, T + 120, 260))
    box(m, (0, 0, 220), (L, T + 50, 80))
    box(m, (0, 0, 0), (L, T, H))
    box(m, (0, 0, H * 0.55), (L, T + 40, 40))
    box(m, (0, 0, H), (L, T + 70, 60))
    for i in range(5):
        x = -L / 2 + 100 + i * 200
        box(m, (x, T / 2 - 50, H + 60), (120, 100, 220))
    for i in range(3):
        cut = new()
        box(cut, (-L / 3 + i * L / 3, 0, H * 0.3), (40, T + 200, 180))
        subtract(m, cut)
    if ruined:
        plane_cut(m, (L * 0.15, 0, H * 0.62))
        plane_cut(m, (-L * 0.1, 0, H * 0.75), pitch=22.0)
        erode(m, 26.4, 9.0, 2.5, 53)
    else:
        erode(m, 26.4, 4.0, 1.4, 59)
    return m


def build_gate():
    """Monumental gate: stepped pylons with cornices, deep lintel with relief bands, arched opening (span 1600, h 3400)."""
    span, h, pw = 1600.0, 3400.0, 620.0
    m = new()
    for s in (-1, 1):
        x = s * (span / 2 + pw / 2)
        box(m, (x, 0, 0), (pw * 1.5, pw * 1.35, 380))
        box(m, (x, 0, 380), (pw * 1.25, pw * 1.15, 120))
        box(m, (x, 0, 500), (pw, pw * 0.92, h - 500))
        box(m, (x, 0, h * 0.48), (pw * 1.08, pw * 1.0, 60))
        box(m, (x, 0, h - 40), (pw * 1.18, pw * 1.08, 80))
        for k in range(4):
            box(m, (x + (k - 1.5) * pw * 0.22, -pw * 0.47, 900), (pw * 0.12, 40, h - 1500))
    box(m, (0, 0, h), (span + pw * 2.5, pw * 1.15, 200))
    box(m, (0, 0, h + 200), (span + pw * 2.2, pw * 1.0, 420))
    box(m, (0, 0, h + 620), (span + pw * 2.6, pw * 1.2, 110))
    for k in range(9):
        box(m, (-span / 2 - pw * 0.9 + k * (span + pw * 1.8) / 8, -pw * 0.53, h + 320), (90, 50, 220))
    infill = new()
    box(infill, (0, 0, h - span / 2 - 40), (span, pw * 0.7, span / 2 + 40))
    arch = new()
    cyl(arch, (0, pw, h - span / 2 - 40), span / 2, pw * 2, steps=40, rot=(90, 0, 0))
    subtract(infill, arch)
    union_into(m, infill)
    erode(m, 39.6, 6.0, 1.8, 61)
    return m


def build_obelisk():
    m = new()
    box(m, (0, 0, 0), (340, 340, 160))
    box(m, (0, 0, 160), (290, 290, 60))
    cone(m, (0, 0, 220), 150, 102, 2400, steps=4, rot=(0, 45, 0))
    cone(m, (0, 0, 2620), 102, 0, 190, steps=4, rot=(0, 45, 0))
    for k in range(6):
        cut = new()
        box(cut, (0, -110, 700 + k * 260), (60, 40, 140))
        subtract(m, cut)
    erode(m, 17.6, 3.0, 1.0, 71)
    return m


def build_colossus(broken=False):
    """Robed standing colossus (height ~1060 at scale 1): hem flare, robe folds, belt, shoulders, arms, head."""
    m = new()
    revolve(m, (0, 0, 0), [(0, 0), (175, 0), (168, 30), (140, 140), (122, 380), (112, 520), (120, 560), (118, 590),
                           (108, 620), (122, 760), (150, 800), (132, 840), (70, 860), (62, 880), (0, 885)], steps=40)
    for i in range(14):
        a = math.radians(i * (360 / 14.0))
        cyl(m, (math.cos(a) * 118, math.sin(a) * 118, 10), 14, 520, steps=8)
    for s in (-1, 1):
        beam(m, (s * 150, 0, 790), (s * 175, 35, 520), 62)
        beam(m, (s * 175, 35, 520), (s * 120, 85, 390), 52)
    revolve(m, (0, 0, 870), [(0, 0), (56, 10), (70, 60), (66, 120), (48, 160), (0, 175)], steps=28)
    box(m, (0, 0, -30), (420, 420, 60))
    if broken:
        plane_cut(m, (0, 0, 470), pitch=11.0, roll=6.0)
        erode(m, 9.0, 5.0, 1.4, 83)
    else:
        erode(m, 9.0, 3.0, 1.0, 89)
    return m


def build_seated_colossus():
    """Seated colossus on a throne (height ~1100): throne, lap, shins, robed torso, head."""
    m = new()
    box(m, (0, 0, 0), (620, 560, 360))
    box(m, (0, 140, 360), (620, 280, 720))
    box(m, (0, -40, 340), (430, 420, 120))
    for s in (-1, 1):
        box(m, (s * 110, -260, 0), (170, 150, 400))
        box(m, (s * 110, -320, 0), (190, 130, 50))
        box(m, (s * 255, -40, 360), (90, 420, 230))
        box(m, (s * 110, -190, 440), (160, 140, 60))
    revolve(m, (0, 40, 460), [(0, 0), (190, 0), (175, 220), (160, 330), (180, 380), (90, 420), (0, 425)], steps=36)
    revolve(m, (0, 40, 880), [(0, 0), (60, 10), (72, 70), (66, 130), (40, 165), (0, 175)], steps=28)
    box(m, (0, 40, 1040), (180, 160, 70))
    erode(m, 9.0, 3.5, 1.1, 97)
    return m


def build_brazier():
    m = new()
    revolve(m, (0, 0, 0), [(0, 0), (90, 0), (90, 25), (60, 45), (48, 70), (40, 200), (52, 220), (110, 250), (130, 300),
                           (124, 304), (100, 262), (0, 262)], steps=36)
    for i in range(4):
        a = math.radians(i * 90)
        beam(m, (math.cos(a) * 40, math.sin(a) * 40, 60), (math.cos(a) * 100, math.sin(a) * 100, 250), 14)
    erode(m, 9.0, 1.2, 0.4, 101)
    return m


def build_crucible():
    m = new()
    revolve(m, (0, 0, 0), [(0, 0), (520, 0), (650, 180), (700, 520), (690, 820), (740, 860), (740, 900), (650, 900),
                           (630, 860), (0, 860)], steps=64)
    for z in (300, 600):
        torus(m, (0, 0, z), 700, 30, 64, 8)
    for s in (-1, 1):
        torus(m, (s * 760, 0, 820), 120, 26, 24, 8, rot=(0, 0, 90))
    erode(m, 30.8, 4.0, 1.2, 103)
    return m


def build_ring():
    """The Sovereign Ring (radius 2600): alloy torus with panel grooves and node blocks."""
    m = new()
    torus(m, (0, 0, 0), 2600, 190, 144, 16)
    for i in range(36):
        a = math.radians(i * 10)
        cut = new()
        box(cut, (math.cos(a) * 2600, math.sin(a) * 2600, -260), (60, 520, 520), rot=(0, math.degrees(a), 0))
        subtract(m, cut)
        if i % 3 == 0:
            box(m, (math.cos(a) * 2600, math.sin(a) * 2600, -230), (360, 300, 460), rot=(0, math.degrees(a), 0))
    bevel(m, 4.0)
    return m


def build_ring_glow():
    m = new()
    torus(m, (0, 0, 0), 2380, 26, 144, 8)
    return m


def build_monument():
    """Dominion monument base: ancient stone with pilasters and cornice (1500 wide, 3360 tall)."""
    m = new()
    box(m, (0, 0, 0), (1500, 1500, 400))
    box(m, (0, 0, 400), (1200, 1200, 2800))
    for side in range(4):
        a = math.radians(side * 90)
        for i in range(5):
            t = -480 + i * 240
            x, y = (math.cos(a) * 610 - math.sin(a) * t, math.sin(a) * 610 + math.cos(a) * t)
            box(m, (x, y, 400), (90, 90, 2700), rot=(0, side * 90, 0))
    box(m, (0, 0, 3200), (1350, 1350, 160))
    erode(m, 39.6, 5.0, 1.5, 109)
    return m


def build_monument_upper():
    """UPG alloy crown for the monument: fins, cap, pyramidal spire (900 wide, ~3500 tall)."""
    m = new()
    box(m, (0, 0, 0), (900, 900, 2400))
    for side in range(4):
        a = math.radians(side * 90 + 45)
        beam(m, (math.cos(a) * 560, math.sin(a) * 560, 0), (math.cos(a) * 470, math.sin(a) * 470, 2400), 90)
    box(m, (0, 0, 2400), (1100, 1100, 200))
    cone(m, (0, 0, 2600), 420, 60, 900, steps=4, rot=(0, 45, 0))
    bevel(m, 6.0)
    return m


def build_gantry_tower():
    """Lattice gantry tower (700 wide, 5200 tall) as one merged mesh."""
    w, h, lv = 700.0, 5200.0, 7
    m = new()
    c = [(-w / 2, -w / 2), (w / 2, -w / 2), (w / 2, w / 2), (-w / 2, w / 2)]
    for cx, cy in c:
        beam(m, (cx, cy, 0), (cx * 0.75, cy * 0.75, h), w * 0.09)
    for k in range(1, lv + 1):
        z0, z1 = h * (k - 1) / lv, h * k / lv
        k0, k1 = 1.0 - 0.25 * (k - 1) / lv, 1.0 - 0.25 * k / lv
        for i in range(4):
            a, b = c[i], c[(i + 1) % 4]
            beam(m, (a[0] * k1, a[1] * k1, z1), (b[0] * k1, b[1] * k1, z1), w * 0.06)
            beam(m, (a[0] * k0, a[1] * k0, z0), (b[0] * k1, b[1] * k1, z1), w * 0.04)
            beam(m, (b[0] * k0, b[1] * k0, z0), (a[0] * k1, a[1] * k1, z1), w * 0.04)
    box(m, (0, 0, h), (w, w, 60))
    return m


def build_truss():
    """Box truss 5200 long: four chords, verticals, diagonals."""
    L, W = 5200.0, 420.0
    m = new()
    for y in (-W / 2, W / 2):
        for z in (0, W):
            beam(m, (-L / 2, y, z), (L / 2, y, z), 44)
    n = 13
    for i in range(n + 1):
        x = -L / 2 + i * L / n
        for y in (-W / 2, W / 2):
            beam(m, (x, y, 0), (x, y, W), 30)
        beam(m, (x, -W / 2, W), (x, W / 2, W), 30)
        if i < n:
            for y in (-W / 2, W / 2):
                beam(m, (x, y, 0), (x + L / n, y, W), 24)
    return m


def build_chain_link():
    m = new()
    torus(m, (0, 0, 0), 26, 8, 20, 8)
    unreal.GeometryScript_MeshTransforms.scale_mesh(m, V(1.0, 1.7, 1.0))
    return m


def build_lantern():
    """Beacon lantern: gallery with railing posts, glazed drum with mullions, domed cap (radius 640)."""
    m = new()
    revolve(m, (0, 0, 0), [(0, 0), (640, 0), (640, 60), (560, 90), (420, 90), (420, 560), (470, 590), (430, 640),
                           (300, 760), (120, 820), (0, 830)], steps=48)
    for i in range(24):
        a = math.radians(i * 15)
        beam(m, (math.cos(a) * 620, math.sin(a) * 620, 60), (math.cos(a) * 620, math.sin(a) * 620, 170), 12)
    torus(m, (0, 0, 170), 620, 9, 64, 6)
    for i in range(12):
        a = math.radians(i * 30 + 15)
        cut = new()
        box(cut, (math.cos(a) * 420, math.sin(a) * 420, 140), (90, 160, 380), rot=(0, i * 30 + 15, 0))
        subtract(m, cut)
    return m


def build_bridge_arch():
    """Arch pier with semicircular opening (span 700, spring height 4000, depth 1000)."""
    span, h, d, t = 700.0, 4000.0, 1000.0, 360.0
    m = new()
    box(m, (0, 0, 0), (d, span + t * 2, h + span / 2 + 200))
    hole = new()
    box(hole, (0, 0, -10), (d + 200, span, h + 10))
    cyl(hole, (0, (d + 200) / 2, h), span / 2, d + 200, steps=40, rot=(90, 90, 0))
    subtract(m, hole)
    erode(m, 35.2, 6.0, 1.8, 131)
    return m


PIECES = {
    "Column": lambda: build_column(True),
    "ColumnBroken": build_broken_column,
    "ColumnDrum": build_drum,
    "TowerRound": build_round_tower,
    "TowerSquare": build_square_tower,
    "Wall": lambda: build_wall(False),
    "WallRuin": lambda: build_wall(True),
    "Gate": build_gate,
    "Obelisk": build_obelisk,
    "Colossus": lambda: build_colossus(False),
    "ColossusBroken": lambda: build_colossus(True),
    "SeatedColossus": build_seated_colossus,
    "Brazier": build_brazier,
    "Crucible": build_crucible,
    "SovereignRing": build_ring,
    "SovereignRingGlow": build_ring_glow,
    "Monument": build_monument,
    "MonumentUpper": build_monument_upper,
    "GantryTower": build_gantry_tower,
    "Truss": build_truss,
    "ChainLink": build_chain_link,
    "Lantern": build_lantern,
    "BridgeArch": build_bridge_arch,
}


def main():
    if not EAL.does_directory_exist(ROOT):
        EAL.make_directory(ROOT)
    for name, fn in PIECES.items():
        if ONLY and name not in ONLY:
            continue
        try:
            save(fn(), name)
        except Exception as exc:  # noqa
            import traceback
            log("ERROR %s: %s" % (name, exc))
            log(traceback.format_exc())


try:
    main()
    RESULT = "PASS" if not any(l.startswith(("ERROR", "FAIL")) for l in LOG) else "PARTIAL"
except Exception as exc:  # noqa
    log("FATAL " + str(exc))
    RESULT = "FAIL"
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
with open(os.path.join(proj, "Saved", "GOTA_BuildArchitecture.txt"), "w", encoding="utf-8") as f:
    f.write("\n".join(LOG) + "\nRESULT: " + RESULT + "\n")
if os.environ.get("GOTA_QUIT_AFTER_BUILD") == "1":
    unreal.SystemLibrary.quit_editor()
