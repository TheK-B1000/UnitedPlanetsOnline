# Combat Milestone 01 — Test Results (Trial of Champions vs Sovereign Champion)

**Date:** 2026-10-30.
**Build:** WarOfGodsEditor Win64 Development, UE 5.8.
**How:** standalone `-game` (and `?listen`) runs driven by `gota.Test.Run` scripts in `Tools/Tests/`. Logs: `LogGOTADuelAI`, `LogGOTATest`.
**Bot:** the "challenger" in the automated matches is the test-only `gota.Duel.Bot`.
- It plays through the same Blueprint functions as player input (`MeleeAttack`, `StartBlock`, `DodgeInDirection`, Q-path `RequestDivineCounter`).
- It reads montage timing, so its play is near-perfect. Its results are an **upper bound** of player skill, not a human playtest.

## Gate results

| Gate | Test | Result | Evidence |
|---|---|---|---|
| 1 — AI fights the player | `duel_gate1*.txt` | **PASS** | Champion engages, lands BP damage, wins rounds vs an idle Warden |
| 2 — Health / death / rounds | all match runs | **PASS** | KO ends rounds, best of 3, results screen, rematch/return (earlier frontend tests) |
| 2 — Blocks work | `duel_block_hold.txt`, `combat_parry_dodge.txt` | **PASS** | Blocked slashes cost 0 HP; `Montage_Shield_Block_React` plays; +4 momentum each |
| 2 — Unblockable attack | same | **PASS (by design)** | GroundSmash (`CanBeBlocked=false`, 25 dmg) goes through a block, announced "UNBLOCKABLE — EVADE" |
| 3 — AI defence | `combat_fight_short.txt` | **PASS** | `Champion read wind-up [Montage_SwordCombo:Default] … -> guard / sidestep / let it through`; guard raised and blocked; sidesteps |
| 3 — Weak point (shield overload) | same + forced | **PASS** | Natural: "shield pressure 3/3 → guard broken 2.5s"; Divine Counter on a broken guard dealt 35 (×1.4) |
| 4 — Perfect parry | fight-bot runs | **PASS** | `+20 (perfect parry)`; BP riposte damages champion |
| 4 — Perfect dodge | `combat_parry_dodge.txt` | **PASS** | `Dodge i-frames … lead=0.20` → `+15 (perfect dodge)`, no damage taken |
| 5 — Feel | `combat_showcase.txt` screenshots | **PASS (visual)** | Sparks, callouts (GUARD BROKEN, DIVINE COUNTER), hit-stop/camera impulse in code; screenshots `71_guard_broken.png`, `72_divine_counter.png` |
| 6 — Divine Counter | all runs | **PASS** | `DIVINE COUNTER … 25 damage (landed=1)`; denied with HUD reason when < 50 momentum |
| 6 — Sovereign Surge | forced + natural | **PASS** | `Sovereign Surge telegraphed (1.1s)` → `released (GroundSmashAttack)` |
| 7 — Phase II at 50 % | all runs | **PASS** | `Sovereign Champion enters PHASE II` → ascension slam |
| 7 — Difficulty select | menu run `hero_select.txt` | **PASS** | Travel URL carries `?Difficulty=Veteran`; server logs the Veteran profile |
| 8 — Listen server (host) | `?listen` match | **PASS** | Director logs `[net ListenServer]`; full match completed 2–0, no errors |
| 8 — Remote client | — | **NOT TESTED** | Needs a second machine/instance joining an AI duel; not run |

## Difficulty matrix (fight bot, full best-of-3 matches)

| Difficulty | HP (Warden vs Champion) | Rounds (winner, length) | Match | Perfect parry | Counters | Guard breaks | Phase II | Surges | Champ guards / sidesteps |
|---|---|---|---|---|---|---|---|---|---|
| Recruit | 160 vs 190 | W 23 s, W 26 s | 2–0 | 3 | 3 | 2 | 2 | 0 | 5 / 0 |
| Veteran | 130 vs 260 | **Champion** 33 s, W 44 s, W 31 s | 2–1 | 9 | 7 | 1 | 3 | 3 | 12 / 7 |
| Ascendant | 110 vs 340 | W 44 s, W 42 s | 2–0 | 8 | 5 | 0 | 2 | 2 | 7 / 6 |
| Veteran `?listen` | 130 vs 260 | W 32 s, W 33 s | 2–0 | 5 | 3 | 0 | 2 | 2 | 5 / 3 |

No ensures or errors in any run.

Reading:
- The near-perfect bot wins most rounds, yet Veteran took a round off it.
- A human, with imperfect parry timing, should see longer and harder rounds.
- **Not yet done:** human playtesting. That's the next tuning input.

## Known issues / not verified

- **Human playtest:** none yet. All tuning is from bot runs.
- **Remote client:** a client joining a Fight AI duel is untested. A replicated mirror, multicast cues and a server RPC are in place.
- **Hold-to-block:** the original block montage ends after 1.38 s even if RMB is held. Optional loop-section fix in `GOTA_AI_DUEL_DESIGN.md` §5.
- **Jump attack:** `Montage_Sword_Jump_Attack` has a launch notify at 0.06 s. Perfect-dodge timing against it can key off the launch instead of the landing hit.
- **Packaged build:** not tested.
