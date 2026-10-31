# Arena overhaul — authoring kit (imported by Tools/GOTA_BuildArenaLevels.py, runs inside the editor).
#
# Three layers, kept separate by actor folder and collision:
#   Gameplay/    fighting floor, invisible boundary, player starts, horde spawns, nav bounds
#                (deterministic, BlockAll / InvisibleWall collision).
#   Dressing/    architecture, cliffs, landmarks, debris. Decorative pieces outside the boundary use
#                NoCollision, so nothing visual can change the combat space.
#   Atmosphere/  sun, sky, fog, clouds, post process, particles, ambient audio.
# All placement is deterministic (seeded RNG), so every build and every client is identical.
import math
import random

import unreal

EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LES = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAL = unreal.EditorAssetLibrary
B = unreal.GOTAEditorUIBuilderLibrary
MATS = "/Game/Arenas/Shared/Materials/MI_GOTA_"
# Nanite copies of the engine primitives (kitbash architecture). Large non-Nanite primitives overflow
# the virtual-shadow-map marking queue; Nanite keeps shadow cost proportional to visible detail.
SHAPES = "/Game/Arenas/Shared/Meshes"
CUBE = SHAPES + "/SM_GOTA_Cube.SM_GOTA_Cube"
CYL = SHAPES + "/SM_GOTA_Cylinder.SM_GOTA_Cylinder"
SPHERE = SHAPES + "/SM_GOTA_Sphere.SM_GOTA_Sphere"
CONE = SHAPES + "/SM_GOTA_Cone.SM_GOTA_Cone"


def ensure_nanite_shapes():
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for name in ("Cube", "Cylinder", "Sphere", "Cone"):
        dst = SHAPES + "/SM_GOTA_" + name
        if not EAL.does_asset_exist(dst):
            EAL.duplicate_asset("/Engine/BasicShapes/" + name, dst)
        m = EAL.load_asset(dst)
        ns = m.get_editor_property("nanite_settings")
        if not ns.get_editor_property("enabled"):
            ns.set_editor_property("enabled", True)
            sme.set_nanite_settings(m, ns, apply_changes=True)
            EAL.save_asset(dst)
AUTHORED_TAG = "GOTA_AuthoredArena"

_cache = {}
STATS = {"actors": 0, "missing": []}


def asset(path):
    if path not in _cache:
        _cache[path] = unreal.load_asset(path)
        if _cache[path] is None:
            STATS["missing"].append(path)
    return _cache[path]


def mat(name):
    return asset(MATS + name if "/" not in name else name)


def V(x, y, z):
    return unreal.Vector(float(x), float(y), float(z))


def R(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=float(roll), pitch=float(pitch), yaw=float(yaw))


def _finish(actor, label, folder, tags=None):
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    if tags:
        actor.set_editor_property("tags", [unreal.Name(t) for t in tags])
    STATS["actors"] += 1
    return actor


# ------------------------------------------------------------------ level lifecycle
def flagstone_floor(radius, inner, rings, materials, rng, z=1.0, folder="Dressing/Flagstones"):
    """Radial flagstone paving laid over the gameplay floor (visual only; the floor below collides)."""
    width = (radius - inner) / float(rings)
    for k in range(rings):
        r0 = inner + k * width
        rm = r0 + width / 2.0
        n = max(8, int(2 * math.pi * rm / (width * 1.15)))
        for i in range(n):
            deg = 360.0 * (i + 0.5) / n + (k % 2) * 180.0 / n
            arc = 2 * math.pi * rm / n
            p = polar(rm, deg, z + rng.uniform(-0.6, 0.6))
            box(p, (width - 10, arc - 10, 4 + rng.uniform(0, 1.5)), rng.choice(materials),
                rot=R(rng.uniform(-0.2, 0.2), deg + rng.uniform(-0.8, 0.8), rng.uniform(-0.2, 0.2)), folder=folder, cast_shadow=False)


def open_fresh_level(path):
    """Creates the level, or reopens it and removes every actor (idempotent rebuilds)."""
    ensure_nanite_shapes()
    if EAL.does_asset_exist(path):
        LES.load_level(path)
        actors = [a for a in EAS.get_all_level_actors() if not isinstance(a, (unreal.WorldSettings,))]
        EAS.destroy_actors(actors)
    else:
        LES.new_level(path)
    STATS["actors"] = 0
    STATS["missing"] = []


def save_level():
    return LES.save_current_level()


# ------------------------------------------------------------------ meshes
def mesh(path, loc, rot=None, scale=(1, 1, 1), material=None, collision="NoCollision", label=None,
         folder="Dressing", cast_shadow=True, hidden=False, tags=None):
    m = asset(path)
    if m is None:
        return None
    a = EAS.spawn_actor_from_object(m, loc, rot or R())
    if a is None:
        return None
    a.set_actor_scale3d(V(*scale) if isinstance(scale, (tuple, list)) else V(scale, scale, scale))
    comp = a.static_mesh_component
    comp.set_mobility(unreal.ComponentMobility.STATIC)
    if material is not None:
        mi = mat(material) if isinstance(material, str) else material
        if mi:
            for i in range(max(1, comp.get_num_materials())):
                comp.set_material(i, mi)
    comp.set_collision_profile_name(collision)
    if collision == "NoCollision":
        comp.set_editor_property("can_character_step_up_on", unreal.CanBeCharacterBase.ECB_NO)
    comp.set_cast_shadow(cast_shadow)
    if hidden:
        a.set_actor_hidden_in_game(True)
    return _finish(a, label, folder, tags)


def box(center, size, material, rot=None, collision="NoCollision", label=None, folder="Dressing", **kw):
    """Cube primitive (100 cm, centred pivot) sized in cm, world-aligned material so no stretching."""
    return mesh(CUBE, center, rot, (size[0] / 100.0, size[1] / 100.0, size[2] / 100.0), material, collision, label, folder, **kw)


def cyl(base, radius, height, material, rot=None, collision="NoCollision", label=None, folder="Dressing", **kw):
    """Cylinder standing on `base` (cm)."""
    c = V(base.x, base.y, base.z + height / 2.0)
    return mesh(CYL, c, rot, (radius / 50.0, radius / 50.0, height / 100.0), material, collision, label, folder, **kw)


def sphere(center, radius, material, scale=(1, 1, 1), collision="NoCollision", label=None, folder="Dressing"):
    return mesh(SPHERE, center, None, (radius / 50.0 * scale[0], radius / 50.0 * scale[1], radius / 50.0 * scale[2]), material, collision, label, folder)


def polar(r, deg, z=0.0, cx=0.0, cy=0.0):
    a = math.radians(deg)
    return V(cx + math.cos(a) * r, cy + math.sin(a) * r, z)


def facing(src, dst):
    return math.degrees(math.atan2(dst.y - src.y, dst.x - src.x))


# ------------------------------------------------------------------ kitbash architecture
def column(base, radius, height, material, capital=None, broken=False, rng=None, folder="Dressing/Columns", collision="NoCollision"):
    """Plinth + shaft (+ capital). Broken columns lose their top; the fallen drums lie beside them."""
    capital = capital or material
    box(V(base.x, base.y, base.z + radius * 0.35), (radius * 2.6, radius * 2.6, radius * 0.7), material, folder=folder, collision=collision)
    box(V(base.x, base.y, base.z + radius * 0.85), (radius * 2.25, radius * 2.25, radius * 0.3), material, folder=folder, collision=collision)
    shaft_h = height if not broken else height * (0.35 + (rng.random() * 0.25 if rng else 0.2))
    cyl(V(base.x, base.y, base.z + radius), radius, shaft_h, material, folder=folder, collision=collision)
    if not broken:
        box(V(base.x, base.y, base.z + radius + height + radius * 0.25), (radius * 2.5, radius * 2.5, radius * 0.5), capital, folder=folder, collision=collision)
    else:
        rng = rng or random.Random(1)
        yaw = rng.uniform(0, 360)
        for i in range(2):
            d = radius * (3.0 + i * 2.3)
            p = polar(d, yaw + rng.uniform(-15, 15), base.z + radius * 0.95, base.x, base.y)
            cyl(V(p.x, p.y, p.z - radius), radius, radius * 2.1, material, rot=R(90, yaw + 90 + rng.uniform(-20, 20), 0), folder=folder, collision=collision)


def arch(center, yaw, span, height, depth, thick, material, folder="Dressing/Arches"):
    """Two piers and a lintel."""
    dx = math.cos(math.radians(yaw + 90)) * (span / 2.0 + thick / 2.0)
    dy = math.sin(math.radians(yaw + 90)) * (span / 2.0 + thick / 2.0)
    for s in (-1, 1):
        box(V(center.x + dx * s, center.y + dy * s, center.z + height / 2.0), (depth, thick, height), material, rot=R(0, yaw, 0), folder=folder)
    box(V(center.x, center.y, center.z + height + thick * 0.4), (depth * 1.1, span + thick * 2.4, thick * 0.8), material, rot=R(0, yaw, 0), folder=folder)


def gate(center, yaw, width, height, material, trim=None, energy=None, folder="Dressing/Gate"):
    """Monumental gate: stepped pylons, deep lintel, optional energy inlay facing the arena."""
    trim = trim or material
    side = width / 2.0
    ux, uy = math.cos(math.radians(yaw + 90)), math.sin(math.radians(yaw + 90))
    fx, fy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    pw = width * 0.32
    for s in (-1, 1):
        px, py = center.x + ux * (side + pw / 2.0) * s, center.y + uy * (side + pw / 2.0) * s
        box(V(px, py, center.z + height * 0.5), (pw * 1.1, pw, height), material, rot=R(0, yaw, 0), folder=folder)
        box(V(px, py, center.z + height * 0.06), (pw * 1.45, pw * 1.3, height * 0.12), trim, rot=R(0, yaw, 0), folder=folder)
        box(V(px, py, center.z + height + pw * 0.2), (pw * 0.8, pw * 0.75, pw * 0.4), trim, rot=R(0, yaw, 0), folder=folder)
        if energy:
            box(V(px - fx * pw * 0.56, py - fy * pw * 0.56, center.z + height * 0.55), (6, pw * 0.12, height * 0.7), energy, rot=R(0, yaw, 0), folder=folder)
    box(V(center.x, center.y, center.z + height + pw * 0.35), (pw * 1.25, width + pw * 2.2, pw * 0.7), trim, rot=R(0, yaw, 0), folder=folder)
    box(V(center.x, center.y, center.z + height - pw * 0.1), (pw * 1.0, width + pw * 0.4, pw * 0.35), material, rot=R(0, yaw, 0), folder=folder)
    if energy:
        box(V(center.x - fx * pw * 0.52, center.y - fy * pw * 0.52, center.z + height + pw * 0.35), (6, width * 0.8, pw * 0.08), energy, rot=R(0, yaw, 0), folder=folder)


def stairs(start, yaw, width, steps, rise, run, material, folder="Dressing/Stairs", collision="NoCollision"):
    fx, fy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    for i in range(steps):
        d = run * (i + 0.5)
        box(V(start.x + fx * d, start.y + fy * d, start.z + rise * (i + 1) / 2.0), (run, width, rise * (i + 1)), material,
            rot=R(0, yaw, 0), folder=folder, collision=collision)


def beam(p0, p1, thick, material, folder="Dressing/Machinery", collision="NoCollision"):
    """Box beam between two points."""
    d = unreal.Vector(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z)
    length = math.sqrt(d.x * d.x + d.y * d.y + d.z * d.z)
    yaw = math.degrees(math.atan2(d.y, d.x))
    pitch = math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y)))
    mid = V((p0.x + p1.x) / 2, (p0.y + p1.y) / 2, (p0.z + p1.z) / 2)
    return box(mid, (length, thick, thick), material, rot=R(pitch, yaw, 0), folder=folder, collision=collision)


def lattice_tower(base, width, height, material, levels=6, folder="Dressing/Machinery"):
    """Four legs with cross bracing — mining gantry / observation mast."""
    h = width / 2.0
    corners = [(-h, -h), (h, -h), (h, h), (-h, h)]
    for cx, cy in corners:
        beam(V(base.x + cx, base.y + cy, base.z), V(base.x + cx * 0.75, base.y + cy * 0.75, base.z + height), width * 0.09, material, folder)
    for lv in range(1, levels + 1):
        z0 = base.z + height * (lv - 1) / levels
        z1 = base.z + height * lv / levels
        k0 = 1.0 - 0.25 * (lv - 1) / levels
        k1 = 1.0 - 0.25 * lv / levels
        for i in range(4):
            a, b2 = corners[i], corners[(i + 1) % 4]
            beam(V(base.x + a[0] * k1, base.y + a[1] * k1, z1), V(base.x + b2[0] * k1, base.y + b2[1] * k1, z1), width * 0.06, material, folder)
            beam(V(base.x + a[0] * k0, base.y + a[1] * k0, z0), V(base.x + b2[0] * k1, base.y + b2[1] * k1, z1), width * 0.04, material, folder)


def chain(p0, p1, material, link=60.0, sag=0.08, folder="Dressing/Chains"):
    """Hanging chain: alternating flattened links along a sagging curve."""
    d = math.sqrt((p1.x - p0.x) ** 2 + (p1.y - p0.y) ** 2 + (p1.z - p0.z) ** 2)
    n = max(2, int(d / link))
    pts = []
    for i in range(n + 1):
        t = i / float(n)
        pts.append(V(p0.x + (p1.x - p0.x) * t, p0.y + (p1.y - p0.y) * t, p0.z + (p1.z - p0.z) * t - math.sin(math.pi * t) * d * sag))
    for i in range(n):
        a, b2 = pts[i], pts[i + 1]
        dx, dy, dz = b2.x - a.x, b2.y - a.y, b2.z - a.z
        L = math.sqrt(dx * dx + dy * dy + dz * dz)
        yaw = math.degrees(math.atan2(dy, dx))
        pitch = math.degrees(math.atan2(dz, math.sqrt(dx * dx + dy * dy)))
        box(V((a.x + b2.x) / 2, (a.y + b2.y) / 2, (a.z + b2.z) / 2), (L * 1.15, link * 0.12, link * 0.42), material,
            rot=R(pitch, yaw, 90.0 if i % 2 else 0.0), folder=folder, cast_shadow=False)


def ring_of_boxes(radius, count, size, z, material, start_deg=0.0, end_deg=360.0, folder="Dressing", collision="NoCollision", skip=None, jitter=None, rng=None, hidden=False, label=None):
    span = end_deg - start_deg
    out = []
    for i in range(count):
        deg = start_deg + span * (i + 0.5) / count
        if skip and skip(deg):
            continue
        p = polar(radius, deg, z)
        sz = size
        if jitter and rng:
            sz = (size[0] * rng.uniform(1 - jitter, 1 + jitter), size[1], size[2] * rng.uniform(1 - jitter, 1 + jitter))
        out.append(box(p, sz, material, rot=R(0, deg + 90, 0), folder=folder, collision=collision, hidden=hidden, label=label))
    return out


def scatter(paths, center, r_min, r_max, count, rng, z=0.0, scale=(0.8, 1.4), material=None, folder="Dressing/Debris",
            arc=(0.0, 360.0), avoid=None, yaw_free=True, collision="NoCollision", sink=0.0):
    out = []
    tries = 0
    while len(out) < count and tries < count * 20:
        tries += 1
        deg = rng.uniform(*arc)
        r = rng.uniform(r_min, r_max)
        p = polar(r, deg, z - sink, center.x, center.y)
        if avoid and avoid(p):
            continue
        s = rng.uniform(*scale)
        a = mesh(rng.choice(paths), p, R(0, rng.uniform(0, 360) if yaw_free else deg + 90, 0), (s, s, s), material, collision, folder=folder)
        if a:
            out.append(a)
    return out


# ------------------------------------------------------------------ gameplay layer
def player_starts(points):
    """points: list of (slot, location, yaw). AArenaPlayerStart drives Duel / Horde / Skirmish slots."""
    for slot, loc, yaw in points:
        a = EAS.spawn_actor_from_class(unreal.ArenaPlayerStart, loc, R(0, yaw, 0))
        a.set_editor_property("player_slot", slot)
        _finish(a, "PlayerStart_Slot%d" % slot, "Gameplay/Starts")


def horde_spawns(radius, count, z=120.0, start_deg=15.0):
    for i in range(count):
        deg = start_deg + 360.0 * i / count
        p = polar(radius, deg, z)
        a = EAS.spawn_actor_from_class(unreal.HordeSpawnPoint, p, R(0, deg + 180, 0))
        _finish(a, "HordeSpawn_%02d" % i, "Gameplay/HordeSpawns")


def invisible_ring(radius, height, count=28, thickness=80.0):
    """Arena boundary: InvisibleWall collision (blocks pawns, not the camera), hidden in game."""
    ring_of_boxes(radius + thickness / 2.0, count, (2 * math.pi * radius / count * 1.08, thickness, height), height / 2.0,
                  "QuarryStone", folder="Gameplay/Boundary", collision="InvisibleWall", hidden=True, label="Boundary")


def nav_bounds(center, size):
    a = B.spawn_box_volume(unreal.NavMeshBoundsVolume, center, size, R(), "NavMeshBounds_Arena")
    if a:
        a.set_folder_path("Gameplay")
    return a


def marker(tag_actor_label="GOTA_ArenaMarker", extra_tags=None):
    """Authored-arena marker read by AArenaGameMode::IsAuthoredArenaWorld."""
    a = EAS.spawn_actor_from_class(unreal.TargetPoint, V(0, 0, 50), R())
    _finish(a, tag_actor_label, "Gameplay", [AUTHORED_TAG] + (extra_tags or []))
    return a


def camera_point(name, loc, pitch, yaw):
    a = EAS.spawn_actor_from_class(unreal.TargetPoint, loc, R(pitch, yaw, 0))
    _finish(a, "Cam_" + name, "Gameplay/CompositionCameras", ["GOTA_Cam"])


# ------------------------------------------------------------------ atmosphere
def lighting(sun_pitch, sun_yaw, sun_color, sun_lux, sky_tint=None, fog_density=0.02, fog_falloff=0.2,
             fog_color=(0.45, 0.42, 0.38), fog_start=1200.0, volumetric=True, vol_scatter=1.0, clouds=True,
             sky_intensity=1.0, fog_albedo=None, fog_extinction=0.6, mie=None, rayleigh=None):
    sun = EAS.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 3000), R(sun_pitch, sun_yaw, 0))
    lc = sun.light_component
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_intensity(sun_lux)
    lc.set_light_color(unreal.LinearColor(*sun_color, 1.0))
    lc.set_editor_property("atmosphere_sun_light", True)
    lc.set_editor_property("volumetric_scattering_intensity", vol_scatter)
    lc.set_editor_property("cast_cloud_shadows", True) if clouds else None
    _finish(sun, "Sun", "Atmosphere")

    atmo = EAS.spawn_actor_from_class(unreal.SkyAtmosphere, V(0, 0, 0), R())
    ac = atmo.get_component_by_class(unreal.SkyAtmosphereComponent)
    if mie is not None:
        ac.set_editor_property("mie_scattering_scale", mie)
    if rayleigh is not None:
        ac.set_editor_property("rayleigh_scattering", unreal.LinearColor(*rayleigh, 1.0))
    _finish(atmo, "SkyAtmosphere", "Atmosphere")

    sky = EAS.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 2000), R())
    sc = sky.light_component
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_editor_property("real_time_capture", True)
    sc.set_intensity(sky_intensity)
    if sky_tint:
        sc.set_light_color(unreal.LinearColor(*sky_tint, 1.0))
    _finish(sky, "SkyLight", "Atmosphere")

    fog = EAS.spawn_actor_from_class(unreal.ExponentialHeightFog, V(0, 0, 0), R())
    fc = fog.component
    fc.set_fog_density(fog_density)
    fc.set_fog_height_falloff(fog_falloff)
    fc.set_editor_property("start_distance", fog_start)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(*fog_color, 1.0))
    fc.set_editor_property("enable_volumetric_fog", volumetric)
    if volumetric:
        fc.set_editor_property("volumetric_fog_scattering_distribution", 0.6)
        fc.set_editor_property("volumetric_fog_extinction_scale", fog_extinction)
        if fog_albedo:
            fc.set_editor_property("volumetric_fog_albedo", unreal.Color(*fog_albedo, 255))
    _finish(fog, "HeightFog", "Atmosphere")

    if clouds:
        cl = EAS.spawn_actor_from_class(unreal.VolumetricCloud, V(0, 0, 0), R())
        _finish(cl, "VolumetricClouds", "Atmosphere")
    return sun


def post_process(exposure_bias=0.0, min_ev=-0.5, max_ev=3.0, bloom=0.6, saturation=1.0, contrast=1.0,
                 gain=(1.0, 1.0, 1.0), vignette=0.35, grain=0.0, shadows_tint=None, temp=6500.0, ao=0.6):
    a = EAS.spawn_actor_from_class(unreal.PostProcessVolume, V(0, 0, 0), R())
    a.set_editor_property("unbound", True)
    s = a.settings
    def setp(name, value):
        s.set_editor_property("override_" + name, True)
        s.set_editor_property(name, value)
    setp("auto_exposure_bias", exposure_bias)
    setp("auto_exposure_min_brightness", min_ev)
    setp("auto_exposure_max_brightness", max_ev)
    setp("bloom_intensity", bloom)
    setp("vignette_intensity", vignette)
    setp("film_grain_intensity", grain)
    setp("white_temp", temp)
    setp("ambient_occlusion_intensity", ao)
    setp("color_saturation", unreal.Vector4(saturation, saturation, saturation, 1.0))
    setp("color_contrast", unreal.Vector4(contrast, contrast, contrast, 1.0))
    setp("color_gain", unreal.Vector4(gain[0], gain[1], gain[2], 1.0))
    if shadows_tint:
        setp("color_gain_shadows", unreal.Vector4(shadows_tint[0], shadows_tint[1], shadows_tint[2], 1.0))
    a.set_editor_property("settings", s)
    _finish(a, "PostProcess", "Atmosphere")
    return a


def fx(path, loc, scale=1.0, label=None, rot=None):
    sysasset = asset(path)
    if sysasset is None:
        return None
    a = EAS.spawn_actor_from_object(sysasset, loc, rot or R())
    if a:
        a.set_actor_scale3d(V(scale, scale, scale))
        _finish(a, label, "Atmosphere/VFX")
    return a


def ambient(path, loc, radius_inner=1500.0, falloff=6000.0, volume=1.0, label=None):
    snd = asset(path)
    if snd is None:
        return None
    a = EAS.spawn_actor_from_object(snd, loc, R())
    if a is None:
        return None
    ac = a.get_component_by_class(unreal.AudioComponent)
    if ac:
        ac.set_editor_property("volume_multiplier", volume)
        ac.set_editor_property("override_attenuation", True)
        att = ac.get_editor_property("attenuation_overrides")
        att.set_editor_property("attenuation_shape_extents", V(radius_inner, 0, 0))
        att.set_editor_property("falloff_distance", falloff)
        ac.set_editor_property("attenuation_overrides", att)
        ac.set_editor_property("auto_activate", True)
    _finish(a, label, "Atmosphere/Audio")
    return a


def point_light(loc, color, intensity, radius, label=None, shadows=False):
    a = EAS.spawn_actor_from_class(unreal.PointLight, loc, R())
    lc = a.light_component
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_light_color(unreal.LinearColor(*color, 1.0))
    lc.set_intensity(intensity)
    lc.set_attenuation_radius(radius)
    lc.set_cast_shadows(shadows)
    _finish(a, label, "Atmosphere/Lights")
    return a


def finalize(arena_label, nav_center, nav_size, level_path=None):
    marker(extra_tags=[arena_label])
    nav_bounds(nav_center, nav_size)
    ok = B.build_editor_navigation()
    saved = save_level()
    if not ok and level_path:
        # A brand-new level only gets its navigation system on reload: reopen, build, save again.
        LES.load_level(level_path)
        ok = B.build_editor_navigation()
        saved = save_level() and saved
    return ok, saved


# ------------------------------------------------------------------ ambience import
AUDIO_ROOT = "/Game/Arenas/Shared/Audio"


def import_ambience(src_dir):
    """Imports Tools/ArenaAudioSource/*.wav (generated by GOTA_BuildArenaAudio.py) as looping SoundWaves."""
    import os as _os
    tasks = []
    for f in sorted(_os.listdir(src_dir)):
        if not f.endswith(".wav"):
            continue
        name = f[:-4]
        if EAL.does_asset_exist(AUDIO_ROOT + "/" + name):
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", _os.path.join(src_dir, f))
        t.set_editor_property("destination_path", AUDIO_ROOT)
        t.set_editor_property("automated", True)
        t.set_editor_property("save", True)
        t.set_editor_property("replace_existing", True)
        tasks.append(t)
    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for f in sorted(_os.listdir(src_dir)):
        if f.endswith(".wav"):
            w = EAL.load_asset(AUDIO_ROOT + "/" + f[:-4])
            if w and "_Amb_" in f and not w.get_editor_property("looping"):
                w.set_editor_property("looping", True)
                EAL.save_asset(AUDIO_ROOT + "/" + f[:-4])


def amb(name):
    return AUDIO_ROOT + "/A_GOTA_Amb_" + name + ".A_GOTA_Amb_" + name
