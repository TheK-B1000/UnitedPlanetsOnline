# Multiplayer combat animation visibility (2026-11-25)

**Defect.** On a client, no player fighter's combat animation played: not the other players' attacks, guards or hit reactions, and not even its own. Clients saw everyone only walk and run while the server ran the fight. Two UPA Warden players reproduced it, so it predates Ayla and affected every hero.

**Fixed** by extending the existing `UGOTAMontageReplicator`, which already solved the same problem for AI enemies. No combat rule changed: damage, hit windows, blocking, parry, perfect dodge, Divine Counter, cooldowns, GAS and the input and RPC paths are untouched. Enemy replication behaves exactly as before.

## Diagnosis

New test commands that run on any machine:

- `gota.Net.MontageTrace <s>` logs every montage start and stop on every character, with its network role: authority, owner or observer.
- `gota.Net.RpcFlags` lists the player pawn's and components' RPC events with their network flags.
- `gota.Net.Stance` prints each pawn's Blueprint `Stance` and whether it replicates.
- `gota.Net.LocalCall <Function>` calls a combat function on this machine's own pawn, as its input would.

With two UPA Warden players, host and client took turns attacking, blocking, hitting and blocking hits.

| Path | Finding |
|---|---|
| Host's own actions | Played on the server |
| Client's actions (input → `SR_` Server RPC, reliable) | Played on the server. Client input reaches the server correctly |
| Player Blueprint's own multicasts | `MC_MeleeAttack`, `MC_StartBlock`, `MC_ParryAttack`, `MC_EndBlock`, `MC_<stance>Stance`: real NetMulticast events (unreliable) |
| Same events on the client, round one | Arrived and did nothing. **The Blueprint `Stance` variable is not replicated** (`replicated=0`). The server held both fighters in melee stance (2) while the client held them unarmed (0). The handlers act only in the matching stance |
| Why stance differed | Each fighter is equipped (Duel `MeleeStance`; Horde `SR_MeleeStance`) once, when it spawns, through a single unreliable stance multicast. A fighter equipped before a client could see it (every round-one Duel fighter, Horde spawns) stayed unarmed on that client. After a round's respawn, with the client already connected, the host's attack did show |
| Hit reactions (`Montage_Sword_Hit`) | Played by the damage system on the server only. Never multicast for players, in any stance |
| Enemy replicator on player pawns | Attached to every combatant, but `ShouldReplicate` skipped players, on the assumption that the Blueprint's `MC_` events covered them |

## Fix (`UGOTAMontageReplicator`)

1. **Player montages replicate.** Each montage start on a player pawn, on the server, is multicast with a per-pawn event id (`MREP send #n` on the server, `MREP #n ... played` or `... already playing, skipped` on clients). A client already playing that montage skips it: the Blueprint's own multicast or the engine's root-motion replication got there first. So nothing plays twice, and interruption, priority and blending stay the Blueprint's. Interruptions replicate as before.
2. **Stance mirror.** The server mirrors the Blueprint `Stance` in one replicated value on the same component. A client whose own copy differs runs the Blueprint's own `MC_<stance>Stance` event locally, exactly what the lost multicast would have done (`STANCE ... local 0 -> server 2 via MC_MeleeStance`). The Blueprint's own attack and guard multicasts then work, along with their weapon, sound and locomotion cosmetics. At most three attempts per change.
3. **Unchanged.** Enemy rules: non-root-motion AI montages, bosses skipped. Authority is unchanged: only the server sends, and clients never request a montage. A client's input still goes only through the existing `SR_` server events.

## Verification

All figures are from the editor, on one PC. A "starved" client is one whose scripted 18 s wait took more than twice as long. A local LLM server (`llama-server`) was running during these sessions, and two runs with starved clients are reported as inconclusive, not as passes.

| Run | Result |
|---|---|
| Two UPA Warden, no added latency (`CombatVis`) | PASS. The client corrected both fighters' stance on join. It saw the host's attacks and guards and its own, plus hits and blocked hits. The server played everything from both players. Every montage started once |
| UPA Warden host, Ayla client (`CombatVisAyla`) | INCONCLUSIVE, client starved (18 s wait took 42 s). Every check except the hit-or-blocked-hit line passed |
| Two UPA Warden, 120 ± 30 ms and 5% loss, two runs | Client saw every attack (3+4 and 4+4 of the server's 7), every hit (1/1, 4/4), its own guards, and 1/2 and 2/2 of the host's guards (the missing one is most likely before the client's trace window opened). Both processes exited by themselves |
| Three players, Story (UPA Warden host; Ayla and UPA Warden clients) | Each client saw both other players' combos (9 and 7) and guards (6 and 6), its own (4 combos, 4 guards) and about 21 hit reactions. Ayla's combo showed her own clip on her screen and on the other client's |

**Full editor suite results.** None of these is yet a clean certification. During every run the GPU was at about 96% use with 10.4 of 12.2 GB of video memory taken by another process, with no Unreal process running, and host frames stalled up to 15 s.

| Run (code) | Passed | Failed | Inconclusive | Notes |
|---|---|---|---|---|
| First full pass, before the fix (Ayla checkpoint code) | 80 | 5 | 2 | Kept as recorded. Failures were F-milestone multiplayer cases plus Ayla cases, with clients starved and host frames up to 15 s |
| Rerun of the affected groups, before the fix, quieter machine | StatusHUD, StatusHUDDeath, StatusFX, Feedback, AylaMP, Ayla perfect dodge: pass | Ayla combat: drift line cut off (script shorter than the 18 s watch). AylaHorde: host killed before the clients joined (test setup) | | Both test scripts fixed |
| First full pass with the fix | 78 | 4 | 7 | All 7 inconclusive are starved clients (18 s waits took 42–53 s). All 4 failures are boss powers that were never released (see below) |
| Rerun of failing and inconclusive groups, with the fix | 9 | 6 | 3 | Same load. Explained below |

**The failures, explained from their logs:**

- **Boss power not released (EnemyPower, Ayla perfect dodge).** This predates the fix and depends on frame stalls. `UGOTABossComponent` resolves an attack only while its clock is inside the hit window (Molten Boulder 0.95–1.00 s, plus 0.05 s). A single frame that jumps past the end counts as a clean miss, so a projectile or hazard is silently never released. Seen with Drakon too, before the fix. The fix does not touch bosses: `ShouldReplicate` returns false for them, as before. Not changed here, because the boss component is a preserved owner. It is the same class of bug milestone E fixed for Titan Charge.
- **Frozen refused (Control and Feedback, multiplayer).** The server logged `Frozen resisted (whole control)`: the target was still inside its 2 s recovery immunity after the Stun, because the scripted 4.5 s gap stretched under host stalls of up to 7 s. Timing, not a rule change.
- **StatusHUD.** A target chip's first-seen value missed its expected 2.5–3.0 s range under load. Previously passed on a quiet machine.
- **Ayla combat guard.** Her block montage was immediately replaced by the block reaction (an AI hit on the guard in the same frame), so her body showed her parry clip and never logged the block mapping. The check now accepts either clip for her guard.

**Clean machine (2026-11-25 afternoon).** The other GPU workload was stopped: the GPU was at 0% and 1.3 GB, and the CPU idle.

| Run (same fix code) | Passed | Failed | Notes |
|---|---|---|---|
| Full editor suite, first pass | 87 | 2 | No inconclusive case; clients ran at normal speed. Both failures were game processes that received `Engine exit requested (reason: ConsoleCtrl RequestExit)` within seconds of launch (UBT inside the same process: exit `0xC000013A`, Ctrl+C), so nothing was tested: StatusFX multiplayer (client never joined) and Defense "Shielded" (no gameplay logged) |
| Those two groups rerun, with the harness fix below | 4 | 0 | StatusFX multiplayer and all three Defense cases |

**Console exit (test harness).** The regression runner and `GOTA_MPSession.py` started the game processes in their own console's process group. A Ctrl+C or close event sent to that console reached them, and they quit at startup. Both scripts now start each game with `CREATE_NEW_PROCESS_GROUP`. The same `ConsoleCtrl RequestExit` signature is in milestone E's packaged Titan Charge client log (`Saved/AbilityRegression/e8/packaged_TitanCharge_mp_client.log`), the "console exit request of unknown source" recorded in `GOTA_GAS_ENHANCEMENT_PLAN.md`. That run was most likely the same harness effect, not a game fault.

## Known limits

- **Unreliable delivery.** Both the Blueprint's multicasts and the replicator's are unreliable, so under heavy packet loss an occasional animation can be lost (one dropped guard is possible in the lag runs above). Gameplay is unaffected: it is decided on the server.
- **Extra local swing.** In the three-player run each client counted one more of its own combos (4) than the server ran for it (3). This is most likely a swing played locally by the Blueprint's input path before the server accepted the first attack. That behaviour predates this fix and is not changed by it.
- **Not checked.** No human session, no separate PCs, and no packaged multiplayer visibility run unless recorded with the package results.
