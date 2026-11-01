"""Fixes installed third-party content so it compiles, cooks and renders under UE 5.8. Safe to re-run.

1. ParagonRampage's RampagePlayerCharacter calls the UE4 VR node ResetOrientationAndPosition, which no
   longer exists; the dead node breaks compilation and the Win64 cook. Removes only call nodes whose
   target function is gone (UGOTAEditorUIBuilderLibrary::RemoveDeadFunctionCalls), compiles and saves.
2. Windwalker Echo: every Echo texture is a virtual texture except the six Costume3 ones, but the cloth
   materials (M_Echo_ClothSimple / M_Echo_ClothGold) sample them as virtual. The editor tolerates the
   mismatch; the cooked game renders the pants, leather and buckle pink. Marks those textures virtual
   like the rest of the set.

Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_FixThirdPartyBlueprints.txt
"""
import os

import unreal

EAL = unreal.EditorAssetLibrary
FIXES = [
    ("/Game/ParagonRampage/Characters/Heroes/Rampage/RampagePlayerCharacter", "ResetOrientationAndPosition"),
]
ECHO_TEX = "/Game/EchoContent/Characters/Echo/Textures/Costume/"
ECHO_VIRTUAL = ["T_echo_Costume3_AO", "T_echo_Costume3_BN", "T_echo_Costume3_D", "T_echo_Costume3_Gold_ORM",
                "T_echo_Costume3_N", "T_echo_Costume3_ORM"]

log = []
for path, fn in FIXES:
    bp = EAL.load_asset(path)
    if not bp:
        log.append("%s: not installed (skipped)" % path)
        continue
    n = unreal.GOTAEditorUIBuilderLibrary.remove_dead_function_calls(bp, fn)
    log.append("%s: removed %d dead '%s' node(s)" % (path, n, fn))

for name in ECHO_VIRTUAL:
    path = ECHO_TEX + name
    if not EAL.does_asset_exist(path):
        found = [p for p in EAL.list_assets("/Game/EchoContent", recursive=True) if p.split(".")[-1] == name]
        path = found[0].split(".")[0] if found else None
    tex = EAL.load_asset(path) if path else None
    if not tex:
        log.append("%s: not installed (skipped)" % name)
        continue
    if tex.get_editor_property("virtual_texture_streaming"):
        log.append("%s: already virtual" % name)
        continue
    tex.set_editor_property("virtual_texture_streaming", True)
    EAL.save_asset(path, only_if_is_dirty=False)
    log.append("%s: now virtual (matches its material samplers)" % name)

open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_FixThirdPartyBlueprints.txt"), "w").write("\n".join(log))
