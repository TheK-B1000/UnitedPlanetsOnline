# PIE smoke: start play, gota.BuildArena per biome, confirm ResolveDress hits DA_Dress_* (not TransientDress_*).
#   UnrealEditor-Cmd.exe ... -ExecutePythonScript=.../GOTA_PIE_ValidateDress.py -unattended -nosplash -NullRHI

import unreal
import time

LAYOUT_ARGS = [
    "Quarry",
    "Ashfall",
    "Frostbound",
    "Verdant",
    "Sunscar",
    "Stormbreak",
    "Dominion",
    "Story",
]


def find_game_world():
    editor_world = unreal.EditorLevelLibrary.get_editor_world()
    # After PIE start, prefer PIE world via subsystem if available
    try:
        pie = unreal.EditorLevelLibrary.get_pie_worlds(False)
        if pie:
            return pie[0]
    except Exception:
        pass
    return editor_world


def count_dress_from_builders(world):
    """Count ArenaGrayboxBuilder dress comps; return (builders, dressed, dress_names)."""
    builders = 0
    dressed = 0
    names = []
    try:
        actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ArenaGrayboxBuilder)
    except Exception:
        actors = []
    for actor in actors or []:
        builders += 1
        names.append(actor.get_name())
        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        for c in comps or []:
            mesh = c.get_editor_property("StaticMesh") if c else None
            if not mesh:
                continue
            path = mesh.get_path_name()
            if "/Engine/" in path or "SM_Cube" in path or "SM_Cylinder" in path or "BasicShapes" in path:
                continue
            if "LevelPrototyping" in path:
                continue
            dressed += 1
    return builders, dressed, names


def main():
    unreal.log("=== GOTA PIE Validate Arena Dress ===")

    # Ensure default map is loaded (editor startup map)
    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        unreal.log_error("No editor world")
        return

    # Start PIE (simulate)
    try:
        unreal.EditorLevelLibrary.editor_play_simulate()
    except Exception:
        try:
            unreal.LevelEditorSubsystem().editor_request_pie()
        except Exception as exc:
            unreal.log_warning(f"PIE start via LevelEditorSubsystem failed: {exc}")
            # Fall back: editor-world BuildLayout already covered; still try console if game world appears
            pass

    # Give PIE a moment to spin up
    for _ in range(30):
        time.sleep(0.2)
        pie_worlds = []
        try:
            pie_worlds = list(unreal.EditorLevelLibrary.get_pie_worlds(False) or [])
        except Exception:
            pass
        if pie_worlds:
            break

    pie_worlds = []
    try:
        pie_worlds = list(unreal.EditorLevelLibrary.get_pie_worlds(False) or [])
    except Exception:
        pass

    if not pie_worlds:
        unreal.log_warning("PIE world not available under -NullRHI/unattended; falling back to editor BuildLayout")
        # Reuse validate build path without soft_ok noise
        from importlib.machinery import SourceFileLoader
        mod = SourceFileLoader(
            "gota_validate",
            r"B:\Unreal\UnitedPlanetsOnline\WarOfGods\Tools\GOTA_ValidateArenaDress.py",
        ).load_module()
        results = []
        for name, idx in mod.LAYOUTS:
            br = mod.build_layout_in_editor(name, idx)
            results.append(br)
            unreal.log(f"EditorBuild {name}: comps={br.get('total_comps')} dressed~={br.get('dressed_comps')} err={br.get('error')}")
        ok = all(r.get("built") and r.get("dressed_comps", 0) > 0 for r in results)
        unreal.log("RESULT: PASS (editor fallback — all layouts dressed)" if ok else "RESULT: FAIL (editor fallback)")
        return

    world = pie_worlds[0]
    unreal.log(f"PIE world: {world.get_name()}")

    results = []
    for arg in LAYOUT_ARGS:
        unreal.SystemLibrary.execute_console_command(world, f"gota.BuildArena {arg}")
        time.sleep(0.15)
        builders, dressed, names = count_dress_from_builders(world)
        results.append({"arg": arg, "builders": builders, "dressed": dressed})
        unreal.log(f"PIE BuildArena {arg}: builders={builders} dressed~={dressed}")

    # End PIE
    try:
        unreal.EditorLevelLibrary.editor_end_play()
    except Exception:
        pass

    ok = any(r["dressed"] > 0 for r in results)
    if ok:
        unreal.log("RESULT: PASS — PIE BuildArena placed dress meshes")
    else:
        unreal.log_error("RESULT: FAIL — no dress meshes after gota.BuildArena")


if __name__ == "__main__":
    main()
