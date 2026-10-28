import unreal
ok = True
checks = []
# Classes
for name in ("ArenaGrayboxBuilder", "ArenaGameMode", "GOTAStoryGameMode", "CombatTimingComponent", "CombatComponent"):
    try:
        cls = getattr(unreal, name, None)
        checks.append(f"class {name}: {'OK' if cls else 'MISSING'}")
        if not cls:
            ok = False
    except Exception as e:
        checks.append(f"class {name}: ERR {e}")
        ok = False
# Soft combat config if any
cfg = "/Game/Combat/DA_CombatTimingConfig"
if unreal.EditorAssetLibrary.does_asset_exist(cfg):
    checks.append(f"CombatTimingConfig: OK {cfg}")
else:
    checks.append("CombatTimingConfig: optional DA absent (C++ defaults OK)")
# Story map
m = "/Game/Story/Maps/L_GOTA_Story_Prologue"
checks.append(f"Story map: {'OK' if unreal.EditorAssetLibrary.does_asset_exist(m) else 'MISSING (fallback ThirdPersonMap)'}")
# Player BP
p = "/Game/Player/BP_Player"
checks.append(f"BP_Player: {'OK' if unreal.EditorAssetLibrary.does_asset_exist(p) else 'MISS'}")
if not unreal.EditorAssetLibrary.does_asset_exist(p):
    ok = False
e = "/Game/Enemies/MeleeEnemy/BP_Enemy_Melee"
checks.append(f"BP_Enemy_Melee: {'OK' if unreal.EditorAssetLibrary.does_asset_exist(e) else 'MISS'}")
unreal.log("=== GOTA Smoke Classes ===")
for c in checks:
    unreal.log(c)
unreal.log("SMOKE: PASS" if ok else "SMOKE: FAIL")
path = unreal.Paths.project_dir() + "Saved/GOTA_SmokeClasses.txt"
with open(path, "w", encoding="utf-8") as f:
    f.write("\n".join(checks + ["SMOKE: PASS" if ok else "SMOKE: FAIL"]) + "\n")
