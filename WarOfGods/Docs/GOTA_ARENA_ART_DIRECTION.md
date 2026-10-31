# Arena Art Direction — the seven arenas

Each arena is an authored level, `/Game/Arenas/Maps/L_Arena_<Name>`, built by `Tools/GOTA_BuildArenaLevels.py`.

**Canon status:** chapter links come from `GOTA_STORY_BIBLE.md`, the section "Arena ↔ chapter". Everything under **Environment story** is *proposed lore* for the art: it explains the architecture and must be approved before it becomes canon.

## Shared rules

| Rule | Why |
|---|---|
| The fighting floor is a 35 m disc (Dominion 44 m) with a low curb, an invisible boundary at 23.5 m (Dominion 27.5 m), and nothing decorative inside it | Combat readability; dressing can never change the fight |
| Landmarks sit *outside* the boundary and *inside* the gameplay camera's view | Impressive scenery the camera actually sees |
| Every floor carries a subtle centre seal (stone disc + faint energy ring) | Orientation and spawn symmetry at a glance |
| Emissives are restrained (energy 7–8, lava 6, cracks 3) and particles stay small, short-lived and off the fighting floor | Attack telegraphs, parries and silhouettes stay readable |
| One sun per arena, its own palette; local lights are low and short-ranged | Distinct identity without washing out characters |

## Titan's Quarry — *weight, scale, history* (quality benchmark)

| | |
|---|---|
| **Canon** | Ch.1 *Quarry Protocol*: the first Sovereign research records surface here |
| **Palette** | Warm golden daylight; rust-orange cut rock; pale sandstone mosaic floor; deep stone shadows |
| **Landmarks** | Five terraced excavation benches (up to 45 m); the broken colossus ("Titan") legs on the north bench; its fallen head; a colossal hand in the apron; the mining gantry with a suspended monolith and hanging work platforms; the broken high bridge; the south monumental gate with UPA survey pylons; distant refinery stacks with smoke |
| **Atmosphere** | Volumetric dust, drifting motes, gantry dust, wind and machinery drone with distant chain clanks |

**Environment story (proposed).**
- **Origin.** The quarry was cut for millennia by a pre-UPG civilisation that carved its titans from the living rock.
- **Expansion.** Industrial excavation under the United Planets enlarged it into a terraced pit and uncovered the toppled colossus.
- **Abandonment.** The excavation was abandoned mid-shift, leaving blocks on the floor, the gantry frozen and the monolith still hanging. This happened after the restricted work recorded in Ch.1.
- **Now.** The UPA keeps the pit as a Trial ground. Its survey pylons still line the road, which hints at who sealed the site.

## Ashfall Crucible — *heat and violence*

| | |
|---|---|
| **Canon** | Ch.3 *Fires of Sovereignty*: Ascendant control technology manufactured here |
| **Palette** | Ash-dimmed red-orange sun; black obsidian and charred basalt; molten orange accents only |
| **Landmarks** | The obsidian island floor above a lava lake; the crucible fortress ring with eight towers and lava-veined faces; molten falls pouring from the battlements; the hanging cauldron pouring metal; two iron bridges; eight ceremonial braziers; distant smoking volcanoes |
| **Gameplay** | All lava is **decorative and unreachable** (beyond the boundary, no damage volumes). Floor fissures are cosmetic. A hazard version would need a server-side damage volume through the existing authority and is not implemented. |

**Environment story (proposed).** A fortress-foundry built where the planet bleeds. Its cauldrons once cast the alloys of the first control collars (Ch.3). The duel island was the old proving floor, where forged work was tested in front of the overseers.

## Frostbound Citadel — *isolation in an enormous frozen fortress*

| | |
|---|---|
| **Canon** | Ch.4 *The Erased Colony*: the sealed Nova Requiem archive |
| **Palette** | Cold blue daylight; slate-grey stone with snow caps; pale ice; very restrained bloom (exposure −0.35) so characters read against snow |
| **Landmarks** | The inner court with a frosted flagstone floor and ice seal; low crenellated walls with buttresses; round towers; frozen warden statues; jagged glacier masses and frozen falls; the broken north bridge and arch; pine belt; snow-capped mountain range |
| **Gameplay** | Snow and ice are **surface finishes only**: identical collision and friction. No slippery physics. |

**Environment story (proposed).** A colony-era citadel swallowed by an advancing glacier. The frozen statues were raised to its wardens before the colony's records were expunged. Its sealed archive is the reason the UPA returns (Ch.4).

## Verdant Sanctuary — *a living ancient sanctuary*

| | |
|---|---|
| **Canon** | Ch.2 *Verdant Protocol*: sentient biological systems react to Resonance |
| **Palette** | Green-gold light through canopy; moss-green temple stone; strong volumetric shafts; soft mist |
| **Landmarks** | Overgrown colonnade with hanging vines; the stepped north temple with a colonnade and broken pediment; guardian statues; the waterfall over a mossy cliff with its pool and mist; the stream around the ring; giant roots arching over the outer ring; 40 great beeches and maples |
| **Gameplay** | All foliage stands **outside** the boundary. No grass or trees on the fighting floor, so enemies and attacks stay visible. |

**Environment story (proposed).** A ceremonial court where an ancient people met the forest. The forest took it back and now answers Resonance (Ch.2). Fireflies and pollen are the living ambience of that reclaimed place.

## Sunscar Expanse — *scale and mystery of a forgotten civilisation*

| | |
|---|---|
| **Canon** | Ch.5 *The Forgotten Worlds*: frontier populations and why they back Rhen |
| **Palette** | Intense warm sun; golden sandstone; light warm dust (never a sandstorm) |
| **Landmarks** | The sun-dial plaza (twelve gold meridian lines); a colonnade half-swallowed by sand; the north gate flanked by two seated colossi; eroded fortification ring; dunes against the walls; a buried ring-engine of a pre-UPG age, still faintly alive; canyon cliffs, buttes and hoodoos; distant ruin spires |

**Environment story (proposed).** A sun-temple city whose engine-ring once powered the region before the frontier was forgotten by the core worlds. The colossi still face the arena. Locals hold their trials here away from UPG oversight (Ch.5).

## Stormbreak Coast — *the power of the ocean and the approaching storm*

| | |
|---|---|
| **Canon** | Ch.6 *The Breaking Fleet*: civil conflict; Ayla's first signal |
| **Palette** | Cool storm light; desaturated wet stone; dark sea; short, capped lightning flashes |
| **Landmarks** | The fortress platform on a sea stack (wet flagstones); battlements; intact and broken towers; the UPA beacon tower; the great bridge to the mainland on sea-rising arches; outer sea stacks with ruined towers; harbour piers; distant fleet hulls; ocean to the horizon |
| **Gameplay** | Wet surfaces are a material look only (same friction). Lightning is `AGOTALightningFlash`: local, cosmetic, at most 6 lux with a 0.6 s double flicker, and disabled by Reduced Motion. |

**Environment story (proposed).** A coastal bastion and fleet beacon. The great bridge saw the fleet split during the civil conflict (Ch.6), and the beacon is where the first counter-signal was heard.

## Shattered Dominion — *the greatest arena ever constructed*

| | |
|---|---|
| **Canon** | Ch.8: SOVEREIGN imposes its control system here |
| **Palette** | One coherent warm sun; regional colour from local light, ground and stone per region |
| **Layout** | A raised central platform (44 m) with a UPG alloy seal; six 60° regions around it (Quarry, Volcanic, Frozen, Forest, Desert, Coastal), each with its own ground, stone and landmarks |
| **Transitions** | Rock spines along each sector edge, so regions meet through rubble rather than a hard material line |
| **Connections** | Six bridges, each in its region's stone |
| **Central landmark** | Six monument towers fusing ancient stone with UPG alloy; the floating **Sovereign Ring** with a light column |
| **Audio** | One ambience zone per region |

**Environment story (proposed).** A neutral ground where every biome of the Dominion was brought together by force: quarried, transplanted and bridged. SOVEREIGN's ring hangs over it as a statement of control (Ch.8).
