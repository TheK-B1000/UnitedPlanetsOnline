# Gods of the Arena — Implementation Roadmap

**Updated:** 2026-10-30 (Combat Milestone 01 — Trial of Champions: Sovereign Champion)  
**Engine:** UE 5.8 ONLY  
**Branch:** `feature/gota-cinematic-frontend`

---

## North-star proof

> One hero, one weapon, one arena, one intelligent enemy, perfect parry, perfect dodge, one divine counter — then the same encounter over the network.

**Canon:** UPG Universe Story Bible **v2.0** — `GOTA_STORY_BIBLE.md` (*The Sovereign Protocol*). Confirmed: UPG/UPA/Earth/republic/Ascendants/1–4; Dir. **Cassian Rhen**; **SOVEREIGN**; Aegis Team Seven; Ayla Uncrowned God; Erebus-7 / Nova Requiem continuity locks. Players are UPA Wardens. Crossover build order: bible Part XIX (do not start Ayla C++ until Playable Integration Gates 1–4 pass).

**Active milestone:** Playable Integration 01 — Titan's Quarry proving ground.  
Track: `Docs/GOTA_PLAYABLE_INTEGRATION_RESULTS.md`. **No new features** until Gates 1–4 pass.

---

## Combat Milestone 01 — Player vs AI duel (2026-10-30)

North-star items, actual status. Evidence: `GOTA_AI_DUEL_TEST_RESULTS.md`; design: `GOTA_AI_DUEL_DESIGN.md`; numbers: `GOTA_COMBAT_TUNING.md`.

| North-star item | Status |
|---|---|
| One hero, one weapon, one arena | **Done** — UPA Warden (BP_Player, sword + shield), Titan's Quarry |
| One intelligent enemy | **Done** — Sovereign Champion: BT_Enemy_Melee plus a server director (fair wind-up reads, guard chain, shield-overload weak point, phase II, Sovereign Surge). Recruit, Veteran and Ascendant profiles. |
| Perfect parry | **Done** — the Blueprint's own `ParryWindow`, recognised in C++ (momentum and cue) |
| Perfect dodge | **Done** — BP dodge i-frames timed against the attack's impact notify |
| One divine counter | **Done** — Divine Momentum (UStatsComponent) → Q → server-validated, damage through `BPC_DamageSystem` |
| Same encounter over the network | **Partial** — listen-server host verified; remote client not tested |

Still open:
- Lock-on input wiring.
- Remote-client duel test.
- Sustained-hold block (optional montage loop; design doc §5).
- Distinct champion art.
- Runtime graybox navmesh root cause (the AI duel spawns on baked navmesh).

---

## Priority order (active)

1. **Gate 1 — Quarry dress PIE** (`DA_Dress_TitansQuarry`, `gota.BuildArena Quarry` / `?Arena=Quarry`)  
2. **Gate 2 — BP combat + death bridge** (`NotifyDied`, `ObserveHealth`, Notify* events)  
3. **Gate 3 — Horde one wave + Duel one match** on Quarry  
4. **Gate 4 — 2p then 4p PIE**; LAN session test separate from PIE auto-connect  
5. Only then: dress remaining arenas / Story polish / menu WBPs  
6. Impl 07 GAS — **deferred**; Impl 08 invites — **later**  

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
| BP graph capture (attack/parry/dodge) | Done — `Tools/GOTA_ExportCombatGraphs.py` → `Saved/GraphAudit`, traced with `Tools/GOTA_GraphFlow.py` |
| Wire Notify* inside `BPC_Attacks` / `BP_Player` | Superseded for the duel — C++ binds `BPC_DamageSystem.OnBlocked` directly (layout-checked); no BP edit needed |
| Lock-on IA wiring verification | Pending editor |
| Listen-server smoke test | Host-side done (Fight AI with `?listen`); remote client pending |

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
