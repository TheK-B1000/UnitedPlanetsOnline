# Gods of the Arena — Cinematic Frontend

**Branch:** `feature/gota-cinematic-frontend`  
**Engine:** UE 5.8  
**Concept:** `Docs/Reference/GOTA_MainMenu_Concept.jpg`

## Recovery

- Existing lobby Blueprints under `Content/MainMenu/` were **not deleted**.
- `L_GOTA_MainMenu.umap` was seeded as a copy of `MainMenu.umap` (same GM/UI host). Dress it in-editor; do not overwrite `MainMenu.umap` until the new flow is verified.
- Rollback: switch off this branch or restore `GameDefaultMap` to `/Game/ThirdPerson/Maps/ThirdPersonMap`.

## What shipped in C++

| Class | Role |
|-------|------|
| `UGOTAMenuSubsystem` | Screen navigation, Continue eligibility, arena cards, quit/story/arena entry |
| `UGOTAMenuSettingsAdapter` | Apply / Cancel / Defaults / display revert over `UGOTAGameUserSettings` |
| `UGOTAGameUserSettings` | Persisted audio, sensitivity, accessibility + engine display settings |
| `UGOTALobbyViewModel` | 4-slot observed lobby snapshot (no session authority) |
| `AGOTAMenuCameraDirector` | Named cinematic shots + blend / skip |
| `UGOTAMainMenuWidget` (+ Settings / Multiplayer / Lobby / ArenaSelection / MenuButton) | BindWidgetOptional bases for `WBP_GOTA_*` |

`UArenaDefinition` gained menu fields: Tagline, Description, Thumbnail, BiomeIds, `bIsSelectable`, `bComingSoon`.

## Config

- `GameDefaultMap` → `/Game/MainMenu/L_GOTA_MainMenu`
- `EditorStartupMap` remains ThirdPerson graybox for combat playtest
- `GameUserSettingsClassName` → `/Script/WarOfGods.GOTAGameUserSettings`
- Asset Manager scans `/Game/Arenas/Data` for Arena/Biome definitions

## Editor assembly checklist (required for acceptance polish)

1. Compile `WarOfGods` (Live Coding or full rebuild).
2. Open `L_GOTA_MainMenu` and keep `BP_MainMenuGameMode`.
3. Place `GOTAMenuCameraDirector`; add CineCameraActors for MainMenu / Settings / Multiplayer; fill `Shots`.
4. On BeginPlay (Level BP or MenuController): `GetGameInstance` → `UGOTAMenuSubsystem` → `BindCameraDirector`.
5. Create Widget Blueprints parented to C++ classes:
   - `WBP_GOTA_MenuButton` → `UGOTAMenuButtonWidget` (`Button_Root`, `Text_Label`)
   - `WBP_GOTA_MainMenu` → `UGOTAMainMenuWidget`
   - `WBP_GOTA_Settings` → `UGOTASettingsWidget`
   - `WBP_GOTA_Multiplayer` → `UGOTAMultiplayerWidget`
   - `WBP_GOTA_Lobby` → `UGOTALobbyWidget`
   - `WBP_GOTA_ArenaSelection` → `UGOTAArenaSelectionWidget`
6. Style gold/stone UI to match concept art; wire focus navigation for gamepad.
7. From `WBP_GOTA_Multiplayer` implementable events, call existing GI:
   - `CreateMPSession` / `JoinMpSession` / `FindSessions`
8. From lobby BP, push names into `UGOTALobbyViewModel::UpsertPlayerSlot` (do not invent a second player list).
9. Reuse existing `WBP_LobbyMenu` until `WBP_GOTA_Lobby` is validated — call both if needed.
10. Dress environment with in-project Megascans / Geometry / Lava assets; place BossyEnemy or FightingAnimsetPro mannequin as foreground warrior with idle.
11. Optional: create `DA_Arena_*` under `Content/Arenas/Data`. Until then, fallback cards mark only **Titan's Quarry** selectable.
12. Nordic Coast remains in VaultCache only — migrate curated assets before claiming Stormbreak Coast selectable.

## Honest feature status

| Feature | Status |
|---------|--------|
| Main menu navigation (C++) | Implemented — needs WBP visuals |
| Continue | **Hidden** until `Saved/SaveGames/GOTA_Campaign_Slot.sav` exists |
| Story Mode | Hub → Chapter 1 Fallen Sanctuary (`Arena=Story`); not Quarry/Dominion |
| Story Continue | Hidden until `GOTA_Campaign_Slot.sav` |
| PvE Arena / Arena cards | Catalog + confirm; only Titan's Quarry travelable |
| Settings apply/persist | Display + custom volumes/sens/accessibility; **no key rebind UI**; **no upscaling** (unsupported) |
| Multiplayer Host/Join/Find | Must call existing BP GI session API |
| Friend invites / matchmaking | **Unavailable** (`AreFriendInvitesSupported() == false`) — OSS not enabled |
| Ready replication | ViewModel supports ready flags; BP RPCs not authored yet |
| Extras | Hidden (`IsExtrasAvailable() == false`) |
| Camera transitions | Director ready; cameras must be placed in level |
| CommonUI | Not added (existing UMG stack preserved) |

## Build

`WarOfGodsEditor` Win64 Development — **Succeeded** (2026-10-27).

## Test plan

- [ ] PIE `L_GOTA_MainMenu` loads without map errors
- [ ] Keyboard / mouse / gamepad can move focus
- [ ] Settings Apply persists across editor restart
- [ ] Unsupported display mode can be reverted
- [ ] Host + 3 clients (LAN) show 4 lobby slots via ViewModel
- [ ] Existing ThirdPerson / ArenaGameMode playtest still works from EditorStartupMap
- [ ] Unimplemented buttons never pretend to work

## Audio / polish follow-ups

- Author Sound Mix / Sound Class assets and bind in Settings Apply
- Ambient menu music + brazier SFX
- Level Sequence optional for entrance (director blends cover screen changes)
