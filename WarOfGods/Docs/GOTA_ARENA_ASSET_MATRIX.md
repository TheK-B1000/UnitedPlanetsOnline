# Arena Asset Matrix

**Source.** The full inventory is `Saved/GOTA_ArenaAssetInventory.json` (`Tools/GOTA_ArenaAssetInventory.py`). It lists 1,075 environment meshes with real sizes and Nanite flags, plus surfaces, 117 FX systems and sounds.

**Asset policy.**
- Assets are loaded selectively per level; no pack is imported wholesale into a map.
- Pack assets are referenced, never modified. Our own derived assets live in `/Game/Arenas/Shared`.

## Shared (all arenas)

| Asset | Path | Notes |
|---|---|---|
| Surface master + 19 MIs | `/Game/Arenas/Shared/Materials/M_GOTA_ArenaSurface`, `MI_GOTA_*` | World-aligned triplanar; non-VT textures only |
| Emissive master + 4 MIs | `M_GOTA_ArenaEmissive`, `MI_GOTA_Lava/LavaCrack/UPGEnergy/GoldEnergy` | Panning pattern |
| Nanite primitives | `/Game/Arenas/Shared/Meshes/SM_GOTA_Cube/Cylinder/Sphere/Cone` | Duplicates of the engine shapes with Nanite on |
| Ambience | `/Game/Arenas/Shared/Audio/A_GOTA_Amb_*` (13 loops), `A_GOTA_SFX_Thunder` | Original procedural audio |
| Particles | StarterContent `P_Ambient_Dust`; Realistic Starter VFX `P_Smoke_A/C/D/F`, `P_Steam_A`, `P_Embers_A`, `P_Fire_Small`, `P_Fireflies`, `P_Water_Leak_A`, `P_Water_Dripping_A`, `P_Sparks_C` | Cascade; small scales, off the fighting floor |
| Lightning | `AGOTALightningFlash` (C++) | Cosmetic, local |

## Material families → textures

| Family | Textures (non-VT) |
|---|---|
| QuarryStone | Megascans `Quarry_Rock_Cliff_01` A/N/R |
| QuarryGravel | Megascans `Quarry_Ground_Gravel_02` |
| QuarryBlock | StarterContent `T_Brick_Cut_Stone` |
| Paving / TempleStone | StarterContent `T_Rock_Sandstone` (different tints) |
| Sandstone / Sand | Megascans `Sandy_Rock_01`, `Rippled_Sand_01` |
| Obsidian / Basalt | StarterContent `T_Rock_Basalt` (dark glossy / matte) |
| FrostStone / WetRock | StarterContent `T_Rock_Slate` (cool / dark wet) |
| CoastStone | StarterContent `T_Rock_Smooth_Granite` + slate normal |
| Snow / Ice | Iceland `T_snow_02`, `T_Snow` |
| MossStone / ForestGround | Iceland `T_MossyCreekStones`, `T_ForestGround` |
| Iron / RustIron / UPGAlloy | StarterContent `T_Metal_Steel`, `T_Metal_Rust`, `T_Metal_Aluminum` + `T_Tech_Panel_N` |

**Not usable in the current master:** Megascans Icelandic 8K surfaces, `Megascans/Rock`, `Megascans/ErodedGround`. All are virtual textures and render black through non-VT samplers. Fix: a VT twin master (follow-up).

## Per arena

| Arena | Landscape / rock | Architecture | Vegetation | Far scenery | FX | Audio |
|---|---|---|---|---|---|---|
| Titan's Quarry | `Geometry/SM_Quarry_Cliff_L_01–03`, `_M_04`, `SM_Quarry_RockPile_S_01–04`, `SM_Quarry_Ridge_S_01–02`, `SM_Quarry_Spire_L_01–02`; Megascans `Quarry_Cliff_08/10` | Kitbash: benches, columns, Titan colossus, gantry towers and truss, chains, bridge, gate, pylons, refinery stacks | — | PhotoR 360° `Canyon_a` | Dust ×10, gantry dust, refinery smoke | Wind ×2 (Starter), quarry wind, machinery |
| Ashfall Crucible | `Geometry/SM_Lava_Cliff_L_01–04`, `SM_Lava_RockMound_S_*`, `SM_Lava_TreeMound_M_04`; `IcelandicLavaSpire_01` | Kitbash fortress ring, towers, gate, crucible, bridges, braziers, obelisks | — | Iceland `SM_Mountain_01` ×4 (volcanoes, basalt MI) | Embers ×4, ash ×3, steam at falls, brazier fire, volcano plumes | Lava ×4, storm wind, Fire01 cue |
| Frostbound Citadel | WinterForest `SM_WinterRock_A–G`, `SM_SnowPile_A–C`, `SM_SnowMountain_A` | Kitbash walls, buttresses, towers, statues, glacier ice masses, frozen falls, bridge/arch | WinterForest `SM_WinterTree_A–F` (34) | Iceland `SM_Iceland_Mountain_02` ×5 (snow MI) | Ice motes ×12 | Frost wind, ice creak ×2 |
| Verdant Sanctuary | Megascans `Massive_Nordic_Coastal_Cliff_veqnci0db` (moss MI) | Kitbash colonnade, temple, statues, roots, vines, stream, waterfall (StarterContent `M_Water_Lake`) | EuropeanBeech `Forest_01–08`, `Sapling_01–04`; NorwayMaple `Forest_01–04`; Megascans `Fern_00`, `Basket_Grass_01` | PhotoR `Afghan_Mountain_b` | Fireflies ×8, pollen ×8, waterfall drops + mist | Birds (Starter), leaves, waterfall |
| Sunscar Expanse | Megascans `Massive_Canyon_Sandstone_Cliff_*` ×3, `Huge_Canyon_Sandstone_Butte_venvbgp`, `*_Hoodoo_*` ×3; `Geometry/Custom/SM_Sand_01/02` (dunes); `Dead_Tree_01` | Kitbash colonnade, gate, seated colossi, fortifications, buried ring-engine, distant spires | Dead trees | PhotoR `Desert` | Dust ×8, sand drift ×5 | Desert wind, sand drift |
| Stormbreak Coast | Megascans `Massive_Nordic_Coastal_Cliff_*` ×5 | Kitbash battlements, towers, beacon, bridge on arches, harbour, ships; ocean (StarterContent `M_Water_Ocean`) | — | Mainland cliff mass | Sea spray ×10, rain shafts ×6, lightning | Surf ×4, rain, storm wind, thunder one-shot |
| Shattered Dominion | Mix of the above per region | Central platform, six monument towers, six bridges, Sovereign Ring | Beech, maple, winter trees (regional) | PhotoR `Afghan_Mountain_a` | Dust, embers, spray (regional) | Six regional zones + centre |

## Licensing

- **Megascans** (Quixel): free for Unreal Engine projects.
- **Engine-content packs** (StarterContent, PhotoR Backgrounds, Iceland Environment, WinterForest, EuropeanBeech, NorwayMaple, Realistic Starter VFX, LavaField / Quarry Collections) were in the project before this milestone. They are used under their Fab / Marketplace licences for UE projects.
- **Generated audio** is original to this project.
- No assets were copied from outside the project.
