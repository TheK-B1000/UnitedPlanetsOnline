# Horde cooperative combat

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
