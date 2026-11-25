"""Makes in-place copies of Ayla's travelling clips. The originals are never modified.

Ayla's skeleton has no root bone, so travel baked into a clip moves the pelvis and the visible body would leave the
server's capsule (which moves only with character movement). Each copy keeps the pelvis's short-term sway, its height
and every rotation, and removes only the slow horizontal travel: new XY = bind XY + (XY - 1 s moving average of XY).

Jumps (VERTICAL) instead keep the take-off crouch and landing dip but drop the rise above standing height: in the air the
capsule itself carries the body up, so the clip's own rise would double the jump.

Output: /Game/Characters/Ayla/Animations/InPlace/AS_Ayla_<clip>_InPlace
Run: UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this> -unattended -nosplash -nullrhi
"""
import os

import unreal

CLIPS = ["Triple_Combo_Attack", "Double_Combo_Attack", "Hit_Reaction", "Quick_Step_and_Spin_Dodge", "Basic_Jump"]
SRC = "/Game/Characters/Ayla/Animations/AS_Ayla_"
DST = "/Game/Characters/Ayla/Animations/InPlace/AS_Ayla_"
VERTICAL = ["Regular_Jump"]
WINDOW = 15  # frames either side at 30 fps: a 1 s average, so steps and lunges go but sway stays
EAL = unreal.EditorAssetLibrary

skel = EAL.load_asset("/Game/Characters/Ayla/Mesh/SKEL_Ayla")
bind = unreal.AnimPoseExtensions.get_bone_pose(unreal.AnimPoseExtensions.get_reference_pose(skel), "pelvis", unreal.AnimPoseSpaces.LOCAL)
for clip in CLIPS + VERTICAL:
    src, dst = SRC + clip, DST + clip + "_InPlace"
    if EAL.does_asset_exist(dst):
        EAL.delete_asset(dst)  # derived asset, rebuilt from the untouched original every run
    seq = EAL.duplicate_asset(src, dst)
    n = unreal.AnimationLibrary.get_num_frames(seq)
    poses = [unreal.AnimationLibrary.get_bone_pose_for_frame(seq, "pelvis", f, False) for f in range(n + 1)]
    xs = [p.translation.x for p in poses]
    ys = [p.translation.y for p in poses]

    def avg(v, i):
        lo, hi = max(0, i - WINDOW), min(len(v) - 1, i + WINDOW)
        return sum(v[lo:hi + 1]) / (hi - lo + 1)

    pos = []
    stand = poses[0].translation.z
    for i, p in enumerate(poses):
        if clip in VERTICAL:
            pos.append(unreal.Vector(p.translation.x, p.translation.y, min(p.translation.z, stand)))
        else:
            pos.append(unreal.Vector(bind.translation.x + xs[i] - avg(xs, i), bind.translation.y + ys[i] - avg(ys, i), p.translation.z))
    rot = [p.rotation for p in poses]
    scl = [p.scale3d for p in poses]
    ctrl = seq.get_editor_property("controller")
    ctrl.open_bracket(unreal.Text("Ayla in-place pelvis"), False)
    ctrl.set_bone_track_keys("pelvis", pos, rot, scl, False)
    ctrl.close_bracket(False)
    EAL.save_asset(dst, only_if_is_dirty=False)
    unreal.log("INPLACE %s: %d keys, travel removed (was start %.0f,%.0f end %.0f,%.0f cm from bind)" % (
        clip, len(pos), xs[0] - bind.translation.x, ys[0] - bind.translation.y, xs[-1] - bind.translation.x, ys[-1] - bind.translation.y))
