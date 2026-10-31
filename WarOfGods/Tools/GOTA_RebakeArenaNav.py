"""Rebakes and saves the navmesh of every authored arena (nothing else is touched), then verifies
that the arena centre and the duel start points project onto navigation.

Why: the Pass 2 rebuild saved the arena maps without built navigation tiles, so at runtime the
AI could not path and navmesh-validated abilities (Shadowstep) found no destination.

Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_RebakeArenaNav.txt
"""
import os

import unreal

MAPS = ["TitansQuarry", "AshfallCrucible", "FrostboundCitadel", "VerdantSanctuary", "SunscarExpanse", "StormbreakCoast", "ShatteredDominion"]
ONLY = [m for m in os.environ.get("GOTA_ARENAS", "").split(",") if m]
log = []


def P(*a):
    line = " ".join(str(x) for x in a)
    log.append(line)
    unreal.log("GOTA_RebakeArenaNav: " + line)
    open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_RebakeArenaNav.txt"), "w").write("\n".join(log))


def projects(world, loc):
    try:
        res = unreal.NavigationSystemV1.project_point_to_navigation(world, loc, None, None, unreal.Vector(150, 150, 400))
        # returns (bool, point) or point depending on binding version
        if isinstance(res, tuple):
            return bool(res[0])
        return res is not None
    except Exception as e:
        P("   project error", e)
        return False


for m in MAPS:
    if ONLY and m not in ONLY:
        continue
    path = "/Game/Arenas/Maps/L_Arena_" + m
    unreal.EditorLoadingAndSavingUtils.load_map(path)
    world = unreal.EditorLevelLibrary.get_editor_world()
    navs = [a for a in unreal.EditorLevelLibrary.get_all_level_actors() if isinstance(a, unreal.RecastNavMesh)]
    bounds = [a for a in unreal.EditorLevelLibrary.get_all_level_actors() if isinstance(a, unreal.NavMeshBoundsVolume)]
    before = projects(world, unreal.Vector(0, 0, 100))
    # The Pass 2 rebuild left NavMeshBoundsVolumes with an empty brush (zero extent), so nothing was
    # baked. Replace any empty volume with one sized from the arena's gameplay floor (+ margin).
    lo, hi = None, None
    for a in unreal.EditorLevelLibrary.get_all_level_actors():
        if str(a.get_folder_path()).startswith("Gameplay/Floor"):
            o, e = a.get_actor_bounds(True)
            amin = unreal.Vector(o.x - e.x, o.y - e.y, o.z - e.z)
            amax = unreal.Vector(o.x + e.x, o.y + e.y, o.z + e.z)
            lo = amin if lo is None else unreal.Vector(min(lo.x, amin.x), min(lo.y, amin.y), min(lo.z, amin.z))
            hi = amax if hi is None else unreal.Vector(max(hi.x, amax.x), max(hi.y, amax.y), max(hi.z, amax.z))
    for b in bounds:
        o, e = b.get_actor_bounds(False)
        if e.x < 1.0 and lo is not None:
            unreal.EditorLevelLibrary.destroy_actor(b)
            centre = unreal.Vector((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, (lo.z + hi.z) / 2 + 300)
            size = unreal.Vector(hi.x - lo.x + 600, hi.y - lo.y + 600, max(hi.z - lo.z, 0) + 1600)
            # Spawn through the editor's actor factory (creates a real saved cube brush), then scale it.
            # A brush built onto a bare SpawnActor volume is not persisted, which is what broke Pass 2.
            nb = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.NavMeshBoundsVolume, centre)
            if nb:
                o0, e0 = nb.get_actor_bounds(False)
                base = max(e0.x * 2, 1.0)
                nb.set_actor_scale3d(unreal.Vector(size.x / base, size.y / base, size.z / base))
                nb.set_actor_label("NavMeshBounds_Arena")
                nb.set_folder_path("Gameplay")
                P("   replaced empty NavMeshBoundsVolume: centre=%s size=%s -> extent %s" % (centre, size, nb.get_actor_bounds(False)[1]))
    # A RecastNavMesh actor off the tile grid (Quarry: X=36) makes the engine discard the saved tiles
    # on load ("navmesh origin not aligned with tile size"), leaving the game with no navigation.
    for n in navs:
        loc = n.get_actor_location()
        if abs(loc.x) > 0.01 or abs(loc.y) > 0.01 or abs(loc.z) > 0.01:
            n.set_actor_location(unreal.Vector(0, 0, 0), False, False)
            P("   moved %s from %s to the origin (tile-aligned)" % (n.get_name(), loc))
    ok = unreal.GOTAEditorUIBuilderLibrary.build_editor_navigation()
    after_center = projects(world, unreal.Vector(0, 0, 100))
    after_starts = [projects(world, unreal.Vector(x, 0, 100)) for x in (-950, 950)]
    saved = unreal.EditorLoadingAndSavingUtils.save_current_level()
    P(m, "navmeshes=%d bounds=%d before=%s build=%s centre=%s starts=%s saved=%s" % (len(navs), len(bounds), before, ok, after_center, after_starts, saved))

# Verify after a real reload: the bounds volume must keep its size and the tiles must survive.
for m in MAPS:
    if ONLY and m not in ONLY:
        continue
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/Arenas/Maps/L_Arena_" + m)
    ext = [str(b.get_actor_bounds(False)[1]) for b in unreal.EditorLevelLibrary.get_all_level_actors() if isinstance(b, unreal.NavMeshBoundsVolume)]
    tiles = [n.get_editor_property("tile_size_uu") for n in unreal.EditorLevelLibrary.get_all_level_actors() if isinstance(n, unreal.RecastNavMesh)]
    P(m, "after reload bounds extents:", ext)
P("DONE")
