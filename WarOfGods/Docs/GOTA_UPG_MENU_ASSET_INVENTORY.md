# UPG Menu — Asset Inventory

All assets are created or refreshed by `Tools/GOTA_BuildMenuUI.py` (idempotent).

## Widget Blueprints

| Asset | C++ parent | Notes |
|---|---|---|
| `/Game/MainMenu/UI/WBP_GOTA_FrontendRoot` | `UGOTAFrontendRootWidget` | Backdrop + 7 screens + modal |
| `/Game/MainMenu/UI/WBP_GOTA_MainMenu` | `UGOTAMainMenuWidget` | Regions A–F |
| `/Game/MainMenu/UI/WBP_GOTA_StoryMode` | `UGOTAStoryMenuWidget` | Synopsis, New / Co-op / Join / Continue, chapter list |
| `/Game/MainMenu/UI/WBP_GOTA_ArenaSelection` | `UGOTAArenaSelectionWidget` | Fight AI / Horde rulesets, 7 arenas, preview |
| `/Game/MainMenu/UI/WBP_GOTA_Multiplayer` | `UGOTAMultiplayerWidget` | Host / Find / Join / Lobby + browser |
| `/Game/MainMenu/UI/WBP_GOTA_Lobby` | `UGOTALobbyWidget` | 4-slot roster, Ready, Start, Leave |
| `/Game/MainMenu/UI/WBP_GOTA_Settings` | `UGOTASettingsWidget` | Video / Audio / Controls / Accessibility |
| `/Game/MainMenu/UI/WBP_GOTA_Extras` | `UGOTAExtrasWidget` | Codex, controls guide, credits |
| `/Game/MainMenu/UI/WBP_GOTA_Modal` | `UGOTAMenuModalWidget` | Quit / error / busy / display confirm |
| `/Game/MainMenu/UI/WBP_GOTA_NavButton` | `UGOTAMenuButtonWidget` (NavRow) | |
| `/Game/MainMenu/UI/WBP_GOTA_FeatureCard` | `UGOTAMenuButtonWidget` (FeatureCard) | |
| `/Game/MainMenu/UI/WBP_GOTA_ActionButton` | `UGOTAMenuButtonWidget` (Action / Tab) | |
| `/Game/MainMenu/UI/WBP_GOTA_ListEntry` | `UGOTAMenuButtonWidget` (ListEntry) | |
| `/Game/MainMenu/UI/WBP_GOTA_IconButton` | `UGOTAMenuButtonWidget` | `IconOverride` per instance |
| `/Game/MainMenu/UI/WBP_GOTA_SettingRow` | `UGOTASettingRowWidget` | |
| `/Game/UI/Duel/WBP_GOTA_DuelHUD` | `UGOTADuelHUDWidget` | Names, health (observer), score, timer, countdown |
| `/Game/UI/Duel/WBP_GOTA_DuelResults` | `UGOTADuelResultsWidget` | Victory / Defeat, Rematch, Return to Main Menu |

## Data and audio

| Asset | Source |
|---|---|
| `/Game/MainMenu/Data/DA_GOTA_MenuContent` | Authored by the builder (copy, lore, chapters, theme, cues, previews) |
| `/Game/Audio/Mix/SC_GOTA_Music`, `SC_GOTA_UI` | Sound classes (no parent, so the SFX mix never touches them) |
| `/Game/Audio/UI/S_GOTA_UI_{Focus,Confirm,Back,Error,Transition}` | Original, `Tools/GOTA_BuildMenuAudio.py` |
| `/Game/Audio/UI/S_GOTA_Menu_Ambience` | Original seamless loop |
| Music: `/Game/StarterContent/Audio/Starter_Music_Cue` | Epic Starter Content (placeholder) |

## Textures (`/Game/MainMenu/UI/Textures`, UI group, no mips, never stream)

| Texture | Origin |
|---|---|
| `T_GOTA_Menu_Backdrop_CleanPlate` | Reference image, baked text inpainted (temporary backdrop) |
| `T_GOTA_Card_{Story,Arena,Multiplayer,Settings,Extras}` | Reference card art, text removed |
| `T_GOTA_Portrait_Warden`, `T_GOTA_Earth_Capital` | Reference crops |
| `T_GOTA_Icon_{Continue,Story,Arena,Multiplayer,Settings,Extras,Quit,Friends,UPGMark}` | Reference icons, luminance-keyed to white (tinted at runtime) |
| `T_GOTA_UPG_Insignia` | **Original** 8-point compass in double ring (reusable faction mark) |
| `T_GOTA_Logo_GodsOfTheArena` | **Original** Cinzel Bold metallic logotype with blade + halo |
| `T_GOTA_Frame`, `T_GOTA_Grad_{H,V,Selection}`, `T_GOTA_Glyph_Diamond`, `T_GOTA_Icon_Plus` | Original UI primitives |
| `T_GOTA_Arena_TitansQuarry`, `T_GOTA_Arena_AshfallCrucible` | Pack thumbnails (192 px — replace with in-game captures) |

Other arena previews reference the existing PhotoR panoramas (`desert`, `Mountain_a_Winter`, `Mountain_c_Summer`). Stormbreak Coast and Shattered Dominion have no preview.

## Fonts (`/Game/MainMenu/UI/Fonts`)

`F_GOTA_Cinzel` (Regular / SemiBold / Bold) and `F_GOTA_Montserrat` (Regular / Medium / SemiBold). Both are runtime-cached composite fonts over imported FontFaces, under SIL OFL 1.1. License texts are in `Tools/MenuFonts/*_OFL.txt`.

## Level and Blueprint edits

| Asset | Change |
|---|---|
| `L_GOTA_MainMenu` | WorldSettings GameMode override → `GOTAFrontendGameMode`. The inherited legacy level script (`BeginPlay → Cast To BP_WarOfGods_GameInstance → UI_ShowMainMenu`) was removed; it spawned the old `WBP_MainMenu` on top of the new frontend. `MainMenu.umap` is untouched. Marker actor: `GOTA_FrontendLevelMarker`. |
| `BP_WarOfGods_PlayerController` | `PlayerCameraManagerClass` → `GOTAPlayerCameraManager` (look sensitivity, invert, camera-shake strength) |
