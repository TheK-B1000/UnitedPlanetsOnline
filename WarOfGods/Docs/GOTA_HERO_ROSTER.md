# Gods of the Arena — Hero Roster & Asset Matrix

**Source of truth:** the approved cards in `Docs/References/Heroes/GOTA_HeroCard_<Hero>.png`.
**Data:** `/Game/Heroes/Data/DA_Hero_<Id>` (`UGOTAHeroDefinition`). Text was transcribed by `Tools/GOTA_HeroContent.py`. Only what the cards state is used; fields a card doesn't state are left empty, never invented.

## Implementation matrix

| Hero | Card | Portrait / thumbnail | Card data in DA | Faction stated | Ability icons | Weapon art | 3D mesh | Anim BP | Combat setup | Selectable | PIE / MP validated |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Ayla — Desert Storm | ✅ | ✅ | ✅ (5 abilities) | ✅ UPA · Ascendant · Mobility/Elemental/Precision | pending | pending | ❌ | ❌ | ❌ | Preview only | — |
| Drakon — Warfall Titan | ✅ | ✅ | ✅ (4) | ❌ not on card | pending | pending | 🟡 prototype: Paragon Greystone | retargeted from BP_Player | shared Warden kit | In Development | Standalone full match ✅ (dev switch) |
| Lysara — Astral Sovereign | ✅ | ✅ | ✅ (5) | ❌ | pending | pending | ❌ | ❌ | ❌ | Preview only | — |
| Kairos — Chronos Warden | ✅ | ✅ | ✅ (5) | ❌ | pending | pending | ❌ | ❌ | ❌ | Preview only | — |
| Zephyr — Wind Rebellion | ✅ | ✅ | ✅ (5) | ❌ | pending | pending | ❌ | ❌ | ❌ | Preview only | — |
| Varoth — Inferno Colossus | ✅ | ✅ | ✅ (5) | ❌ | pending | pending | 🟡 prototype: Paragon Sevarog | retargeted (pose needs tuning) | shared Warden kit | In Development | Standalone full match ✅ (dev switch) |
| Nyx — Shadow Weaver | ✅ | ✅ | ✅ (5) | ✅ UPA · Ascendant · Intelligence/Stealth/Control | pending | pending | 🟡 prototype: Paragon Countess | retargeted from BP_Player | shared Warden kit | In Development | Standalone full match ✅ (dev switch) |
| Orion — Starforge Sentinel | ✅ | ✅ | ✅ (5) | ❌ | pending | pending | ❌ | ❌ | ❌ | Preview only | — |
| **UPA Warden (Prototype)** | — | Warden crop | partial (no card stats) | UPA · Warden | — | — | ✅ `BP_Player` | ✅ | ✅ existing | **Playable** | Standalone ✅ (Fight AI) |

**Update (2026-10-31):** Drakon, Nyx and Varoth now wear Paragon prototype bodies over the BP_Player combat frame. See `GOTA_PARAGON_INTEGRATION.md`. They stay In Development until each has its own moveset and abilities and has been verified in multiplayer.

**Original audit finding:** the project has no skeletal mesh, animation set, weapon or combat configuration for any of the eight heroes. Existing characters are the UE mannequins (`BP_Player`), the Paragon heroes (Rampage, Feng Mao, Gideon, Sparrow), `SKM_Mage` and BossyEnemy. Per the brief, none of these is substituted for a roster hero. The heroes are previewable, and SELECT HERO explains why it's disabled.

**Pending derivatives (agreed):** ability icons and weapon art stay in the source cards for now, because their positions differ per card. `FGOTAHeroAbilityInfo::Icon` exists for when they are cut.

## Card statistics

ATTACK / DEFENSE / MOBILITY / TECHNIQUE are **presentation values** (`FGOTAHeroCardStats`, 0–10). Nothing in combat reads them. When hero gameplay exists, map them explicitly to combat configuration; don't use them as multipliers.

| Hero | ATK | DEF | MOB | TEC |
|---|---|---|---|---|
| Ayla | 8 | 6 | 9 | 8 |
| Drakon | 9 | 9 | 6 | 7 |
| Lysara | 7 | 8 | 8 | 9 |
| Kairos | 8 | 8 | 9 | 9 |
| Zephyr | 8 | 6 | 10 | 8 |
| Varoth | 10 | 9 | 6 | 8 |
| Nyx | 8 | 7 | 9 | 10 |
| Orion | 8 | 9 | 8 | 8 |

## Open canon questions (need the author)

- Ayla's card shows two quotes. The left-panel line is used as `Quote`; "I was made to protect, not to be controlled." is not stored yet.
- Ayla is UPA on her card, while the story bible ties her to Nova Requiem. The card value is kept; please confirm.
- Factions for Drakon, Lysara, Kairos, Zephyr, Varoth and Orion are not stated. Zephyr's biography calls him a "Wind Warden"; that was not turned into a faction.

## Adding a hero later (no code changes)

1. Add the card to `Docs/References/Heroes`.
2. Add tile/crop coordinates in `Tools/GOTA_BuildHeroArt.py`.
3. Add the entry to `Tools/GOTA_HeroContent.py`.
4. Re-run both build scripts.

To make a hero playable, set `GameplayPawnClass` and `Availability=Playable` on its DA. The registry, selection screen, server validation and spawn hook need no changes.
