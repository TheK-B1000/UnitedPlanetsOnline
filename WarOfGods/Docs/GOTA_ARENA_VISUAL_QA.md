# Arena Visual QA

**Method.** Each authored level is loaded in standalone `-game` (1600×900) with the Fight AI duel. `Tools/Tests/arena_<key>_shots.txt` captures these shots into `Saved/Screenshots/GOTA/`:

| Shot | View |
|---|---|
| `80` | Gameplay camera with the HUD |
| `81` | Spawn view |
| `82` | Gate / landmark |
| `83` | Centre |
| `84` | Elevated |
| `85` | Establishing |
| `86` | Live combat (test bot) |

All views except 80 and 86 are composition cameras with the UI hidden.

**Before.** Every arena was the runtime graybox: flat coloured boxes in `ThirdPersonMap` under a magenta sky, as in `Saved/Screenshots/GOTA/6*_fight.png` from Combat Milestone 01.

## Checklist per arena

| Check | Quarry | Ashfall | Frostbound | Verdant | Sunscar | Stormbreak | Dominion |
|---|---|---|---|---|---|---|---|
| Graybox look replaced by coherent environment | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Distinct landmarks visible from gameplay cameras | ✅ colossus, gantry, terraces | ✅ fortress, crucible, falls | ✅ towers, glacier | ✅ temple, waterfall, forest | ✅ colossi gate, hoodoos | ✅ beacon, bridge, stacks | ✅ Sovereign Ring, towers |
| Fighting floor readable; characters stand out | ✅ | ✅ (after the exposure fix) | ✅ | ✅ | ✅ | ✅ | ✅ |
| Nothing decorative inside the boundary | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Particles small, brief and off the floor | ✅ (after the particle fix) | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Lighting identity per brief | ✅ warm gold | ✅ ash red, dark obsidian | ✅ cold blue | ✅ green-gold | ✅ desert sun | ✅ cool storm | ✅ coherent + regional |

## Issues found and fixed during QA

| Issue | Fix |
|---|---|
| All arenas black | The exposure range assumed a physical sun; it now targets the ~10 lux sun. |
| Quarry pit in shadow | Sun raised to 41°; sky fill increased. |
| Black floors and blocks | Virtual-texture surfaces in a non-VT material. Families were moved to non-VT texture sets. |
| Plain grey floor disc | Radial flagstone paving (≈170 slabs per arena). |
| VSM "non-Nanite marking queue overflow" | Kitbash primitives moved to Nanite copies. |
| Ashfall blown out (user report) | Exposure −0.6; lava emissive 35→6; cracks 18→3 and reshaped into short fissures; every light cut 4–8×; fog thinned. |
| Combat sparks lingered (user report) | Smaller system, a third of the size, destroyed after 0.5 s; ambient embers fewer, smaller and pushed off the floor; dust halved. |
| Frostbound "grey drum" | Walls lowered so the glacier shows; buttresses; towers with crenellations; frosted flagstones; snow drifts. |
| Stormbreak walled in by cliffs, white establishing shot | Sea stacks scaled below the platform; beacon light 400k→15k. |
| Sun-dial / energy lines too hot | Gold energy 7→2.5, UPG energy 8→4, thinner meridians. |

## Known visual gaps (honest)

- **Kitbash architecture.**
  - Columns, gates, walls and colossi are textured primitives with world-aligned materials. They read well at gameplay distance but lack sculpted detail (no carved reliefs, mouldings or chips).
  - Production quality needs modelled or Megascans architectural meshes.
- **Lava** is an emissive slab with a panning texture: no flow mesh, heat haze or splash VFX.
- **Waterfalls and stream** use StarterContent water materials on boxes. They need a proper waterfall material/mesh and spline river.
- **Snowfall and rain** are approximated (motes, dripping emitters). Real precipitation needs Niagara systems; none exist in the project.
- **No decals**, so there is no grime, cracks or blood blending at the bases of structures.
- **No landscape actors.** Ground is meshes plus slabs, and terrain blending is done with rock scatter.
- **Hero Select and loading screen** still show arena preview art from the old menu set, not screenshots of the new levels.
