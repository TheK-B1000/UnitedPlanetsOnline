# Story Mode — Prologue / Chapter 1: The Fallen Sanctuary

**Engine:** UE 5.8  
**Canon:** `GOTA_STORY_BIBLE.md` v2.0 — *The Sovereign Protocol*  
**Status:** C++ vertical slice linked; `L_GOTA_Story_Prologue.umap` authored  
**Recovery point:** `15f85ba`  
**Players:** Aegis Team Seven Wardens (1–4)

## Fiction

UPA Aegis Team Seven deploys to a remote UPG-administered world after a research station stops transmitting. Locals believe a guardian-god protects their sanctuary. The guardian is an experimental security construct (**Oathbound Sentinel**) using an unknown energy system — early breadcrumb toward Resonance / Sovereign Protocol (named later).

Players are **Authority Wardens**, not myth wanderers. Shared-universe depth (Incorruptible / Gemborne / Ayla) begins in earnest at Ch.4–7; prologue stays standalone-readable.

## Map

| Asset | Path |
|-------|------|
| Prologue map | `/Game/Story/Maps/L_GOTA_Story_Prologue` |
| Authoring script | `Tools/CreateStoryPrologueMap.py` |
| Runtime dress | `AArenaGrayboxBuilder` layout `FallenSanctuary` |

If the map is missing, menu/`gota.Story` falls back to `ThirdPersonMap` + `GOTAStoryGameMode`.

## Mission flow (code ↔ bible)

| Phase | UI objective | Bible beat | Server proof |
|-------|--------------|------------|--------------|
| Active | Reach Sanctuary Gates | Secure approach | Objective overlap |
| Active | Clear Perimeter Hostiles | Hostile constructs | Enemy pawns invalid |
| Checkpoint | Activate Ascendant Beacon | Communications checkpoint | Overlap + save |
| Active | Defeat Sanctuary Guardian | Oathbound Sentinel | Guardian invalid |
| Done | Site Secured — Report to Command | Evidence / expand investigation | `bMissionCompleted` |

Internal ids (`ActivateDivineShrine`, etc.) stay stable for saves.

Phases: `NotStarted → Intro → Active ↔ CheckpointReached → Completed | Failed`

## C++ files

| Class | Role |
|-------|------|
| `AGOTAStoryGameMode` | Mission orchestration, spawns, save, 4 starts |
| `AGOTAStoryGameState` | Replicated mission/objective/checkpoint |
| `AGOTAStoryObjectiveActor` | Server-validated triggers |
| `UGOTAStorySaveGame` | Campaign save |
| `UGOTAStoryMissionDefinition` | Optional data asset |
| `UGOTAStoryMenuWidget` | New / Continue / Host / Join / Back |

## Saves

| Slot | Use |
|------|-----|
| `GOTA_Campaign_Solo` | Solo New Game |
| `GOTA_Campaign_Host` | Host co-op |
| `GOTA_Campaign_Slot` | Legacy Continue mirror |

## Combat authority

`BPC_DamageSystem` owns health. Prefer `UCombatTimingComponent::ResolveIncomingAttack` before apply (Impl 04–06). Encounter complete = server pawn validity. Co-op combat not claimed network-correct until damage is server-authoritative.

## Playtest

```
gota.Story
gota.Story DivineShrine
gota.StoryReturnMenu
gota.Combat.DumpTiming
```

## Next story milestones (bible Part XIX)

1. UPA briefing / identity dressing on prologue  
2. Quarry Protocol — first Sovereign research records  
3. Compact UPA hub (not mega starship)  
4. Act I Verdant + Ashfall  
5. Later: Erased Colony → Painted Door → Ayla God-Class (after Playable Integration gates)

## Out of scope (this slice)

Full campaign, Ayla pawn, GAS, matchmaking, Dominion as Story spawn.
