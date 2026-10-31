# Arena overhaul — Phase 0 asset inventory (run in the full editor):
#   UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript=<abs>/Tools/GOTA_ArenaAssetInventory.py -unattended
# Writes Saved/GOTA_ArenaAssetInventory.json: every candidate environment mesh with its real
# size (cm), Nanite flag, triangle count and material; plus surface texture sets.
import json
import os

import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
OUT = os.path.join(PROJECT, "Saved", "GOTA_ArenaAssetInventory.json")
AR = unreal.AssetRegistryHelpers.get_asset_registry()

ROOTS = [
    "/Game/Megascans/3D_Assets", "/Game/Megascans/3D_Plants", "/Game/Geometry", "/Game/Iceland_Environment",
    "/Game/WinterForestSet/Meshes", "/Game/EuropeanBeech/Geometry", "/Game/NorwayMaple/Geometry",
    "/Game/PhotoR_Backgrounds/Meshes", "/Game/StarterContent/Architecture", "/Game/StarterContent/Props",
    "/Game/A_Surface_Footstep/Environment_Assets", "/Game/Props",
]

meshes = []
for root in ROOTS:
    flt = unreal.ARFilter(package_paths=[root], recursive_paths=True,
                          class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")])
    for data in AR.get_assets(flt):
        path = str(data.package_name) + "." + str(data.asset_name)
        mesh = unreal.load_asset(path)
        if not mesh:
            continue
        box = mesh.get_bounding_box()
        size = box.max - box.min
        try:
            nanite = bool(mesh.get_editor_property("nanite_settings").enabled)
        except Exception:
            nanite = None
        try:
            tris = unreal.EditorStaticMeshLibrary.get_number_triangles(mesh, 0) if hasattr(unreal, "EditorStaticMeshLibrary") else -1
        except Exception:
            tris = -1
        mats = []
        try:
            for sm in mesh.get_editor_property("static_materials"):
                mi = sm.get_editor_property("material_interface")
                mats.append(mi.get_path_name() if mi else None)
        except Exception:
            pass
        meshes.append({
            "path": path, "folder": str(data.package_path),
            "size": [round(size.x), round(size.y), round(size.z)],
            "pivot_min_z": round(box.min.z), "nanite": nanite, "tris": tris, "materials": mats[:3],
        })
    unreal.log("inventory %s: %d meshes so far" % (root, len(meshes)))

surfaces = {}
flt = unreal.ARFilter(package_paths=["/Game/Megascans/Surfaces", "/Game/Megascans/Rock", "/Game/Megascans/ErodedGround",
                                     "/Game/Iceland_Environment/Textures", "/Game/EuropeanBeech/Textures/Tileable",
                                     "/Game/NorwayMaple/Textures/Tileable"],
                      recursive_paths=True, class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "Texture2D")])
for data in AR.get_assets(flt):
    surfaces.setdefault(str(data.package_path), []).append(str(data.asset_name))

materials = []
flt = unreal.ARFilter(package_paths=["/Game"], recursive_paths=True,
                      class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "Material")])
for data in AR.get_assets(flt):
    n = str(data.asset_name).lower()
    if any(k in n for k in ["lava", "water", "ocean", "ice", "snow", "magma", "emissive", "glow", "fog", "cloud", "sky"]):
        materials.append(str(data.package_name))

systems = []
for cls in [("/Script/Niagara", "NiagaraSystem"), ("/Script/Engine", "ParticleSystem")]:
    flt = unreal.ARFilter(package_paths=["/Game"], recursive_paths=True, class_paths=[unreal.TopLevelAssetPath(*cls)])
    for data in AR.get_assets(flt):
        systems.append(cls[1] + " " + str(data.package_name))

sounds = []
flt = unreal.ARFilter(package_paths=["/Game"], recursive_paths=True,
                      class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "SoundWave"), unreal.TopLevelAssetPath("/Script/Engine", "SoundCue")])
for data in AR.get_assets(flt):
    p = str(data.package_name)
    if "Vocal" not in p and "Paragon" not in p:
        sounds.append(p)

with open(OUT, "w", encoding="utf-8") as f:
    json.dump({"meshes": meshes, "surfaces": surfaces, "materials": materials, "systems": systems, "sounds": sounds}, f, indent=1)
unreal.log("GOTA inventory written: %d meshes, %d surface folders, %d materials, %d fx, %d sounds -> %s"
           % (len(meshes), len(surfaces), len(materials), len(systems), len(sounds), OUT))

if os.environ.get("GOTA_QUIT_AFTER_BUILD") == "1":
    unreal.SystemLibrary.quit_editor()
