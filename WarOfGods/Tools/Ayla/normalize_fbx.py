"""Headless Blender: re-export Meshy's Ayla FBX files so Unreal imports one consistent unit and scale.

Why: Unreal drops the Blender "Armature" node and folds its transform into the pelvis bind pose but not into the
animation keys, so the takes played at 1/100 size. Each armature is baked to identity (rotation and scale applied, its
location keys scaled to match), converted to centimetres (scene unit 1 cm) and exported with no FBX scaling. Nothing is retargeted, renamed or cleaned up;
the bones, weights, bind pose and keys are otherwise untouched.

Usage: blender -b --factory-startup --python normalize_fbx.py -- <in.fbx> <out.fbx> <mesh|anim> [take name]
"""
import sys

import bpy

args = sys.argv[sys.argv.index("--") + 1:]
src, dst, kind = args[0], args[1], args[2]
take = args[3] if len(args) > 3 else None

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps, scene.render.fps_base = 30, 1.0
bpy.ops.import_scene.fbx(filepath=src, automatic_bone_orientation=False, use_anim=True)

arm = next(o for o in scene.objects if o.type == "ARMATURE")


def fcurves(action):
    if getattr(action, "fcurves", None):
        return list(action.fcurves)
    out = []
    for layer in getattr(action, "layers", []):
        for strip in layer.strips:
            for bag in strip.channelbags:
                out.extend(bag.fcurves)
    return out


# World scale of the armature, including any parent (Meshy's skin-less exports sit under a rotated "RootNode").
world_scale = arm.matrix_world.to_scale()
factor = world_scale.x
if abs(world_scale.x - world_scale.y) > 1e-4 or abs(world_scale.x - world_scale.z) > 1e-4:
    raise RuntimeError("non-uniform armature scale %s" % (tuple(world_scale),))
print("NORMALIZE %s: armature world scale %.4f" % (src.split("\\")[-1], factor))

# Unparent (keeping the world transform), then bake rotation and scale into the armature and its children.
bpy.ops.object.select_all(action="DESELECT")
for ob in scene.objects:
    if ob.type in ("ARMATURE", "MESH"):
        ob.select_set(True)
bpy.context.view_layer.objects.active = arm
if arm.parent:
    bpy.ops.object.parent_clear(type="CLEAR_KEEP_TRANSFORM")
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
for ob in list(scene.objects):
    if ob.type == "EMPTY" and not ob.children:
        bpy.data.objects.remove(ob)

# Pose-space location keys are in the old units: scale them with the rest pose.
if abs(factor - 1.0) > 1e-6:
    scaled = 0
    for act in bpy.data.actions:
        for fc in fcurves(act):
            if fc.data_path.endswith(".location"):
                for kp in fc.keyframe_points:
                    kp.co.y *= factor
                    kp.handle_left.y *= factor
                    kp.handle_right.y *= factor
                scaled += 1
    print("NORMALIZE scaled %d location curves by %.4f" % (scaled, factor))

# Centimetres: scale the armature (and the mesh with it) by 100, bake that in, scale the location keys to match, and
# set the scene unit to 1 cm so the FBX carries cm values and no node scale for Unreal to fold into the pelvis.
CM = 100.0
bpy.ops.object.select_all(action="DESELECT")
arm.scale = (CM, CM, CM)
bpy.context.view_layer.update()
arm.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
for ob in scene.objects:
    if ob.type == "MESH":
        bpy.ops.object.select_all(action="DESELECT")
        ob.select_set(True)
        bpy.context.view_layer.objects.active = ob
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for act in bpy.data.actions:
    for fc in fcurves(act):
        if fc.data_path.endswith(".location"):
            for kp in fc.keyframe_points:
                kp.co.y *= CM
                kp.handle_left.y *= CM
                kp.handle_right.y *= CM
scene.unit_settings.system = "METRIC"
scene.unit_settings.scale_length = 0.01
for ob in scene.objects:
    print("NORMALIZE after cm: %s scale=%s dims=%s" % (ob.name, tuple(round(v, 3) for v in ob.scale), tuple(round(v, 2) for v in ob.dimensions)))
print("NORMALIZE pelvis rest head %s" % (tuple(round(v, 2) for v in arm.data.bones["pelvis"].head_local),))

if take:
    for act in bpy.data.actions:
        act.name = take

bpy.ops.object.select_all(action="DESELECT")
for ob in scene.objects:
    if ob.type == "ARMATURE" or (kind == "mesh" and ob.type == "MESH"):
        ob.select_set(True)
bpy.ops.export_scene.fbx(filepath=dst, use_selection=True, object_types={"ARMATURE", "MESH"} if kind == "mesh" else {"ARMATURE"},
                         apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE", global_scale=1.0,
                         axis_forward="-Z", axis_up="Y", primary_bone_axis="Y", secondary_bone_axis="X",
                         add_leaf_bones=False, use_armature_deform_only=False, armature_nodetype="NULL",
                         bake_anim=True, bake_anim_use_all_actions=True, bake_anim_use_nla_strips=False,
                         bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0, bake_anim_step=1.0,
                         mesh_smooth_type="FACE", use_mesh_modifiers=False, path_mode="AUTO", embed_textures=False)
print("NORMALIZE wrote %s (%d actions)" % (dst, len(bpy.data.actions)))
