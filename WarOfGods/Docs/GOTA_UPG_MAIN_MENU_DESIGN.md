# Gods of the Arena — UPG Main Menu Design

**Reference:** `Docs/References/GOTA_UPG_MainMenu_Visual_Reference.png` (1672×941). This is the source of truth for composition, colour and hierarchy.

## Composition (1920×1080 design space, anchored for other aspects)

| Region | Content | Anchor | Asset / widget |
|---|---|---|---|
| A | UPG insignia + "UNITED PLANETS / OF GALAXIES" + "EARTH • CAPITAL • UPG" | top-left | `T_GOTA_UPG_Insignia` (original), editable text |
| B | GODS OF THE ARENA logotype + editable subtitle + ornament | top-centre | `T_GOTA_Logo_GodsOfTheArena` (original Cinzel metallic treatment); `Text_GameSubtitle` from content |
| C | Continue, Story Mode, Arena Mode, Multiplayer, Settings, Extras, Quit | left column | `WBP_GOTA_NavButton` ×7 |
| D | Profile: portrait, local name, rank, friends, + | top-right | Name = OS account; rank = "AUTHORITY WARDEN". No level/XP (no progression system). Friends and + disabled with explanation. |
| E | UPG / Earth / Capital of Humanity panel + rotating institutional message | right | `T_GOTA_Earth_Capital`, `Text_Slogan` (cycles `InstitutionalMessages` every 9 s) |
| F | Five illustrated cards: Story, Arena, Multiplayer, Settings, Extras | bottom row, stretches | `WBP_GOTA_FeatureCard` ×5 (same actions as the nav rows) |

## Design tokens (`FGOTAMenuTheme` in `DA_GOTA_MenuContent`)

| Token | Value (linear) | Use |
|---|---|---|
| Background | 0.006, 0.009, 0.022 | deep midnight navy |
| Surface | 0.010, 0.014, 0.030 @ 0.78 | panels |
| Gold | 0.78, 0.55, 0.22 | frames, selection sweep |
| GoldBright | 1.00, 0.82, 0.48 | focused labels, titles |
| TechBlue | 0.25, 0.55, 1.00 | progress, tech accents |
| Text / TextMuted / TextDisabled | warm off-white / silver-blue / grey | |
| Danger / Success | restrained crimson / cool cyan | |
| FocusBlendSeconds 0.14 · ScreenEnterSeconds 0.28 · CardLift 8 px | | motion |

**Typography (SIL OFL 1.1, bundled in `Tools/MenuFonts`):** Cinzel (Regular / SemiBold / Bold) for display and labels; Montserrat (Regular / Medium / SemiBold) for body text and small spaced captions.

## Interaction states (one button class, five styles)

| State | NavRow | FeatureCard | Action / Tab / ListEntry |
|---|---|---|---|
| Normal | plain label, white icon | art at 72% brightness, faint frame | dim frame |
| Hover | soft frame (45%) | partial lift | brighter frame |
| Focus | full gold frame, gold sweep, gold label + icon | lift 8 px, scale 1.025, full frame | full frame + sweep |
| Selected (persistent) | — | — | strong frame (75%), used for the chosen tab / arena |
| Pressed | 1.5% squash | 2% squash | 1.5% squash |
| Disabled | grey, no sweep, can't take focus | art at 35% | grey |

Hover never steals focus, so the mouse and the gamepad can't fight. Hover and focus look different on purpose.

## Motion

- Screen enter: 0.28 s ease-out fade + 28 px slide. Interruptible: a new screen hides the old one instantly.
- Backdrop: 46 s Lissajous drift, ±14 px, 1.035–1.043 scale.
- Focus transitions: 0.14 s.
- Reduced Motion setting: no drift, no slide, no lift, instant camera cuts.

## Audio

All UI cues are original (synthesized by `Tools/GOTA_BuildMenuAudio.py`): focus tick, gold confirm chime, falling-fourth back, muted error pulse, deployment swell. Ambience is a seamless 22 s starship-hum loop. Music is the Unreal Starter Content music cue (a placeholder until an original score exists).

The music and ambience loops are created once per menu world and never stacked. They stop on travel.

## Backdrop — current state (honest)

The 3D UPG Earth orbital scene has **not** been built. The project has no Earth, sci-fi city, starship or Warden-armour assets to make it from, and the brief rules out low-effort substitutes.

The current backdrop is a **clean plate of the reference art**: every baked text region is inpainted, and live UMG widgets sit exactly over those areas. The brief allows this as a temporary milestone. While it is active, 3D world rendering is suppressed (`bSuppressWorldBehindBackdrop`), so the menu costs almost no GPU.

The camera director (`AGOTAMenuCameraDirector`) and per-screen camera routing are still wired and take over once a 3D scene and cameras are placed in `L_GOTA_MainMenu`. Set `bSuppressWorldBehindBackdrop=false` at that point.
