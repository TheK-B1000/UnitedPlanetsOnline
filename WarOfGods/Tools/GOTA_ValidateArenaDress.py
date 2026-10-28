# Validate dress DAs resolve + ArenaGrayboxBuilder places dress meshes.
# Prefer running AFTER GOTA_CreateAllDressDAs.py.
#   UnrealEditor-Cmd.exe ... -ExecutePythonScript=.../GOTA_ValidateArenaDress.py -unattended -nosplash -NullRHI

import unreal

DA_PATHS = [
    "/Game/Arenas/Data/DA_Dress_TitansQuarry",
    "/Game/Arenas/Data/DA_Dress_AshfallCrucible",
    "/Game/Arenas/Data/DA_Dress_FrostboundCitadel",
    "/Game/Arenas/Data/DA_Dress_VerdantSanctuary",
    "/Game/Arenas/Data/DA_Dress_SunscarExpanse",
    "/Game/Arenas/Data/DA_Dress_StormbreakCoast",
    "/Game/Arenas/Data/DA_Dress_ShatteredDominion",
    "/Game/Arenas/Data/DA_Dress_FallenSanctuary",
]

# EArenaGrayboxLayout order
LAYOUTS = [
    ("TitansQuarry", 0),
    ("AshfallCrucible", 1),
    ("FrostboundCitadel", 2),
    ("VerdantSanctuary", 3),
    ("SunscarExpanse", 4),
    ("StormbreakCoast", 5),
    ("ShatteredDominion", 6),
    ("FallenSanctuary", 7),
]


def soft_package_path(soft) -> str:
    """Normalize SoftObjectPtr / SoftObjectPath to a /Game/... package path."""
    if soft is None:
        return ""
    candidates = []
    try:
        if hasattr(soft, "to_soft_object_path"):
            soft = soft.to_soft_object_path()
    except Exception:
        pass
    for getter in ("get_long_package_name", "get_asset_path_string", "get_package_name"):
        try:
            if hasattr(soft, getter):
                candidates.append(getattr(soft, getter)())
        except Exception:
            pass
    candidates.append(str(soft))
    for raw in candidates:
        s = (raw or "").strip().strip("'\"")
        if not s or s.startswith("<") or s in ("None", "None.None"):
            continue
        # SoftObjectPath print form: /Game/Path/Asset.Asset
        package = s.split(".", 1)[0].strip().strip("'\"")
        if package.startswith("/"):
            return package
    return ""


def soft_ok(soft):
    package = soft_package_path(soft)
    if not package:
        return False
    try:
        return unreal.EditorAssetLibrary.does_asset_exist(package)
    except Exception:
        return False


def validate_da(path: str) -> dict:
    result = {"path": path, "exists": False, "cliffs": 0, "debris": 0, "pillars": 0, "landmarks": 0, "ok_meshes": 0, "total_meshes": 0}
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        return result
    dress = unreal.EditorAssetLibrary.load_asset(path)
    if not dress:
        return result
    result["exists"] = True

    cliffs = dress.get_editor_property("CliffMeshPool") or []
    debris = dress.get_editor_property("DebrisMeshPool") or []
    pillars = dress.get_editor_property("PillarMeshPool") or []
    slots = dress.get_editor_property("DressSlots") or []
    result["cliffs"] = len(cliffs)
    result["debris"] = len(debris)
    result["pillars"] = len(pillars)
    result["landmarks"] = len(slots)

    softs = list(cliffs) + list(debris) + list(pillars)
    for slot in slots:
        try:
            softs.append(slot.get_editor_property("Mesh"))
        except Exception:
            pass

    result["total_meshes"] = len(softs)
    result["ok_meshes"] = sum(1 for s in softs if soft_ok(s))
    return result


def resolve_layout_enum(index: int):
    try:
        values = [
            unreal.ArenaGrayboxLayout.TITANS_QUARRY,
            unreal.ArenaGrayboxLayout.ASHFALL_CRUCIBLE,
            unreal.ArenaGrayboxLayout.FROSTBOUND_CITADEL,
            unreal.ArenaGrayboxLayout.VERDANT_SANCTUARY,
            unreal.ArenaGrayboxLayout.SUNSCAR_EXPANSE,
            unreal.ArenaGrayboxLayout.STORMBREAK_COAST,
            unreal.ArenaGrayboxLayout.SHATTERED_DOMINION,
            unreal.ArenaGrayboxLayout.FALLEN_SANCTUARY,
        ]
        return values[index]
    except Exception:
        return index


def build_layout_in_editor(name: str, index: int) -> dict:
    """Spawn graybox builder, assign layout, build, count static mesh comps with non-cube meshes."""
    out = {"layout": name, "built": False, "total_comps": 0, "dressed_comps": 0, "error": ""}
    try:
        # Load a lightweight map if needed
        world = unreal.EditorLevelLibrary.get_editor_world()
        if not world:
            out["error"] = "no editor world"
            return out

        loc = unreal.Vector(index * 15000.0, 0.0, 0.0)
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ArenaGrayboxBuilder, loc)
        if not actor:
            out["error"] = "spawn failed"
            return out

        actor.set_editor_property("Layout", resolve_layout_enum(index))
        actor.set_editor_property("bApplyVisualDress", True)
        actor.set_editor_property("bSpawnPlayerStarts", False)
        actor.set_editor_property("bSpawnBiomeVolumes", False)
        actor.build_layout()
        out["built"] = True

        comps = actor.get_components_by_class(unreal.StaticMeshComponent)
        out["total_comps"] = len(comps) if comps else 0
        dressed = 0
        for c in comps or []:
            mesh = c.get_editor_property("StaticMesh") if c else None
            if mesh and "BasicShapes" not in str(mesh.get_path_name()) and "LevelPrototyping" not in str(mesh.get_path_name()):
                # Count anything that isn't engine cube/cylinder prototyping as dress-ish;
                # also count LevelPrototyping as prototype.
                path = mesh.get_path_name()
                if "/Engine/" in path or "SM_Cube" in path or "SM_Cylinder" in path:
                    continue
                dressed += 1
        out["dressed_comps"] = dressed

        unreal.EditorLevelLibrary.destroy_actor(actor)
    except Exception as exc:
        out["error"] = str(exc)
    return out


def main():
    unreal.log("=== GOTA Validate Arena Dress DAs ===")
    da_results = [validate_da(p) for p in DA_PATHS]
    missing_da = [r["path"] for r in da_results if not r["exists"]]
    for r in da_results:
        if not r["exists"]:
            unreal.log_error(f"MISSING DA {r['path']}")
        else:
            unreal.log(
                f"DA OK {r['path']} cliffs={r['cliffs']} debris={r['debris']} pillars={r['pillars']} "
                f"landmarks={r['landmarks']} meshes={r['ok_meshes']}/{r['total_meshes']}"
            )

    unreal.log("=== BuildLayout smoke (editor world) ===")
    build_results = []
    for name, idx in LAYOUTS:
        br = build_layout_in_editor(name, idx)
        build_results.append(br)
        if br["error"]:
            unreal.log_error(f"Build {name}: ERROR {br['error']}")
        else:
            unreal.log(f"Build {name}: comps={br['total_comps']} dressed~={br['dressed_comps']}")

    all_da = all(r["exists"] for r in da_results)
    any_dress = any(r["dressed_comps"] > 0 for r in build_results)
    if all_da and any_dress:
        unreal.log("RESULT: PASS — all DAs present and at least one layout placed dress meshes")
    elif all_da:
        unreal.log_warning("RESULT: PARTIAL — DAs present but dress mesh count was 0 (check soft loads / NullRHI)")
    else:
        unreal.log_error(f"RESULT: FAIL — missing DAs: {missing_da}")


if __name__ == "__main__":
    main()
