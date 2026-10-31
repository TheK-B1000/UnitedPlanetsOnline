# Arena Environment Pipeline

Authored, saved Unreal levels replace the runtime graybox for all seven arenas. Everything is reproducible from scripts. A rebuild produces identical actors, collision and navmesh, because all scattering uses fixed seeds.

## Order of operations (editor scripts run in the full editor)

```
1  python Tools/GOTA_BuildArenaAudio.py                      # original ambience loops + thunder one-shot (numpy, offline)
2  UnrealEditor-Cmd WarOfGods.uproject -ExecutePythonScript=Tools/GOTA_ArenaMaterials.py
       # M_GOTA_ArenaSurface (world-aligned triplanar, macro anti-tiling) + M_GOTA_ArenaEmissive (panning)
       # 19 surface MIs + 4 emissive MIs in /Game/Arenas/Shared/Materials
       # GOTA_REBUILD_MASTERS=1 recreates the two masters; otherwise only the instances update
3  GOTA_ARENAS=Quarry,Ashfall,Frostbound,Verdant,Sunscar,Stormbreak,Dominion \
   UnrealEditor-Cmd WarOfGods.uproject -ExecutePythonScript=Tools/GOTA_BuildArenaLevels.py
       # imports ambience, (re)builds each level, bakes the navmesh, saves
4  UnrealEditor WarOfGods.uproject "/Game/Arenas/Maps/L_Arena_<Name>?game=/Script/WarOfGods.DuelGameMode?...?Opponent=AI" -game \
       -ExecCmds="gota.Test.Run Tools/Tests/arena_<key>_shots.txt"   # composition + gameplay screenshots
```

Set `GOTA_QUIT_AFTER_BUILD=1` for unattended runs. Results are written to `Saved/GOTA_ArenaMaterials.txt` and `Saved/GOTA_BuildArenaLevels.txt` (actor count, navmesh, missing assets).

`Tools/GOTA_ArenaAssetInventory.py` writes `Saved/GOTA_ArenaAssetInventory.json`: 1,075 meshes with real sizes, Nanite flags, surfaces, FX and sounds.

## Three layers (actor folders)

| Layer | Folder | Collision | Contents |
|---|---|---|---|
| 1 Gameplay | `Gameplay/*` | `BlockAll` floor and curb; `InvisibleWall` boundary (blocks pawns, not the camera) | Floor disc (top Z = 0), apron, curb (18 cm step), boundary ring, `AArenaPlayerStart` slots 0–3, 6 `AHordeSpawnPoint`s, `NavMeshBoundsVolume`, authored-arena marker, composition-camera markers |
| 2 Dressing | `Dressing/*` | `NoCollision` (everything is outside the boundary) | Architecture kit pieces, Megascans cliffs and rocks, foliage, landmarks, horizon meshes |
| 3 Atmosphere | `Atmosphere/*` | n/a | Sun, sky atmosphere, sky light, height fog (volumetric), volumetric clouds, unbound post process, particles, ambient sounds (attenuated), local lights, lightning |

Dressing never decides gameplay: the fighting floor is a flat authored disc, and all decoration stands beyond the invisible boundary.

## Runtime integration (C++)

| Piece | Behaviour |
|---|---|
| `AArenaGameMode::IsAuthoredArenaWorld` | True if any actor carries the tag `GOTA_AuthoredArena`; then the runtime graybox is **skipped** |
| `AArenaGameMode::GetAuthoredArenaMap(EArenaId)` | `/Game/Arenas/Maps/L_Arena_<Name>` if the package exists |
| Menu (`UGOTAMenuSubsystem::RequestLaunchArena`) | Travels to the authored map when one exists; otherwise falls back to `ThirdPersonMap` plus graybox |
| Duel | On authored maps, slots 0 and 1 face each other across the floor; the champion spawns at its own start; no runtime nav bounds are added |
| Horde | Uses the authored `AHordeSpawnPoint`s and the existing bounds |

**Navigation.** The navmesh is baked in the editor by `UGOTAEditorUIBuilderLibrary::BuildEditorNavigation`.
- The helper releases the unattended editor's nav build locks before building.
- A brand-new level gets its nav system only on reload, so `finalize` reopens the level and builds again.
- `RuntimeGeneration=Dynamic` (project setting) still covers runtime changes.
- The navmesh is a deterministic static surface, so the old "runtime graybox floors don't generate nav" problem no longer applies.

## Kit (`Tools/GOTA_ArenaKit.py`)

| Function | Builds |
|---|---|
| `box`, `cyl`, `sphere`, `mesh` | Primitives use **Nanite copies** of the engine shapes (`/Game/Arenas/Shared/Meshes/SM_GOTA_*`), which avoids virtual-shadow-map overflow from big non-Nanite primitives. All primitives use world-aligned materials, so scaling never stretches textures. |
| `column`, `arch`, `gate`, `stairs`, `beam`, `lattice_tower`, `chain` | Architecture kitbash |
| `flagstone_floor` | Radial paving (≈170 slabs) |
| `scatter` | Seeded, with an avoid-predicate |
| `ring_of_boxes` | Walls, benches, rings |
| `gameplay_floor`, `player_starts`, `horde_spawns`, `invisible_ring`, `nav_bounds`, `marker`, `camera_point` | Gameplay layer |
| `lighting`, `post_process`, `fx`, `ambient`, `point_light` | Atmosphere |
| `import_ambience`, `amb(name)` | Audio |
| `finalize` | Marker + nav bounds + nav bake + save (with reload retry) |

## Materials

**Master.** `M_GOTA_ArenaSurface` has these parameters:

| Parameter | Use |
|---|---|
| `Albedo`, `Normal`, `Roughness` | Texture objects |
| `TileSize` | Texture tiling (cm) |
| `MacroScale`, `MacroStrength` | Second large-scale sample that breaks tiling |
| `Tint` | Colour multiplier |
| `RoughnessChannel` | Dot-product mask; set it to (0,0,0,0) to use a constant roughness |
| `RoughnessScale`, `RoughnessBias` | Roughness adjustment |
| `Metallic`, `ProjectionContrast` | Metal value; triplanar blend sharpness |

**Virtual textures render black in it.**
- The Megascans Icelandic 8K surfaces and `Megascans/Rock` are virtual-texture assets, so they render black in this non-VT master.
- Families therefore use non-VT sets: Megascans quarry/sandy/rippled, Iceland snow/forest/moss, StarterContent cut stone/slate/granite/basalt/metals.
- A VT twin master is listed as follow-up work.
- Megascans 3D assets keep their own (working) material instances.

## Audio

Ambience loops are original and procedurally generated by `GOTA_BuildArenaAudio.py`; they are not third-party sounds:

| Group | Loops |
|---|---|
| Wind | Quarry, desert, frost, storm |
| Quarry | Machinery |
| Fire and water | Lava, waterfall, surf, rain, thunder |
| Ground and plants | Ice creak, forest leaves, sand drift |

There is also a one-shot thunder. StarterContent wind, birds and fire cues are used where suitable. Every source is an `AmbientSound` with attenuation (no global 2D ambience), and combat sounds stay on top.

## Adding or changing an arena

1. Edit its recipe in `Tools/GOTA_BuildArenaLevels.py` (layers in order: floor → dressing → atmosphere → cameras → `finalize`).
2. Rebuild just that arena: `GOTA_ARENAS=<Key>`.
3. Run its shots script and compare against `Docs/GOTA_ARENA_VISUAL_QA.md`.
4. Seeds are fixed. Change a seed only on purpose, since it moves every scattered prop.
