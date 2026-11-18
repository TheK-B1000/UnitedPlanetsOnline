# Horde cooperative combat

## RC14: one perfect-dodge rule for everyone (2026-11-18, bot evidence)

**Decision (2026-11-17, option 1 below):** keep the 0.35 s perfect-dodge window; the listen host and remote players get identical server-authoritative protection through the qualifying attack's impact (+0.15 s); protect that attack only, not unrelated hits; mistimed dodges stay vulnerable; the Blueprint dodge animation, movement, parry system and attack timing are unchanged. Code `e0f794f`; package `B:\Unreal\GOTA_Builds\RC14_e0f794f` (fresh cook, 3,937 packages, 0 incrementally skipped).

**How it works.**

- When the server accepts a dodge (host: the Blueprint dodge's i-frame edge on the server; remote: the client's dodge notice), it records the **qualifying impact**: the attacker, its montage and the damage notify (`PlayMontageNotify`) about to land, valid until impact + 0.15 s.
- The qualifying attack is whichever attack is about to land on that player within 650 cm: the assigned opponent's or any other enemy's, soonest impact first. An attack is a playing montage with a damage notify ahead. The Blueprint `Attacking` flag is no longer required: it is set partway into the swing, so a host dodging 0.30–0.35 s early found "no attack" while remote dodges, judged a trip later, qualified.
- `UGOTAMontageReplicator` (now on every combatant, players included for Duel PvP; it still multicasts only AI montages) listens to `OnPlayMontageNotifyBegin`. It is bound when the pawn spawns, so it runs before the enemy Blueprint's Play Montage listener, which is added per montage and applies the damage. When the recorded notify fires, the dodger's damage system gets `IsInvincible` for the rest of that frame (cleared at the end of the frame, never while Veiled owns it). One impact per dodge.
- Not covered: other enemies' attacks, the same enemy's next swing or next notify, an impact after the guard expired, and dodges started more than 0.35 s before the impact.
- The RC13 remote-only window (blocking every attacker until impact + 0.15 s) is gone. Boss dodge protection (`IsDodgeProtected`, checked by the boss's own damage path) is unchanged; dodges against regular enemies no longer extend it.
- Unchanged and still different by nature: the Blueprint dodge's own 0.20 s i-frames exist on the dodging machine. For the host that machine is the server, so those 0.20 s also cover other enemies; a remote player's do not reach the server. Equalising that would mean adding blanket server i-frames for remote players or removing the Blueprint's for the host. Not done (the decision was not to introduce blanket invulnerability or change the dodge itself); a human playtest should say whether it matters.

**Validation** (dodge-lead runs: every player in dodge mode, dodging when the attack is 0.10 / 0.20 / 0.25 / 0.30 / 0.35 / 0.50 s away, healing harness; remote clients 120 ± 30 ms, 5% loss; bins are the server-measured lead):

| Server lead at the dodge | Editor `rc14_lead_e3` host: armed / hit by that attack | remote | Packaged `rc14p_lead` host | remote |
|---|---|---|---|---|
| 0.00–0.10 s | 13 / 0 | 73 / 0 | 7 / 0 | 42 / 0 |
| 0.10–0.20 s | 4 / 0 | 16 / 0 | 4 / 0 | 10 / 0 |
| 0.20–0.25 s | 19 / 0 | 4 / 0 | 16 / 0 | 14 / 0 |
| 0.25–0.30 s | 3 / 0 | 7 / 0 | 4 / 0 | 11 / 0 |
| 0.30–0.36 s | 3 / 0 | 4 / 0 | 6 / 0 | 4 / 0 |
| over 0.36 s (too early, not armed) | 2 dodges, 2 hit | 20 dodges, 9 hit | 3 dodges, 0 hit | 37 dodges, 21 hit |

- Guards (the dodged impact refused): editor 134, packaged 106; **0 landed anyway**. Expired guards: 2 / 0.
- Not blanket: hits by other enemies within 0.35 s before to 0.15 s after a guarded impact still landed (editor 9, packaged 0 in that run), and the guarded attacker's follow-up swings landed (8 and 16).
- Too-early dodges that were not hit escaped by moving out of reach (a dodge is still a movement).
- Before (RC14 investigation, `rc13p_dodge`): host 6 of 27 armed dodges counted as hit, of which 3 real, all at 0.22–0.29 s; remote protected by a blanket window.

**Regression:** editor suite 45/46 first pass (GaleRush multiplayer: the host froze 38 s compiling shaders for the client's hero, the client's script finished before the cast; GaleRush 8/8 in two reruns); packaged suite **46/46 first pass**. Packaged battery: full match to Victory (`score=13875`, Horde boss defeated, clients up 1,049 s after it, 0 RHI errors), public-menu Horde 4/4 ready, Story float and net (0 unresolved movement bases), Duel rematch, client and host leave, Drakon versus Rampage (parries, perfect dodges, Divine Counter stuns, Phase II, boss defeated); no crash, assertion or ensure in any log. Editor: Story, Duel rematch, Rampage pass.

**Legitimate matches (bots, no healing, nothing injected):** packaged `rc14p_legit` cleared waves 1–3 and lost wave 4 at 5/15, one `outcome=Defeat score=4800`, the best bot result so far; editor `rc14_legit` lost wave 2 at 6/11. Bots in normal mode dodge only the ground smash (1–4 dodges per match), so the dodge rule barely enters these; the spread is run-to-run variance.

**Test tools:** `gota.Net.Fight <s> dodge <lead>`; `Tools/Tests/mp_dodge_lead_*.txt`; `GOTA_DodgeReport.py` bins by lead and checks guards; `GOTA_MPSession.py` launches clients once the host is listening and removes old logs of the same tag first (a cold host start took 58 s, and a stale log once satisfied the check); the suite reports a host stall of 15 s or more as INCONCLUSIVE.

## RC14: host dodges (2026-11-17, bot evidence)

**Question (R13-2):** the listen host had 6 of 27 armed dodges hit anyway in `rc13p_dodge`; remote players 1 of 68. Mistiming or inconsistency?

**Telemetry added (development only):** `TELEM dodge` now logs the armed dodge's time to impact (`lead=`, was -1), and the server logs how long the Blueprint's own dodge i-frames lasted (`Dodge i-frames (server-edge) lasted …`). `GOTA_DodgeReport.py` counts a hit by the dodged enemy up to the impact (+0.15 s) as "hit anyway" even if a perfect line follows, and prints the i-frame lengths.

**Findings:**

- 3 of the 6 RC13 cases were miscounted: they were perfect dodges at a 0.03–0.06 s lead, and the "hit" was the enemy's **next** swing 0.72–0.75 s later. The old report matched any hit within 0.8 s.
- The healing harness can mask a hit: health is restored before the dodge resolves, so a hit dodge can also log `perfect_dodge`. The report now checks hits first.
- Blueprint dodge i-frames last **0.20 s** (0.16–0.24 s, 47 dodges; `rc14_dodge_e1`). The longer 1.3 s spans were not dodges (armed=0).
- A dodge is **armed** (counts as a perfect-dodge attempt) when the impact is up to `PerfectDodgeLeadSeconds` = **0.35 s** away.
- The host's real hits all had a lead of 0.22–0.29 s and landed exactly at the impact, after its 0.20 s i-frames had ended. With leads up to 0.20 s the host was never hit. Some host dodges with longer leads still avoided the hit by moving out of range.
- Remote players (RC13 fix) are protected by the server until the impact + 0.15 s, so their effective window is the full 0.35 s.

**Conclusion:** an inconsistency, not only mistiming. Same input, same enemy: a remote player is safe dodging 0.2–0.35 s early, the host is not. Not changed, because it is a design decision on the dodge window. Options:
1. Give the host the same server protection as remote players (window 0.35 s for everyone; most forgiving).
2. Protect remote players only for the Blueprint i-frame length (about 0.20 s), so everyone gets the Blueprint's window. Remote players then lose what latency costs them.
3. Arm a perfect dodge only within the i-frame length (about 0.20 s), and keep the protection as it is.

Recommendation: decide after the human playtest asks whether dodging feels too strict or too lenient; meanwhile option 1 is the smallest code change and makes host and remote equal. **Option 1 approved and implemented (above).**

## RC13: human combat validation (2026-11-17)

Code `a941f8c` (game) and `04b69f8` (test harness only). Package `B:\Unreal\GOTA_Builds\RC13_a941f8c`. No enemy damage, health, count or difficulty was changed; no GAS feature was added (Frozen / Stun stay on hold). Matrix rows R13-1 to R13-13.

**Evidence type.** Everything in this section comes from test bots on one machine with emulated latency. **No human has played RC13 yet.** Human results go only in session sheets marked `Evidence type: HUMAN` (`GOTA_HORDE_PLAYTEST.md`); bot numbers are never quoted as human results.

### Remote perfect dodges against regular enemies (fixed)

Remote players' armed perfect dodges were still hit about half the time: the dodge invulnerability was set only on the client, and the server applied the hit. Now, when the server accepts an armed dodge from a remote player against a regular enemy, it sets `IsInvincible` until the attack's impact plus 0.15 s, then clears it (never while Veiled, which owns the same flag). Telemetry: `TELEM dodge` and `TELEM perfect_dodge`; report `python Tools/GOTA_DodgeReport.py <tag>`.

| Run | Remote armed dodges | Perfect | Hit anyway | Host armed / perfect / hit anyway |
|---|---|---|---|---|
| `rc13_dodge_e1` (editor, before) | 75 | 40 | **35** | 16 / 12 / 2 |
| `rc13_dodge_e2` (editor, after) | 80 | 80 | **0** | 11 / 8 / 3 |
| `rc13p_dodge` (packaged RC13) | 68 | 67 | **1** | 27 / 20 / 6 |

The host's own dodges are local and were not changed; 6 of its 27 armed dodges were still hit in the package. Whether a human host feels that is a playtest question (sheet item: "dodged but still hit").

### Per-wave player telemetry

At the end of each wave the server logs one `TELEM wave_player` line per player: hits taken, damage taken and dealt, blocks, parries, dodge attempts, armed and perfect dodges, deaths, kills, time of first death and `avoided` (blocks + parries + perfect dodges against hits). `python Tools/GOTA_HordeTelemetry.py <tag>` prints the table. These are the numbers to fill in after a human session.

### Regression flakiness: cause found in the test environment

The RC12 first pass (44/46: GaleRush and GravityWell multiplayer) and the RC13 failures had one thing in common: a starved client. Its script clock counts frame time, so an 18 s wait took 57–227 s of real time while other software loaded the machine (about 26 GB of 48 in use, high CPU), and the client checked after the host had already quit. With a client getting frames, three back-to-back multiplayer passes were 33/33. Harness (`04b69f8`): clients launch once the host is listening, host scripts start with `waitplayers 2 120` and stay up 75 s after the action, the suite waits up to 180 s, and a failing case whose client wait took over 36 s is **INCONCLUSIVE (client starved)**: it fails the run and is never reported as a pass. The RC12 record stays 44/46 first pass. Packaged RC13 suite: 45/46, the one failure INCONCLUSIVE (TitanCharge, starved client).

### Battery (packaged RC13, bots)

Full match with the healing harness: one Victory, clients up 1,067–1,068 s after it, 0 crashes. Legitimate match: wave 1 cleared, wave 2 lost at 8/11. Public-menu Horde with `?Players=4`, Story (0 unresolved movement bases), Duel rematch, Drakon versus Rampage (3 parries, 19 dodges, 5 Divine Counters, defeated), client and host leave: all pass. **A legitimate five-wave victory was not achieved.** Best legitimate run so far, editor `rc13_legit_e1` (RC13 code, fight driver also dodges the unblockable ground smash; no healing, nothing injected): wave 1 cleared in 47.5 s (7/7, 1 death), wave 2 cleared in 45.0 s (11/11, 1 death), wave 3 lost at 9/12 after 50.5 s, one `outcome=Defeat score=3375`. RC12c reached 9/11 in wave 2.

### RC13 profile (packaged, rendered client, waves 1–2, one 60 s window)

Capture `rc13p_focus` (`Tools/Tests/mp_perf_focus_client.txt`): Unreal Insights with channels `cpu,gpu,frame,bookmark` only, a CSV profile and `gota.Net.Perf` over the same bookmarked window. Four players, 120 ± 30 ms, 5% loss; the machine had other software running (26–30.6 of 48 GB in use), so absolute numbers are an upper bound.

| Measure | Value |
|---|---|
| Frame | 17.20 ms average, median 16.87, p95 21.75; 2 frames over 50 ms, 1 over 100 ms |
| Game thread | 10.50 ms (of which about 6.7 ms is `Sync_RenderingThread`: waiting for the render thread) |
| Render thread | 17.02 ms: **the bottleneck** |
| RHI thread | 5.62 ms |
| GPU | 5.19 ms (`gota.Net.Perf`: 4.5–5.1 ms) |
| Memory | 1.66 GB physical used by the game (peak 1.83 GB) |
| Network | down 572 KB / 60 s = **9.5 KB/s**, up 237 KB = **4.0 KB/s**; lost in 0, lost out 1 |

Game thread per frame (Insights, inclusive, about 3,550 frames): `FEngineLoop::Tick` 17.2 ms, `Sync_RenderingThread` 6.7, character skeletal mesh tick (`CharacterMesh0`) 2.4, skeletal animation completion 1.55, character movement 1.4.

Render-side timers (Insights inclusive totals; these add up across the render thread and its parallel workers, so they rank costs but are not per-frame render-thread time): `SceneRender` 109 s, `RenderGraphExecute` 76, visibility commands 22.4, volumetric cloud shadow 7.2, post-processing 5.8, Nanite geometry 5.1, TSR 5.0, diffuse indirect and AO 4.6, shadow depths 4.2, Lumen screen probes 4.1, Lumen scene update 4.0, FX pre-render 3.9, virtual shadow maps 3.0.

Reading: the client is render-thread CPU bound, not GPU bound (GPU 5.2 ms against 17 ms). The largest named cost is visibility and mesh draw command setup, where skinned meshes build their commands every frame (`r.SkeletalMesh.UseCachedMDCs=0`, about +1.4 ms measured in RC11). The volumetric cloud shadow is the largest single lighting cost and is a candidate for a cheaper setting. Enemy montage replication is not a measurable cost: the montage multicast adds about 4 KB/s of downstream in total (RC12 comparison), and animation on the game thread is about 1.5 ms. Compared with RC12c (17.5 ms frame, 12.2 KB/s down over a 120 s window) nothing got worse.

**Trace trustworthiness.** The trace opens and analyses in Unreal Insights, and the CPU timers agree with the CSV profile (`FEngineLoop::Tick` 17.2 ms against a 17.2 ms CSV frame). The GPU timestamp warnings cover 9 interleaved events with a largest error of 0.28 ms, which is negligible. The `MemAlloc` "invalid tag" errors in the analysis log appear even with memory tracing switched off, so they come from Insights' analysis, not from the game. **Memory tracing is not needed** for this task: the game uses about 1.7 GB of 48 GB and frame cost is the question. The earlier full-channel trace made Insights itself use 11.7 GB and hang; it was stopped, and that was an analysis-tool problem. **None of the Insights warnings is a GOTA crash:** checked separately, the game logs have no crash, assertion or `LogRHI: Error`, and `rc13p_focus_exits.txt` shows the host and the profiled client exited 0 by themselves; the two `-nullrhi` clients were ended by the harness at the end of the session (exit 1, recorded as "killed by harness").

## RC12: enemy combat intelligence and playability (2026-11-16)

Code `336b6cb`. No enemy damage, health or count was changed. Packaged results are in `GOTA_MULTIPLAYER_TEST_MATRIX.md` "RC12".

### Six combat defects found and fixed

| # | Defect (evidence) | Cause | Fix |
|---|---|---|---|
| 1 | **Spinning sword: 90 of 100 health from one attack**, landing 24/25 | Authored, not a hit-registration bug: `Montage_Sword_SpinningAttack` had 1 `Slash` + 8 `AOESlash` damage notifies (radius 300) every 0.5 s; each notify hits each target once | Damage track rewritten: `Slash` 1.224 s, `AOESlash` 2.0 / 3.25 / 4.5 s. At most 4 contacts (40) per target, 1.25 s apart. Wind-up, animation, radius and 10 per contact unchanged (`Tools/GOTA_FixEnemySpinWindows.py`) |
| 2 | **Remote players could not see enemy attacks**: every client sample showed 0 of 6 attacking enemies animating; client bots never blocked or evaded | The enemy template (`BP_Enemy_Base`, `BPC_Attacks`) plays all 105 attack montages on the server; no multicast (the player Blueprint has `SR_/MC_` events, enemies do not) | `UGOTAMontageReplicator` multicasts each non-root-motion AI montage start and interruption; clients play it locally (cosmetic; damage stays on the server). Clients now see 1–4 of 6 nearby enemies mid-attack per sample |
| 3 | **Dead enemies kept fighting**: hundreds of hits by already-dead enemies in RC11 and RC12 logs, up to 20 s after death; two survivors were killed one second into an intermission | The template leaves the body and its behavior tree running; it can also queue one more swing after the killing blow | On death (server, once per life) an AI stops its brain, movement and montages (`UStatsComponent::StopDefeatedAI`); a defeated enemy cannot start an attack montage. After: 0 hits from dead enemies |
| 4 | **Horde heroes fought unarmed and could not block**: 59 block attempts, 0 accepted | Heroes spawn in `Stance 0` (unarmed); Duel heroes are in melee stance, Horde relied on the `1` key, which nothing tells a player | The server draws the weapon through the hero's own `SR_MeleeStance` when a player is ready, at each wave and after a respawn. After: 39 attempts → 8 blocks + 9 perfect parries; the hero alone killed 9 enemies in 150 s |
| 5 | **Enemies idled out of sight range**: 7–9 stalls per match; one match stuck 6 minutes | The template attacks only what its sight sense has seen; wave enemies spawn 12–16 m away and a dead target leaves them Passive | `UGOTAEnemyTargeting` drives the template's own `SetStateAsAttacking` / `SetStateAsSeeking`: acquire a reachable living player inside the leash (6,000 cm of the enemy and of its spawn point), reacquire when the target dies, drop a target it cannot close on for 8 s, recover after 4 path failures in 10 s, go home when nothing is reachable. After: 0–1 stalls per match, 0 move failures in the last run |
| 6 | **Surviving a wave was worse than dying**: survivors entered wave 2 at 30–80 health, the fallen respawned at 100 | Intermission respawned the dead at full health and left survivors damaged | **Approved rule (2026-11-17):** `bRestoreSurvivorsDuringIntermission` (on) restores survivors at the intermission; revisit only after human balance testing (partial healing, recovery resources or difficulty-specific rules are possible later) |

Ranged movement: the 342 / 229 `Invalid` failures came from one or two ranged enemies retrying `Move_To` toward a player they could not reach (enemy on the navmesh, no path). Recovery (5 above) moves them to the end of a partial path or home; the last healing-harness run had 0 movement failures and 0 stalls over 64 enemies.

### Spin fairness (editor, two players, healing harness, forced spins)

| Situation | Result |
|---|---|
| Stationary, two players together | at most 40 per spin across both (≤ 4 contacts per target); was 90 per target |
| Walking away when the spin starts | 3 hits (30) over three forced spins, against 22 hits standing |
| Blocking | spin contacts are blockable (`CanBeBlocked`), not parryable; the scripted hold-block tool cannot hold a timed block, so this was measured only through the fight driver's reactive blocks (19 accepted blocks / 7 parries in one full match) |
| Perfect dodge, remote defenders under latency | not separately measured for regular enemies; client bots evaded 7–14 spins/smashes per match once montages replicated |

Attack data (from the enemy Blueprint): sword slash 10 (blockable, parryable), jump attack 10 (blockable, parryable), spin 10 per contact (blockable, not parryable), ground smash 25 area (cannot be blocked or parried).

### Full match against fighting enemies (editor, four bots, no healing, nothing injected)

| Run | Change since previous | Result |
|---|---|---|
| `rc12_full_e1` | spin fix, targeting | wave 1 lost in 24 s, 0 kills |
| `rc12_full_e2` | driver: focus nearby damaged enemy, evade | wave 1 lost in 48 s, 1 kill |
| `rc12_full_e3` | enemy montages replicated | wave 1 lost in 67 s, 1 kill |
| `rc12_full_e4` | heroes armed (blocks work) | **wave 1 cleared** (35.5 s); wave 2 lost (two survivors killed by dead enemies in the intermission) |
| `rc12_full_e5` | dead enemies stopped | wave 1 cleared, 0 deaths; wave 2 lost: survivors entered at 30–80 health |
| `rc12_full_e6` | survivors restored at intermission | wave 1 cleared; **wave 2: 10 of 11 killed**, then wiped (score 2,050) |

Packaged RC12c, same rules: `rc12c_legit` cleared wave 1 and lost wave 2 at 9 of 11 kills (one Defeat, 0 client crashes).

Two more defects only the package showed, both fixed in RC12c: a stack overrun in every reflection call helper (`InitializeStruct` on a `ParmsSize` buffer wrote Blueprint locals past it; RC12's host crashed on the first wave-1 target), and the Horde boss freezing invincible after its Phase II roar because garbage collection unloaded its soft-referenced definition (reproduced with `obj gc`; RC12c keeps a hard reference on the server).

**A legitimate five-wave victory against fighting enemies was not achieved.** The bots are now the limit (they still stand in some attacks and do not retreat when low). With the healing harness the whole match runs: waves 1–5 cleared, the Quarry Behemoth spawned with `AIController_0`, attacked three different players (target switches), died, exactly one `outcome=Victory score=13875` (`rc12_ai_e2`). That proves progression, boss engagement and the victory path, not difficulty. Packaged RC12c repeated it (`rc12c_e2e`): one Victory, boss with 7 attacks and 4 target switches, and the rendered client stayed up 1,063 s after a real boss kill (0 crashes, 0 RHI errors): the RC10 renderer fix holds with fighting enemies.

### Renderer and network cost (packaged RC12c rendered client, waves 1–2, 120 s CSV)

Frame 17.5 ms (p95 22.2), render thread 17.3 ms, game thread 10.9 ms, RHI 5.3 ms, GPU 5.3 ms, 1.6 GB used; 18 frames over 50 ms and 4 over 100 ms (one 1.6 s). RC11 same scenario: frame 15.4, render 15.4, game 8.4. The rise is one run each on a loaded machine (13 GB free against 20 GB in RC11) with more enemies alive at once (enemies now fight in place instead of idling) and enemy montages now animating on clients; it is not attributed. The RC11 A/B (+1.4 ms render thread for `r.SkeletalMesh.UseCachedMDCs=0`) stands. Profile on a clean machine with Unreal Insights before release.

Replication (rendered client, same 120 s window, 120 ms / 5% loss): down 12.2 KB/s, up 4.0 KB/s (RC11: 8.2 / 3.0). The increase fits the new montage multicasts (unreliable, one per enemy attack start or interruption) and more enemies fighting at once. Packet loss recovery normal (lostOut 1–3).

---

## RC11: real combat validation (2026-11-16)

**Build:** packaged `B:\Unreal\GOTA_Builds\RC11_a8e6e87` (Win64 Development, fresh cook, 3,937 packages, 0 incrementally skipped) with every fix since RC9: the post-victory renderer setting, the Horde controller fix (wave enemies fight), the Story shrine guard, the ready-up phase and development telemetry. Four players on one machine, clients at 120 ± 30 ms and 5% loss. Telemetry report: `python Tools/GOTA_HordeTelemetry.py <tag>`.

### Ready-up before wave 1

Before RC11 the first player to log in (the listen host) started the 5 s prepare timer, so the host fought wave 1 alone while teammates were still joining.

- **Rule (server):** a player is ready once it has loaded the map, its client has acknowledged its pawn (`ServerAcknowledgePossession`), it has a hero and is alive. `AHordeGameMode::EvaluateReadiness` runs at each login and every second.
- **Who is expected:** the lobby passes its player count (`?Players=N`); a direct launch waits for 1; never fewer than are connected; at most four.
- **Start:** when all expected players are ready, the existing 5 s countdown runs. **Timeout:** 60 s (`?ReadyTimeout=`), then the match starts with whoever is ready. **Leaving:** a player who leaves before the start is no longer waited for.
- **Clients see it:** `AHordeGameState::ExpectedPlayers` / `ReadyPlayers`, `AHordePlayerState::bReadyForMatch`; the HUD shows `WAITING FOR PLAYERS r / n (seconds left)`.
- Not a new lobby framework: the existing session travel, phases and four-player cap are unchanged.

| Case | Build | Result |
|---|---|---|
| Four players join over 26–35 s | editor, packaged (`rc11_ready_all`, `rc11p_ready_wipe2`, `rc11p_aival_A`) | waited until 4/4 ready, then countdown; wave 1 started with 4 living every time |
| Four expected, two join | editor `rc11_ready_timeout` | timeout after 30 s, started with 2 of 4 |
| Four expected, one quits before the start | editor `rc11_ready_quit` | `left before the start; now waiting for 3`, started with 3 |

Also fixed: `HordePhase` never reached Victory or Defeat (it stayed `BossActive` / `WaveActive`), because `CompleteVictory` / `CompleteDefeat` set the match ended before the phase change and `TryTransition` then refused it.

### Enemies that fight back (packaged)

| Behaviour | Result | Evidence |
|---|---|---|
| Wave enemies get an AI controller | **pass** | `Horde spawned … controller=AIC_Enemy_Base_C_…` for every spawn |
| Detect and pursue players | **pass, with stalls** | enemies close to melee range and attack; but 7–9 enemies per match stood still 15–35 m from every player with no focus target (`TELEM stall`), see below |
| Attack in range | **pass** | melee attempts at 70–400 cm; hit rates per attack in the table below |
| Server-authoritative damage | **pass** | every hit goes through the victim's `BPC_DamageSystem` on the server; telemetry reads health before and after on the server |
| Change targets | **pass** | 3 of 15 enemies switched target in `rc11_aival_e1`; boss target switching not reached |
| Take hero damage and statuses | **pass** | 147 ability hits (editor), 59 Burning ticks on enemies (packaged `rc11p_aival_A`) |
| Die and award kill credit | **pass** | 15 kill credits, 0 team-only kills (`rc11p_aival_A`) |
| Natural team-wipe defeat | **pass** | `rc11p_ready_wipe2`: four fighting players killed 10–46 s into wave 1, exactly one `outcome=Defeat`, nothing injected |
| Horde boss with a controller | **not reached** | no run got past wave 2 against fighting enemies |

The enemy-behaviour runs used `gota.Net.Sustain` (test harness: players are healed, enemies untouched) so bots lived long enough to exercise deaths and kill credit. Those runs are not difficulty evidence.

### Measured difficulty (test bots, no healing)

| Run | Wave 1 | Player deaths, seconds into wave 1 |
|---|---|---|
| editor `rc11_ready_all` | defeat after 18 s, 0 kills, 40 hits on players | 8.5, 12.3, 12.3, about 17.6 |
| packaged `rc11p_ready_wipe2` | defeat after 46 s, 0 kills, 37 hits | median 10.2, mean 18.4 |

Per attack (packaged `rc11p_aival_A`, 302 hits on players in wave 1):

| Enemy attack | Attempts | Landed | Damage per landed attack |
|---|---|---|---|
| `Montage_Sword_SpinningAttack` | 25 | 24 | median 105, mean 127 (one target: 9 hits of 10 in 3.8 s) |
| `Montage_Melee_GroundSmash` | 30 | 29 | median 50 |
| `Montage_Sword_Slash` | 45 | 37 | 10 |
| `Montage_Sword_Jump_Attack` | 26 | 18 | 10 |
| `Montage_Rifle_Fire` (ranged) | 87 | 79 | 10 |

Heroes have 100 health. Enemies attack about 10 times a minute each. Enemy time-to-kill against the bots (first hit to death): melee median 45 s, ranged 68 s.

**Gameplay problems (not test-driver problems):**

1. **The spinning sword attack.** One wave-1 grunt's spin takes 90 of a hero's 100 health from a target that stays in it (9 damage ticks of 10 over 3.8 s), and it lands 24–26 times out of 25–26. Whether people can read and escape it is the first playtest question. Not changed.
2. **Enemies lose interest at range.** Enemies more than about 12–15 m from every player stand still with no focus target. In packaged `rc11p_ready_wipe` (host script not loaded, so the host stood still at spawn), the last living player was never approached and the match sat at 1 of 4 alive with 7 enemies for six minutes. A player who hangs back can stall a wave indefinitely. Perception or aggro range in the enemy AI; not changed.
3. **Ranged enemies fail to move.** `BP_Enemy_Ranged` logged 41 `Invalid` and 342 (editor) movement failures: their move requests fail outright, often 20 m from the players. Navigation for the ranged role needs a look.
4. Enemy damage does not scale with player count (only enemy count and concurrency do).

**Test-driver limits (not gameplay):** the bots stand in the spin, rarely block on remote clients, and swing at moving enemies. They lose wave 1 without healing; that says nothing about people.

### Not verified in RC11

- Human players (see `GOTA_HORDE_PLAYTEST.md`).
- A five-wave victory against fighting enemies, and the Horde boss with a controller.
- A real lobby with ready-up on separate PCs.

---

# Horde cooperative combat (RC6)

**Date:** 2026-11-07. **Branch:** `feature/gota-cinematic-frontend`. **Package:** fresh Win64 Development cook at `B:\Unreal\GOTA_Builds\RC6`. RC2–RC5 were not modified. Nothing was pushed.

This pass connects the existing fighter, ability, and Quarry Behemoth components to Horde. It does not add an ability framework or placeholder attacks. Health stays on `BPC_DamageSystem`. `UStatsComponent` remains the replicated mirror. Horde GameMode still owns waves, score, one victory, and one defeat.

## What changed

- `GOTACombatLive::FindCombatTarget`: living boss, else nearest living non-player with a damage system. Players are never targets.
- Fighter dodge/challenger ticks keep a living player opponent (Duel). Otherwise they take that combat target.
- Ability `GetOpponentPawn` uses the same fallback. Arc damage skips same-`TeamNumber` players.
- Divine Counter and `ValidateServer` reject a defeated owner.
- Horde finale class is `/Game/Bosses/BP_Boss_QuarryBehemoth`. If the spawned pawn has no boss component, `DA_Boss_QuarryBehemoth` is applied (900 health, 4 attacks).
- Each enemy spawn refreshes player fighters. It does not re-grant ability sets.
- `gota.Net.Fight <seconds>` (Development) drives the **local pawn only**: move toward the current target, Blueprint `MeleeAttack` inside 260 cm every 0.85 s, `RequestActivate` on the next configured slot inside 500 cm every 3 s. No kill injection, no momentum grant, no forced outcome.

The boss wave definition was not changed: 1 boss (score 2500, no extra per player) plus 2 grunts (score 100, +1 per extra player). With three living players that is 1 boss + 4 grunts, which matches the `enemies=5` line at boss start.

## Hero ability inventory

Source of truth: `Tools/GOTA_BuildHeroAbilities.py`, assets `DA_AbilitySet_*` on `DA_Hero_*`. Card text that is not implemented is noted. Input on the local controller: **E / R / C / V** and the D-pad map to slots 0–3 (`RequestActivate` → `Server_Activate`). **Q** and gamepad face-top are Divine Counter. Melee, block, and dodge stay on the existing Blueprint actions (`MeleeAttack`, `StartBlock`, `DodgeToLocation`). Ultimates cost 100 Divine Momentum. A hit grants +2 momentum. The server rejects cooldown, momentum, range, LOS, and death.

| Hero | Slot | Ability | Kind | Damage | Cooldown | Cost | Notes |
|---|---|---|---|---|---|---|---|
| Drakon (HP scale 1.15) | E | Molten Strike | Melee arc 300 / 150° | 22 | 6 s | 0 | Ignite DoT from the card is not implemented |
| | R | Titan Charge | Charge 750, unblockable, knock 900 | 14 | 10 s | 0 | |
| | C | Dragon Roar | Burst radius 400, knock 850 | 6 | 12 s | 0 | |
| | V | Unchained Fury | Burst radius 450, unblockable | 45 | 40 s | 100 | |
| Nyx (0.85) | E | Shadow Step | Teleport 650 | 0 | 7 s | 0 | Nav, sight, and collision checked |
| | R | Mind Spike | 3 hits × 9, range 260 | 9 | 7 s | 0 | Stagger, not a hard stun |
| | C | Void Shield | Veiled 1.3 s | 0 | 14 s | 0 | |
| | V | Soul Drain | Aura 450, Dread 4 s | 18 | 40 s | 100 | Energy steal from the card is a health drain |
| Varoth (1.05) | E | Inferno Slam | Burst radius 350, launch | 22 | 8 s | 0 | |
| | R | Ashen Charge | Charge 700, knock 950 | 16 | 11 s | 0 | |
| | C | World Burn | Aura 500, Dread 4 s | 5 | 16 s | 0 | One tick, not a DoT |
| | V | Colossus Form | Unbreakable 6 s, magnitude 0.5 | 0 | 40 s | 100 | Damage reduction and stagger immunity. The card's power/area bonus is not implemented |
| Kairos (1.0) | E | Time Lash | Aura 450, Slowed 3 s at 0.4 | 6 | 12 s | 0 | Attacks and abilities are not slowed |

Zephyr, Lysara, and Orion have sets and are not part of this four-player Horde roster.

## What was executed (not just attached)

`rc6fight` and `rc6fight2` on the RC6 package, port 7820 / 7821. Four processes. Host `-nullrhi`. Clients 1 and 2 rendered. Client lag 120 ms ±30, 5% loss. `gota.Horde.SkipPrep` only skipped the opening countdown.

Logged casts and hits include:

- Drakon: Molten Strike 22, Titan Charge 14, Dragon Roar 6, Unchained Fury denied until momentum existed, then the 45-damage burst.
- Nyx: Shadow Step teleport, Mind Spike 9×3, Void Shield Veiled, Soul Drain 18 after momentum was earned (`denied (Momentum)` before that).
- Varoth: Inferno Slam 22, Ashen Charge 16, World Burn 5 + Dread, Colossus Form `momentum-100`, then cooldown denials. Varoth's **client** died in `rc6fight2` (below); those casts are from the earlier `rc6fight` log and from the server pawn before the crash.
- Kairos: Time Lash 6 + Slowed, then cooldown denials.

Those hits are `LogGOTAAbility` lines with `landed=1` against `BP_Enemy_*` and, in the finale, `BP_Boss_QuarryBehemoth`. Kill credit lines name the player id (`Horde kill credit: DESKTOP-… +100/+125/+400/+2500`).

## Boss

`rc6fight2` at 22:51:22: `Boss QuarryBehemoth configured on BP_Boss_QuarryBehemoth_C_… (4 attacks)` and `Horde boss: Quarry Behemoth configured hp=900`. One configure. ClawCombo windows hit at body distance 140 (reach 260). Some windows landed on the host for 9 and 11 (`landed=1`). Some windows on Kairos's pawn returned `landed=0` (the damage system rejected them; they were not applied twice). The host's replicated health at the later dump was **33/100**. The boss is not host-only.

At 22:54:14 Nyx's player id received **+2500**, the boss score. Score went 7275 → 9775 with `defeated=1`, then +100 (`defeated=2`, score 9875). Phase stayed `BossActive`. Last life line: `enemies=3`, `living=3/3`, `outcome=None`. The Behemoth died to ability damage. The grunt adds that ship with the boss wave were still alive when the harness hit its timeout. That is not a victory.

Charge and Earthshaker did not run during this melee. No perfect parry, perfect dodge, or Divine Counter line was logged against the Horde boss. `gota.Net.RemoteDefend` was armed at client start for 400–500 s, and the boss arrived around minute 16, so the defend ticker had already stopped. RC5 still has the remote charge perfect-dodge and the Story Behemoth counter evidence. Those were not re-proven inside a Horde wave on RC6.

## Waves, wipe, and the four-player requirement

`rc6fight2` cleared five waves without `ForceClear`:

| Wave | Intermission | Score | Living |
|---|---|---|---|
| 1 | 22:37:08 | 650 | 3/3 |
| 2 | 22:39:40 | 1900 | 3/3 |
| 3 | 22:42:07 | 3250 | 3/3 |
| 4 | 22:48:02 | 5700 | 3/3 |
| 5 | 22:51:13 | 7275 | 3/3 |

Living was 3 because the rendered Varoth client (`rc6fight2_client2`) aborted at 22:36:12:

`Assertion failed: bPrevious` in `GPUSkinVertexFactory.cpp:1348` (`FGPUBaseSkinAPEXClothVertexFactory::ClothShaderType::GetClothBufferIndexForReading`), while hero cloth meshes (Countess, Gideon, Sevarog) were being applied. Process memory was about 1.7 GB. This is an engine cloth/skin assert on a second rendered window, not a gameplay disconnect.

`rc6fight3` (port 7823, fight armed for 1400 s) kept **4/4** and both rendered clients into wave 2, with the same lag settings, then stalled: after 23:11 the host kept casting Molten Strike, Titan Charge, and Dragon Roar at ~135 cm and Time Lash was on cooldown, but further `hit` lines stopped. At 23:19 the phase was still wave 2, `enemies=2`, score 1650, `outcome=None`. That run was stopped. It does not count as a five-wave clear.

`rc6wipe` (port 7822): four players stood still after SkipPrep. Wave 1 had four grunts. For the whole ~2 minutes, `living=4/4`, score 0, `outcome=None`. No `gota.Net.Hurt`. Wave-1 grunts did not down an idle team in that window. The boss **did** damage the host in `rc6fight2` (100 → 33). A full wipe and the between-wave respawn that follows a down were not observed.

Nobody died during the clears, so Horde's intermission respawn path was not exercised by this combat. Score and kill credit were read from the listen server. A separate client results-widget check was not captured.

Nullrhi host sample from the shorter `rc6fight` (waves 1–3, harness timeout while wave 4 was up): avg frame 9.62 ms, max 71.79 ms, GPU 0 (no RHI), 0 hitches. The RC4 359 ms fourth-login hitch was not traced with Unreal Insights.

## Implemented vs tested

| Feature | In the RC6 binary | Shown in a Horde log |
|---|---|---|
| Four selected heroes and their ability counts | Yes | Yes |
| Melee + the abilities in the table, including cooldown and momentum denies | Yes | Yes, including hits on the Behemoth |
| Same-team arc filter | Yes | No friendly-fire denial line was captured |
| Dead players cannot activate abilities or Divine Counter | Yes | No death occurred, so the deny path was not logged |
| Boss targets more than the host | Yes | Yes (host and Kairos pawns) |
| Claw damage, no double apply on a rejected window | Yes | Yes (`landed=1` and `landed=0`) |
| Charge / Earthshaker tells, claw parry, latency dodge, server Divine Counter | Existing boss/fighter code | Not observed in Horde |
| Five-wave progression and per-player kill credit | Existing Horde rules | Yes (`rc6fight2`) |
| Exactly one victory | Existing Horde rule (all finale enemies dead) | No. Boss died, adds remained, `outcome=None` |
| Exactly one defeat from enemy damage | Existing wipe rule | No. Idle team survived wave 1 |
| Downed respawn between waves | Existing intermission respawn | Not observed |
| 120 ms ±30 / 5% loss on the clients | Test setup | Yes, on the fight runs |
| Two rendered clients alive for the whole clear | — | No. `rc6fight3` kept them and stalled; `rc6fight2` crashed one |

## Wave 2 stall (`rc6fight3`)

The run was stopped on purpose. The exit code is that stop, not a crash. The server never stopped accepting casts.

Last damage was Nyx's Mind Spike on `BP_Enemy_Ranged_C_2147481993` at 23:11:44, the same second as that pawn's +125 kill credit. After that, Drakon kept being granted Molten Strike, Titan Charge, and Dragon Roar against `BP_Enemy_Ranged_C_2147482002`, and Nyx kept casting at `BP_Enemy_Melee_C_2147482045`. The only denials were cooldown, momentum, and Shadow Step `NoSafeDestination`. There is no rejected cast RPC and no "not live" denial.

The host's fight distance stayed 135 cm from 23:05 through 23:19. Charge reach is about 130 cm, and Dragon Roar / Molten Strike / Time Lash are all larger than 135 cm, so range, cooldown, and "untargetable" do not explain a total miss. Those two enemies had already taken hits (the ranged pawn four times, the last at 23:10:20; the melee pawn nine times, the last at 23:11:15). They were still alive and still had a damage system, which is why Horde kept counting them.

Varoth was the only pawn whose distance was still changing, and that client stopped sending attacks at 23:10:27 while its rendered frame rate fell to about one frame per second on `FlushAsyncLoading`. The other three fighters were already pinned. Straight-line `AddMovementInput` holds a pawn against quarry geometry; ability resolves then correctly refuse the hit because line of sight fails. That is the test driver, not a new networking fault and not a corrupted enemy.

`gota.Net.Fight` now strafes when the gap has not closed for a second and is still past 110 cm. A cast that hits nobody logs `missed nearest=… horiz vert los`. `gota.Horde.Watch <seconds>` logs `HORDE STALL` if a player stays within 500 cm and neither kills nor ability contacts advance. `mp_rc6stall_*.txt` runs that check in under a minute.

Editor retest `rc6stall` (uncooked, RC6 package not modified): four heroes joined, 32 ability hits, 3 kill credits, 1 miss at 496 cm with `los=1` (outside the roar), 0 `HORDE STALL`. A client logged `FIGHT strafe` when its gap stopped closing. No crash.

## Blockers

1. Finale victory requires the grunt adds to die after the Behemoth. The fight driver did not finish them before timeout, and a later four-player run stalled on wave 2 when casts stopped landing (likely LOS or a target the bots would not close).
2. Wave-1 grunts did not kill four idle players in two minutes, so a real team wipe is still unproven. Do not substitute `gota.Net.Hurt`.
3. Horde claw parry, perfect dodge, and Divine Counter need the remote defend ticker armed **while the boss is alive**. Starting it at join is too early.
4. A second rendered client can abort in `GPUSkinVertexFactory.cpp:1348` while several clothed hero meshes spawn. Four rendered windows remain unsupported on one GPU.
5. The 359 ms fourth-player hitch is still untraced. No Shipping cook. No second machine.

## RC6 regressions outside the Horde fight

| Run | Result |
|---|---|
| `rc6story` | Four Story logins from Host/Find/Join. Drakon plus three Nyx (the client script sets Nyx). Ability sets applied. The client script quits, and the later dump listed two pawns |
| `rc6rampage` | Scripted `gota.Boss.Attack` on Quarry Behemoth / Rampage: 2 perfect parries, perfect dodges on Earthshaker, Charge, and Grasp, Divine Counter stun, player stayed 130/130. Not a free fight |
| `rc6leave` | Client `gota.Net.Leave` returned to Main Menu. Host stayed up and went from 2 players to 1 |
| `rc6charge` | Failed. The lagged client joined a Duel-vs-AI match with no pawn, so RemoteDefend never dodged. The host took BehemothCharge for 18 (hp 52/130) |
| Duel PvP 1v1, HUD/respawn lifecycle, Unreal Insights, Shipping, second PC | Not run on this cook |

## Counts for this milestone

Horde combat checks that were actually run on RC6: **5 pass** (kits, five waves, Behemoth kill credit, boss damage to host and another pawn, kill credit per player), **3 fail** (one victory, one real wipe, four living players through a full clear), **4 not observed** (Horde parry, dodge, Divine Counter, intermission respawn).

Other RC6 packaged checks: **3 pass** (Story entry, scripted Rampage, client leave), **1 fail** (remote charge dodge setup), **5 not run** (Duel PvP, HUD/respawn, Insights, Shipping, second PC).

Component attachment alone is not in the pass count.
