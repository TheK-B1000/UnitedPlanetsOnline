# Gods of the Arena — UPG Frontend Architecture

**Engine:** UE 5.8 · **Module:** `WarOfGods` (flat `Public/` + `Private/`) · **Branch:** `feature/gota-cinematic-frontend`

Who this is for: engineers and designers working on the frontend, Arena launch flow, or settings.

## Layers

| Layer | Owner | Responsibility |
|---|---|---|
| A — Presentation | `WBP_GOTA_*` (UMG) on C++ bases in `UI/Widgets/` | Layout, focus visuals, motion, sounds. No game/session logic. |
| B — Coordination | `UGOTAMenuSubsystem` (GameInstance subsystem, the only menu coordinator) | Current screen + history, modal layer, guarded travel, action routing, Continue eligibility, Arena launch, menu audio. |
| C — Features | `AGOTAStoryGameMode` + `UGOTAStorySaveGame`, `ADuelGameMode`, `AHordeGameMode`, `UGotaSessionSubsystem`, `UGOTAMenuSettingsAdapter` | Each keeps its domain. The menu only requests and reports. |
| D — Content | `DA_GOTA_MenuContent` (`UGOTAMenuContentDefinition`) | Copy, icons, card art, theme tokens, audio cues, lore, chapters, arena previews, Horde availability. |

## Dependency map

```
L_GOTA_MainMenu (WorldSettings → AGOTAFrontendGameMode, no pawn)
  └─ AGOTAFrontendPlayerController::BeginPlay ─┐
UGOTAMenuSubsystem::HandlePostLoadMap ─────────┴─► PresentFrontendUI()  (idempotent per world)
        │ creates once ▼
  WBP_GOTA_FrontendRoot (UGOTAFrontendRootWidget)
     ├─ Image_Backdrop (clean-plate, slow drift; world rendering suppressed behind it)
     ├─ MainMenu · StoryMode · ArenaSelection · Multiplayer · Lobby · Settings · Extras   (UGOTAMenuScreenWidget)
     └─ Modal (UGOTAMenuModalWidget)
        │ every nav row AND feature card ─► ExecuteMenuAction(EGOTAMenuAction)
        ▼
  UGOTAMenuSubsystem
     ├─ Continue / Story ─► UGOTAStorySaveGame (read-only) ─► OpenLevel(L_GOTA_Story_Prologue, GOTAStoryGameMode, ?Checkpoint)
     ├─ Arena ─► IsArenaRulesetAvailable ─► RequestLaunchArena
     │            FightAI → ThirdPersonMap?game=DuelGameMode?Arena=Quarry?BestOf=3?Opponent=AI
     │            Horde   → ThirdPersonMap?game=HordeGameMode?Arena=Quarry (gated by DA flag)
     ├─ Multiplayer ─► UGotaSessionSubsystem (Null OSS, LAN, max 4) ─► host travels to L_GOTA_MainMenu?listen
     ├─ Settings ─► UGOTAMenuSettingsAdapter ─► UGOTAGameUserSettings (+ AGOTAPlayerCameraManager in game)
     └─ Quit ─► ConfirmQuit modal ─► UKismetSystemLibrary::QuitGame (ends PIE in editor)
```

## Screen model

`EGOTAMenuScreen` (appended, never reordered — values are serialized in camera shots): `MainMenu, StoryMode, Settings, Multiplayer, Lobby, ArenaSelection, Extras, Loading`.
Fight AI and Horde setup are two rulesets of the single Arena screen rather than separate screens.

- Only the active screen is `SelfHitTestInvisible`; all others are `Collapsed`, so only one screen receives input.
- Back: `Esc`, `Backspace`, or gamepad (B) reach `UGOTAFrontendRootWidget::NativeOnKeyDown`, which calls `HandleBackInput()`. That closes the modal first, otherwise steps back. Navigating to Main clears history, so Back never loops.
- Focus: screens remember their last focused control (`TrackFocus`) and restore it on return. When a modal closes, the root restores focus to the active screen. If a nav key arrives with nothing focused (mouse click on empty space, pad picked up), the root re-seats focus.

## Modal layer

`EGOTAMenuModal`: `ConfirmQuit`, `Error`, `Busy`, `ConfirmDisplay`.
- While open, the modal is `Visible` (its dim layer swallows clicks) and its buttons use explicit/Stop navigation rules, so focus can't leak to the screen underneath.
- `Busy` cannot be cancelled (travel is committed).
- `ConfirmDisplay` auto-reverts after 15 s.

## Travel guard

`BeginGuardedTravel` refuses re-entry (`bTravelInProgress`), checks the map package exists, shows `Busy`, then calls `OpenLevel`. Completion is observed, never timed:
- **Success:** `PostLoadMapWithWorld` → `ClearFrontendState`.
- **Failure:** `GEngine->OnTravelFailure` / `OnNetworkFailure` → error modal, or a pending error shown when the menu next appears.

The presented world is tracked explicitly (`PresentedWorld`), because widgets are outered to the GameInstance and `GetWorld()` follows the current world.

## Fight AI (Trial of Champions)

`ADuelGameMode` with `?Opponent=AI`:
- Slot 0 is the human `PlayerController`. Slot 1 is the AI champion (`AIChampionClass`, default `BP_Enemy_Melee`) with its own `AIController`, plus a bot `ADuelPlayerState` (name, score, round wins). No second PlayerController is faked.
- Every round respawns both fighters fresh, so health always comes from the Blueprint damage system's defaults; C++ never writes health. `RemovePawnOwnedHud` removes the `W_PlayerHUD` that `BP_Player` adds on BeginPlay, so HUDs don't stack.
- The AI brain is paused during preparation/countdown/round-end and resumed when the round goes active.
- Elimination and timeout use `UGOTACombatantObserver`: a read-only reflection bridge onto `BPC_DamageSystem` (`Health`, `Max Health`, `Is Dead`). It is the same health authority the rest of the game uses.
- Auto-start happens at `BeginPlay`. Local hosts log in before BeginPlay, so starting in `PostLogin` was reset by BeginPlay.
- Results: `ADuelHUD` shows `WBP_GOTA_DuelResults` on MatchEnd (Game+UI input, cursor) and hides it when the phase leaves MatchEnd. So Rematch → results → Rematch repeats cleanly.

## Settings — what has a real effect

| Setting | Effect path |
|---|---|
| Display mode, resolution | `UGameUserSettings` + confirm/auto-revert modal (baseline is the last *applied* value) |
| Graphics preset + 9 scalability groups, VSync, frame cap | `UGameUserSettings` |
| Master volume | `FAudioDevice::SetTransientPrimaryVolume` |
| Sound effects | Runtime `USoundMix` override on the project default sound class |
| Music / Interface | Menu audio components play through `SC_GOTA_Music` / `SC_GOTA_UI` with user volume |
| Look sensitivity, invert Y | `AGOTAPlayerCameraManager::ProcessViewRotation` (assigned on `BP_WarOfGods_PlayerController`) |
| Camera shake | `UGOTACameraShakeModifier::AddCameraShake` scales or suppresses shakes |
| Reduced motion | Disables backdrop drift, screen slide, card lift and camera blends |
| Interface scale | `UUserInterfaceSettings::ApplicationScale` |

**Not offered (no backing system):** key rebinding, dialogue volume, subtitles, VFX intensity. The fields remain in `UGOTAGameUserSettings` for later.

## Sessions

`UGotaSessionSubsystem` is the only session backend (Null OSS):
- **Blueprint path disabled:** reflection calls into the BP GameInstance (`CreateMPSession` / `JoinMpSession` / `DestroySession` / `TravelToMap`) are off (`bUseBlueprintSessionEvents=false`). `JoinMpSession` needs a `SessionResult` struct that can't be passed by reflection, and the BP `DestroySession` targets a different session name.
- **Host:** after `CreateSession` succeeds, the host opens `L_GOTA_MainMenu?listen`; the menu presents the Lobby because a session is active.
- **Lobby cap:** `AGOTAFrontendGameMode::PreLogin` rejects a 5th player.

## Editor tooling

| Path | Purpose |
|---|---|
| `Tools/GOTA_BuildMenuArt.py` (offline) | Clean plate, card art, icons, insignia, logotype from the reference + original art |
| `Tools/GOTA_BuildMenuAudio.py` (offline) | Original synthesized UI cues + ambience |
| `Tools/GOTA_BuildMenuUI.py` (editor) | Imports, `DA_GOTA_MenuContent`, all WBPs, level/controller wiring. Idempotent. |
| `UGOTAEditorUIBuilderLibrary` (C++, `WITH_EDITOR` only) | Widget-tree authoring, composite fonts, level-script clearing |
| `Private/UI/GOTAFrontendTestCommands.cpp` (non-shipping) | `gota.Test.Run/Shot/Key/Probe`, `gota.Menu.*`, `gota.Duel.*` — scripted runtime validation |

Run the editor builder with the full editor (Slate is required for imports):

```
set GOTA_QUIT_AFTER_BUILD=1
UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript="<abs>\Tools\GOTA_BuildMenuUI.py" -unattended -stdout
```
