# Gods of the Arena: Multiplayer Test Matrix

**Date:** 2026-11-02. **Editor rows** are Development on `4f6642a` / `ae00b81`. **Packaged rows** are the UE 5.8 Development cook `B:\Unreal\GOTA_Builds\RC1_0a93ace` from `0a93ace`. `Pass2_Paragon` is not evidence for these fixes.

**How the sessions run:**
- `Tools/GOTA_MPSession.py` starts **separate game processes** on one machine (`UnrealEditor -game`, not PIE): a
  listen-server host plus N clients joining `127.0.0.1`.
- Each process runs a script from `Tools/Tests`.
- Verification uses the dev commands `gota.Net.*` (Players / Hurt / KillEnemy / ForgeParry / AuditRPCs /
  InspectDamage) and log lines from the server and every client.
- Logs: `Saved/MPTests/<tag>_host.log`, `<tag>_client<N>.log`.

**Result legend:**

| Result | Meaning |
|---|---|
| **PASS** | Ran, and the expected log evidence was present |
| **FAIL** | Ran, and something was wrong |
| **NOT RUN** | Not executed in this pass |
| **COMPILE** | Builds only |

## Results

| # | Test | Mode / setup | Result | Evidence |
|---|---|---|---|---|
| 1 | Two-player regression: ability over the network (host Zephyr + client) | Duel, `GOTA_AbilityRegression.py --only GaleRush` | **PASS** | 4/4: hit+cooldown, sidestep miss (invalid target), death during cast, **multiplayer (listen server + client)**: server-side hit on the remote player, client plays the cast animation on the replicated hero body |
| 1b | Drakon vs Rampage combat (from `f79d856`) still works with the new health mirror and RPC guard | Duel vs boss, `slice_mechanics.txt`, run twice | **PASS** | 11/11 then 9/9 perfect parries, 3/3 perfect dodges, counters stun (`Boss STUNNED … 90 damage`), **0 rejected RPCs** |
| 2 | Four-player PIE session | — | **NOT RUN** | PIE needs the interactive editor. Multi-process standalone sessions (#3) were used instead; they are the stronger test, since each player is a real separate process with its own net driver |
| 3 | **Four-player standalone session** | Horde, host + 3 clients (`horde4d`) | **PASS** | `Horde PostLogin players=4`; every client lists 4 players with the right local player; heroes validated per player (`accepted hero Nyx / Varoth / Kairos`) |
| 3b | Four-player standalone session | Story, host + 3 clients (`story4`) | **PASS** | 4 players, heroes validated, co-op respawn (below) |
| 3c | Four-player **packaged** session | Story + Horde, cook `RC1_0a93ace` | **PARTIAL** | Story `pkgstory2` PASS. Horde join PASS, Horde combat script **NOT COMPLETED**. See § Packaged RC1 |
| 4 | Server-authoritative health on clients | Horde 4P, Story 4P | **PASS** | Before the fix: clients always showed 100/100. After: clients show the host's health live (`hp=60 → 20 → 0 dead=1 → respawn 100`, `story4_client1`); everyone at `hp=0/100 dead=1` after a wipe (`horde4d_client*`) |
| 4b | Client cannot forge combat outcomes | Story 4P, each client runs `gota.Net.ForgeParry` (forged `SR_ParryAttack` at the farthest pawn) | **PASS** | Server: `rejected SR_ParryAttack (failed server validation)` × 3 (one per client); no damage applied |
| 5 | Parry / dodge / Divine Counter | Duel vs boss (host) | **PASS** (host) | See 1b. **Not tested with a remote client as defender** (needs a human or a client-side bot) |
| 5b | Ability replication to clients | Duel 2P | **PASS** | #1 multiplayer case: `body plays Ability_RMB (ok`, `BODY [client] … hero=Zephyr` |
| 6 | AI attacks against several players | Story 4P | **PASS** (partial) | `BP_Enemy_Boss` attacked and killed the host repeatedly while clients idled elsewhere. Kill credit names the boss. A boss targeting several players wasn't scripted |
| 7 | Death, respawn and results exactly once | Horde 4P | **PASS** | Kill → `eliminated=1` → wave clear → `respawned … as BP_Player_C_4` (full health). 4-player wipe → `outcome=2` (Defeat) set once, enemies cleaned up |
| 7b | Kill credit per player | Horde 4P | **PASS** | `Horde kill credit: <player> +100`, then `kills=1` / `kills=2` per killer on host and clients (was: always the first player in the list) |
| 7c | Co-op respawn | Story 4P | **PASS** | `is down - respawn in 6s` → `respawned … as BP_Player_C_n`. All four downed → all four respawned |
| 7d | Duel forfeit exactly once | Duel PvP, fighter process hard-killed | **PASS** | `fighter in slot 1 left - forfeit` → `Duel match complete outcome=1` (once) |
| 8 | Player cap | Horde: 5th player. Duel: 3rd player | **PASS** | `PreLogin failure: Server full.` on both; the rejected client gets `Server full.` |
| 8b | Client disconnect mid-match | Duel (above) | **PASS** | See 7d |
| 8c | Host graceful quit | Horde 4P | **PASS** | Clients: `Host closed the connection.` → main menu |
| 8d | Host hard kill (crash / power loss) | Horde, host + 1 client | **PASS** (slow) | Client: `ConnectionTimeout` after **60 s** → `LogGOTAMenu: Network failure` → main menu. Recommend a shorter `ConnectionTimeout` |
| 8e | Late join / reconnect | — | **NOT RUN** as a scripted case. Clients joining after the host's match started (Horde phase 3) worked in every 4P run | |
| 8f | Travel (menu → session → match) | — | **NOT RUN** in this pass. Sessions start directly on the match map; the menu Host/Find/Join path was not re-tested |
| 9 | **Latency and packet loss** | Horde, host + 4 clients. Clients: 120 ms ±30 lag, 5% loss. Host: 60 ms, 3% loss | **PASS** | Pings ~230–244 ms. Kill credit exact, respawn worked, Defeat once (`outcome=2`), clients converged to the same state (`hp=0/100`, kills) |
| 10 | Delegate lifecycle | Code review + runs | **PASS** (review) | `UStatsComponent::EndPlay` unbinds Blueprint delegates and clears its timer; death broadcast once per life; Horde results buttons use `AddUniqueDynamic`. No duplicate deaths or outcomes in any run. **No automated rebinding test across travel** |
| — | **Crash: D3D12 results-screen crash in PvP** | Duel PvP 2P, after a forfeit, idle on results | **FAIL (1/1)** prior; **PASS (6/6 + rematch)** after fix | See stability pass below |

## Stability pass (2026-11-01 night, uncommitted → then committed on branch)

**Root cause (PvP forfeit path):** delayed `D3D12RHI` Parallel BasePass AV while idling on results. Correlated with (1) orphan `W_PlayerHUD` ticking without a valid pawn and (2) dressed hero meshes left in the world. Same-frame PostLogin dress→destroy was a related Fight AI trigger; spectators-only start removed it but was **not** sufficient alone (`duelsoakB*` still 3/3 CRASH).

**Fix:** `bStartPlayersAsSpectators`; on `CompleteMatch` destroy remaining fighters + strip `W_PlayerHUD`; MP clamps `SetGlobalTimeDilation` to actor-local; Story safe respawn + 2.5 s invuln.

| # | Test | Result | Evidence |
|---|---|---|---|
| S1 | Forfeit → results idle soak (`duelsoakC1`–`C6`) | **PASS 6/6** | Each: `forfeit` → `match complete` → **NO** `EXCEPTION_ACCESS_VIOLATION`; `hud_msgs=0` |
| S2 | Rematch / multi-spawn soak (`duelrematch1`) | **PASS 1/1** | `NO_CRASH`, 4 fighter spawns (round recreation) |
| S3 | Global slow-mo clamp (`slowmo1`, listen + 1 client) | **PASS** | `ABILITY slomo 0.150` → `clamped SetGlobalTimeDilation … CustomTimeDilation locally` |
| S4 | Story 4P wipe → protected respawn (`storyrespawn2`) | **PASS** | 4 downs → 4× `respawned … (protection 2.5s)` → all `hp=100/100 dead=0` |
| S5 | Packaged 4P with this fix | **PARTIAL** | Cook `RC1_0a93ace` exists. PvP and Story re-ran in that package. Horde combat did not. See below |

**Editor counts:** forfeit crash soaks **6 pass / 0 fail**; rematch **1/1**; P2 **1/1**; P3 **1/1**.

## Packaged RC1 (2026-11-02)

Development cook only (`RunUAT BuildCookRun`, exit 0). Logs: `Saved/MPTests/pkg*.log`. A run that did not execute is **NOT RUN** or **NOT COMPLETED**, not a pass. Invalid attempts (`pkgsoak*`, `pkgf*`) killed the bootstrap stub or shared port 7777 with a leftover process; they are not results.

| # | Test | Rendering | Result | Evidence |
|---|---|---|---|---|
| P4-1 | Cook | — | **PASS** | `B:\Unreal\GOTA_Builds\RC1_0a93ace` |
| P4-2 | PvP forfeit soak | Host rendered. `pkga6` client rendered. `pkga1`–`pkga5` clients `-nullrhi` | **PASS 6/6** | Each: slot 1 forfeit, `outcome=1`, crash lines 0 |
| P4-3 | PvP rematch | Host rendered, client `-nullrhi` | **PASS** | `pkgrematch3`: 4 fighter spawns, rounds 1–2, score 2:0, `Player0Wins`, crash lines 0 |
| P4-4 | Slow-mo clamp | Host rendered, client `-nullrhi` | **PASS** | `pkgslow` clamp log |
| P4-5 | Story 4P death + safe respawn | Host rendered, 3 clients `-nullrhi` | **PASS** | `pkgstory2`: heroes Drakon/Nyx/Varoth/Kairos, 4× `hp=-899`, 4× protected respawn. First attempt `pkgstory` **NOT COMPLETED** (HUD log flood) |
| P4-6 | Horde 4P kills, wipe, results | `pkghorde` / `pkghorde2` host rendered. `pkghorde3` host `-nullrhi`. Clients `-nullrhi` | **NOT COMPLETED** | `players=4` and `waves=5`. No `kill credit`, no `outcome=`. Process killed at timeout. No crash dump |
| P4-7 | 120 ms / 5% loss + forged RPC | `pkglag` host and client 1 rendered; clients 2–3 `-nullrhi` | **PASS (forge)** | 3× `rejected SR_ParryAttack`. Pings on later runs ~200–254 ms. Horde kills under lag **NOT COMPLETED** |
| P4-8 | Legitimate remote parry | — | **NOT RUN** | No client defender |
| P4-9 | Menu Host → Find → Join | — | **NOT RUN** | Manual. `gota.Menu.Go` was not driven |
| P4-10 | Drakon vs Rampage | Rendered host, no clients | **PASS** | `pkgboss2`: 8 perfect parries, 8 perfect dodges, counter stun 90 damage, 0 rejected RPCs |
| P4-11 | Packaged cap + disconnect | — | **NOT RUN** | Editor #8–#8d only |
| P4-12 | GPU crash / sustained performance | Rendered hosts as above, never four rendered clients | **No crash observed** | No GPU dump. No FPS or memory capture |

**Counts:** packaged forfeit **6/6**; rematch **1/1**; slow-mo **1/1**; Story wipe **1/1**; boss regression **1/1**; Horde combat **0 completed**; menu **0**; remote parry **0**.

## How to re-run

```text
# 4-player Horde (kill credit, death, respawn, wipe)
python Tools/GOTA_MPSession.py --tag horde4 --url "/Game/Arenas/Maps/L_Arena_TitansQuarry?game=/Script/WarOfGods.HordeGameMode?Mode=Horde?Arena=Quarry" --host-script mp4_horde_host.txt --client-script mp4_client.txt --clients 3 --client-heroes Nyx,Varoth,Kairos --client-exec "t.MaxFPS 30" --render-clients 0 --join-delay 30 --stagger 5 --timeout 800
# + latency/loss and a 5th (rejected) player
... --tag hordelag --host-script mp4_horde_lag_host.txt --clients 4 --client-exec "t.MaxFPS 30,NetEmulation.PktLag 120,NetEmulation.PktLagVariance 30,NetEmulation.PktLoss 5" --host-exec "NetEmulation.PktLag 60,NetEmulation.PktLoss 3"
# 4-player Story (co-op respawn, forged-RPC rejection)
... --tag story4 --url "/Game/Story/Maps/L_GOTA_Story_Prologue?game=/Script/WarOfGods.GOTAStoryGameMode?Mode=Story" --host-script mp4_story_host.txt --client-script mp4_client_forge.txt --clients 3
# Duel cap + forfeit;  host hard kill
... --tag duelcap --url "...DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3" --host-script mp_duel_cap_host.txt --client-script mp_duel_client.txt --clients 2 --kill-client 1 --kill-client-at 75
... --tag hostkill --url "...HordeGameMode..." --host-script mp_hostkill_host.txt --client-script mp_duel_client.txt --clients 1 --kill-host-at 120
```

**Environment notes:**
- Four game windows rendering on one GPU crashed one client (device removed). Clients therefore run `-nullrhi`
  while the host renders.
- A `-nullrhi` host stalled in the editor, so editor hosts render. Packaged Horde was also tried with `--host-nullrhi`
  (`pkghorde3`); the combat script still did not finish.
- Packaged runs: `GOTA_PACKAGED` points at the staged `WarOfGods.exe` (the runner resolves the inner binary) and
  `GOTA_PORT` picks a free port. Do not reuse 7777 if an old `WarOfGods.exe` is still bound there.
- The first launch after a code change is slow (shader/DDC warm-up). Allow `--join-delay 30` or more and a long
  `--timeout`.

## Recommended follow-up tests

1. Finish packaged Horde: the host script clock stops advancing after four players join, so kill credit, wipe, and results never run. Editor rows #7 and #7b are not a substitute.
2. Manual menu playtest: Host → Find → Join → Start → match → return, four players, including the session cap and a disconnect.
3. Remote-client defender under 120 ms, so a real parry is measured separately from forged-RPC rejection.
4. Stop `W_PlayerHUD::GetPercent` from reading a null `PlayerRef` (Story log flood).
5. Teach Story and Horde to honor `?DevHeroes=1` the way `ADuelGameMode` does.
6. A manual 4-machine LAN playtest (rendering on every machine), then a Shipping cook.
