# Story Chapter 1 — The Fallen Sanctuary

Story Mode does **not** load Titan's Quarry or Shattered Dominion. It opens a dedicated prologue space: ruined sanctuary outside the Pantheon Coliseum gates.

## Menu flow

```
Main Menu
  └─ Story Mode          → EGOTAMenuScreen::StoryMode
       ├─ Start Chapter 1 — Solo
       ├─ Start Chapter 1 — Host Co-op (?listen, 4 slots)
       ├─ Continue         (hidden until GOTA_Campaign_Slot.sav)
       └─ Back
```

Travel options: `Arena=Story` + `Mode=Story` → `EArenaGrayboxLayout::FallenSanctuary`.

Preferred map: `/Game/Story/Maps/L_GOTA_Story_Prologue`  
Fallback until that map exists: `ThirdPersonMap` with the same URL options (runtime graybox).

## Layout (graybox)

| Zone | Purpose |
|------|---------|
| Courtyard + ruined temple (north) | Arrival / exploration |
| Warrior encampment (SW) | Spawn cluster (4 non-overlapping starts) |
| Training clearing (east) | First encounter space |
| Gate path (south) | Approach toward Coliseum |
| Distant coliseum silhouette (~Y -9000) | Visual destination only |

## Chapter 1 beat sheet (design)

1. **Arrival** — spawn at camp; short cinematic later  
2. **Exploration** — move / camera / environment  
3. **First encounter** — basic enemies in east clearing  
4. **First divine power** — unlock counterattack (content TBD)  
5. **The Gates** — miniboss at southern gate → journey toward Coliseum  

## Editor follow-up

1. Create `Content/Story/Maps/L_GOTA_Story_Prologue` (duplicate ThirdPersonMap or empty + lighting).  
2. Set World Settings GameMode Override → `ArenaGameMode` (or leave global).  
3. Place `AArenaGrayboxBuilder` with Layout = Fallen Sanctuary, **or** rely on GameMode auto-build with `?Arena=Story`.  
4. Author `WBP_GOTA_StoryMode` parenting `UGOTAStoryMenuWidget`; show it when `OnMenuScreenChanged` → StoryMode.  
5. Dress with stone / forest / ruins assets already in project.  
6. Later: geographically connect to Shattered Dominion without making Dominion the Story spawn.

## Console / PIE

```
gota.BuildArena Story
open ThirdPersonMap?Arena=Story?Mode=Story
```
