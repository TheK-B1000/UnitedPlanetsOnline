"""Builds /Game/Characters/Ayla/Test/L_Ayla_AnimTest: Ayla in her bind pose plus one copy per test animation, side by side,
on a floor with a sun and sky light. Isolated: nothing outside /Game/Characters/Ayla/Test is written.

Run: UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this> -unattended -nosplash -nullrhi
Copies stand in a grid facing -X (towards the PlayerStart), 6 per row, 170 cm apart, rows 260 cm deeper. By default every
AS_Ayla_* sequence gets a copy; GOTA_AYLA_TEST_ANIMS (comma list) picks a subset. The layout is logged ("AYLA TEST <name>
row r col c") so screenshots can be cut per copy.
"""
import os

import unreal

LEVEL = "/Game/Characters/Ayla/Test/L_Ayla_AnimTest"
ANIMS = [a.strip() for a in os.environ.get("GOTA_AYLA_TEST_ANIMS", "").split(",") if a.strip()] or sorted(
    p.split("/")[-1].split(".")[0] for p in unreal.EditorAssetLibrary.list_assets("/Game/Characters/Ayla/Animations", recursive=False))
EAL = unreal.EditorAssetLibrary
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if EAL.does_asset_exist(LEVEL):
    EAL.delete_asset(LEVEL)   # this test level is ours alone and is rebuilt each run
les.new_level(LEVEL)

floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
floor.static_mesh_component.set_static_mesh(EAL.load_asset("/Engine/BasicShapes/Plane"))
floor.set_actor_scale3d(unreal.Vector(40, 40, 1))
floor.set_actor_label("Floor")
sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(0, -45, 135))
sun.set_actor_label("Sun")
sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300))
sky.light_component.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property("cubemap", EAL.load_asset("/Engine/MapTemplates/Sky/DaylightAmbientCubemap"))
sky.light_component.set_editor_property("intensity", 1.0)
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-600, 0, 100))

mesh = EAL.load_asset("/Game/Characters/Ayla/Mesh/SK_Ayla")
entries = [None] + ANIMS
PER_ROW, SPACING, DEPTH = 6, 170.0, 260.0
for i, name in enumerate(entries):
    row, col = divmod(i, PER_ROW)
    y = (col - (PER_ROW - 1) / 2.0) * SPACING
    x = row * DEPTH
    # Meshy's mesh faces +Y like Unreal's mannequins; yaw 90 turns it to face -X, the camera side.
    a = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(x, y, 0), unreal.Rotator(0, 0, 90))
    comp = a.skeletal_mesh_component
    comp.set_skinned_asset_and_update(mesh)
    if name:
        seq = EAL.load_asset("/Game/Characters/Ayla/Animations/" + name)
        comp.set_animation_mode(unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        comp.set_editor_property("animation_data", unreal.SingleAnimationPlayData(anim_to_play=seq, saved_looping=True, saved_playing=True))
    a.set_actor_label("Ayla_" + (name or "BindPose"))
    unreal.log("AYLA TEST %s row %d col %d at (%.0f, %.0f)" % (name or "BindPose", row, col, x, y))
les.save_current_level()
unreal.log("AYLA TEST level saved with %d copies" % len(entries))
