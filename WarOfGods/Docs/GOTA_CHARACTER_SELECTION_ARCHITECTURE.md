# Character Selection — Architecture, Multiplayer, Loading & QA

## Responsibilities (one class each)

| Concern | Owner |
|---|---|
| What a hero is | `UGOTAHeroDefinition` (PrimaryDataAsset, type `GOTAHero`, `/Game/Heroes/Data`). Editorial block separate from the gameplay block; art is soft-referenced. |
| Which heroes exist + whether a choice is legal | `UGOTAHeroRegistry` (GameInstance subsystem). Loads definitions via the AssetManager; `ValidateSelection(HeroId, Mode)` checks exists, `Playable`, mode allowed and pawn class loadable. |
| What the local player wants | `UGOTAHeroRegistry::LocalSelection` — intent only, not authoritative |
| What the server accepted | `ADuelPlayerState::HeroId` (replicated `FName`; no textures or classes are replicated) |
| How it becomes a pawn | `ADuelGameMode::InitNewPlayer` re-validates `?Hero=` on the **server**. `GetDefaultPawnClassForController` spawns the definition's `GameplayPawnClass`. A client never supplies a class. |
| Presentation | `WBP_GOTA_HeroSelect` / `WBP_GOTA_HeroTile`, Duel HUD (`Image_LocalHero`), results (`Image_Hero`, `Text_HeroName`). All resolve art locally from the ID. |
| Loading presentation | `UGOTALoadingScreenWidget` (`WBP_GOTA_LoadingScreen`) via `GetMoviePlayer()->SetupLoadingScreen`. It renders during the blocking `LoadMap` and auto-completes when loading finishes; no percentage is shown. |

## Flow (implemented)

1. Main Menu → Arena Mode → *Trial of Champions* → **Enter the Arena** → `BeginArenaHeroSelection` → **Hero Select**.
2. Focus or hover previews a hero (portrait loads on demand; tiles use small never-streaming thumbnails).
3. **Select Hero** is enabled only when the registry validates the hero. Preview-only heroes show their reason.
4. Confirm → `LaunchPendingArena` → `?Hero=<id>` → loading screen → Titan's Quarry.
5. The server re-validates the hero. If the request is invalid or missing, it falls back to the first playable hero and logs the rejection.
6. The HUD shows the accepted hero's thumbnail and name; results show its portrait, title and the real score.

## Not implemented yet (honest)

| Item | Status / plan |
|---|---|
| Lobby hero selection (LAN) | **Not implemented.** Plan: a frontend PlayerState with replicated `HeroId` + `Ready` and `Server_RequestHero(FName)` validated by `UGOTAHeroRegistry`. The lobby roster renders from RepNotify. On match start, clients pass the accepted ID in their travel URL, which the Duel/Horde `InitNewPlayer` already validates. Duplicate-hero rules would be a per-mode flag. |
| Match intro (versus / team lineup) | **Not implemented.** A reusable participant entry widget is needed. |
| Opponent / difficulty choice for Fight AI | **Not implemented.** The champion is the configured `AIChampionClass` and is not a roster hero. |
| Loading screen | **Implemented, not visually verified.** MoviePlayer can't be captured by the scripted screenshot path, and it's disabled in editor/PIE by design. Needs a packaged-build check. |
| Horde / Story hero selection | Not wired. Story keeps its canonical character. |

## QA evidence (standalone `-game`, `Tools/Tests/hero_select.txt`)

| Test | Result |
|---|---|
| Registry finds 9 definitions via AssetManager | PASS (`Hero registry: 9 heroes`) |
| Hero Select opens from Arena; Ayla shown with approved layout, one portrait, stats 8/6/9/8, 5 abilities, weapon | PASS (`40_heroselect_ayla.png`) |
| Preview-only hero cannot be selected (Enter on Ayla stays on Hero Select; button disabled with reason) | PASS |
| Keyboard roster navigation (Right ×8) previews each hero | PASS (`41_*`, `42_*`, `43_*`) |
| Prototype Warden selected → `?Hero=UPAWarden` → server accepted → replicated `HeroId` read back | PASS |
| HUD thumbnail + hero name; results portrait + title + real score | PASS (`44_*`, `45_results_hero.png`) |
| Invalid hero ID on the URL is rejected server-side | Implemented, not scripted |
| Mouse, gamepad hardware, 21:9 / 4K | Not tested |
| Multiplayer replication with two clients | Not tested (lobby selection not implemented) |
| Packaged build | Not tested |
