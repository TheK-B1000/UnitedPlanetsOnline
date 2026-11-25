"""IK Rig and IK Retargeter that let Ayla's body follow the shared combat frame (BP_Player, UE5 Mannequin).

Same method as Tools/GOTA_BuildParagonRigs.py (FK chains, pelvis retarget root, fuzzy chain map, chain-to-chain
alignment, foot snapped to the ground), kept in Ayla's own folder. Her 28-bone skeleton has UE4-style names, no fingers
and no twist bones, so the chains stop at the hands.

Output: /Game/Characters/Ayla/Rigs/IK_GOTA_Ayla, RTG_GOTA_Manny_Ayla
Run: UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this> -unattended -nosplash -nullrhi
"""
import unreal

OUT = "/Game/Characters/Ayla/Rigs"
SOURCE_RIG = "/Game/Characters/Mannequins/Rigs/IK_Mannequin"
SOURCE_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"
MESH = "/Game/Characters/Ayla/Mesh/SK_Ayla"
CHAINS = [
    ("Spine", "spine_01", "spine_03"),
    ("Neck", "neck_01", "neck_01"),
    ("Head", "head", "head"),
    ("LeftClavicle", "clavicle_l", "clavicle_l"),
    ("RightClavicle", "clavicle_r", "clavicle_r"),
    ("LeftArm", "upperarm_l", "hand_l"),
    ("RightArm", "upperarm_r", "hand_r"),
    ("LeftLeg", "thigh_l", "ball_l"),
    ("RightLeg", "thigh_r", "ball_r"),
]
EAL = unreal.EditorAssetLibrary


def fresh(name, cls, factory):
    path = OUT + "/" + name
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)  # derived from SK_Ayla and the mannequin rig; rebuilt every run
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, OUT, cls, factory)


mesh = unreal.load_asset(MESH)
src_rig, src_mesh = unreal.load_asset(SOURCE_RIG), unreal.load_asset(SOURCE_MESH)
rig = fresh("IK_GOTA_Ayla", unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
c = unreal.IKRigController.get_controller(rig)
if not c.set_skeletal_mesh(mesh):
    raise RuntimeError("SK_Ayla rejected by IK Rig")
c.set_retarget_root("pelvis")
for chain, start, end in CHAINS:
    c.add_retarget_chain(chain, start, end, "")
unreal.log("AYLA RIG chains %s" % [str(ch.chain_name) for ch in c.get_retarget_chains()])
EAL.save_loaded_asset(rig)

rtg = fresh("RTG_GOTA_Manny_Ayla", unreal.IKRetargeter, unreal.IKRetargetFactory())
r = unreal.IKRetargeterController.get_controller(rtg)
S, T = unreal.RetargetSourceOrTarget.SOURCE, unreal.RetargetSourceOrTarget.TARGET
r.set_ik_rig(S, src_rig)
r.set_ik_rig(T, rig)
r.set_preview_mesh(S, src_mesh)
r.set_preview_mesh(T, mesh)
r.add_default_ops()
r.assign_ik_rig_to_all_ops(S, src_rig)
r.assign_ik_rig_to_all_ops(T, rig)
r.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
# Ayla's bind pose is an A-pose; align her chains to the mannequin's reference pose, then put her feet on the ground.
r.auto_align_all_bones(T, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
r.snap_bone_to_ground("ball_l", T)
unreal.log("AYLA RIG retargeter ops %s" % [str(r.get_op_name(i)) for i in range(r.get_num_retarget_ops())])
EAL.save_loaded_asset(rtg)
