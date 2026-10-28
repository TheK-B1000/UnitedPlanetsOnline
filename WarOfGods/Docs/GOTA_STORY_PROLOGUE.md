# Story Prologue — The Fallen Sanctuary

**Canon:** `GOTA_STORY_BIBLE.md` v2.0 — *The Sovereign Protocol*  
**Unit:** UPA Aegis Team Seven — Authority Wardens  
**Boss:** Oathbound Sentinel (experimental guardian / unknown energy system)  

Act I opener: Aegis Team Seven investigates a silent UPG research site. Locals worship an Ascendant “guardian” as a god. The ruins read as a temple; underneath is unauthorized security infrastructure that later connects to the Sovereign Protocol (do not name Protocol in prologue UI).

Story Mode does **not** load Titan's Quarry or Shattered Dominion. Preferred map: `/Game/Story/Maps/L_GOTA_Story_Prologue` (fallback: `ThirdPersonMap` + runtime FallenSanctuary graybox).

## Menu flow

```
Main Menu
  └─ Story Mode
       ├─ Start Chapter 1 — Solo
       ├─ Start Chapter 1 — Host Co-op (?listen, 4 slots)
       ├─ Continue
       └─ Back
```

Travel: `Arena=Story` + `Mode=Story` → `EArenaGrayboxLayout::FallenSanctuary`.

## Layout (graybox)

| Zone | Purpose |
|------|---------|
| Courtyard + ruined sanctuary (north) | Arrival / investigation |
| Warden LZ (SW) | 4 non-overlapping spawns |
| Perimeter clearing (east) | First hostiles / constructs |
| Gate path (south) | Approach toward Coliseum silhouette |
| Distant coliseum (~Y -9000) | Arena Initiative landmark (visual) |

## Beat sheet (bible-aligned)

1. **Deploy** — UPA credentials / Vey briefing (cinematic later)  
2. **Contact** — local survivors; low-tech cultural overlay  
3. **Investigate** — missing research team  
4. **Defend / clear** — hostile constructs  
5. **Ascendant Beacon** — communications checkpoint (UI: Activate Ascendant Beacon)  
6. **Oathbound Sentinel** — defeat/disable guardian  
7. **Extract evidence** — unauthorized energy / security network → investigation expands toward Quarry Protocol  

**Moral fork:** prioritize evidence recovery vs civilian protection under pressure — both completable; trust/consequences differ.

## Dress rule

Stone / forest / ruins first. Add UPA drones, holograms, tech props selectively. No megacity prologue. No Ayla / Erebus-7 spoilers in prologue dialogue.

## Console

```
gota.BuildArena Story
gota.Story
gota.Story DivineShrine
```
