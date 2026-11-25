"""Headless Blender: report the skeleton, meshes and animation takes of each Meshy FBX given after '--'.
Read-only: imports into an empty scene and never saves. Writes one JSON report per run to the path in AYLA_REPORT."""
import bpy, json, os, sys

files = sys.argv[sys.argv.index("--") + 1:]
report = {}
for path in files:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    before = set(bpy.data.actions)
    bpy.ops.import_scene.fbx(filepath=path, automatic_bone_orientation=False, use_anim=True)
    scene = bpy.context.scene
    r = {"fps": scene.render.fps / scene.render.fps_base, "armatures": [], "meshes": [], "actions": []}
    for ob in bpy.context.scene.objects:
        if ob.type == "ARMATURE":
            bones = ob.data.bones
            roots = [b.name for b in bones if b.parent is None]
            r["armatures"].append({"name": ob.name, "bones": len(bones), "roots": roots,
                                   "bone_names": [b.name for b in bones],
                                   "scale": list(ob.scale), "rot": list(ob.rotation_euler)})
        elif ob.type == "MESH":
            me = ob.data
            tris = sum(len(p.vertices) - 2 for p in me.polygons)
            quads = sum(1 for p in me.polygons if len(p.vertices) == 4)
            dims = list(ob.dimensions)
            r["meshes"].append({"name": ob.name, "verts": len(me.vertices), "faces": len(me.polygons), "quads": quads,
                                "tris": tris, "dims": dims, "groups": len(ob.vertex_groups),
                                "materials": [m.name for m in me.materials if m],
                                "parent": ob.parent.name if ob.parent else None})
    for act in bpy.data.actions:
        fr = act.frame_range
        r["actions"].append({"name": act.name, "start": fr[0], "end": fr[1], "frames": fr[1] - fr[0] + 1,
                             "seconds": (fr[1] - fr[0]) / max(r["fps"], 1)})
    report[os.path.basename(path)] = r
with open(os.environ["AYLA_REPORT"], "w", encoding="utf-8") as f:
    json.dump(report, f, indent=1)
