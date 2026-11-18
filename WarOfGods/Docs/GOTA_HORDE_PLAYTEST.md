# Horde human playtest: waves 1 and 2

**Build:** packaged RC14 (`B:\Unreal\GOTA_Builds\RC14_e0f794f`, Win64 Development; one perfect-dodge rule for host and remote players). **Players:** 2 to 4 people, ideally a Drakon, a Nyx, a Varoth and a Kairos. **Time:** about 60 minutes for three sessions plus the questionnaire.

The purpose is to learn whether waves 1 and 2 are readable, fair and fun against enemies that fight back, and whether blocking and dodging work for people (including remote players). It is not a pass/fail test, and no combat number changes until its data and answers are reviewed.

**Human evidence only.** Every session is recorded with the session sheet below. A session counts as a human playtest only if every hero was controlled by a person with keyboard/mouse or gamepad, and no test command (`gota.*`) was used during play. Bot runs (`gota.Net.Fight`, `gota.Net.Sustain`) are recorded elsewhere (`GOTA_HORDE_COOP_COMBAT.md`) and are never quoted as human results.

## Current rules (approved)

- **Survivors are healed to full at each intermission**, like players who died and respawn (`bRestoreSurvivorsDuringIntermission = true`, approved 2026-11-17, subject to future balance testing).
- Heroes start each wave with their weapon drawn; blocks, perfect parries and perfect dodges work against wave enemies.
- Enemy attacks are visible to every player (remote players included).
- Enemy damage, health and counts are unchanged since RC11 except the spinning sword's damage windows (below).

## What players will face

| Enemy attack | Damage | Tell | Answer |
|---|---|---|---|
| Sword slash | 10 | short swing | block, parry, dodge, step back |
| Jump attack | 10 | leap from up to ~4 m | block, parry, dodge |
| Spinning sword | 10 per contact, up to 4 contacts (40) in 3 m | 1.2 s wind-up, then contacts at 1.2, 2.0, 3.25, 4.5 s | block (not parry), dodge, walk out |
| Ground smash | 25 in 3 m | overhead wind-up, ~1.2 s | **cannot be blocked or parried**: dodge or walk out |
| Rifle (ranged) | 10 | aim, shot | block, dodge, cover |

Heroes have 100 health. Wave 1: 4 grunts + 1 per extra player. Wave 2 adds ranged enemies.

## Before the session

1. Copy the RC14 archive to each PC. Separate PCs on one network are best (real latency). One PC with several windows works for a first pass.
2. On the host, delete old `Windows\WarOfGods\Saved\Logs\WarOfGods*.log`.
3. Telemetry is on in Development builds (`gota.Telemetry 1`); nothing to enable.
4. Fill in the session sheet header (players, heroes, PCs, network).
5. Optional on one-PC setups: a remote player may add latency from the console (`~`): `NetEmulation.PktLag 120`, `NetEmulation.PktLagVariance 30`, `NetEmulation.PktLoss 5`. Write it on the sheet.

## Starting a match (public menu, no console)

1. Host: **MULTIPLAYER > HOST GAME**. In the lobby press **MODE: TRIAL OF CHAMPIONS** once: it becomes **MODE: WRATH OF THE GODS**.
2. Everyone else: **MULTIPLAYER > FIND GAMES**, select the host, **JOIN GAME**.
3. Everyone picks a hero; the host presses **START HORDE**.
4. The HUD shows **WAITING FOR PLAYERS r / n** until everyone has loaded; then a 5 s countdown starts wave 1.

## Sessions

| # | What to do | Stop when |
|---|---|---|
| 1 | Play normally. No instructions about tactics. | Wave 2 cleared or team wiped |
| 2 | Same heroes. Tell the group only: "Block or dodge just before a hit lands; the ground smash cannot be blocked." | Wave 2 cleared or wiped |
| 3 | Players swap heroes. If wave 2 was cleared twice, continue as far as the team gets (a full match is welcome; note it). | Wipe or victory |

An observer (not playing) writes down, with the match clock:

- Each player death: what killed it (slash, jump, spin, smash, rifle, several enemies) and whether the player saw it coming.
- Each time a player said an attack was unreadable, a block or dodge "should have worked", or a hit felt delayed.
- Whether players stayed together, revived the tempo after the intermission, helped a teammate.
- Rubber-banding, teleporting, frame drops.

## Collecting the data

After each session, on the host: console `gota.Telemetry.Dump`, quit to the main menu, copy `Windows\WarOfGods\Saved\Logs\WarOfGods.log` to `Saved\Playtests\<date>_<session>.log`, then:

`python Tools/GOTA_HordeTelemetry.py Saved/Playtests/<date>_<session>.log`

The report gives each wave's duration and deaths; **each player, each wave**: damage in and out, hits taken, blocks, parries, dodges, perfect dodges, the avoided ratio (blocks + parries + perfect dodges over everything that reached them), deaths and time to first death; each enemy attack type's hit rate and damage; enemy time-to-kill; dodges seen by the server split host / remote.

## Session sheet (one per session)

```
Date / session #:            Build: RC14_e0f794f
Evidence type: HUMAN         (every hero player-controlled, no gota.* commands during play: yes / no)
Players and heroes:          P1 ____ (host, hero ____)  P2 ____ (hero ____)  P3 ____  P4 ____
PCs / network:               one PC / LAN / internet; emulated latency on: ____
Result:                      wave reached ____ ; outcome ____ ; deaths ____
Observer notes:              (deaths and causes, unreadable attacks, failed defences, network issues)
Telemetry report saved as:   Saved/Playtests/____
```

## Questionnaire (each player, after session 3)

Scale 1–5 unless noted.

1. Wave 1 difficulty (1 trivial, 3 about right, 5 impossible). Wave 2 the same.
2. Could you tell an attack was coming in time to block or dodge? Which attack was hardest to read?
3. When you blocked or dodged at the right moment, did it work? (1 never, 5 always.) Any attack where it did not? Did you ever dodge and still get hit by that same attack? Host and remote players: did dodging feel the same?
4. When you died, did you understand why? Did deaths feel fair?
5. Each of your hero's abilities: did it matter? (1 useless, 5 essential.) Most used / never used?
6. Responsiveness (1 laggy, 5 instant). Remote players: did attacks and your dodges feel in sync?
7. Did working as a team matter? (1 no difference, 5 could not win alone.)
8. Full healing between waves: right, too generous, or too punishing?
9. Did you want to keep playing? Why or why not?
10. Free text: the one change you would make first.

## Bot baseline (for comparison only, not human evidence)

RC12c–RC14 automated runs with four test bots (`gota.Net.Fight`, no healing): wave 1 cleared, wave 2 often lost at 6–10 of 11 kills; best run (packaged RC14) cleared waves 1–3 and lost wave 4. Bot dodge check (RC14, healing harness, every lead from 0.10 to 0.35 s): no armed dodge, host or remote, was hit by the attack it dodged. These describe the test bots, not people.

## After the playtest

Bring sheets, reports and questionnaires together before any change. Each proposed change cites its data (for example "3 of 4 players rated the ground smash 1–2 for readability; 6 of 9 deaths were smashes"). Possible directions, none decided: stronger telegraphs (sound, VFX) before numbers; spin or smash timing; wave 2 concurrency for small teams; enemy damage scaling by player count; partial instead of full intermission healing. Do not tune against the test bots.
