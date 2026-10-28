# Gods of the Arena — Wrath of the Gods (PvE Horde)

**Mode:** Wave Survival  
**Players:** 1–4 (host + 3)  
**First arena:** Titan's Quarry  
**Target:** 5 regular waves + boss finale

## Fantasy

Space Marine 2-style escalating pressure with Gods of the Arena melee identity: parry, stagger, divine momentum, and a climactic Sanctuary Guardian (existing `BP_Enemy_Boss`).

## Match loop

1. Lobby / `gota.Horde` / Arena Selection → travel with `HordeGameMode`
2. WaitingForPlayers → Preparing (server deadline countdown)
3. Waves 1–5 with intermissions (respawn eliminated players)
4. Boss finale
5. Victory / Defeat → results → Replay or Return to Lobby

## Wave design (runtime defaults if no DataAsset)

| Wave | Composition | Intent |
|------|-------------|--------|
| 1 | 4 melee | Rhythm |
| 2 | 3 melee + 2 ranged | Positioning |
| 3 | 4 melee + 2 ranged | Pressure |
| 4 | 3 melee + 2 ranged + 1 elite | Champion check |
| 5 | 5 melee + 3 ranged | Endurance |
| Boss | `BP_Enemy_Boss` + 2 melee adds | Finale |

Counts scale with extra players (`CountPerExtraPlayer`), not raw health multipliers.

## Scaling policy

- Enemy count and concurrent cap grow per additional player
- Boss count stays 1; adds may scale lightly
- Disconnect ≠ combat elimination
- Team wipe (zero living pawns during WaveActive/BossActive) → Defeat

## Entry points

- Console: `gota.Horde`
- Frontend: PvE Arena → select Titan's Quarry → opens map with `game=/Script/WarOfGods.HordeGameMode?Mode=Horde`
- URL options: `Mode=Horde`, `Arena=Quarry`

## Out of scope (this milestone)

- Endless / Gauntlet / Boss Rush / Multi-Biome Conquest
- Friend invites / matchmaking
- Persistent currencies / online leaderboards
- Perfect-parry score bonuses (until server-validated)
- Claiming Ashfall/Frost/etc. Horde-ready without spawn+nav verification
