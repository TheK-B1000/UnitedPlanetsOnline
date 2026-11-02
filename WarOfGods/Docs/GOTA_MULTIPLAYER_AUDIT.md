# Gods of the Arena: Multiplayer Audit

**Date:** 2026-11-01. **Branch:** `feature/gota-cinematic-frontend`, after `f79d856` (Drakon vs Rampage slice).
**Scope:** four total players (listen-server host plus three clients) across Story co-op, Wrath of the Gods (Horde)
and Trial of Champions (Duel).
**Method:**
- Read the C++ source.
- Dumped every *loaded* Blueprint/C++ class's RPCs and replicated properties from a running game
  (`gota.Net.AuditRPCs`).
- Inspected the Blueprint damage system (`gota.Net.InspectDamage`).
- Confirmed findings in live listen-server sessions (`Tools/GOTA_MPSession.py`).

Status legend:
- **Correct:** works as intended, keep it.
- **Incomplete:** works partly or for fewer players.
- **Broken:** wrong behaviour today, with evidence.
- **Fixed:** fixed in this pass and verified (see `GOTA_MULTIPLAYER_TEST_MATRIX.md`).

## 1. Inventory

| Area | Classes | Notes |
|---|---|---|
| Game instance / sessions | `BP_WarOfGods_GameInstance`, `UGotaSessionSubsystem` (Null OSS, LAN), `UGOTAMenuSubsystem` | Host / Find / Join / JoinByAddress / Destroy. `HostSession` clamps 2–4. Travel options per mode (`BuildTravelOptions`). Menu subsystem handles `OnNetworkFailure` / `OnTravelFailure` |
| Frontend | `AGOTAFrontendGameMode` (`PreLogin` caps the hall at 4), `AGOTAFrontendPlayerController`, `UGOTALobbyViewModel` (UI projection only) | Ready flags are **local only**: no replicated lobby state |
| Duel (Trial of Champions) | `ADuelGameMode` : `AArenaGameMode`, `ADuelGameState` (15 replicated), `ADuelPlayerState` (8 replicated, HeroId with OnRep) | **1v1 only** (`SoftMaxPlayers = 2`; vs AI = 1 human, extras spectate) |
| Horde (Wrath of the Gods) | `AHordeGameMode` : `AArenaGameMode`, `AHordeGameState` (12 replicated), `AHordePlayerState`, `AHordeEncounterDirector` | `SoftMaxPlayers = 4` |
| Story | `AGOTAStoryGameMode` : `AGameModeBase`, `AGOTAStoryGameState` (8 replicated), objective actors | `SoftMaxPlayers = 4`, four player starts |
| Player pawn | `BP_Player` (Blueprint, parent `ACharacter`): 24 RPCs, **all Unreliable**. `BP_WarOfGods_PlayerController` (`SR_SpawnPlayer` Reliable, `EnableAllInput`) | Combat input lives here |
| Health authority | `BPC_DamageSystem` (Blueprint component): Health, MaxHealth, IsDead, IsBlocking, IsInvincible. Delegates `OnDeath()`, `OnBlocked(CanBeParried, DamageCauser)`, `OnDamageResponse(Response, DamageCauser)` | **Not replicated** (component replication off, 0 replicated properties) |
| C++ combat layer | `UStatsComponent` (replicated: Health/Max/Stamina/DivineMomentum), `UGOTADuelFighterComponent` (momentum mirror, perfect parry/dodge, Divine Counter), `UGOTAHeroAbilityComponent` (6 RPCs, 6 replicated), `UGOTABossComponent` (4 multicasts, 5 replicated), `UGOTAHeroBodyComponent` (HeroId OnRep), `UCombatTimingComponent` (legacy, 4 Server RPCs), `ULockonComponent` (local) | |
| Observers | `UCombatBootstrapSubsystem`, `UGOTACombatantObserver` (reflection reads of the BP damage system), GameState delegates (`OnDuelCombatCue`, `OnHordeMatchOutcomeChanged`, `OnStoryMissionCompleted`, …) | |

## 2. What is already correct (keep)

- **Ability requests are server-validated** (`UGOTAHeroAbilityComponent`).
  - The client sends only a slot index.
  - The server checks round state, death, busy, cooldown, momentum, range and line of sight, and picks the target
    itself.
- **Divine Counter request** (`Server_RequestDivineCounter`): the server checks momentum, range and round state,
  and consumes momentum itself.
- **Boss and Fight AI brains run on the server only.** Clients receive state and multicast presentation.
- **Duel match flow.**
  - `CompleteMatch` is guarded to run once.
  - A fighter leaving mid-round forfeits.
  - The server picks the hero (`InitNewPlayer` validates `?Hero=` against the registry).
- **Horde match end.** `CompleteVictory` / `CompleteDefeat` are guarded by `bMatchEnded`. The enemy defeat counter
  is guarded per enemy (`bCountedDead`). Cleanup unbinds before destroying.
- **Session cap.**
  - `AGameSession::ApproveLogin` rejects joins at `MaxPlayers`. The Arena and Story modes set it from
    `SoftMaxPlayers` (Duel 2, Horde/Story 4).
  - The frontend has an explicit 4-player `PreLogin` cap.
- **Cosmetic body replication.** `UGOTAHeroBodyComponent::HeroId` uses OnRep, and clients build the body locally.
  Verified earlier in the 2-player ability multiplayer tests.
- **Unreliable multicasts for cosmetics** (impacts, cues) and **reliable** only for discrete events (cast start,
  phase cue, presentation).

## 3. Broken (with evidence) → fixed in this pass

| # | Problem | Evidence | Fix |
|---|---|---|---|
| B1 | **Health never reaches clients.** `BPC_DamageSystem` is not replicated, so every client HUD and client-side death check reads a stale local copy (always full health) | `gota.Net.InspectDamage`: `replicated=0`, no replicated properties. Audit: `BP_Player` has 0 replicated properties | The server mirrors Blueprint health and death into one replicated `UStatsComponent` per combatant (players, enemies, bosses). `UGOTACombatantObserver::ReadVitals` reads that mirror on clients |
| B2 | **`UStatsComponent` was created separately on server and clients.** Replication never applied, so momentum, health OnRep and `OnDied` never fired from the server | `UCombatBootstrapSubsystem` called `NewObject` on every machine with no authority check | Created on the server only, set to replicate. Clients use the replicated instance |
| B3 | **Horde: dead players never counted as dead.** Blueprint death leaves the pawn in the world (health −400, pawn valid), so team-wipe Defeat never triggered and the match could not end | Live: both players at `hp=-400 dead=1` but `eliminated=0`, `ended=0` (session `audit_horde`) | Alive = has a pawn whose health is above 0 (`IsCombatantDefeated`) |
| B4 | **Horde: eliminated players never respawned.** `RespawnEliminatedPlayers` relied on B3, and `RestartPlayer` would keep the dead pawn | Same session | Unpossess and destroy the dead pawn, then `RestartPlayer` and clear the eliminated flag |
| B5 | **Horde kill credit went to whoever was first in the player list**, not the killer | Code: "Attribute to first non-eliminated player … until kill credit RPCs exist" | Kill credit comes from the server's damage record. `OnDamageResponse` records the causer for Blueprint hits; `FGOTABlueprintCombat::ApplyDamage` records it for C++ hits. `OnEnemyDefeated` now passes the killer's PlayerState. Kills with no player damage on record count for the team only |
| B6 | **Horde: an enemy killed but not yet destroyed kept the wave open** | Director counted only destroyed pawns | Also counts health ≤ 0 as defeated (credited at the moment of death) |
| B7 | **Story: no player death handling at all.** A dead player stayed dead for the rest of the mission | No death or respawn code in `AGOTAStoryGameMode` | Co-op respawn: down → 6 s → dead body removed → `RestartPlayer` |
| B8 | **Horde and Story ignored each player's hero choice.** Every player got the default body | `AArenaGameMode::SetPlayerDefaults` read only `ADuelPlayerState`; Story passed `NAME_None` | New `AGOTAPlayerState` (replicated, server-validated `HeroId` + `OnHeroChanged`). Horde's PlayerState derives from it, Story uses it, and both validate `?Hero=` like the Duel |
| B9 | **Duel: a spectator leaving ended the match** as a forfeit for slot 1 | `Logout` treated `DuelSlot == INDEX_NONE` as slot 1 | Only a fighter with a duel slot forfeits |
| B10 | **Horde results screen bound its buttons with `AddDynamic` in `NativeConstruct`.** Re-showing the widget doubled the bindings, so one click sent two Replay / Return requests | Code | `AddUniqueDynamic` |
| B11 | **Client-trusted Blueprint combat RPCs.** `SR_ParryAttack(AttackTarget)` let a client name any actor for the parry riposte (5 damage + stagger) with no range, timing or state check. `SR_OnBlocked(CanbeParried, DamageCauser)` let a client claim a block | RPC audit of `BP_Player` | `BP_Player` reparented to the new native `AGOTAPlayerCharacter`, whose `ProcessEvent` validates these RPCs on the server: target alive, in range, defending on the server, rate-limited. Dead pawns can't attack, block or parry |
| B12 | **All 24 `BP_Player` RPCs are Unreliable,** including discrete client→server inputs (attack, block, parry). Packet loss silently dropped them | RPC audit | The `SR_*` server events are now Reliable (discrete requests, never per-frame). `MC_*` cosmetic multicasts stay Unreliable |

**Not a game bug:** with four game windows rendering on one GPU, one client crashed with
`D3D12Util.TerminateOnGPUCrash` (device removed). The automated four-player sessions therefore run the clients with
`-nullrhi`, which still exercises networking, gameplay and animation, but not rendering.

## 4. Incomplete (documented, not changed in this pass)

| # | Gap | Why not now | Follow-up |
|---|---|---|---|
| I1 | **Trial of Champions is 1v1 only.** No 2v2 or 4-player FFA, so four players cannot all fight in PvP. The 3rd and 4th player in a Duel are rejected at login (session cap 2) | Duel combat (`UGOTADuelFighterComponent`) is built around one `Opponent`; scoring is `ScorePlayer0/1`. Multi-opponent PvP is a feature (PvP design: Milestone B 2v2, Milestone C FFA), not a networking fix | Generalise to teams: per-team score array in the GameState, opponent = nearest enemy-team fighter |
| I2 | **Hero abilities, momentum, perfect parry/dodge, Divine Counter and bosses are Duel-only.** Their "is the round live" check and opponent lookup read `ADuelGameState` / `UGOTADuelFighterComponent` | Gameplay port, not a network fix | Mode-agnostic "combat live" query plus target selection for co-op |
| I3 | **Lobby ready flags are local.** No replicated lobby PlayerState in the frontend | UI/flow work | Replicated ready flag on the frontend PlayerState |
| I4 | **Blueprint melee hit detection runs where `BP_Player` / `BPC_Attacks` trace** (server-side `SR_MeleeAttack` → `MC_MeleeAttack`). No server rewind or lag compensation for player-vs-player hits | Needs a C++ hit-validation path or a Blueprint rework | Server-side lag compensation (rewind the target by the attacker's half RTT) for PvP melee |
| I5 | **Parry and dodge timing are judged in server time.** A remote client's block reaches the server ~RTT/2 late, so the effective parry window shrinks by one-way latency | Needs deliberate design and tuning | See the architecture doc §5 (favour-the-defender grace for PvE) |
| I6 | **Kill credit for a Blueprint one-shot kill** (`BPC_DamageSystem` omits the causer on the lethal hit) falls back to the last non-lethal hitter within 10 s, or team-only | Blueprint limitation | Have `BPC_DamageSystem` pass the causer to `OnDeath` |
| I7 | **Story has no mission-fail rule.** A full-team wipe just respawns everyone after 6 s | Design decision | Fail → reload checkpoint, if desired |
| I8 | **Test and dev console commands** (`gota.Duel.*`, `gota.Test.*`, `gota.Boss.*`) are compiled into Shipping, but act only on the host. The new `gota.Net.*` commands are excluded from Shipping | | Wrap the older command files in `#if !UE_BUILD_SHIPPING` |
| I9 | **Host migration:** none. Host leaves → clients get a network failure and return to the menu (`UGOTAMenuSubsystem::HandleNetworkFailure`) | Out of scope for listen-server Null OSS | Document as expected behaviour |
| I10 | `WBP_GOTA_HordeHUD` widget asset missing (warning at Horde load) | Content | Rebuild the Horde HUD widget |
| I11 | **`BP_Player`'s parry graph calls `SetGlobalTimeDilation`** (slow-motion on parry). Global time dilation on the listen server slows the world for **every** player, and on a client it desyncs that client's simulation | Seen in the exported `BP_Player` EventGraph; which machine runs it is not yet traced | Replace with actor-local `CustomTimeDilation` (as the C++ hit-stop already does) or a camera-only effect |
| I12 | **Story respawn uses the normal player starts.** In the Prologue an idle host respawned next to a level-placed `BP_Enemy_Boss` and was killed repeatedly (seen in the 4-player Story session) | Level design / spawn choice | Respawn at the active checkpoint or the start farthest from living enemies |
| I13 | The Horde Wave-2 spawn and the Horde HUD were not exercised visually with 4 players (all clients ran `-nullrhi`) | Test environment: four rendering game windows on one GPU caused a GPU device-removed crash | Run a manual 4-machine LAN playtest |

## 5. Trust boundary (summary; full table in `GOTA_MULTIPLAYER_ARCHITECTURE.md`)

| Action | Client sends | Server decides |
|---|---|---|
| Melee attack | `SR_MeleeAttack()` (now Reliable; dropped while dead) | Swing, traces, damage via `BPC_DamageSystem` on the server |
| Block | `SR_StartBlock()` / `SR_EndBlock()` | Block state; damage reduction in `TakeDamage` on the server |
| Parry riposte | `SR_ParryAttack(Target)` | **Validated** (alive, ≤ 4.5 m, server-side defending within 0.6 s, ≤ 1 per 0.35 s) |
| Perfect parry / dodge credit | nothing | `UGOTADuelFighterComponent` on the server (parry window / i-frames / boss impact time) |
| Ability | `Server_Activate(Slot)` | Everything else (cooldown, momentum, target, range, LOS) |
| Divine Counter | `Server_RequestDivineCounter()` | Momentum, range, round state, damage, boss stun |
| Health / death | nothing | `BPC_DamageSystem` on the server, mirrored to `UStatsComponent` |
| Score, kills, round/match outcome | nothing | GameMode → GameState / PlayerState |
| Hero selection | `?Hero=` join option | Registry validation (`InitNewPlayer` / `AGOTAPlayerState::AcceptHeroFromOptions`) |
