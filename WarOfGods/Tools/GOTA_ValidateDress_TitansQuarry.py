# GOTA Playable Integration 01 — Task 1 validation (Editor Python)
# Verifies DA_Dress_TitansQuarry + soft mesh refs without requiring PIE screenshots.
# Writes: Saved/GOTA_Dress_TitansQuarry_Validate.txt

import unreal
import os

ASSET_PATH = "/Game/Arenas/Data/DA_Dress_TitansQuarry"
OUT_REL = "Saved/GOTA_Dress_TitansQuarry_Validate.txt"

EXPECTED = [
    "/Game/Geometry/SM_Quarry_Cliff_L_01",
    "/Game/Geometry/SM_Quarry_Cliff_L_02",
    "/Game/Geometry/SM_Quarry_Cliff_L_03",
    "/Game/Geometry/SM_Quarry_Cliff_M_01",
    "/Game/Geometry/SM_Quarry_Cliff_M_02",
    "/Game/Geometry/SM_Quarry_Cliff_M_03",
    "/Game/Geometry/SM_Quarry_RockPile_S_01",
    "/Game/Geometry/SM_Quarry_RockPile_S_02",
    "/Game/Geometry/SM_Quarry_Ridge_S_01",
    "/Game/Geometry/SM_Quarry_Spire_L_01",
    "/Game/Geometry/SM_Quarry_Spire_L_02",
    "/Game/Megascans/3D_Assets/Limestone_Rocks_01/SM_Limestone_Rocks_01",
    "/Game/Megascans/3D_Assets/Limestone_Rubble_01/SM_Limestone_Rubble_01",
    "/Game/Megascans/3D_Assets/Limestone_Slab_01/SM_Limestone_Slab_01",
    "/Game/Megascans/3D_Assets/Quarry_Outcrop_01/SM_Quarry_Outcrop_01",
    "/Game/Geometry/Custom/SM_RockGround_01",
]


def project_dir():
    return unreal.Paths.project_dir()


def write_report(lines):
    path = os.path.join(project_dir(), OUT_REL.replace("/", os.sep))
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    unreal.log(f"Wrote {path}")


def main():
    lines = ["GOTA Task1 Dress Validation", f"DA={ASSET_PATH}"]
    ok = True

    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        lines.append("FAIL: DA_Dress_TitansQuarry missing")
        # Try create script path
        create_script = os.path.join(project_dir(), "Tools", "GOTA_CreateDress_TitansQuarry.py")
        lines.append(f"Run create script: {create_script}")
        write_report(lines)
        return False

    dress = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    lines.append(f"LOAD: {dress}")

    for p in EXPECTED:
        exists = unreal.EditorAssetLibrary.does_asset_exist(p)
        lines.append(f"{'OK' if exists else 'MISS'} mesh {p}")
        if not exists:
            ok = False

    # Soft pool sizes if readable
    try:
        cliffs = dress.get_editor_property("CliffMeshPool") or []
        debris = dress.get_editor_property("DebrisMeshPool") or []
        pillars = dress.get_editor_property("PillarMeshPool") or []
        slots = dress.get_editor_property("DressSlots") or []
        lines.append(f"POOL cliffs={len(cliffs)} debris={len(debris)} pillars={len(pillars)} slots={len(slots)}")
        if len(cliffs) < 3 or len(debris) < 3:
            lines.append("WARN: dress pools look sparse — re-run GOTA_CreateDress_TitansQuarry.py")
            ok = False
    except Exception as e:
        lines.append(f"WARN: could not read pools ({e})")

    lines.append("RESULT: PASS" if ok else "RESULT: FAIL")
    lines.append("PIE still required: open ThirdPersonMap?Arena=Quarry ; gota.BuildArena Quarry")
    write_report(lines)
    if ok:
        unreal.log("Task1 soft-ref validation PASS (PIE visual check still required)")
    else:
        unreal.log_error("Task1 soft-ref validation FAIL")
    return ok


if __name__ == "__main__":
    main()
