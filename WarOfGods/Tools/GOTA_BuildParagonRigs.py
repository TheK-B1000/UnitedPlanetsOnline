"""Builds IK Rigs + IK Retargeters that let a Paragon body follow the UE5 Mannequin combat character.

The playable character (BP_Player) keeps its Mannequin skeleton, combat montages and hit windows.
At runtime a Paragon mesh is attached to it and copies the pose through the retargeter
(UGOTARetargetAnimInstance). Paragon meshes are prototype bodies, not final god designs.

Output: /Game/Heroes/Paragon/Rigs/IK_GOTA_<Body>, RTG_GOTA_Manny_<Body>
Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_BuildParagonRigs.txt
"""
import os
import unreal

OUT_DIR = "/Game/Heroes/Paragon/Rigs"
SOURCE_RIG = "/Game/Characters/Mannequins/Rigs/IK_Mannequin"
SOURCE_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"

BODIES = {
    "Greystone": "/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone",
    "Countess": "/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/SM_Countess",
    "Sevarog": "/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog",
    # Windwalker Echo (Epic sample, migrated by Tools/GOTA_MigrateEcho.py): UE5 Mannequin bone layout.
    "Echo": "/Game/EchoContent/Characters/Echo/Meshes/Echo",
}
# UE5 Mannequin layout (Echo): full spine/neck and finger chains so hand grips on the gameplay sword hold.
CHAINS_UE5 = [
    ("Spine", "spine_01", "spine_05", None),
    ("Neck", "neck_01", "neck_02", None),
    ("Head", "head", "head", None),
    ("LeftClavicle", "clavicle_l", "clavicle_l", None),
    ("RightClavicle", "clavicle_r", "clavicle_r", None),
    ("LeftArm", "upperarm_l", "hand_l", "hand_l"),
    ("RightArm", "upperarm_r", "hand_r", "hand_r"),
    ("LeftLeg", "thigh_l", "ball_l", "foot_l"),
    ("RightLeg", "thigh_r", "ball_r", "foot_r"),
] + [("%s%s" % (side, f.capitalize()), "%s_%s_%s" % (f, "metacarpal" if f != "thumb" else "01", s[0].lower()), "%s_03_%s" % (f, s[0].lower()), None)
     for side, s in (("Left", "l"), ("Right", "r")) for f in ("thumb", "index", "middle", "ring", "pinky")]
UE5_BODIES = {"Echo"}

ONLY = [b.strip() for b in os.environ.get("GOTA_BODIES", "").split(",") if b.strip()]

# Manual chain fallback (UE4 / Paragon naming) if auto-detection does not recognise the skeleton.
CHAINS = [
    ("Spine", "spine_01", "spine_03", None),
    ("Neck", "neck_01", "neck_01", None),
    ("Head", "head", "head", None),
    ("LeftClavicle", "clavicle_l", "clavicle_l", None),
    ("RightClavicle", "clavicle_r", "clavicle_r", None),
    ("LeftArm", "upperarm_l", "hand_l", "hand_l"),
    ("RightArm", "upperarm_r", "hand_r", "hand_r"),
    ("LeftLeg", "thigh_l", "ball_l", "foot_l"),
    ("RightLeg", "thigh_r", "ball_r", "foot_r"),
]

log = []


def P(*a):
    line = " ".join(str(x) for x in a)
    log.append(line)
    unreal.log("GOTA_ParagonRigs: " + line)
    with open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_BuildParagonRigs.trace.txt"), "a") as f:
        f.write(line + "\n")


def bone_names(mesh):
    return [str(n) for n in mesh.get_editor_property("skeleton").get_reference_pose().get_bone_names()]


def fresh_asset(name, cls, factory):
    path = "%s/%s" % (OUT_DIR, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, OUT_DIR, cls, factory)


def build_rig(body, mesh):
    rig = fresh_asset("IK_GOTA_" + body, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    P(body, "step set_skeletal_mesh")
    if not c.set_skeletal_mesh(mesh):
        raise RuntimeError("mesh rejected by IK Rig")
    # Manual FK chains. The engine's auto-template + auto-FBIK path crashes on Paragon skeletons
    # in 5.8 (array index -1 inside IKRigEditor), and FK chains are enough to carry the pose.
    names = set(bone_names(mesh))
    P(body, "bones", len(names), sorted(n for n in names if any(k in n for k in ("neck", "head", "ball", "foot", "thigh", "calf", "spine"))))
    c.set_retarget_root("pelvis")
    for chain, start, end, goal in (CHAINS_UE5 if body in UE5_BODIES else CHAINS):
        if end not in names and goal in names:
            end = goal  # e.g. Sevarog: legs end at the foot (no ball/toe bones)
        if start in names and end in names:
            P(body, "  chain", chain, start, end)
            c.add_retarget_chain(chain, start, end, "")
        else:
            P(body, "  skip chain", chain, start, end)
    chains = [str(ch.chain_name) for ch in c.get_retarget_chains()]
    P(body, "root", c.get_retarget_root(), "chains", len(chains), chains)
    unreal.EditorAssetLibrary.save_loaded_asset(rig)
    return rig


def build_retargeter(body, src_rig, dst_rig, src_mesh, dst_mesh):
    rtg = fresh_asset("RTG_GOTA_Manny_" + body, unreal.IKRetargeter, unreal.IKRetargetFactory())
    c = unreal.IKRetargeterController.get_controller(rtg)
    S, T = unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetSourceOrTarget.TARGET
    P(body, 'step c.set_ik_rig')
    c.set_ik_rig(S, src_rig)
    c.set_ik_rig(T, dst_rig)
    c.set_preview_mesh(S, src_mesh)
    c.set_preview_mesh(T, dst_mesh)
    P(body, 'step c.add_default_ops')
    c.add_default_ops()
    c.assign_ik_rig_to_all_ops(S, src_rig)
    c.assign_ik_rig_to_all_ops(T, dst_rig)
    P(body, 'step c.auto_map_chains')
    c.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    # Paragon reference poses differ from the Mannequin; align the target to the source pose.
    P(body, 'step c.auto_align_all_bones')
    c.auto_align_all_bones(T, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    try:
        P(body, 'step c.snap_bone_to_ground')
        target_bones = set(bone_names(dst_mesh))
        ground_bone = "ball_l" if "ball_l" in target_bones else "foot_l"
        if ground_bone in target_bones:  # snapping to a missing bone crashes the editor
            c.snap_bone_to_ground(ground_bone, T)
    except Exception as e:
        P(body, "  snap to ground failed:", e)
    ops = [str(c.get_op_name(i)) for i in range(c.get_num_retarget_ops())]
    P(body, "retargeter ops", ops)
    unreal.EditorAssetLibrary.save_loaded_asset(rtg)
    return rtg


def main():
    unreal.EditorAssetLibrary.make_directory(OUT_DIR)
    src_rig = unreal.load_asset(SOURCE_RIG)
    src_mesh = unreal.load_asset(SOURCE_MESH)
    if not src_rig or not src_mesh:
        P("FAIL source rig/mesh missing")
        return
    for body, mesh_path in BODIES.items():
        if ONLY and body not in ONLY:
            continue
        mesh = unreal.load_asset(mesh_path)
        if not mesh:
            P(body, "FAIL mesh missing", mesh_path)
            continue
        try:
            rig = build_rig(body, mesh)
            build_retargeter(body, src_rig, rig, src_mesh, mesh)
            P(body, "OK")
        except Exception as e:
            P(body, "FAIL", e)


main()
open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_BuildParagonRigs.txt"), "w").write("\n".join(log))
