# Arena Implementation Status (2026-10-31)

## Summary

| Arena | Map | Actors | Navmesh (baked) | Standalone load + Fight AI round | Gameplay screenshots | Perf measured | Status |
|---|---|---|---|---|---|---|---|
| Titan's Quarry | `/Game/Arenas/Maps/L_Arena_TitansQuarry` | 1,183 | ✅ | ✅ (champion paths and fights) | ✅ 80–87 | ✅ | **Environment built**; quality benchmark |
| Ashfall Crucible | `L_Arena_AshfallCrucible` | 645 | ✅ | ✅ | ✅ | ✅ | Environment built |
| Frostbound Citadel | `L_Arena_FrostboundCitadel` | 668 | ✅ | ✅ | ✅ | ✅ | Environment built |
| Verdant Sanctuary | `L_Arena_VerdantSanctuary` | 659 | ✅ | ✅ | ✅ | ✅ | Environment built |
| Sunscar Expanse | `L_Arena_SunscarExpanse` | 413 | ✅ | ✅ | ✅ | ✅ | Environment built |
| Stormbreak Coast | `L_Arena_StormbreakCoast` | ~395 | ✅ | ✅ | ✅ | ✅ | Environment built |
| Shattered Dominion | `L_Arena_ShatteredDominion` | 932 | ✅ | ✅ | ✅ | ✅ | Environment built |

**Validation scope.**
- **Engine:** "Standalone" = `UnrealEditor.exe -game`. No PIE session and no packaged build were run.
- **Round coverage:** every arena completed at least one full duel round with the AI champion (6 of 7 rounds won by the test bot or the champion by KO), so starts, collision, navigation and combat work on the authored floors.
- **Horde:** spawn points are authored, but a Horde wave was **not** run on the new maps.

## Per arena

Every arena has the same gameplay layer:
- Floor disc with top at Z = 0, plus a curb (18 cm step) and an apron.
- `InvisibleWall` boundary.
- Starts 0–3 at 9.5 m (Dominion 12.5 m).
- 6 Horde spawns.
- Nav bounds of 52×52×14 m (Dominion 62 m).

| Arena | Lighting | Atmosphere / FX | Audio | Outstanding issues |
|---|---|---|---|---|
| Quarry | Sun 41°, warm (1.0, 0.84, 0.64), 10 lux; sky 2.4; warm fog; exposure +0.75 | Dust, gantry dust, refinery smoke | Wind ×3, machinery | Kitbash colossus lacks sculpted detail; no decals |
| Ashfall | Ash sun 48°, 4.5 lux; dim red fog; exposure −0.6 | Embers ×4 (outside floor), ash, steam, braziers, volcano plumes | Lava ×4, storm wind, fire | Lava is an emissive slab (no flow or heat haze); still warm-heavy grade |
| Frostbound | Cold sun 30°, 8 lux; blue fog; bloom 0.35 | Ice motes | Frost wind, ice creak ×2 | Glacier masses are textured boxes; no snowfall system |
| Verdant | Sun 52°, green-gold; strong volumetrics | Fireflies, pollen, waterfall drops + mist | Birds, leaves, waterfall | Waterfall/stream use StarterContent water on boxes; highest GPU cost |
| Sunscar | Sun 38°, 11 lux; light dust | Dust, sand drift | Desert wind, sand | Dunes are mesh mounds (no landscape); colossi are blocky |
| Stormbreak | Storm sun 24°, 3.5 lux; heavy grey-blue fog; desaturated | Sea spray, cosmetic lightning (`AGOTALightningFlash`) | Surf ×4, rain, storm wind, thunder | No rain particles; ocean is the StarterContent material on a plane |
| Dominion | Sun 40°, neutral; regional local lights | Regional particles | 6 regional zones + centre | Regions are dressing wedges; the central ring is kitbash |

## Code and tools delivered

**C++**
- `AArenaGameMode::IsAuthoredArenaWorld` / `GetAuthoredArenaMap`: the graybox is skipped on authored maps.
- Duel start and champion logic for authored maps.
- Menu routes Fight AI to the authored map of the selected arena. Arenas with an authored map are now launchable.
- `UGOTAEditorUIBuilderLibrary::BuildEditorNavigation` (lock release + rebuild) and `SpawnBoxVolume` (brush volumes from Python).
- `AGOTALightningFlash`: cosmetic, local lightning.
- Test commands: `gota.Test.Cam` (composition camera), `gota.Test.Perf` (perf probe), `gota.Test.Portrait`; the script runner routes viewport commands (`HighResShot`).

**Editor tools**
- `Tools/GOTA_ArenaAssetInventory.py`
- `Tools/GOTA_ArenaMaterials.py`
- `Tools/GOTA_ArenaKit.py`
- `Tools/GOTA_BuildArenaLevels.py`
- `Tools/GOTA_BuildArenaAudio.py`
- `Tools/Tests/arena_<key>_shots.txt`

**Assets**
- 7 levels.
- 2 master materials and 23 instances.
- 4 Nanite primitives.
- 13 ambience loops and 1 thunder one-shot.

## Remaining work to reach production quality (in order)

1. **Architecture meshes.** Replace kitbash primitives at the landmarks (colossi, gates, column capitals, crucible, beacon) with modelled or Megascans architectural meshes; keep the gameplay layer unchanged.
2. **VT master material.** Add a virtual-texture twin of `M_GOTA_ArenaSurface` to unlock the Icelandic 8K surfaces (obsidian, moss, cracked rock) and `Megascans/Rock`.
3. **Landscape and blending.** Add a Landscape (or a mesh-terrain blend material) under each apron, plus decals at structure bases.
4. **Water and weather.** Use the Water plugin or a proper waterfall material and spline river (Verdant), plus Niagara snowfall (Frostbound), rain (Stormbreak) and lava flow and heat haze (Ashfall).
5. **Validation.**
   - Run a Horde wave per arena.
   - Run listen server + 3 clients on two arenas.
   - Take PIE screenshots.
   - Package and verify that the maps are listed in `MapsToCook` (not yet added).
   - Capture Unreal Insights traces at Medium and Low scalability.
6. **Arena preview art.** Use the menu's arena preview art from these screenshots.
