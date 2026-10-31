# Arena Environment Pass 2: replace prototype geometry, upgrade materials

**Goal.** Replace the most visible primitive landmarks with detailed architecture, fix the Megascans material gap, and keep gameplay collision, navigation and frame time intact.

## What was built

### 1. Procedural architecture library (`Tools/GOTA_BuildArchitecture.py`, Geometry Script, free)

The library holds 23 Nanite static meshes in `/Game/Arenas/Shared/Architecture/SM_GOTA_Arch_*`, all built in-editor from code: no purchased or downloaded assets. It uses these techniques:
- Revolved profiles: column bases, entasis shafts, capitals, statue robes, brazier, crucible, lantern.
- Boolean cuts: column fluting, crenels, slit windows, drain slits, gate arch, bridge arch, ring panel grooves.
- Plane cuts: broken columns, ruined walls, the broken colossus.
- Uniform remesh plus two octaves of Perlin displacement: erosion and chipped edges.

| Piece | Used in |
|---|---|
| Column / ColumnBroken / ColumnDrum | Quarry, Verdant (colonnade + temple), Sunscar, Dominion regions; every `K.column` call |
| TowerRound | Frostbound towers, Stormbreak towers + beacon shaft |
| TowerSquare | Ashfall fortress towers |
| Wall / WallRuin | Ashfall fortress ring, Frostbound curtain walls, Sunscar fortifications, Stormbreak battlements |
| Gate | Every `K.gate` call (Quarry, Ashfall, Frostbound, Sunscar) |
| Obelisk | Quarry survey pylons, Ashfall obelisks |
| Colossus / ColossusBroken | Titan colossus (Quarry), Frostbound wardens, Verdant guardians |
| SeatedColossus | Sunscar gate colossi |
| Brazier, Crucible | Ashfall |
| SovereignRing (+Glow), Monument (+Upper) | Shattered Dominion |
| GantryTower, Truss, ChainLink | Quarry gantry, all chains |
| Lantern | Stormbreak beacon |
| BridgeArch | Every `K.arch` call (Quarry bridge, Frostbound arch, Stormbreak bridge piers) |

A dev-only showroom level (`/Game/Arenas/Dev/L_ArchShowroom`, not cooked; `Tools/Tests/arch_showroom_shots.txt`) displays the library.

### 2. Virtual-texture surface master

- `M_GOTA_ArenaSurfaceVT` uses the same world-aligned graph as the main surface master, but with virtual-texture samplers. Each slot's sampler type comes from its texture's compression: Color, Linear Color or Masks.
- It unlocks the Megascans Icelandic 8K surfaces: `ObsidianVT`, `CrackedRockVT`, `MossRockVT`, `RockyVT`.
- These are now used for the Quarry, Verdant, Frostbound and Stormbreak flagstones, and for a third of Verdant's columns.
- `Megascans/Rock` stays excluded: its roughness maps use a different compression class from the ORD masks, so they can't share a sampler slot.

### 3. Gameplay safety

- **Layers.** Every swapped piece is in the Dressing layer (`NoCollision`, outside the boundary). The floor, curb, boundary, starts, Horde spawns and nav bounds are unchanged.
- **Builds.** All seven levels rebuild with `nav=True` and no missing assets.
- **Testing.** Every arena loads in game and runs the AI duel.

## Before / after

- **Before:** `Saved/Screenshots/GOTA/Before_Pass2/8x_<arena>_*.png`.
- **After:** `Saved/Screenshots/GOTA/8x_<arena>_*.png`.
- Both sets were taken from the same composition cameras (`Tools/Tests/arena_<key>_shots.txt`).

## Performance (uncapped, 15 s live combat, RTX 5070)

See `GOTA_ARENA_PERFORMANCE.md` for the method.

**Status (2026-10-31): not yet valid. Pass 2 has no confirmed frame-time result.**

**Why the attempts so far don't count:**
- The first attempts overlapped other editor processes.
- The clean run was spoiled by a local LLM (Ollama `qwen3:14b`), which held about 10.5 GB of the 12 GB VRAM and kept the GPU about 31% busy.
- From Frostbound onward, frame times were 90–450 ms because of that contention.

**Partial evidence:** Quarry and Ashfall ran before the contention peaked.
- Quarry: GPU 5.83 ms against a 6.37 ms baseline.
- Ashfall: GPU 6.75 ms against a 6.69 ms baseline.
- So these two show no sign of a GPU regression.

**To finish:**
1. Re-run `perf_all` with nothing else using the GPU (`ollama ps` empty).
2. Use the same settings: 1600×900, `r.VSync 0`, `t.MaxFPS 0`, `Tools/Tests/arena_<key>_perf.txt`.

**Architecture and materials after the 2026-10-31 repository recovery:** verified intact in all seven arenas from the recaptured after-shots, including the first Dominion after-set.

**Packaged build (2026-10-31): two shipping bugs found and fixed.**
- **Nanite flag.** The arena master materials lacked *Used with Nanite*, so cooked games drew every arena surface with the default material. The editor hides this by setting the flag on the fly.
  - Fixed on the assets with `Tools/GOTA_FixNaniteUsage.py`.
  - `GOTA_ArenaMaterials.py` now sets the flag on new masters.
- **Cook list.** C++ soft-loads some content that the cooker never discovered: the duel champion (`/Game/Enemies`), the Horde UI, the player, the framework and the spark VFX.
  - Without the champion, the duel stayed in countdown forever.
  - Fixed with `DirectoriesToAlwaysCook` entries in `DefaultGame.ini`.
- **Result:** the rebuilt package (`B:\Unreal\GOTA_Builds\Pass2_Paragon`) plays a full Fight AI match with correct materials.

**Note:** Titan's Quarry logs 12 missing `/Game/Geometry/SM_Quarry_*` references. These appear in the original baseline logs too, so they pre-date Pass 2 and the recovery. The map renders without them.

## Free asset candidates

These are for figurative statues that procedural modelling can't match. See `GOTA_ARENA_FREE_ASSET_CANDIDATES.md`.

## Still not AAA (honest)

- **Statues** are stylised robed figures (revolve + noise): no faces, hands or sculpted drapery. Real sculpts from the free Infinity Blade packs (if confirmed free) or Megascans would be the upgrade.
- **Seated colossi** are still blocky up close.
- **No decals**, no terrain blending under structures, no vertex-painted moss or dirt.
- **VFX** (lava flow, heat haze, waterfall, snowfall, rain) were not upgraded in this pass.
