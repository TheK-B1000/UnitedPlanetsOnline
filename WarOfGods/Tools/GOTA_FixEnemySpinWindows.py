# RC12 content fix (editor Python): the enemy spinning sword attack's damage windows.
#
# Montage_Sword_SpinningAttack had 1 "Slash" (single target, t=1.22) and 8 "AOESlash" pulses (BPC_Attacks AOEDamage,
# radius 300) every 0.5 s from 1.5 to 5.0 s: a target that stayed in it took 9 hits (90 of 100 health) with no gap to
# react. This keeps the wind-up, the animation, the opening Slash, the AOE radius and the damage per hit, and spaces the
# AOE contacts 1.25 s apart (2.0, 3.25, 4.5): one spin can hit a target at most 4 times (40), and every contact is
# followed by time to step out, block or dodge. Only track "2" (the PlayMontageNotify damage events) is rewritten;
# the sword-trail particle track is untouched. Idempotent. Run with -run=pythonscript.
import os
import unreal

PATH = "/Game/Animations/Melee/Montage_Sword_SpinningAttack"
TRACK = "2"
WINDOWS = [(1.224, "Slash"), (2.0, "AOESlash"), (3.25, "AOESlash"), (4.5, "AOESlash")]

AL = unreal.AnimationLibrary
proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
report = os.path.join(proj, "Saved", "GraphAudit", "_spin_fix.txt")
m = unreal.EditorAssetLibrary.load_asset(PATH)
lines = []


def damage_names():
    names = []
    for ev in AL.get_animation_notify_events_for_track(m, TRACK):
        n = ev.get_editor_property("notify")
        names.append(str(n.get_editor_property("notify_name")) if n else "-")
    return names


before = damage_names()
lines.append("before: %d damage notifies %s" % (len(before), before))
AL.remove_animation_notify_events_by_track(m, TRACK)
for t, name in WINDOWS:
    notify = AL.add_animation_notify_event(m, TRACK, t, unreal.AnimNotify_PlayMontageNotify)
    notify.set_editor_property("notify_name", name)
after = damage_names()
lines.append("after: %d damage notifies %s at %s" % (len(after), after, [t for t, _ in WINDOWS]))
ok = after == [n for _, n in WINDOWS]
if ok:
    unreal.EditorAssetLibrary.save_loaded_asset(m, only_if_is_dirty=False)
lines.append("saved=%d" % (1 if ok else 0))
with open(report, "w") as f:
    f.write("\n".join(lines))
