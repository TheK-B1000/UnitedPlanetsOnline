"""Hand IK for retargeted bodies whose proportions keep the hands away from the gameplay hands (wide shoulders,
long or splayed arms): Steel, the lane minions...

FK chains copy rotations, so a body with wider shoulders or longer arms puts its hands somewhere else than the
hidden gameplay mannequin's hands (where the weapon traces and grips are). This adds, to each body's IK rig, a
two-bone limb solver per arm (upperarm -> hand) with a hand goal, assigns the goals to the arm retarget chains,
and adds the retargeter's IK Goals op with BlendToSource on the arms: the goal sits on the source (gameplay)
hand, and only the elbow bend adapts. Shoulders, proportions and the silhouette are untouched.

Run after Tools/GOTA_BuildParagonRigs.py (it rebuilds rigs from scratch):
  GOTA_BODIES=Steel,MinionLane UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Env GOTA_HAND_IK_BLEND (default 1.0): BlendToSource translation weight.
Env GOTA_BODY_SCALE (default 1.0): the hero/enemy BodyScale of these bodies. The visible body is a child of the
gameplay mesh scaled by BodyScale, so source-space goals would land BodyScale short; a Scale Source op
(factor 1/BodyScale) compensates.
Report: Saved/GOTA_AddHandIK.txt
"""
import os

import unreal

RIGS = "/Game/Heroes/Paragon/Rigs"
BODIES = [b.strip() for b in os.environ.get("GOTA_BODIES", "").split(",") if b.strip()]
BLEND = float(os.environ.get("GOTA_HAND_IK_BLEND", "1.0"))
BODY_SCALE = float(os.environ.get("GOTA_BODY_SCALE", "1.0"))
ARMS = [("LeftArm", "LeftHandIK", "upperarm_l", "hand_l"), ("RightArm", "RightHandIK", "upperarm_r", "hand_r")]
log = []


def P(*a):
    log.append(" ".join(str(x) for x in a))
    unreal.log("GOTA_AddHandIK: " + log[-1])


def patch_rig(body):
    rig = unreal.load_asset("%s/IK_GOTA_%s" % (RIGS, body))
    c = unreal.IKRigController.get_controller(rig)
    existing = {str(g.get_editor_property("goal_name")) for g in c.get_all_goals()}
    for chain, goal, root, hand in ARMS:
        if goal in existing:
            P(body, "goal exists", goal)
            continue
        c.add_new_goal(goal, hand)
        idx = c.add_solver("/Script/IKRig.IKRigLimbSolver")
        c.set_start_bone(root, idx)
        c.connect_goal_to_solver(goal, idx)
        c.set_retarget_chain_goal(chain, goal)
        P(body, "limb solver", idx, root, "->", hand, "goal", goal, "chain", chain, "goal now", c.get_retarget_chain_goal(chain))
    unreal.EditorAssetLibrary.save_loaded_asset(rig)


def patch_retargeter(body):
    rtg = unreal.load_asset("%s/RTG_GOTA_Manny_%s" % (RIGS, body))
    c = unreal.IKRetargeterController.get_controller(rtg)
    names = [str(c.get_op_name(i)) for i in range(c.get_num_retarget_ops())]
    idx = next((i for i in range(c.get_num_retarget_ops()) if _is_ik(c, i)), None)
    if idx is None:
        idx = c.add_retarget_op("/Script/IKRig.IKRetargetIKChainsOp")
        run_rig = names.index("Run IK Rig") if "Run IK Rig" in names else None
        if run_rig is not None:
            c.move_retarget_op_in_stack(idx, run_rig)
        idx = next(i for i in range(c.get_num_retarget_ops()) if _is_ik(c, i))
    if abs(BODY_SCALE - 1.0) > 1e-3:
        si = next((i for i in range(c.get_num_retarget_ops()) if str(c.get_op_name(i)).startswith("Scale Source")), None)
        if si is None:
            si = c.add_retarget_op("/Script/IKRig.IKRetargetScaleSourceOp")
            c.move_retarget_op_in_stack(si, 0)
            si = next(i for i in range(c.get_num_retarget_ops()) if str(c.get_op_name(i)).startswith("Scale Source"))
        sc = c.get_op_controller(si)
        ss = sc.get_settings() if hasattr(sc, "get_settings") else None
        if ss is not None:
            ss.set_editor_property("source_scale_factor", 1.0 / BODY_SCALE)
            sc.set_settings(ss)
        else:
            P(body, "scale source op has no settings controller:", type(sc))
        P(body, "scale source factor", 1.0 / BODY_SCALE)
        idx = next(i for i in range(c.get_num_retarget_ops()) if _is_ik(c, i))
    c.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, unreal.load_asset("%s/IK_GOTA_%s" % (RIGS, body)))
    c.run_op_initial_setup(idx)
    ctrl = c.get_op_controller(idx)
    settings = ctrl.get_settings()
    chains = list(settings.get_editor_property("chains_to_retarget"))
    out = []
    for ch in chains:
        name = str(ch.get_editor_property("target_chain_name"))
        arm = name in ("LeftArm", "RightArm")
        ch.set_editor_property("enable_ik", arm)
        if arm:
            ch.set_editor_property("blend_to_source", BLEND)
            ch.set_editor_property("blend_to_source_translation", 1.0)
            ch.set_editor_property("blend_to_source_rotation", 0.0)
        out.append(ch)
    settings.set_editor_property("chains_to_retarget", out)
    ctrl.set_settings(settings)
    P(body, "ops", [str(c.get_op_name(i)) for i in range(c.get_num_retarget_ops())],
      "IK chains", [(str(ch.get_editor_property("target_chain_name")), ch.get_editor_property("enable_ik"), ch.get_editor_property("blend_to_source")) for ch in out])
    unreal.EditorAssetLibrary.save_loaded_asset(rtg)


def _is_ik(c, i):
    try:
        return isinstance(c.get_op_controller(i), unreal.IKRetargetIKChainsController)
    except Exception:
        return False


for body in BODIES:
    try:
        patch_rig(body)
        patch_retargeter(body)
        P(body, "OK")
    except Exception as e:
        P(body, "FAIL", repr(e))
open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_AddHandIK.txt"), "w").write("\n".join(log))
