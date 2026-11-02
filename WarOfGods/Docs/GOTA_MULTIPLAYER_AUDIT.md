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

---

## Stability pass status (2026-11-01 night, editor)

**Baseline:** `4f6642a`, `ae00b81`, `8e85490`, then `0a93ace`. Editor soaks are in the matrix § Stability pass. Packaged results are the next section. `Pass2_Paragon` is not evidence for these fixes.

| Priority | Status | Notes |
|---|---|---|
| P1 Duel results crash | **Fixed (editor + packaged)** | Attempt B teardown. Editor `duelsoakC1`–`C6` 6/6. Packaged `pkga1`–`pkga6` 6/6, no `EXCEPTION_ACCESS_VIOLATION` |
| P2 Global slow-mo | **Fixed (editor + packaged)** | Clamp holds in the package (`pkgslow`) |
| P3 Story safe respawn | **Fixed (editor + packaged)** | `pkgstory2`: 4 downs, 4 protected respawns |
| P4 Packaged 4P | **Partial** | Cook succeeded. PvP, Story, slow-mo, and Drakon vs Rampage ran in the package. Horde kill/wipe/results did not. Menu flow not run |

**Production readiness:** **not a release candidate.** RC2–RC4 packages were preserved. RC5 is a copy of the RC4 cook at `B:\Unreal\GOTA_Builds\RC5` with a newer Development exe. Shared combat is live via `GOTACombatLive`. Remote charge dodge under lag now perfect-dodges without making mistimed dodges free. Four-player Story vs spawned Quarry Behemoth exercised shared parry/dodge/momentum. One machine only. No Shipping cook.

## Packaged RC1 (2026-11-02)

**Build:** UE 5.8 Development Win64, cooked from `0a93ace` plus the local harness fixes below. Output: `B:\Unreal\GOTA_Builds\RC1_0a93ace`. AutomationTool exit 0. This is not a Shipping cook, so `gota.*` test commands exist.

**What the package actually ran**

| Check | Result | Evidence |
|---|---|---|
| Clean cook | **PASS** | `BuildCookRun` Win64 Development, archive dir above |
| PvP forfeit, 1v1 | **PASS 6/6** | `pkga1`–`pkga6`: `fighter in slot 1 left - forfeit`, `outcome=1`, no crash. `pkga6` rendered the client; `pkga1`–`pkga5` clients were `-nullrhi`. Hosts rendered |
| PvP rematch | **PASS** | `pkgrematch3`: rounds 1 and 2, four fighter spawns, score 2:0 `Player0Wins`, then idle on MatchEnd, no crash |
| Slow-mo clamp | **PASS** | `pkgslow`: `ABILITY slomo 0.150` then `clamped SetGlobalTimeDilation … CustomTimeDilation locally`. Host rendered, client `-nullrhi` |
| Story 4P heroes, death, respawn | **PASS** | `pkgstory2`: Drakon, Nyx, Varoth, Kairos; `players=4`; four `NET hurt … hp=-899`; four `respawned … (protection 2.5s)` at the same start. Host rendered, clients `-nullrhi`. No crash |
| Horde 4P waves, kills, wipe, results | **NOT COMPLETED** | `pkghorde`, `pkghorde2`, `pkghorde3`: four heroes and `players=4`, director `waves=5`. The host script never got past its opening wait, so kill credit, wipe, and `outcome=` were not executed. No crash dump. Do not treat the editor Horde rows as this package |
| Latency + forged parry | **PASS (forge only)** | `pkglag`: three `rejected SR_ParryAttack`. Clients `PktLag 120`, variance 30, `PktLoss 5`. Host `PktLag 60`, `PktLoss 3`. Later runs showed pings about 200–254 ms. Kill credit under lag was **not** executed |
| Legitimate remote parry | **NOT RUN** | No client-side defender. Forge rejection is not that test |
| Host → Find → Join | **NOT RUN** | Sessions travel straight onto the match map. Needs a manual menu playtest |
| Drakon vs Rampage | **PASS** | `pkgboss2` (rendered host, no clients): 8 perfect parries, 8 perfect dodges, Divine Counter 90 damage and `Boss STUNNED`, 0 rejected RPCs, no crash. `pkgboss` (unmodified `slice_mechanics.txt`) lost the round before the bot engaged |
| GPU / sustained performance | **No crash observed** | No `EXCEPTION_ACCESS_VIOLATION` and no GPU-crash dump in the runs above. No frame-time or memory capture, so this is not a performance pass. First Story attempt (`pkgstory`) wrote ~26 MB of `W_PlayerHUD` `GetPercent` null-ref / divide-by-zero warnings and never reached the wipe |

**Harness notes (this package, not in `0a93ace`):** the staged bootstrap exe exits after spawning `Binaries\Win64\WarOfGods.exe`, so the runner launches the inner exe. `GOTA_PORT` avoids a leftover process on 7777. `gota.Heroes.AllowInDevelopment` is set from `Engine.ini` at startup because `?DevHeroes=1` is applied only by `ADuelGameMode`; without that, Story and Horde reject Drakon and fall back to UPAWarden (`pkgstory`, `pkghorde`). `Slate.bAllowThrottling=0` is set the same way. `t.IdleWhenNotForeground` is cheat-flagged and cannot be set from `Engine.ini`; the runner sends it on the console instead.

## Packaged RC2 (2026-11-02)

**Build:** fresh UE 5.8 Win64 Development cook. AutomationTool exit 0. Archive `B:\Unreal\GOTA_Builds\RC2`. The RC1 directory was not overwritten. Development, so `gota.*` commands exist. Every process below is on this machine. No second physical machine was available.

The Horde stall in RC1 was the test-script clock, not a stuck wave. `FTSTicker` hands the callback one frame's delta and the old runner sampled that only every 0.05 s, so a fast packaged host never finished `wait 70`. The runner now ticks every frame and still caps one step at 0.5 s so a hitch cannot skip a wait.

| Check | Result | Evidence |
|---|---|---|
| Clean cook | **PASS** | `BuildCookRun` exit 0, new archive directory |
| Horde 4P, five waves + boss | **PASS** | `rc2hordewin`: host rendered, 3 clients `-nullrhi`, port 7791. Six `Horde kill credit` lines (five +100 and a boss +2500). `Horde match complete outcome=Victory score=6250` once. `BossActive` then `ended=1`. No `EXCEPTION` |
| Horde intermission respawn | **PASS** | Same log: one player hurt during wave 1, `Horde: respawned …` on the clear, `living=4/4` in the next wave |
| Horde 4P team wipe | **PASS** | `rc2hordewipe`: four `NET hurt`, one `Horde match complete outcome=Defeat score=0`. No second complete line. No crash |
| Story HUD | **PASS** | `rc2story`: `GetPercent now reads replicated stats` once. `Accessed None` 0, divide-by-zero 0, with `Log LogScript` left on |
| Story respawn spread | **PASS** | Five protected respawns (one player died twice). The four that overlapped in time landed at different XY: `(-1064,-190)`, `(-951,-77)`, `(-1064,-399)`, `(-952,-297)`. Protection 2.5 s. Host rendered, clients `-nullrhi` |
| Dev heroes in Story and Horde | **PASS** | `DevHeroes=1` is applied in `AArenaGameMode` and `AGOTAStoryGameMode` before login. Shipping builds ignore the option. Horde/Story logs show Drakon, Nyx, Varoth, Kairos |
| Forged parry | **PASS** | `rc2parry`: client `PktLag 120`, variance 30, `PktLoss 5`. Server logged `rejected SR_ParryAttack`. That session had no boss (URL omitted `?Boss=`), so it does not prove a parry |
| Legitimate remote parry | **NOT RUN** | Client armed `gota.Net.RemoteParry` and finished with no timed block. A Fight AI boss fight allows one human; that human is the listen-server host. A joining client is the second fighter only when there is no boss, or a spectator when there is one. The remote pawn was not the defender against Quarry Behemoth |
| Drakon vs Quarry Behemoth (Rampage body) | **PASS** | `rc2boss3`, rendered host, no clients, `?Boss=QuarryBehemoth`. 2 perfect parries, 4 perfect dodges, Divine Counter 90 damage, `Boss STUNNED`, boss hp 800/900, player hp 130. `SetGlobalTimeDilation redirected to local CustomTimeDilation`. Script quit then logged slot 0 forfeit `outcome=2` (host leaving). No crash. An earlier `rc2boss` / `rc2boss2` without `?Boss=` is not this result (`BOSS none`) |
| PvP forfeit | **PASS (1, not a 6/6 soak)** | `rc2forfeit`: `fighter in slot 1 left - forfeit`, `outcome=1`. Host rendered, client `-nullrhi`. The process quit about 8 s later, so the old ~67 s results-screen crash window was not re-soaked |
| Four-player cap | **PASS** | `rc2cap`: Horde `players=4`, then client 4 `PreLogin failure` / `Server full.` All processes `-nullrhi` |
| Client disconnect while the server is up | **NOT RUN** | The harness killed the host before it killed a joined client, so the server never logged that leave |
| Host disconnect back to menu | **NOT RUN** | Host was hard-killed. Client logs did not show a network-failure return to the menu before those clients were stopped |
| Menu Host → Find → Join | **NOT RUN** | Sessions travel straight onto the match map. `gota.Menu.Go` was not driven |
| Frame time / GPU / memory | **NOT RUN** | No Unreal Insights capture. Cook peak was the cooker, not a play session. No crash dump in the runs above |
| Second physical machine | **NOT RUN** | Only local processes |

**Open at the end of RC2** (closed or narrowed in RC3 below)

1. No legitimate remote perfect parry. The listen-server host is the only human fighter in a boss encounter, so a lagged client cannot be the defender without a different slot setup.
2. Menu Host → Find → Join was not driven.
3. A joined client leaving, and a host drop returning clients to the menu, were not cleanly logged on RC2. The four-player cap was.
4. The Blueprint parry node still calls `SetGlobalTimeDilation`. RC2 redirects that call in multiplayer (`rc2boss3`); the node is still in the asset. `gota.Ability.Slomo` still calls the C++ API, which the same hook and the tick clamp convert.
5. Development package only. No Shipping cook, no frame-time or memory profile, no second machine.
6. Shared combat (parry, dodge, Divine Counter, abilities in Story and Horde) and 2v2 are not started. They wait on the remote-parry gap and a menu/disconnect pass.

## Packaged RC3 (2026-11-02)

**Build:** UE 5.8 Win64 Development cook, archive `B:\Unreal\GOTA_Builds\RC3`. RC2 was not overwritten. Later code (per-player travel hero, listen-flag strip, camera-only parry, remote defender, session clear) was linked into `WarOfGods.exe` and copied over the RC3 inner binary. No second cook. Development, so `gota.*` exists. Every process is on this machine.

Menu Host / Find / Join calls `UGotaSessionSubsystem` through `gota.Menu.Session` after `gota.Menu.Go Multiplayer`. That is the same C++ the buttons call. The widgets were not clicked with the mouse. The Host button still sets PvP Duel when it creates the session. The lobby no longer overwrites the mode on open or on Start, so a later Horde selection is what traveled.

| Check | Result | Evidence |
|---|---|---|
| Menu Host → Find → Join → Horde | **PASS** | `rc3menu3`, host and one client both rendered, Null OSS port 7777. Find `results=1`, join `button=1`. Client travel URL `HordeGameMode?Hero=Nyx` with no `listen`. Host URL `?listen?Hero=Drakon`. Both accepted. Titans Quarry |
| Two rendered heroes | **PASS** | Same log. Host and client both: Drakon `mesh=Greystone`, Nyx `mesh=SM_Countess`, `dilation=1.00`, ping 13–14 ms. Two windows, not four |
| Host leave, client returns and session clears | **PASS** | `rc3menuleave2`, both `-nullrhi`. Client dump `in=1 screen=Lobby`, then `Returned to the main menu; session cleared`, then `in=0 screen=MainMenu`. `rc3menu3` (rendered) also logged `returning to Entry` and `Frontend presented` on both processes; that binary did not yet clear the session flag |
| Client leave, host stays in the match | **PASS** | `rc3leave`, port 7801, both `-nullrhi`, direct URL. Client `NET leave` then frontend on `L_GOTA_MainMenu`. Host went from 2 players to 1 and stayed in Horde. Same run logged Drakon/Greystone and Nyx/SM_Countess on both processes (client was `-nullrhi`, so the mesh applied; it was not a rendered shot) |
| Legitimate remote parry and dodge | **PASS** | `rc3parry3`, port 7804, `?Defender=Remote?Boss=QuarryBehemoth`. Client rendered, host `-nullrhi`. Client `PktLag 120`, variance 30, `PktLoss 5`, logged ping 141 ms. Two `Momentum … (perfect parry)`, boss `parried=2` then Staggered, hp 890/900, player 130/130. Earthshaker `REMOTE dodge` and `landed=0`. Host pawn none, `duelSlot=-1`. Client is the defender |
| Parry feedback does not scale the pawn | **PASS** | Same client log: `parry feedback is local camera only (CustomTimeDilation=1.00)`. End dump `dilation=1.00` |
| Frame time, GPU, memory, replication | **PASS (short sample)** | `rc3menu3` `gota.Net.Perf` on the rendered Horde pair: frame 11.81–18.41 ms, GPU 5.72–9.24 ms, used physical ~1394–1411 MB, net in 88–118 KB, net out 71–93 KB, packets lost 0. An earlier two-window wave-1 sample (`rc3menu2`) hit frame 223–357 ms and GPU 230–281 ms during first-frame shader misses. Not an Unreal Insights capture |
| Story entered from the menu | **NOT RUN** | Horde was the mode that traveled. The mode command can select Story; that run was not made |
| Mouse click of Host / Find / Join | **NOT RUN** | Same subsystem calls as the buttons, after the menu was opened by command |
| Remote Divine Counter | **NOT RUN** | Parry and dodge only |
| Charge-attack dodge | **NOT RUN** | `bCharging` is server-only, so the client cannot time a charge from replicated state |
| Four rendered players | **NOT RUN** | Two windows maximum. Four rendered clients previously removed the D3D12 device |
| Second physical machine | **NOT RUN** | |
| Shipping cook | **NOT RUN** | |
| Shared Story/Horde combat, 2v2 | **NOT RUN** | Still the next phase, after this list |

RC2 rows for Horde victory, Horde defeat, Story HUD, Story respawn spread, the fifth-player rejection, and forged `SR_ParryAttack` stay as recorded on `11f2602`. They were not repeated.

## Packaged RC4 (2026-11-02)

**Build:** `B:\Unreal\GOTA_Builds\RC4` is a file copy of the RC3 cook. The RC4 inner exe is the Development binary built after `8b886f0`. RC2 and RC3 were not modified. No second cook. Every process is on this machine (`DESKTOP-9MUFES4`). There is no second computer on this run.

Menu clicks are a left mouse-down delivered to that button's `SButton` (`gota.Menu.Click`). The first attempt routed the event through an empty widget path and crashed (`rc4story`). The passing run does not. Clicking a session row now focuses it before Join, which is what a pointer click has to do for the result index.

| Check | Result | Evidence |
|---|---|---|
| Story selected in the lobby and traveled | **PASS** | `rc4story4`, both windows rendered. Clicks: STORY, NEW CO-OP CAMPAIGN, JOIN A CAMPAIGN, FIND GAMES, the session row, START MISSION. Lobby dump `mode=Story`. Client URL `GOTAStoryGameMode?Hero=Nyx` with no `listen`. Host URL `?listen?Hero=Drakon`. `Story PostLogin players=2`. Client welcomed on `L_GOTA_Story_Prologue` |
| Hero appearance on that travel | **PASS** | Client dump: local Nyx `mesh=SM_Countess`, remote Drakon `mesh=Greystone`, ping 18 ms. Host applied `SM_Countess` with authority. The lobby has no hero tiles. The hero id was stored with `gota.Menu.Session hero` after the clicks. The hero-select screen launches a local arena and was not the multiplayer path |
| Remote Divine Counter, 120 ms / 5% loss | **PASS** | `rc4combat`, client rendered, host `-nullrhi`, client `PktLag 120` variance 30 `PktLoss 5`. Three perfect parries to momentum 69, then `REMOTE counter mom=66`. Server `Boss STUNNED by Divine Counter` and `90 damage (landed=1, momentum left 19)` |
| Charge state visible to the defender | **PASS** | Same fight and `rc4charge`. Client: `Boss charge replicated; defensive lead uses distance / ChargeSpeed`, then `REMOTE dodge … BehemothCharge charging=1`. Only `bCharging` was replicated. Direction and the hit stay on the server |
| Charge dodge avoids the hit | **FAIL (RC4)** | `rc4charge`: sidestep while `charging=1`, hit 18. Fixed in RC5 |
| Four players, rendered where practical | **PASS (two windows)** | `rc4four`, Story prologue, port 7814. Host and client 1 rendered. Clients 2 and 3 `-nullrhi`. `players=4`. Drakon/Greystone, Nyx/SM_Countess, Varoth/Sevarog, Kairos/Gideon. Four rendered windows were not opened. That previously removed the D3D12 device |
| CPU / GPU / net sample, shader hitches | **PASS (20 s)** | Same host. 1193 samples, avg frame 16.78 ms, max 359.55 ms, avg GPU 6.52 ms, max GPU 17.54 ms, 1 hitch. The hitch is the frame the fourth player logged in; GPU on that frame was 8.8 ms, so it was not a GPU shader stall. Two `PSO/Encountered new graphics PSO` events at capture frame 2. End sample: 1731 MB used, 1977 MB peak, net in 380 KB, net out 222 KB, lost 0. CSV id `B363489D496E54A0A18641AB9DDDF139`. No csv file was left on disk after `csvprofile stop` |
| Second physical machine | **NOT RUN** | Only this PC |
| Shipping cook, shared combat implementation | **NOT RUN (RC4)** | Implemented in RC5 |

## Packaged RC5 (2026-11-02)

**Build:** `B:\Unreal\GOTA_Builds\RC5` is a file copy of the RC4 cook. Inner exe is the Development binary with shared combat + charge-dodge fix. RC2–RC4 were not modified. One machine.

| Check | Result | Evidence |
|---|---|---|
| Remote charge dodge under 120 ms / 5% loss | **PASS** | `rc5charge3`. Client notifies; server `perfect-dodged`; defender `hp=130/130`, momentum +15 |
| Mistimed charge dodge still takes damage | **PASS** | `rc5miss`. Lead gate 0.05 — no charge dodge armed; `BehemothCharge hit … 18.0 (landed=1)`, `hp=112/130` |
| Shared combat-live in Story | **PASS** | `rc5coop2`. Four fighters, `combatLive=1`, all `opp=BP_Boss_QuarryBehemoth` |
| Four-player Story vs Quarry Behemoth | **PASS** | `rc5coop2` via `gota.Boss.Spawn`. Earthshaker and BehemothCharge `perfect-dodged`. Nyx mom=45, Varoth mom=15. All four at 100/100 |
| Join hitch root cause | **OPEN** | RC4 hitch was game-thread (GPU 8.8 ms). `rc5coop2` nullrhi host: avg 5.72 ms, max 94 ms, 0 hitches in a 12 s sample after join. No definitive sync-load culprit isolated yet |
| Horde wave combat, full Divine Counter in Story, Shipping, second PC | **NOT RUN** | |

---

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
| I11 | **`BP_Player`'s parry graph calls `SetGlobalTimeDilation`** | **Redirected, then made cosmetic:** multiplayer forces world dilation back to 1. `ApplyLocalParrySlow` no longer sets `CustomTimeDilation`. It pulses the local camera FOV for 0.25 s (`rc3parry3`: `parry feedback is local camera only (CustomTimeDilation=1.00)`). The Blueprint node is still in the asset | Optional: delete the BP node |
| I12 | **Story respawn unsafe near enemies** | **Fixed and spread:** clearance, 2.5 s protection, and a 160 cm ring so simultaneous respawns do not share a capsule (`rc2story`) | — |
| I13 | The Horde Wave-2 spawn and the Horde HUD were not exercised visually with 4 players (all clients ran `-nullrhi`) | Test environment: four rendering game windows on one GPU caused a GPU device-removed crash | Run a manual 4-machine LAN playtest |
| I14 | **`W_PlayerHUD::GetPercent` null `PlayerRef`** | **Fixed:** native `GetPercent` reads `UStatsComponent` and returns 0 if the pawn or max health is missing. The local HUD also binds `OnHealthChanged` and unbinds on EndPlay. `rc2story` had zero `Accessed None` and zero divide-by-zero with logging left on | — |
| I15 | **Packaged Horde script never reached combat** | **Fixed:** script clock. `rc2hordewin` victory and `rc2hordewipe` defeat on RC2. RC1 logs stay as the pre-fix evidence | — |
| I16 | **`?DevHeroes=1` was Duel-only** | **Fixed:** `UGOTAHeroRegistry::ApplyDevHeroesOption` runs from Arena and Story `InitGame`. Shipping ignores it. Dev heroes are not publicly selectable when the cvar stays 0 | — |

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
