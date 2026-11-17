# Horde human playtest: waves 1 and 2

**Build:** packaged RC12c (`B:\Unreal\GOTA_Builds\RC12c_05704ae`, Win64 Development). **Players:** 2 to 4 people. **Time:** about 45 minutes for three sessions plus the questionnaire.

The purpose is to learn whether wave 1 and wave 2 are readable, fair and fun against enemies that now fight back. A playtest is not a pass/fail test, and no balance number changes until its data and answers have been reviewed (see "After the playtest").

No result in this document is a human result until real people have played. The bot runs in `GOTA_GAS_ENHANCEMENT_PLAN.md` say nothing about human difficulty.

## Before the session

1. Copy the RC11 archive to each PC (or run several copies on one PC for a first pass; real separate PCs are better for latency).
2. Delete any old `Windows\WarOfGods\Saved\Logs\WarOfGods*.log` on the host so the session's log is the only one.
3. Telemetry is on by default in Development builds (`gota.Telemetry 1`). Nothing to enable. It is not in Shipping.
4. Decide the hero for each player and write it down. For the first session, use the heroes people would pick themselves.
5. Optional latency on one machine: open the console (`~`) on a client and run `NetEmulation.PktLag 120`, `NetEmulation.PktLagVariance 30`, `NetEmulation.PktLoss 5`. Write down who used it.

## Starting a match

Use **RC12c** (`B:\Unreal\GOTA_Builds\RC12c_05704ae`), not RC11: RC12c has the fixes the bot runs exposed (visible enemy attacks for remote players, armed heroes who can block, no damage from dead enemies, the boss fix).

1. Host: **MULTIPLAYER > HOST GAME**. In the lobby, press **MODE: TRIAL OF CHAMPIONS** once: it switches to **MODE: WRATH OF THE GODS**.
2. Everyone else: **MULTIPLAYER > FIND GAMES**, select the host, **JOIN GAME**.
3. Pick heroes, then the host presses **START HORDE**. The lobby passes the player count to the match (`?Players=N`).

Fallback (Development console `~`): `gota.Menu.Session mode Horde` then `host` / `find` / `join` / `start`, and `gota.Menu.Session hero <Hero>`.
4. In the arena the HUD shows **WAITING FOR PLAYERS r / n** until everyone has loaded and has a hero. Then a 5 s countdown starts wave 1. If someone never finishes loading, the match starts after 60 s with whoever is ready. Note if that happens.
5. Do not use `gota.Horde.SkipPrep`, `gota.Net.Sustain` or any other test command during a playtest.

## Sessions

| # | Players | What to do | Stop when |
|---|---|---|---|
| 1 | All (2–4) | Play normally. No instructions about tactics. | Wave 2 is cleared, or the team is wiped |
| 2 | All | Same heroes. Before starting, tell the group only this: "Enemies telegraph their big attacks; watch for the spin." | Wave 2 cleared or wiped |
| 3 | All | Players swap heroes. | Wave 2 cleared or wiped |

An observer (not playing) notes, with the match clock:

- Every player death and what killed it (spinning sword, ground smash, rifle, several enemies at once).
- Moments when a player said they did not see an attack coming, or did not know why they died.
- Whether players stayed together or split up, and whether anyone helped a teammate.
- Any rubber-banding, teleporting or delayed hits a player mentions.

## Collecting the data

After each session, on the host:

1. In the console run `gota.Telemetry.Dump` (writes the running summary).
2. Quit to the main menu. Copy `Windows\WarOfGods\Saved\Logs\WarOfGods.log` to `Saved\Playtests\<date>_<session>.log` in the project.
3. Report: `python Tools/GOTA_HordeTelemetry.py Saved/Playtests/<date>_<session>.log`

The report gives, per session: how long the ready wait took, each wave's duration and deaths, each player's hits taken, damage, deaths and life length, every enemy attack type with its hit rate and damage per landed attack, enemy time-to-kill, attacks per minute, simultaneous attackers, movement failures and stalls.

## Questionnaire (each player, after session 3)

Scale 1–5 unless noted.

1. Wave 1 difficulty: 1 trivial, 3 about right, 5 impossible. Wave 2 the same.
2. Could you tell an enemy attack was coming in time to block or dodge? (1 never, 5 always.) Which attack was hardest to read?
3. When you died, did you understand why? (1 never, 5 always.)
4. Did deaths feel fair? (1 never, 5 always.)
5. For each of your hero's abilities: did it matter? (1 useless, 5 essential.) Which ability did you use most, and which never?
6. Did the controls feel responsive? (1 laggy, 5 instant.) If you had emulated latency, did it change anything?
7. Did working with your team matter? (1 no difference, 5 could not win alone.)
8. Did you want to play wave 3? (yes / no / why)
9. Free text: the one change you would make first.

## Changed in RC12 (tell the observer, not the players)

- **Spinning sword attack:** the wind-up (1.2 s before the first hit), animation, radius (3 m) and damage per contact (10) are unchanged, but the spin now has 4 contacts (one slash at 1.2 s, then area pulses at 2.0, 3.25 and 4.5 s) instead of 9 every 0.5 s. One spin can take at most 40 health from a target that stays in it (was 90). It can be blocked (not parried), dodged, or walked out of. Watch whether players read the wind-up and step out.
- **Enemies keep fighting:** wave enemies now always pick a reachable player inside the arena instead of standing still out of sight range. A player who hangs back will be hunted.
- **Ground smash:** unchanged; 25 damage in a 3 m area, cannot be blocked or parried. Watch whether players read it.
- **Heroes start with their weapon drawn** (they used to start unarmed in Horde and could not block). Blocks and perfect parries now work.
- **Remote players now see enemy attack animations** (before, only the host did). Ask remote players whether attacks are readable.
- **Survivors are healed to full at the intermission** (the fallen already came back at full health). Ask whether this feels right; it is a rule change awaiting sign-off.

## What we already know from the bot runs (to check with people)

These come from RC11 telemetry with test bots, and are questions for the playtest, not conclusions:

- One `Montage_Sword_SpinningAttack` from a wave-1 grunt lands 9 hits of 10 on one target in 3.8 s: 90 of a hero's 100 health. Bots never escape it. Can people see it and get out?
- `Montage_Melee_GroundSmash` landed 26 of 27 times on bots at about 50 damage per landed attack.
- With no healing, four bots died 12–28 s into wave 1. Four players who do nothing die 29–40 s in.
- Enemy damage does not scale with player count; enemy count and concurrency do.

## After the playtest

Bring the logs, reports and questionnaires together before any change. Write down for each proposed change the data that supports it (for example "4 of 4 players rated the spin 1–2 for readability, and 11 of 14 deaths were the spin"). Possible directions, none decided:

- Readability before numbers: a stronger telegraph, wind-up or sound for the spin and the smash.
- Spin: fewer damage ticks, lower damage per tick, or a hit cap per target per attack.
- Wave 1 concurrency for small teams.
- Enemy damage scaling by player count.

Do not tune against the test bots.
