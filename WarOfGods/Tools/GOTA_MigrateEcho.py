"""Migrates Windwalker Echo's VISUAL assets (body mesh + hair-card mesh and their dependencies) from
the Windwalker Echo sample into WarOfGods with Unreal's migration (dependencies included, package
paths preserved under /Game/EchoContent/Characters/Echo). Skips maps, gameplay blueprints, input,
LiveLink/ARKit face rigs and groom strands (HairStrands plugin not required).

Run INSIDE the Echo project (UE 5.8 loads the 5.2 assets; nothing is saved back to the source):
  UnrealEditor-Cmd B:/Unreal/WindwalkerEcho/WindwalkerEcho.uproject -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: <WarOfGods>/Saved/GOTA_MigrateEcho.txt
"""
import os

import unreal

DEST_PROJECT = "B:/Unreal/UnitedPlanetsOnline/WarOfGods"
ROOTS = ["/Game/EchoContent/Characters/Echo/Meshes/Echo", "/Game/EchoContent/Characters/Echo/Meshes/Echo_Hair"]
SKIP_CLASSES = {"GroomAsset", "GroomBindingAsset", "World"}
log = []


def P(*a):
    log.append(" ".join(str(x) for x in a))
    unreal.log("GOTA_MigrateEcho: " + log[-1])


ar = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
seen, stack = set(), list(ROOTS)
while stack:
    p = stack.pop()
    if p in seen or not p.startswith("/Game"):
        continue
    assets = ar.get_assets_by_package_name(p)
    if assets and str(assets[0].asset_class_path.asset_name) in SKIP_CLASSES:
        P("skip", p, assets[0].asset_class_path.asset_name)
        continue
    seen.add(p)
    for d in ar.get_dependencies(p, opts) or []:
        stack.append(str(d))
classes = {}
for p in seen:
    a = ar.get_assets_by_package_name(p)
    c = str(a[0].asset_class_path.asset_name) if a else "?"
    classes[c] = classes.get(c, 0) + 1
P("packages to migrate:", len(seen), sorted(classes.items()))

src_content = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir())
dst_content = os.path.join(DEST_PROJECT, "Content")
migrated = False
try:
    # Unreal's own Migrate (Asset Actions > Migrate), unattended: no prompts, never overwrite.
    mo = unreal.MigrationOptions()
    for k, v in (("prompt", False), ("ignore_dependencies", True), ("assets_to_overwrite", None)):
        try:
            mo.set_editor_property(k, v)
        except Exception:
            pass
    unreal.AssetToolsHelpers.get_asset_tools().migrate_packages([unreal.Name(p) for p in sorted(seen)], dst_content, mo)
    migrated = os.path.exists(os.path.join(dst_content, "EchoContent/Characters/Echo/Meshes/Echo.uasset"))
    P("AssetTools.migrate_packages done:", migrated)
except Exception as e:
    P("migrate_packages unavailable/failed:", e)

# Fallback: the same file copy Migrate performs (same relative path under Content), never overwriting.
copied = skipped = 0
import shutil
for p in ([] if migrated else sorted(seen)):
    rel = p[len("/Game/"):]
    for ext in (".uasset", ".umap", ".uexp", ".ubulk"):
        s = os.path.join(src_content, rel + ext)
        if os.path.exists(s):
            d = os.path.join(dst_content, rel + ext)
            if os.path.exists(d):
                skipped += 1
                continue
            os.makedirs(os.path.dirname(d), exist_ok=True)
            shutil.copy2(s, d)
            copied += 1
P("files copied:", copied, "already present:", skipped)
open(os.path.join(DEST_PROJECT, "Saved", "GOTA_MigrateEcho.txt"), "w").write("\n".join(log))
