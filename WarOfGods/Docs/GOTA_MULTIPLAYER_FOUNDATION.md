# Multiplayer Foundation — Status & Playtest

## Audit follow-up ([Audit multiplayer session stack](fe515f65-4df8-49a1-97e7-804994ca56b6))

The audit correctly found legacy BP authority (`CreateMPSession`, `JoinMpSession`, MenuController `DestroySession`, Find on `WBP_MainMenu`). Current C++ already has `UGotaSessionSubsystem` + Null OSS in `DefaultEngine.ini`. Follow-up fixes:

- **Do not** reflection-call GI `FindSessions` (that event is on `WBP_MainMenu`) — Null OSS find path only  
- Join BP name: try **`JoinMpSession`** then `JoinMPSession`  
- Destroy: try **MenuController** `DestroySession` first  
- Explicitly enable **OnlineSubsystemNull** in `.uproject`  
- Invites / matchmaking remain unavailable (Impl 08 Later)

## Honest status

| Feature | Status |
|---------|--------|
| Host (LAN / Null OSS + BP `CreateMPSession` fallback) | Wired via `UGotaSessionSubsystem` |
| Find Sessions | Null OSS LAN search (not GI) |
| Join (first found / BP `JoinMpSession`) | Wired; join-by-address supported |
| Friend invites / matchmaking | **Unavailable** (not fabricated) |
| Champion Hall projection | Refresh from session + GameState `PlayerArray` into `UGOTALobbyViewModel` (not authority) |
| 1v1 Duel (Titan's Quarry, best-of-3) | `ADuelGameMode` / `ADuelGameState` / `ADuelPlayerState` |
| Skirmish | Preserved (`AArenaGameMode` default) |
| PvE Horde | Preserved (`AHordeGameMode`) |
| Server-auth damage migration | Contract documented; BP `BPC_DamageSystem` still owns health |

## Recommended playtest (smallest complete path)

### A) PIE Listen Server (always works)

1. Open `ThirdPersonMap`.
2. Play → Net Mode: **Listen Server** → **2 Players**.
3. Console: `gota.Duel`  
   Or travel: `open ThirdPersonMap?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?listen`
4. Wait for both clients → countdown → fight.
5. Round ends when one pawn is destroyed/invalid; first to **2** round wins takes the match.

### B) Menu Host path

1. PIE / standalone → `L_GOTA_MainMenu`.
2. Multiplayer → **Host Game** → Champion Hall.
3. Select Arena → Titan's Quarry (defaults Duel mode).
4. **Start Duel** → ServerTravel to Quarry with `DuelGameMode`.
5. Second player: Find → Join, or PIE client, or `open <hostip>`.

## Architecture notes

- `UGOTALobbyViewModel` = UI projection only.
- `UGotaSessionSubsystem` = session create/find/join + match travel options.
- Duel elimination mirrors Horde: server polls pawn validity during `InRound` (does not invent a second damage system).
- Observer `UCombatComponent` remains local presentation; replication is GameState/PlayerState.

## Remaining limitations

- Ready flags are local-projected until a replicated lobby PlayerState is added for the menu map.
- Find/Join quality depends on Null OSS LAN; packaged Steam/EOS not configured.
- BP damage authority not yet replaced — deaths that leave the pawn alive won't score a round.
- Arena selection file may need a manual re-save if locked during tooling; Host→Start still defaults Quarry Duel.
