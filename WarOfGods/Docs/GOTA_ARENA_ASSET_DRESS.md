# Arena Asset Dress — Prototype Replacement

**Status:** Canonical `DA_Dress_*` DataAssets for **all** graybox layouts (2026-10-27)  
**Rule:** Soft-load only; missing mesh → skip that slot; gameplay cubes keep collision.

## Resolve order

`ArenaVisualDressDefaults::ResolveDress`:

1. Assigned `VisualDress` soft ptr on the builder  
2. Canonical `/Game/Arenas/Data/DA_Dress_<Layout>`  
3. Transient pilot (fallback only if DA missing)

## Persisted DAs

| Layout | DataAsset |
|--------|-----------|
| Titan's Quarry | `/Game/Arenas/Data/DA_Dress_TitansQuarry` |
| Ashfall Crucible | `/Game/Arenas/Data/DA_Dress_AshfallCrucible` |
| Frostbound Citadel | `/Game/Arenas/Data/DA_Dress_FrostboundCitadel` |
| Verdant Sanctuary | `/Game/Arenas/Data/DA_Dress_VerdantSanctuary` |
| Sunscar Expanse | `/Game/Arenas/Data/DA_Dress_SunscarExpanse` |
| Stormbreak Coast | `/Game/Arenas/Data/DA_Dress_StormbreakCoast` |
| Shattered Dominion | `/Game/Arenas/Data/DA_Dress_ShatteredDominion` |
| Fallen Sanctuary | `/Game/Arenas/Data/DA_Dress_FallenSanctuary` |

Asset sources (cliffs / debris / landmarks) match the former transient pilots (`SM_Quarry_*`, `SM_Lava_*`, Iceland/WinterForest, Beech, Sandstone, Dolerite, Limestone ruins).

Graybox **ring walls** are tagged `GOTA_PlaceholderBoundary` and hidden (collision kept) when dress places successfully.

## Tools

```
Tools/GOTA_CreateAllDressDAs.py      # create/refresh all DA_Dress_*
Tools/GOTA_ValidateArenaDress.py     # DA soft-ref check + BuildLayout smoke
Tools/GOTA_PIE_ValidateDress.py      # PIE / gota.BuildArena smoke when available
```

## Playtest

```
gota.BuildArena Quarry
gota.BuildArena Ashfall
gota.BuildArena Frostbound
gota.BuildArena Verdant
gota.BuildArena Sunscar
gota.BuildArena Stormbreak
gota.BuildArena Dominion
gota.BuildArena Story
```

Log expect: `ArenaVisualDress applied N landmarks (+pools) from DA_Dress_<Name>` (not `TransientDress_*`).
