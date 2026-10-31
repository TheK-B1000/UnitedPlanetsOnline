"""Makes installed third-party Blueprints compile (and therefore cook) under UE 5.8.

ParagonRampage's RampagePlayerCharacter calls the UE4 VR node ResetOrientationAndPosition, which no
longer exists; the dead node breaks compilation and the Win64 cook. This removes only call nodes whose
target function is gone (UGOTAEditorUIBuilderLibrary::RemoveDeadFunctionCalls), compiles and saves.
Safe to re-run: a clean Blueprint reports 0 removed.

Run: UnrealEditor-Cmd <uproject> -ExecutePythonScript=<this> -unattended -nop4 -nosplash
Report: Saved/GOTA_FixThirdPartyBlueprints.txt
"""
import os

import unreal

FIXES = [
    ("/Game/ParagonRampage/Characters/Heroes/Rampage/RampagePlayerCharacter", "ResetOrientationAndPosition"),
]
log = []
for path, fn in FIXES:
    bp = unreal.EditorAssetLibrary.load_asset(path)
    if not bp:
        log.append("%s: not installed (skipped)" % path)
        continue
    n = unreal.GOTAEditorUIBuilderLibrary.remove_dead_function_calls(bp, fn)
    log.append("%s: removed %d dead '%s' node(s)" % (path, n, fn))
open(os.path.join(unreal.Paths.project_saved_dir(), "GOTA_FixThirdPartyBlueprints.txt"), "w").write("\n".join(log))
