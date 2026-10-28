# Gods of the Arena — Trial of Champions (PvP Design)

**Mode:** 1v1 Duel  
**Players:** 2 (host + 1)  
**Arena:** Titan's Quarry  
**Ruleset:** Best-of-3 (first to 2 round wins)

## Fantasy

Legendary warriors duel with God of War–precise defense and Space Marine–weighty strikes. Every round is a skill check: telegraphed attacks, committed recovery, readable parries.

## Match loop

1. Multiplayer → Host/Join → Champion Hall → Start Duel (`BestOf=3`)
2. WaitingForPlayers → PreparingRound → Countdown (3s, combat locked)
3. RoundActive (90s default) — elimination or timeout
4. RoundResolving → Intermission → next round, or MatchCompleted
5. Results → Rematch or Return to Lobby

## Rules (data-driven via `UDuelMatchDefinition`)

| Setting | Default |
|---------|---------|
| Best-of | 3 |
| Countdown | 3s |
| Round duration | 90s |
| Round end banner | 3s |
| Intermission | 2s |
| Mid-round respawn | Off |
| Timeout policy | Higher observed health wins; equal → unscored draw |
| Simultaneous KO | Unscored draw, continue |

## Combat authority (hard dependency)

- **Apply damage:** still `BPC_DamageSystem` (Blueprint).
- **Match elimination:** server uses pawn validity + `UCombatComponent::OnCharacterDied` (requires BP `NotifyDied` on lethal).
- **No second health system.** `UStatsComponent` stays observation (`ObserveHealth`) unless BP migrates.
- Perfect parry / perfect dodge / divine counters: **dependencies**, not claimed complete.

## Extension (later)

- **Milestone B:** 2v2 Gods versus Titans  
- **Milestone C:** 4-player Ascension FFA  

Do not start B/C until 1v1 PIE authority tests pass.

## Entry

- Console: `gota.Duel`
- Lobby Start Duel → travel with `DuelGameMode?BestOf=3`
