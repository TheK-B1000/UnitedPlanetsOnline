"""Imports the Meshy "Ironfeather Valkyrie" Ayla (2026-11-24 export) into an isolated /Game/Characters/Ayla folder.

Run (editor, no window):
  UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this> -unattended -nosplash -nullrhi
Source: the working copies in GOTA_AYLA_SOURCE (default B:\\Unreal\\GOTA_SourceAssets\\Ayla\\Meshy_20261124); the Desktop
originals are never read. Report: Saved/GOTA_ImportAyla.json.

Ayla gets her own skeleton: Meshy's "UE5 Mannequin" template is a 28-bone skeleton (pelvis root, no root bone, no
fingers or twist bones) that matches no skeleton in this project. Nothing outside /Game/Characters/Ayla is written.
Refuses to run if that folder already exists (no overwrite).
"""
import json
import os

import unreal

SRC = os.environ.get("GOTA_AYLA_SOURCE", r"B:\Unreal\GOTA_SourceAssets\Ayla\Meshy_20261124")
STEM = "Meshy_AI_Ironfeather_Valkyrie_20261124_animated_ue5_mannequin_"
# The FBX files come from the Blender normalization pass (Tools/Ayla/normalize_fbx.py, see Docs/GOTA_AYLA_IMPORT.md): Meshy's own
# files import with a x100 scale on the pelvis bind pose that the takes lack, so every animation played at 1/100 size.
NORM = os.environ.get("GOTA_AYLA_NORMALIZED", os.path.join(os.path.dirname(SRC), "Normalized"))
MAIN = os.path.join(NORM, "Ayla_Character.fbx")
EXTRA = {"Backflip": os.path.join(NORM, "Ayla_Backflip.fbx"), "Mirror_Viewing": os.path.join(NORM, "Ayla_Mirror_Viewing.fbx")}
TEX = {"BaseColor": "", "Metallic": "_metallic", "Normal": "_normal", "Roughness": "_roughness"}

ROOT = "/Game/Characters/Ayla"
MESH_DIR, TEX_DIR, MAT_DIR, ANIM_DIR = ROOT + "/Mesh", ROOT + "/Textures", ROOT + "/Materials", ROOT + "/Animations"
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
report = {"source": SRC, "warnings": []}


def tasks(*t):
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(list(t))


def task(filename, dest, name, options=None):
    t = unreal.AssetImportTask()
    t.filename = filename
    t.destination_path = dest
    t.destination_name = name
    t.automated = True
    t.replace_existing = False
    t.save = False
    if options:
        t.options = options
    return t


def fbx_options(skeleton=None, mesh=True):
    o = unreal.FbxImportUI()
    kind = unreal.FBXImportType.FBXIT_SKELETAL_MESH if mesh else unreal.FBXImportType.FBXIT_ANIMATION
    for k, v in (("import_mesh", mesh), ("import_as_skeletal", True), ("mesh_type_to_import", kind), ("original_import_type", kind),
                 ("import_materials", False), ("import_textures", False), ("import_animations", True),
                 ("create_physics_asset", mesh), ("automated_import_should_detect_type", False)):
        o.set_editor_property(k, v)
    if skeleton:
        o.set_editor_property("skeleton", skeleton)
    sk = o.get_editor_property("skeletal_mesh_import_data")
    # Meshy's FBX is authored in metres. Without unit conversion Unreal puts a x100 scale on the pelvis in the bind pose
    # while the takes' pelvis keys carry none, so every animation shrinks the body to 1/100.
    for k, v in (("import_morph_targets", False), ("convert_scene", True), ("convert_scene_unit", True), ("import_uniform_scale", 1.0)):
        try:
            sk.set_editor_property(k, v)
        except Exception as e:
            report["warnings"].append("mesh option %s: %s" % (k, e))
    an = o.get_editor_property("anim_sequence_import_data")
    for k, v in (("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME),
                 ("use_default_sample_rate", False),   # keep Meshy's 30 fps
                 ("convert_scene", True), ("convert_scene_unit", True), ("import_uniform_scale", 1.0), ("remove_redundant_keys", False)):
        try:
            an.set_editor_property(k, v)
        except Exception as e:
            report["warnings"].append("anim option %s: %s" % (k, e))
    return o


def anims_in(path):
    return [a for a in (EAL.load_asset(p) for p in EAL.list_assets(path, recursive=False)) if isinstance(a, unreal.AnimSequence)]


def clip_name(seq, fallback):
    # Legacy FBX import names a take "<asset>_Anim_<Armature|Take>" or similar; keep only the take.
    n = seq.get_name()
    for cut in ("_Anim_Armature_", "_Anim_"):
        if cut in n:
            n = n.split(cut, 1)[1]
    n = n.replace("Armature_", "").replace("_BaseLayer", "")
    return n or fallback


REPORT_ONLY = os.environ.get("GOTA_AYLA_REPORT_ONLY") == "1"  # re-read what an earlier run imported, import nothing

if not REPORT_ONLY:
    if EAL.does_directory_exist(ROOT) and EAL.list_assets(ROOT):
        raise RuntimeError(ROOT + " already has assets; this import never overwrites")

    # The legacy FBX importer honours FbxImportUI (the Interchange FBX path ignores it). This switch is per-session.
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")

    # ---- textures
    for slot, suffix in TEX.items():
        png = os.path.join(SRC, "All_animation", STEM + "All_animation", STEM + "All_animation" + suffix + ".png")
        tasks(task(png, TEX_DIR, "T_Ayla_" + slot))
        tex = EAL.load_asset(TEX_DIR + "/T_Ayla_" + slot)
        if slot == "Normal":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("flip_green_channel", False)
        elif slot != "BaseColor":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        report.setdefault("textures", {})[slot] = {"path": tex.get_path_name(), "size": [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()],
                                                   "srgb": tex.get_editor_property("srgb")}

    # ---- mesh, its own skeleton, physics asset, and every take in the combined file
    tasks(task(MAIN, MESH_DIR, "SK_Ayla", fbx_options()))
    mesh = EAL.load_asset(MESH_DIR + "/SK_Ayla")
    if not mesh:
        raise RuntimeError("skeletal mesh import failed")
    skeleton = mesh.skeleton
    phys = mesh.physics_asset
    # Rename the generated skeleton and physics asset to the project's prefixes.
    EAL.rename_asset(skeleton.get_path_name().split(".")[0], MESH_DIR + "/SKEL_Ayla")
    if phys:
        EAL.rename_asset(phys.get_path_name().split(".")[0], MESH_DIR + "/PHYS_Ayla")
    skeleton = EAL.load_asset(MESH_DIR + "/SKEL_Ayla")

    # Takes imported with the mesh land next to it: move each to Animations/AS_Ayla_<take>.
    moved = []
    for seq in anims_in(MESH_DIR):
        name = "AS_Ayla_" + clip_name(seq, seq.get_name())
        EAL.rename_asset(seq.get_path_name().split(".")[0], ANIM_DIR + "/" + name)
        moved.append(name)
    report["main_takes_imported"] = len(moved)

    # ---- the two separately exported actions, onto Ayla's skeleton
    for take, fbx in EXTRA.items():
        tasks(task(fbx, ANIM_DIR + "/_incoming", "AS_Ayla_" + take, fbx_options(skeleton=skeleton, mesh=False)))
        got = anims_in(ANIM_DIR + "/_incoming")
        if not got:
            report["warnings"].append("no animation imported from " + take)
        for seq in got:
            EAL.rename_asset(seq.get_path_name().split(".")[0], ANIM_DIR + "/AS_Ayla_" + take)
        for leftover in EAL.list_assets(ANIM_DIR + "/_incoming"):
            report["warnings"].append("unexpected asset from %s import: %s" % (take, leftover))
    if EAL.does_directory_exist(ANIM_DIR + "/_incoming") and not EAL.list_assets(ANIM_DIR + "/_incoming"):
        EAL.delete_directory(ANIM_DIR + "/_incoming")

    # ---- material from Meshy's four PBR maps
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_Ayla", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    # Without this flag the game silently swaps in the grey default material on a skeletal mesh.
    mat.set_editor_property("used_with_skeletal_mesh", True)


    def sample(slot, x, y, sampler):
        node = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y)
        node.set_editor_property("texture", EAL.load_asset(TEX_DIR + "/T_Ayla_" + slot))
        node.set_editor_property("sampler_type", sampler)
        return node


    base = sample("BaseColor", -600, -300, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    metal = sample("Metallic", -600, 0, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    rough = sample("Roughness", -600, 250, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    normal = sample("Normal", -600, 500, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    MEL.connect_material_property(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(metal, "R", unreal.MaterialProperty.MP_METALLIC)
    MEL.connect_material_property(rough, "R", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)
    MEL.recompile_material(mat)
    # Slots come back as copies: set the material on each copy, then write the whole list back.
    slots = []
    for m in mesh.get_editor_property("materials"):
        m.set_editor_property("material_interface", mat)
        slots.append(m)
    mesh.set_editor_property("materials", slots)

    # ---- save, then record what exists
    for path in EAL.list_assets(ROOT, recursive=True):
        EAL.save_asset(path, only_if_is_dirty=False)

mesh = EAL.load_asset(MESH_DIR + "/SK_Ayla")
skeleton = EAL.load_asset(MESH_DIR + "/SKEL_Ayla")
bounds = mesh.get_bounds()
for slot in TEX:
    tex = EAL.load_asset(TEX_DIR + "/T_Ayla_" + slot)
    report.setdefault("textures", {})[slot] = {"path": tex.get_path_name(), "size": [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()],
                                               "srgb": tex.get_editor_property("srgb"),
                                               "compression": str(tex.get_editor_property("compression_settings"))}
report["mesh"] = {"path": mesh.get_path_name(), "skeleton": skeleton.get_path_name(),
                  "physics_asset": mesh.physics_asset.get_path_name() if mesh.physics_asset else None,
                  "bounds_extent_cm": [bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z],
                  "bounds_origin_cm": [bounds.origin.x, bounds.origin.y, bounds.origin.z],
                  "material_slots": [str(m.material_slot_name) for m in mesh.materials],
                  "materials": [m.material_interface.get_path_name() if m.material_interface else None for m in mesh.materials]}
if mesh.physics_asset:
    try:
        report["mesh"]["physics_bodies"] = len(mesh.physics_asset.get_editor_property("skeletal_body_setups"))
    except Exception:
        report["mesh"]["physics_bodies"] = "not readable from Python"
report["animations"] = []


def safe(fn, default=None):
    try:
        return fn()
    except Exception as e:
        return "%s: %s" % (default, e) if default else None


for seq in sorted(anims_in(ANIM_DIR), key=lambda s: s.get_name()):
    frames = safe(lambda: unreal.AnimationLibrary.get_num_frames(seq))
    secs = safe(lambda: round(seq.get_play_length(), 3))
    pelvis = safe(lambda: unreal.AnimationLibrary.get_bone_pose_for_frame(seq, "pelvis", 0, False).translation)
    report["animations"].append({"name": seq.get_name(), "seconds": secs, "frames": frames,
                                 "fps": round(frames / secs, 2) if isinstance(frames, int) and isinstance(secs, float) and secs > 0 else None,
                                 "skeleton": safe(lambda: seq.get_skeleton().get_name()),
                                 "pelvis_frame0_cm": [round(pelvis.x, 2), round(pelvis.y, 2), round(pelvis.z, 2)] if pelvis is not None else None})
# Scale sanity: the bind pose and every take must agree (no scale on any bone, pelvis about 90 cm up).
try:
    ref = unreal.AnimPoseExtensions.get_reference_pose(skeleton)
    pose = {}
    for b in ("pelvis", "spine_01", "head", "foot_l"):
        w = unreal.AnimPoseExtensions.get_bone_pose(ref, b, unreal.AnimPoseSpaces.WORLD)
        l = unreal.AnimPoseExtensions.get_bone_pose(ref, b, unreal.AnimPoseSpaces.LOCAL)
        pose[b] = {"world_cm": [round(w.translation.x, 2), round(w.translation.y, 2), round(w.translation.z, 2)],
                   "local_cm": [round(l.translation.x, 2), round(l.translation.y, 2), round(l.translation.z, 2)],
                   "scale": [round(l.scale3d.x, 3), round(l.scale3d.y, 3), round(l.scale3d.z, 3)]}
    report["reference_pose"] = pose
except Exception as e:
    report["warnings"].append("reference pose not readable: %s" % e)
for a in report["animations"]:
    seq = EAL.load_asset(ANIM_DIR + "/" + a["name"])
    t = safe(lambda: unreal.AnimationLibrary.get_bone_pose_for_frame(seq, "pelvis", 0, False))
    if t is not None:
        a["pelvis_frame0_scale"] = [round(t.scale3d.x, 3), round(t.scale3d.y, 3), round(t.scale3d.z, 3)]
report["created"] = sorted(EAL.list_assets(ROOT, recursive=True))
out = os.path.join(unreal.Paths.project_saved_dir(), "GOTA_ImportAyla.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(report, f, indent=1)
unreal.log("GOTA_ImportAyla: %d animations, report %s" % (len(report["animations"]), out))
