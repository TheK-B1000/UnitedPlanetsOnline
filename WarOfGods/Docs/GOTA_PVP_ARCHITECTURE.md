# Trial of Champions — Technical Architecture

## Classes

| Class | Role |
|-------|------|
| `ADuelGameMode` | Server match lifecycle, timeout, spawn/reset, combat enable, death bind |
| `ADuelGameState` | Replicated phase, scores, deadlines, `bCombatEnabled`, outcome |
| `ADuelPlayerState` | Slot, wins, elim/deaths, ready, Server rematch/lobby RPCs |
| `UDuelMatchDefinition` | DataAsset rules (best-of, durations, timeout policy) |
| `IDuelFighterInterface` | BP round heal + combat gate without C++ health ownership |
| `ADuelHUD` / `UGOTADuelHUDWidget` / `UGOTADuelResultsWidget` | Presentation |

Skirmish / Horde remain on their own GameModes.

## State machine

`WaitingForPlayers → PreparingRound → Countdown → InRound → RoundEnd → Intermission` (repeat)  
`→ MatchEnd → ReturningToLobby`

Guards: `bMatchComplete`, `bRoundResolving` prevent double EndRound / late death after resolve.

## Combat bridge (smallest)

1. On spawn/reset: bind `UCombatComponent::OnCharacterDied` → `HandleCombatantDied`.
2. On death event: mark eliminated, `Destroy` pawn if still valid, `EvaluateRound`.
3. Poll (0.25s) remains safety net for Destroy-without-NotifyDied.
4. Round reset: `IDuelFighterInterface::ResetForDuelRound` + `Stats->ObserveHealth(Max, Max)`.

**BP editor checklist (required for fair MP):**

- `BPC_DamageSystem` server branch: after HP change → `ObserveHealth`; on lethal → `NotifyDied` then Destroy/Disable pawn.
- Optional: implement `IDuelFighterInterface` on `BP_Player` for full heal/stamina/weapon reset.

## Network

- Listen server, max 2 for this mode.
- Clients never decide round winners; they display GS replication.
- Rematch / lobby: owning client → `ADuelPlayerState` Server RPCs.
- Disconnect during Countdown/InRound/Preparing → opponent match win.

## Launch

```
gota.Duel
# or
OpenLevel ThirdPersonMap?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3?listen
```
