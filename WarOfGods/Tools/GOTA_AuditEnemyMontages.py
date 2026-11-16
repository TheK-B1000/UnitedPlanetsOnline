# Audit helper (editor Python): list every notify of the enemy attack montages (time, track, class, the
# PlayMontageNotify name that BPC_Attacks switches on) into Saved/GraphAudit/_enemy_montages.txt.
import os
import unreal

proj = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
out = os.path.join(proj, "Saved", "GraphAudit")
os.makedirs(out, exist_ok=True)
MONTAGES = [
    "/Game/Animations/Melee/Montage_Sword_SpinningAttack",
    "/Game/Animations/Melee/Montage_Melee_GroundSmash",
    "/Game/Animations/Melee/Montage_Sword_Slash",
    "/Game/Animations/Melee/Montage_Sword_Jump_Attack",
    "/Game/Animations/Melee/Montage_SwordCombo",
]
AL = unreal.AnimationLibrary
lines = []
for path in MONTAGES:
    m = unreal.EditorAssetLibrary.load_asset(path)
    if not m:
        lines.append("%s: not found" % path)
        continue
    lines.append("%s length=%.3f tracks=%s" % (path, m.get_play_length(), list(AL.get_animation_notify_track_names(m))))
    for track in AL.get_animation_notify_track_names(m):
        for ev in AL.get_animation_notify_events_for_track(m, track):
            notify = ev.get_editor_property("notify")
            state = ev.get_editor_property("notify_state_class")
            obj = notify or state
            name = ""
            if obj:
                try:
                    name = obj.get_editor_property("notify_name")
                except Exception:
                    name = "-"
            lines.append("  track=%s t=%.3f class=%s notifyName=%s" % (
                track, AL.get_animation_notify_event_trigger_time(ev) if hasattr(AL, "get_animation_notify_event_trigger_time") else -1.0,
                obj.get_class().get_name() if obj else "None", name))
with open(os.path.join(out, "_enemy_montages.txt"), "w") as f:
    f.write("\n".join(lines))
