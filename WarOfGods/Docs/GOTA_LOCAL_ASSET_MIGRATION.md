# Gods of the Arena — Local Asset Migration

**Engine:** UE 5.8 only (`WarOfGods`)  
**VaultCache root:** `B:\Epic\VaultCache` (launcher configured; do not delete)  
**Rule:** Prefer Migrate / Add-to-Project. Do not Explorer-copy orphan `.uasset` files.

---

## Phase A — Discovery summary (updated 2026-10-27)

### Classification

| Pack | VaultCache | In WarOfGods Content | Status |
|------|------------|----------------------|--------|
| Quarry Collections | `QuarryCollections` | `Content/Geometry/SM_Quarry_*` + Megascans Quarry/Limestone | **IN_PROJECT** |
| Lava Field Collections | `LavaFieldCollections` | `Content/Geometry/SM_Lava_*` (+ Megascans lava overlap) | **PARTIAL IN_PROJECT** |
| Moab Desert | `Moab` | Geometry + Megascans Moab trees under Content | **IN_PROJECT** (2026-10-27) |
| Iceland | `Iceland` | Geometry/Assembly ICL_* + Megascans Iceland | **IN_PROJECT** (2026-10-27) |
| Nordic Coast | `Nordic` | Geometry/Assembly + Megascans Nordic Coast | **IN_PROJECT** (2026-10-27) |
| European Beech | `MS_Beech_UE51_V2` | `Content/EuropeanBeech` | **IN_PROJECT** |
| Norway Maple | `MS_Maple_UE51` | `Content/NorwayMaple` | **IN_PROJECT** |
| Mountain Tops | `FREEMounc1f4bef6b908V1` | No | VAULT_ONLY |
| Free Animation Library | yes | `Content/FreeAnimationLibrary` | **IN_PROJECT** |
| Realistic Starter VFX Vol 2 | yes | `Content/Realistic_Starter_VFX_Pack_Vol2` | **IN_PROJECT** |
| [18] Free Spells Niagara | yes | `Content/Free_Spells` | **IN_PROJECT** |
| Fighting Animset Pro | yes | `Content/FightingAnimsetPro` | **IN_PROJECT** |

Quarry was Add-to-Project into WarOfGods (full Geometry set on disk). Runtime dress still uses a **pilot subset** only.

---

## Phase D — Titan's Quarry pilot (active)

### On disk (verified)

- `Content/Geometry/SM_Quarry_Cliff_L_01..03`, `M_01..04`, `Ridge_S_*`, `RockPile_S_*`, `Spire_L_*` (~336 MB Geometry alone)
- Megascans: `Quarry_Cliff_01..16`, `Limestone_*`, `Quarry_Outcrop_01`, `Quarry_Ground_Gravel_01`

### Runtime wiring

1. `ArenaVisualDressDefaults::FillTitansQuarryPilot` — soft refs for cliffs / debris / landmarks.
2. `AArenaGrayboxBuilder::ApplyVisualDress` → `ResolveDress`:
   - Assigned `VisualDress` soft ptr, else
   - Canonical `/Game/Arenas/Data/DA_Dress_TitansQuarry`, else
   - Transient Quarry pilot (works immediately without Editor DA).
3. Gameplay graybox collision unchanged; dress meshes `bVisualOnly=true`.
4. Asset Manager scans `ArenaVisualDress` under `/Game/Arenas/Data` (AlwaysCook).

### Operator next steps

1. Hot-reload / compile C++ (`ArenaVisualDressDefaults`).
2. Optional persist DA: Editor → Execute Python Script  
   `Tools/GOTA_CreateDress_TitansQuarry.py`
3. PIE: `gota.BuildArena Quarry`
4. Tune scales/radii if cliffs dwarf the pit; confirm spawns/nav clear.
5. Only then expand dressing or start Ashfall (`SM_Lava_*` already local).

---

## Phase E–G — All-biome dress DAs (landed)

| Arena | Source | Status |
|-------|--------|--------|
| Titan's Quarry | `SM_Quarry_*` + Limestone | `DA_Dress_TitansQuarry` |
| Ashfall | `SM_Lava_*` | `DA_Dress_AshfallCrucible` |
| Frostbound | Iceland + WinterForestSet | `DA_Dress_FrostboundCitadel` |
| Verdant | Beech/Maple + moss | `DA_Dress_VerdantSanctuary` |
| Sunscar | RockSandstone + sand/succulents | `DA_Dress_SunscarExpanse` |
| Stormbreak | Dolerite + WaterPlane | `DA_Dress_StormbreakCoast` |
| Dominion | Composite landmarks | `DA_Dress_ShatteredDominion` |
| Fallen Sanctuary | Limestone + Beech fringe | `DA_Dress_FallenSanctuary` |

See `GOTA_ARENA_ASSET_DRESS.md`. Combat / Horde / Duel systems untouched. Transient pilots remain as ResolveDress fallback only.

---

## Migration log

| Date | Pack | Action | Result |
|------|------|--------|--------|
| 2026-10-27 | Discovery | Classified vault vs project | Complete |
| 2026-10-27 | Quarry | Dress hooks + pilot manifest | Code ready |
| 2026-10-27 | Quarry | Add-to-Project into WarOfGods | **IN_PROJECT** |
| 2026-10-27 | Quarry | Transient pilot + ResolveDress + AM scan | **Pending PIE** |
| 2026-10-27 | Iceland/Nordic/Moab | Launcher Add-to-Project stuck at 100% | **Root cause:** installing to `K:\UnrealProjects\WarOfGods` (~68 GB free) while packs need ~80 GB; VaultCache already complete on `B:\Epic\VaultCache` |
| 2026-10-27 | Iceland/Nordic/Moab | Bypass via `Tools/GOTA_InstallVaultPackToProject.ps1` → B WarOfGods | **IN_PROJECT** (Nordic copied ~19 GB; Iceland/Moab already present). Demo Maps skipped. |
| 2026-10-27 | All arenas | `GOTA_CreateAllArenaDress.py` — 8× `DA_Dress_*` soft pools/landmarks | **PASS** (Cmd). PIE visual per arena still operator. |

### Validation checklist (Quarry pilot)

- [x] Meshes present under WarOfGods Content
- [ ] Materials compile in Editor
- [x] `gota.BuildArena Quarry` / editor BuildLayout places dress from `DA_Dress_TitansQuarry`
- [ ] NavMesh / AI paths clear
- [ ] Listen-server layout matches
- [x] `DA_Dress_TitansQuarry` (+ all biome DAs) created via `GOTA_CreateAllDressDAs.py`
- [ ] Packaged cook includes soft-ref meshes

---

## Non-goals

- Redownloading Fab packs already in VaultCache  
- Mass-placing every Quarry_Cliff Megascans mesh at runtime  
- Replacing `AArenaGrayboxBuilder` layout logic  
- Changing combat authority  
