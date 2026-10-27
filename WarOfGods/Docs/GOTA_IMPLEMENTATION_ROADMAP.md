# Gods of the Arena — Implementation Roadmap

**Updated:** 2026-10-27 (Phase 1 in progress)  
**Engine:** UE 5.8 ONLY  
**Branch:** `cpp-branch`

---

## North-star proof

> One hero, one weapon, one arena, one intelligent enemy, perfect parry, perfect dodge, one divine counter — then the same encounter over the network.

Then expand Story (1–5 players), PvE waves, PvP duels on that foundation. **Do not scaffold the three modes yet.**

---

## Priority order (active)

1. Verified UE 5.8 playable baseline  
2. Existing combat behavior capture (editor)  
3. Lock-on integration (`IA_Lockon` → `ULockonComponent`)  
4. Typed combat event integration (BP → `Notify*`)  
5. Stats ownership / replication verification (observation mode)  
6. First networked combat smoke test (1 listen + 1 client)  
7. Combat timing improvements (DataAsset windows)  
8. Perfect parry  
9. Perfect dodge  
10. Divine counterattack  
11. Full networked combat vertical slice  

---

## Phase 0 — Complete

Audit docs, UE 5.8 upgrade, flat Source layout, Lockon/Combat/Stats shells, professional gitignore.

---

## Phase 1 — Engineering foundation (current)

| Item | Status |
|------|--------|
| UE 5.8 module build | Done |
| Asset cleanup / biome plan | Done — see `GOTA_ASSET_CLEANUP.md` |
| Stats observation mode (no dual health) | Done — `bAuthoritativeMutations=false` |
| Runtime component bootstrap + event logging | Done — `UCombatBootstrapSubsystem` |
| Combat design spec | Done — `GOTA_COMBAT_DESIGN.md` |
| GAS ADR | Done — defer — `ADR_GAS_ADOPTION.md` |
| Editor open / CompileAllBlueprints | In progress / operator |
| BP graph capture (attack/parry/dodge) | Pending editor |
| Wire Notify* inside `BPC_Attacks` / `BP_Player` | Pending editor (bootstrap cannot invent outcomes) |
| Lock-on IA wiring verification | Pending editor |
| Listen-server smoke test | Pending |

**BP wiring checklist (editor, minimal):**

1. Open `BP_Player` — confirm Enhanced Input `IA_Lockon` calls `ToggleLockon` on `ULockonComponent` (or add Call).  
2. On successful hit in `BPC_Attacks` / damage apply: `Get Component Combat` → `NotifyHitConfirmed`.  
3. On block success → `NotifyBlockSucceeded`.  
4. On parry success → `NotifyParrySucceeded` (`bPerfect` as applicable).  
5. Optionally on health change → `ObserveHealth` (do **not** enable authoritative Stats yet).

---

## Phase 2 — Combat vertical slice

Neutral arena map + sword/axe + melee enemy + tunable parry/dodge/divine counter. Use FightingAnimsetPro / BossyEnemy anims. Dress later biomes from Megascans/EuropeanBeech **without** committing 30GB libraries.

---

## Phase 3 — Multiplayer combat

Authoritative damage/parry validation; 2-client tests; document latency.

---

## Phase 4+ (summary)

| Phase | Focus |
|-------|--------|
| 4 | Thin Story encounter + PvE wave + PvP duel prototypes |
| 5 | Five-player co-op |
| 6 | Combat polish + revisit GAS |
| 7 | Enemy director / boss |
| 8–10 | Story, modes expansion, ship |

---

## Arena biomes (content track, parallel)

| Arena | Biome | Asset feed |
|-------|-------|------------|
| Neutral | Training pit | ThirdPerson + LevelPrototyping |
| Forest | Sacred grove | EuropeanBeech + plants |
| Volcanic | Ash/lava | LavaFieldCollections + FX |
| Canyon | Cliffs/hoodoo | Megascans Rock/Cliff (curated) |
| Ruins | Temple | Pedestals + Props + IB FX |

Folders: `Content/Arenas/{Neutral,Forest,Volcanic,Canyon,Ruins}/`
