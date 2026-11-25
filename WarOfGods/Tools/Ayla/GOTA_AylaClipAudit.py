"""Read-only audit of Ayla's animation sequences: how far the pelvis (her root bone) travels and turns in each clip.

Ayla's skeleton has no root bone, so any travel baked into a clip moves the pelvis, and the visible body would drift
away from the gameplay capsule. Reports, per sequence: horizontal pelvis offset at the first and last frame and its
maximum distance from the bind position, vertical range, and pelvis yaw change from first to last frame.

Run: UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this> -unattended -nosplash -nullrhi
Report: Saved/GOTA_AylaClipAudit.json (and a table in the log)
"""
import json
import math
import os

import unreal

ANIM_DIR = "/Game/Characters/Ayla/Animations"
EAL = unreal.EditorAssetLibrary
skel = EAL.load_asset("/Game/Characters/Ayla/Mesh/SKEL_Ayla")
bind = unreal.AnimPoseExtensions.get_bone_pose(unreal.AnimPoseExtensions.get_reference_pose(skel), "pelvis", unreal.AnimPoseSpaces.LOCAL)
bx, by = bind.translation.x, bind.translation.y
rows = []
for path in sorted(EAL.list_assets(ANIM_DIR, recursive=True)):
    seq = EAL.load_asset(path)
    if not isinstance(seq, unreal.AnimSequence):
        continue
    n = unreal.AnimationLibrary.get_num_frames(seq)
    xs, ys, zs, yaws = [], [], [], []
    for f in range(n + 1):
        t = unreal.AnimationLibrary.get_bone_pose_for_frame(seq, "pelvis", min(f, n), False)
        xs.append(t.translation.x - bx)
        ys.append(t.translation.y - by)
        zs.append(t.translation.z)
        # yaw of the pelvis's forward axis projected on the ground (component space; the pelvis is the root)
        fwd = t.rotation.rotate_vector(unreal.Vector(0, 1, 0))
        yaws.append(math.degrees(math.atan2(fwd.y, fwd.x)))
    dist = [math.hypot(x, y) for x, y in zip(xs, ys)]
    dyaw = (yaws[-1] - yaws[0] + 180) % 360 - 180
    turn_max = max(abs((y - yaws[0] + 180) % 360 - 180) for y in yaws)
    rows.append({"name": seq.get_name(), "seconds": round(seq.get_play_length(), 2),
                 "start_xy": [round(xs[0], 1), round(ys[0], 1)], "end_xy": [round(xs[-1], 1), round(ys[-1], 1)],
                 "max_dist": round(max(dist), 1), "net_travel": round(math.hypot(xs[-1] - xs[0], ys[-1] - ys[0]), 1),
                 "z_range": [round(min(zs), 1), round(max(zs), 1)], "yaw_change": round(dyaw, 1), "yaw_max": round(turn_max, 1)})
out = os.path.join(unreal.Paths.project_saved_dir(), "GOTA_AylaClipAudit.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump(rows, f, indent=1)
for r in rows:
    unreal.log("CLIP %-34s %5.2fs start=%-14s end=%-14s max=%6.1f net=%6.1f z=%s yaw=%6.1f yawmax=%6.1f" % (
        r["name"], r["seconds"], r["start_xy"], r["end_xy"], r["max_dist"], r["net_travel"], r["z_range"], r["yaw_change"], r["yaw_max"]))
