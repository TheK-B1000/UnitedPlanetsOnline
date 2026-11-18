"""Regression suite for the signature abilities: Gale Rush (Zephyr), Gravity Well (Lysara), Time Lash (Kairos),
Gravity Slam (Orion), Titan Charge (Drakon), Shadow Step (Nyx), Inferno Slam (Varoth).

For every ability it checks, from the game logs: successful activation, cooldown refusal, an invalid target
(out of range / out of radius / no line of sight), death during the cast, and multiplayer (listen server + client).
Each case launches the game with a Tools/Tests script and matches expected (and forbidden) log lines.

  python Tools/GOTA_AbilityRegression.py                 # editor -game (uses the compiled editor build)
  python Tools/GOTA_AbilityRegression.py --packaged      # the packaged Win64 build (GOTA_PACKAGE_DIR)
  python Tools/GOTA_AbilityRegression.py --only GaleRush # one ability
  python Tools/GOTA_AbilityRegression.py --check-only    # re-evaluate the last logs without launching anything

Exit code 1 if any check fails. Logs go to Saved/AbilityRegression/.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UPROJECT = os.path.join(ROOT, "WarOfGods.uproject")
EDITOR = os.environ.get("GOTA_EDITOR", r"K:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe")
PACKAGE_DIR = os.environ.get("GOTA_PACKAGE_DIR", r"B:\Unreal\GOTA_Builds\Pass2_Paragon\Windows")
LOGS = os.path.join(ROOT, "Saved", "AbilityRegression")
QUARRY = "/Game/Arenas/Maps/L_Arena_TitansQuarry?game=/Script/WarOfGods.DuelGameMode?Mode=Duel?Arena=Quarry?BestOf=3"

# (name, url-options, script, [(regex, expect)])  expect: True = must appear, False = must NOT appear, int = exact count.
CASES = {
    "GaleRush": [
        ("hit+cooldown", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "ability_galerush.txt",
         [(r"GaleRush hit BP_Enemy\S+ for 16\.0 \(landed=1\)", True), (r"GaleRush denied \(Cooldown\)", True)]),
        ("sidestep miss (invalid target)", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "ability_galerush_side.txt",
         [(r"GaleRush \(Charge\) cast", 2), (r"GaleRush hit", 1)]),  # sidestep misses, the control dash hits
        ("death during cast", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "ability_death_cast.txt",
         [(r"GaleRush cancelled", True), (r"GaleRush hit", False)]),
    ],
    "GravityWell": [
        ("hit+cooldown+range+fizzle", "?Opponent=AI?Hero=Lysara?DevHeroes=1", "ability_gravitywell.txt",
         [(r"GravityWell hit BP_Enemy\S+ for 10\.0 \(landed=1\)", True), (r"GravityWell denied \(Cooldown\)", True),
          (r"GravityWell denied \(OutOfRange\)", True), (r"pull target lost .* fizzled", True)]),
        ("line of sight + wall", "?Opponent=AI?Hero=Lysara?DevHeroes=1", "ability_gravitywell_los.txt",
         [(r"GravityWell denied \(NoLineOfSight\)", True), (r"ENCROACH .*inGeometry=1", False)]),
        ("death during cast", "?Opponent=AI?Hero=Lysara?DevHeroes=1", "ability_death_cast.txt",
         [(r"GravityWell cancelled", True), (r"GravityWell hit", False)]),
    ],
    "TimeLash": [
        ("hit+slow+restore+cooldown+radius", "?Opponent=AI?Hero=Kairos?Foe=Zephyr?DevHeroes=1", "ability_timelash_reg.txt",
         [(r"TimeLash hit BP_Enemy\S+ for 6\.0", True), (r"status Slowed 3\.0s x0\.40", True),  # exact walk values: status_slowed.txt (deterministic)
          (r"TimeLash denied \(Cooldown\)", True), (r"meshAnimRate=(?!1\.00)", False)]),
        ("slowed mechanics (stance change, restore)", "?Opponent=AI?Hero=Zephyr?DevHeroes=1", "status_slowed.txt",
         [(r"ABILITY f0 move walk=120 ", True), (r"GaleRush \(Charge\) cast", True)]),
        ("death during cast", "?Opponent=AI?Hero=Kairos?DevHeroes=1", "ability_death_cast.txt",
         [(r"TimeLash cancelled", True), (r"status Slowed", False)]),
    ],
    "GravitySlam": [
        ("hit+cooldown+radius+escape+interrupt", "?Opponent=AI?Hero=Orion?DevHeroes=1", "ability_gravityslam.txt",
         [(r"GravitySlam hit BP_Enemy\S+ for 20\.0 \(landed=1\)", True), (r"GravitySlam denied \(Cooldown\)", True),
          (r"GravitySlam cancelled \(interrupted\)", True)]),
        ("unblockable vs blocking player (AI)", "?Opponent=AI?Hero=UPAWarden?Foe=Orion?DevHeroes=1", "ability_gravityslam_ai.txt",
         [(r"GravitySlam hit BP_Player\S+ for 20\.0 \(landed=1\)", True)]),
        ("death during cast", "?Opponent=AI?Hero=Orion?DevHeroes=1", "ability_death_cast.txt",
         [(r"GravitySlam cancelled", True), (r"GravitySlam hit", False)]),
    ],
    # Card-aligned signatures of the Pass 1 gods.
    "TitanCharge": [
        ("hit+cooldown+sidestep", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "ability_titancharge.txt",
         [(r"TitanCharge \(Charge\) cast", 3), (r"TitanCharge hit BP_Enemy\S+ for 14\.0 \(landed=1\)", 2),
          (r"TitanCharge denied \(Cooldown\)", True)]),
        ("breaks guard (AI vs blocking player)", "?Opponent=AI?Hero=UPAWarden?Foe=Drakon?DevHeroes=1", "ability_titancharge_block.txt",
         [(r"TitanCharge hit BP_Player\S+ for 14\.0 \(landed=1\)", True)]),
        ("death during cast", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "ability_death_cast_s1.txt",
         [(r"TitanCharge cancelled", True), (r"TitanCharge hit", False)]),
    ],
    "ShadowStep": [
        ("blink behind + cooldown + forward blink", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "ability_shadowstep.txt",
         [(r"ShadowStep \(Teleport\) cast", 2), (r": teleport V\(", 2), (r"ShadowStep denied \(Cooldown\)", True)]),
        ("death during cast", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "ability_death_cast.txt",
         [(r"ShadowStep cancelled", True), (r": teleport V\(", False)]),
    ],
    "InfernoSlam": [
        ("hit+cooldown+radius", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "ability_infernoslam.txt",
         [(r"InfernoSlam hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1), (r"InfernoSlam denied \(Cooldown\)", True),
          (r"InfernoSlam \(AreaBurst\) cast", 2)]),
        ("death during cast", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "ability_death_cast.txt",
         [(r"InfernoSlam cancelled", True), (r"InfernoSlam hit", False)]),
    ],
    # GAS status effects (Docs/GOTA_GAS_ENHANCEMENT_PLAN.md). Dread: walk x0.65, hero-ability damage x0.825.
    "Dread": [
        ("walk+damage once+Time Lash overlap+expiry", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "status_dread.txt",
         [(r"status Dread 7\.0s x0\.35 .*\[GAS walk x0\.65 dmg x0\.8[23]\]", True),
          (r"MoltenStrike hit BP_Enemy\S+ for 18\.[12] \(landed=1\)", 1),     # 22 x 0.825, scaled exactly once
          (r"GAS move scale=0\.65", True), (r"GAS move scale=0\.60", True),
          (r"GAS move scale=0\.(39|3[0-9]|2)", False),                          # the two slows never compound
          (r"gas=\[Dread [0-9.]+s x0\.35\] walkx=0\.65", True),               # Time Lash expired first, Dread still up
          (r"GAS move restored", True), (r"MoltenStrike hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),
          (r"cue GameplayCue\.GOTA\.Status\.Dread on", True), (r"cue GameplayCue\.GOTA\.Status\.Dread removed", True)]),
        ("Nyx Soul Drain applies it; reset clears it", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "status_dread_soul.txt",
         [(r"SoulDrain hit BP_Enemy\S+ for 18\.0 \(landed=1\)", True), (r"status Dread 4\.0s x0\.35", True),
          (r"cleared 1 status effect\(s\) \(reset\)", True)]),
        ("Varoth World Burn applies it; it expires", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "status_dread_burn.txt",
         [(r"WorldBurn hit BP_Enemy\S+ for 5\.0 \(landed=1\)", True), (r"status Dread 4\.0s x0\.35", True),
          (r"cue GameplayCue\.GOTA\.Status\.Dread removed", True)]),
        ("death clears every status", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "status_death.txt",
         [(r"cleared 2 status effect\(s\) \(death\)", True)]),
    ],
    # Veiled: untargetable by abilities + Blueprint IsInvincible; coexists with Dread.
    "Veiled": [
        ("untargetable+invincible+Dread coexists+expiry", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "status_veiled.txt",
         [(r"status Veiled 3\.0s x0\.00", True), (r"Veiled on -> IsInvincible=1", True), (r"Veiled off -> IsInvincible=0", True),
          (r"gas=\[(Dread [0-9.]+s x0\.35 Veiled|Veiled [0-9.]+s x0\.00 Dread)", True),    # both at once (the old slot replaced Dread)
          # hp unchanged across the veiled Molten Strike and Blueprint hit, then the post-veil Blueprint hit lands
          (r"(?s)ABILITY f1 BP_Enemy\S+ hp=([0-9.]+)/.*?ABILITY f1 BP_Enemy\S+ hp=\1/.*?ABILITY f1 BP_Enemy\S+ hp=\1/.*?ABILITY f1 BP_Enemy\S+ hp=(?!\1/)", True),
          (r"gas=\[Dread [0-9.]+s x0\.35\] walkx=0\.65", True),                            # Dread outlived the veil
          (r"MoltenStrike hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),                     # none while veiled, one after
          (r"cue GameplayCue\.GOTA\.Status\.Veiled on", True), (r"cue GameplayCue\.GOTA\.Status\.Veiled removed", True)]),
        ("Nyx Void Shield applies it", "?Opponent=AI?Hero=Nyx?DevHeroes=1", "status_veiled_shield.txt",
         [(r"status Veiled 1\.3s", True), (r"Veiled on -> IsInvincible=1", True), (r"Veiled off -> IsInvincible=0", True)]),
    ],
    # Unbreakable: Blueprint IsInterruptible off, refund of Magnitude x each hit, no cast interrupt; coexists with Slowed.
    "Unbreakable": [
        ("refund+no interrupt+Slowed coexists+expiry", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "status_unbreakable.txt",
         [(r"status Unbreakable 4\.0s x0\.50", True), (r"Unbreakable on -> IsInterruptible=0", True),
          (r"Unbreakable absorbed 10\.0 of 20\.0", True), (r"Unbreakable absorbed 5\.0 of 10\.0", True),
          (r"gas=\[(Unbreakable [0-9.]+s x0\.50 Slowed|Slowed [0-9.]+s x0\.40 Unbreakable)", True),
          (r"InfernoSlam hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),                      # the armoured cast went through
          (r"InfernoSlam cancelled \(interrupted\)", 1),                                    # the unarmoured one did not
          (r"Unbreakable off -> IsInterruptible=1", True), (r"gas=\[Slowed [0-9.]+s x0\.40\] walkx=0\.60", True)]),
        ("Varoth Colossus Form applies it", "?Opponent=AI?Hero=Varoth?DevHeroes=1", "status_colossus.txt",
         [(r"status Unbreakable 6\.0s x0\.50", True), (r"Unbreakable absorbed 10\.0 of 20\.0", True)]),
    ],
    # Boss status policy (Quarry Behemoth: immune to Slow and Stun). No multiplayer case: a boss duel is one player.
    "Boss": [
        ("Time Lash slow resisted + feedback, Dread weakens only", "?Opponent=AI?Hero=Kairos?DevHeroes=1?Boss=QuarryBehemoth", "boss_policy.txt",
         [(r"status policy QuarryBehemoth immune=\[Slow Stun\]", True),
          (r"TimeLash hit BP_Boss\S+ for 6\.0 \(landed=1\)", True),                         # the damage still lands
          (r"BP_Boss\S*: status Slowed resisted \(whole movement \) from BP_Player", True),
          (r"FEEDBACK THE QUARRY BEHEMOTH RESISTS SLOW", True),
          (r"BP_Boss\S*: status Slowed [0-9]", False), (r"BP_Boss\S*: GAS move", False),     # never slowed
          (r"BP_Boss\S*: status Dread 9\.0s x0\.35 .*\[GAS walk x1\.00 dmg x0\.8[23]\]", True),
          (r"Boss ClawCombo hit BP_Player\S+ for 9\.0 \(landed=1\)", True),                   # before Dread
          (r"Boss ClawCombo hit BP_Player\S+ for 7\.4 \(landed=1\)", True),                   # 9 x 0.825 with Dread
          (r"ABILITY f1 gas=\[Dread [0-9.]+s x0\.35\] immune=\[Slow Stun\] walkx=1\.00 dmgx=0\.8[23]", True)]),
        ("Divine Counter, parry and dodge unchanged", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "slice_mechanics.txt",
         [(r"Boss STUNNED by Divine Counter from \S+ for 2\.6s", 2), (r"DIVINE COUNTER by \S+ on BP_Boss\S+: 90 damage \(landed=1", 2),
          (r"Boss PARRIED by", True), (r"perfect dodge", True), (r"status policy QuarryBehemoth immune=\[Slow Stun\]", True)]),
        ("boss death clears statuses, keeps policy", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "boss_death.txt",
         [(r"BP_Boss\S*: cleared 1 status effect\(s\) \(death\)", True), (r"Combatant BP_Boss\S+ defeated", True)]),
    ],
    # Burning (Fire): damage over time through the Blueprint damage system. 2.0 a tick, one tick a second, no tick on
    # application. "Burning tick on" lines carry the health before and after, which is the proof of the damage.
    "Burning": [
        ("Molten Strike ignites: impact once, three exact ticks", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "burn_molten.txt",
         [(r"MoltenStrike hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),
          (r"BP_Enemy\S+: status Burning 3\.0s x2\.00 \(from BP_Player", 1),
          (r"Burning applied: 2\.0 per tick every 1\.00s for 3\.0s, owner BP_Player", 1),
          (r"BP_Player\S+: Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\) hp 238\.0 -> 236\.0 \[GOTA\.Element\.Fire\]", 1),
          (r"Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\) hp 236\.0 -> 234\.0", 1),
          (r"Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\) hp 234\.0 -> 232\.0", 1),
          (r"Burning tick on", 3),                                                          # no tick on application, no fourth
          (r"Momentum BP_Player\S+ \+3 \(hit landed\)", 1),                                  # the strike; ticks are not hits
          (r"ABILITY f0 BP_Player\S+ hp=\S+ momentum=5 ", 3),                                # 3 + 2 from the strike, unchanged by the burn
          (r"ABILITY f1 BP_Enemy\S+ hp=238\.0/", 1), (r"ABILITY f1 BP_Enemy\S+ hp=232\.0/", 2),   # still 232 two seconds later
          (r"ABILITY f1 gas=\[Burning [0-9.]+s x2\.00\]", 1),
          (r"GAS standalone ascs=2 withStatus=1 burning=1 flames=1 burnTicks=0", 1),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=3 burnDamage=6\.0", 1),
          (r"cue GameplayCue\.GOTA\.Status\.Burning on BP_Enemy", True), (r"cue GameplayCue\.GOTA\.Status\.Burning removed from BP_Enemy", 1),
          (r"burn fx P_Fire_Small on BP_Enemy", 1), (r"burn fx removed from BP_Enemy", 1), (r"burn fx missing", False)]),
        ("re-application refreshes, never compounds; repeated cycles clean up", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "burn_refresh.txt",
         [(r"Burning applied: 2\.0 per tick", 5), (r"Burning refreshed: 2\.0 per tick", 1), (r"\(hit landed\)", False),
          (r"GAS standalone ascs=2 withStatus=1 burning=1 flames=1 burnTicks=1 ", 1),        # one effect, one flame after the refresh
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=4 burnDamage=8\.0", 1),
          (r"ABILITY f1 BP_Enemy\S+ hp=252\.0/", 1),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=12 burnDamage=24\.0", 1),
          (r"ABILITY f1 BP_Enemy\S+ hp=236\.0/", 1),
          (r"burn fx P_Fire_Small on BP_Enemy", 5), (r"burn fx removed from BP_Enemy", 5)]),
        ("coexists with Dread and Slowed; obeys Veiled, Unbreakable; no cast interrupt", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "burn_status.txt",
         [(r"ABILITY f1 gas=\[(?=[^\]]*Burning)(?=[^\]]*Dread)(?=[^\]]*Slowed)[^\]]*\] immune=\[\] walkx=0\.60", 1),   # A
          (r"GAS move scale=0\.(39|3[0-9]|2)", False),
          (r"Burning tick on BP_Enemy\S+ for 2\.0 \(landed=\d\) hp ([0-9.]+) -> \1 ", 2),     # B: two ticks under the veil do nothing
          (r"Veiled on -> IsInvincible=1", 1), (r"Veiled off -> IsInvincible=0", 1),
          (r"Unbreakable absorbed 1\.0 of 2\.0", 3),                                         # C
          (r"MoltenStrike hit BP_Enemy\S+ for 18\.[12] \(landed=1\)", 1),                     # D: impact weakened by Dread
          (r"BP_Player\S+: Burning tick on BP_Enemy\S+ for 2\.0 ", 15),                       # A3 + B3 + C3 + D3 + E3 (his strike ignites again), all 2.0
          (r"Burning tick on BP_Player\S+ for 2\.0 \(landed=1\)", 3),                         # E: Drakon burns
          (r"MoltenStrike hit BP_Enemy\S+ for 22\.0 \(landed=1\)", 1),                        # E: his cast went through
          (r"MoltenStrike cancelled", False),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=18 burnDamage=32\.0", 1)]),   # 18 ticks, two blocked by the veil
        ("death by burning: kill credit, cleanup, no tick after death", "?Opponent=AI?Hero=Drakon?DevHeroes=1", "burn_death.txt",
         [(r"ABILITY f1 BP_Enemy\S+ hp=3\.0/", 1),
          (r"Burning tick on BP_Enemy\S+ for 2\.0 \(landed=1\) hp 3\.0 -> 1\.0", 1),
          (r"Burning tick on", 2),
          (r"Combatant BP_Enemy\S+ defeated \(by BP_Player", 1),
          (r"BP_Enemy\S*: cleared 1 status effect\(s\) \(death\)", 1),
          (r"burn fx removed from BP_Enemy", 1),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=2 ", 1)]),
        ("boss: burns by default; Burn immunity and resistance + feedback", "?Opponent=AI?Hero=Drakon?DevHeroes=1?Boss=QuarryBehemoth", "burn_boss.txt",
         [(r"status policy QuarryBehemoth immune=\[Slow Stun\]", True),
          (r"MoltenStrike hit BP_Boss\S+ for 22\.0 \(landed=1\)", 3),                         # the impact never changes
          (r"BP_Boss\S*: status Burning 3\.0s x2\.00", 1),
          (r"Burning tick on BP_Boss\S+ for 2\.0 \(landed=1\)", 3),
          (r"BP_Boss\S*: status Burning resisted \(whole burn \) from BP_Player", 1),
          (r"FEEDBACK THE QUARRY BEHEMOTH RESISTS BURNING", 1),
          (r"BP_Boss\S*: status Burning resisted \(reduced\) from BP_Player", 1),
          (r"FEEDBACK THE QUARRY BEHEMOTH SHRUGS OFF PART OF BURNING", 1),
          (r"BP_Boss\S*: status Burning 1\.5s x1\.00", 1),
          (r"Burning tick on BP_Boss\S+ for 1\.0 \(landed=1\)", 1),
          (r"Burning tick on", 4),
          (r"GAS standalone ascs=2 withStatus=0 burning=0 flames=0 burnTicks=4 burnDamage=7\.0", 1)]),
    ],
}
# Multiplayer: host hero, host script, host-log regex (the ability landing on the remote player), client animation.
MP = {
    "GaleRush": ("Zephyr", "gale_mp_host.txt", r"GaleRush hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Ability_RMB"),
    "GravityWell": ("Lysara", "grav_mp_host.txt", r"GravityWell hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Ability_R_InPlace"),
    "TimeLash": ("Kairos", "timelash_mp_host.txt", r"TimeLash hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Burden"),
    "GravitySlam": ("Orion", "slam_mp_host.txt", r"GravitySlam hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Steel_Ability_GroundSmash_End"),
    "TitanCharge": ("Drakon", "titancharge_mp_host.txt", r"TitanCharge hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Attack_RMB"),
    "ShadowStep": ("Nyx", "shadowstep_mp_host.txt", r": teleport V\(", "Ability_RMB"),
    "InfernoSlam": ("Varoth", "infernoslam_mp_host.txt", r"InfernoSlam hit BP_Player\S+ for [0-9.]+ \(landed=1\)", "Swing3_Slow"),
}
# Multiplayer for statuses: host hero, host script, host-log checks, client-log checks (the remote player's own view).
MP_STATUS = {
    "Dread": ("Nyx", "dread_mp_host.txt",
              [(r"SoulDrain hit BP_Player\S+ for [0-9.]+ \(landed=1\)", True), (r"status Dread 4\.0s x0\.35", True)],
              [(r"cue GameplayCue\.GOTA\.Status\.Dread on BP_Player", True), (r"overlay \S+ -> M_GOTA_Overlay_Dread on BP_Player", True), (r"BP_Player\S*: GAS move scale=0\.65", True),
               (r"BP_Player\S*: GAS move scale=0\.60", True), (r"BP_Player\S*: GAS move restored", True),
               (r"GAS move scale=0\.(39|3[0-9]|2)", False), (r"cue GameplayCue\.GOTA\.Status\.Dread removed from BP_Player", True)]),
    "Veiled": ("Drakon", "veiled_mp_host.txt",
               [(r"Veiled on -> IsInvincible=1", True), (r"MoltenStrike hit BP_Player\S+ for 22\.0 \(landed=1\)", 1)],
               [(r"cue GameplayCue\.GOTA\.Status\.Veiled on BP_Player", True), (r"overlay \S+ -> M_GOTA_Overlay_Veiled on BP_Player", True),
                (r"overlay M_GOTA_Overlay_Veiled -> None on BP_Player", True), (r"cue GameplayCue\.GOTA\.Status\.Veiled removed from BP_Player", True)]),
    "Unbreakable": ("Drakon", "unbreakable_mp_host.txt",
                    [(r"Unbreakable on -> IsInterruptible=0", True), (r"Unbreakable absorbed 11\.0 of 22\.0", True)],
                    [(r"cue GameplayCue\.GOTA\.Status\.Unbreakable on BP_Player", True), (r"overlay \S+ -> M_GOTA_Overlay_Unbreakable on BP_Player", True),
                     (r"overlay M_GOTA_Overlay_Unbreakable -> M_GOTA_Overlay_Slowed on BP_Player", True),   # priority hand-off at expiry
                     (r"BP_Player\S*: GAS move scale=0\.60", True),
                     (r"cue GameplayCue\.GOTA\.Status\.Unbreakable removed from BP_Player", True)]),
    "Burning": ("Drakon", "burn_mp_host.txt",
                [(r"MoltenStrike hit BP_Player\S+ for 22\.0 \(landed=1\)", 1), (r"BP_Player\S+: status Burning 3\.0s x2\.00", 1),
                 (r"Burning tick on BP_Player\S+ for 2\.0 \(landed=1\)", 3),
                 (r"GAS listen-server ascs=2 withStatus=0 burning=0 flames=0 burnTicks=3 burnDamage=6\.0", 1)],
                [(r"cue GameplayCue\.GOTA\.Status\.Burning on BP_Player", True), (r"burn fx P_Fire_Small on BP_Player", 1),
                 (r"FEEDBACK STATUS BURNING on", 1), (r"FEEDBACK STATUS BURNING off", 1),
                 (r"burn fx removed from BP_Player", 1), (r"cue GameplayCue\.GOTA\.Status\.Burning removed from BP_Player", 1),
                 (r"Burning tick on", False)]),                                               # ticks are server-only
}


def exe_and_args(packaged):
    if packaged:
        return [os.path.join(PACKAGE_DIR, "WarOfGods.exe")], []
    return [EDITOR, UPROJECT], ["-game"]


def launch(packaged, url, script, log, extra=(), timeout=420, wait=True):
    exe, mode = exe_and_args(packaged)
    args = exe + [url] + mode + ["-windowed", "-ResX=1280", "-ResY=720", "-nosplash", "-unattended", "-nop4", "-log",
                                 "-abslog=" + log, '-ExecCmds=gota.Heroes.AllowInDevelopment 1,gota.Test.Run Tools/Tests/' + script] + list(extra)
    if os.path.exists(log):
        os.remove(log)
    p = subprocess.Popen(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if not wait:
        return p
    try:
        p.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        p.kill()
    return p


def read(log):
    return open(log, encoding="utf-8", errors="ignore").read() if os.path.exists(log) else ""


def check(text, expectations):
    out = []
    for rx, expect in expectations:
        if isinstance(expect, bool):
            out.append((rx, expect, (re.search(rx, text) is not None) == expect))
        else:
            n = len(re.findall(rx, text))
            out.append(("%s (x%d, got %d)" % (rx, expect, n), True, n == expect))
    return out


STAMP_RX = re.compile(r"^\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\]", re.M)


def to_s(stamp):
    """HH.MM.SS:mmm -> seconds."""
    return int(stamp[0:2]) * 3600 + int(stamp[3:5]) * 60 + int(stamp[6:8]) + int(stamp[9:12]) / 1000.0


def first_time(text, rx):
    """Timestamp of the first log line matching rx, or '-'."""
    for line in text.splitlines():
        if re.search(rx, line):
            m = STAMP_RX.match(line)
            return m.group(1)[11:] if m else "?"
    return "-"


def keep_failure(run_dir, *logs):
    """Copy a failing case's logs to Saved/AbilityRegression/failures/<run>/ so a rerun cannot overwrite them."""
    os.makedirs(run_dir, exist_ok=True)
    for log in logs:
        if os.path.exists(log):
            shutil.copy2(log, run_dir)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--packaged", action="store_true")
    ap.add_argument("--only")
    ap.add_argument("--check-only", action="store_true")
    ap.add_argument("--mp-only", action="store_true", help="only the multiplayer (listen server + client) cases")
    ap.add_argument("--repeat", type=int, default=1, help="run the selected cases N times in a row")
    a = ap.parse_args()
    total_fails = 0
    for rep in range(max(1, a.repeat)):
        if a.repeat > 1:
            print("\n=== pass %d of %d ===" % (rep + 1, a.repeat))
        total_fails += run_suite(a)
    return 1 if total_fails else 0


def run_suite(a):
    run_dir = os.path.join(LOGS, "failures", time.strftime("%Y%m%d_%H%M%S"))
    os.makedirs(LOGS, exist_ok=True)
    if a.packaged and not a.check_only:  # the packaged game reads test scripts from its own project folder
        dst = os.path.join(PACKAGE_DIR, "WarOfGods", "Tools", "Tests")
        os.makedirs(dst, exist_ok=True)
        for f in os.listdir(os.path.join(ROOT, "Tools", "Tests")):
            shutil.copy2(os.path.join(ROOT, "Tools", "Tests", f), dst)
    mode = "packaged" if a.packaged else "editor"
    fails = 0
    for ability, cases in CASES.items():
        if a.only and a.only != ability:
            continue
        for name, opts, script, exp in ([] if a.mp_only else cases):
            log = os.path.join(LOGS, "%s_%s_%s.log" % (mode, ability, re.sub(r"\W+", "_", name)))
            if not a.check_only:
                launch(a.packaged, QUARRY + opts, script, log)
            text = read(log)
            results = check(text, exp) + [("no crash", True, "Fatal error" not in text)]
            ok = all(r[2] for r in results)
            fails += 0 if ok else 1
            if not ok:
                keep_failure(run_dir, log)
            print("%-5s %-12s %-42s %s" % ("PASS" if ok else "FAIL", ability, name, "" if ok else
                  "; ".join(("missing " if must else "unexpected ") + rx for rx, must, good in results if not good)))
            sys.stdout.flush()
        if ability not in MP_STATUS and ability not in MP:
            continue
        if ability in MP_STATUS:
            hero, host_script, host_checks, client_checks = MP_STATUS[ability]
        else:
            hero, host_script, host_rx, anim = MP[ability]
            host_checks = [(host_rx, True)]
            client_checks = [(r"body plays " + anim + r" \(ok", True), (r"BODY \[client\].*hero=" + hero, True)]
        host_log = os.path.join(LOGS, "%s_%s_mp_host.log" % (mode, ability))
        client_log = os.path.join(LOGS, "%s_%s_mp_client.log" % (mode, ability))
        if not a.check_only:
            host = launch(a.packaged, QUARRY + "?Hero=%s?DevHeroes=1?listen" % hero, host_script, host_log, wait=False)
            # Start the client once the host is listening (a cold host start took 49 s; a fixed 15 s launched the
            # client too early and it never joined). The host script itself waits for the client (waitplayers).
            deadline = time.time() + 150
            time.sleep(5)
            while time.time() < deadline and not re.search(r"(GameNetDriver.*listening|Listening on port|IpNetDriver listening)", read(host_log)):
                time.sleep(1)
            time.sleep(2)
            launch(a.packaged, "127.0.0.1?Hero=UPAWarden", "gale_mp_client.txt", client_log, extra=["-LogCmds=LogGOTAAbility Verbose"], timeout=300)
            try:
                host.wait(timeout=180)  # host scripts stay up 75 s after their action so a slow client can finish checking
            except subprocess.TimeoutExpired:
                host.kill()
        h, c = read(host_log), read(client_log)
        results = check(h, host_checks) + check(c, client_checks) + [("no crash", True, "Fatal error" not in h + c)]
        ok = all(r[2] for r in results)
        fails += 0 if ok else 1
        # Connection timeline: when the host listened, when the client logged in and finished loading, when the client
        # script ran. A missing client line with an early or absent join points at timing, not the ability.
        timing = "host listen %s, client join %s, client map %s, client script %s, client body %s" % (
            first_time(h, r"(GameNetDriver.*listening|Listening on port|IpNetDriver listening)"),
            first_time(h, r"Join succeeded"), first_time(c, r"LoadMap: .*L_Arena_TitansQuarry"),
            first_time(c, r"Running .*gale_mp_client"), first_time(c, r"BODY \[client\]"))
        # A starved client stretches its script clock (frame time, capped per frame): report the real length of its
        # first 18 s wait. Far above 18 s means the client process was not getting frames.
        t0, t1 = first_time(c, r"\[script\] wait 18"), first_time(c, r"\[script\] > gota.Test.Body")
        if t0 not in ("-", "?") and t1 not in ("-", "?"):
            timing += ", client 'wait 18' took %.1fs" % (to_s(t1) - to_s(t0))
        # A client whose 18 s script wait took over twice as long was starved of frames (machine load): its client-side
        # checks prove nothing either way. Report INCONCLUSIVE instead of PASS / FAIL (it still fails the run).
        starved_s = None
        if t0 not in ("-", "?") and t1 not in ("-", "?"):
            starved_s = to_s(t1) - to_s(t0)
        elif t0 not in ("-", "?") and first_time(c, r"\[script\] complete") == "-":
            starved_s = float("inf")
        # A host that stalled (one frame of 15 s or more, e.g. compiling shaders for a hero body on first use) runs its
        # timed steps after the client's script has already finished: also INCONCLUSIVE, never PASS.
        stalls = [float(x) for x in re.findall(r"Very long time between ticks\. DeltaTime: ([\d.]+)", h)]
        host_stall = max(stalls) if stalls else 0.0
        if host_stall:
            timing += ", host's longest frame %.1fs" % host_stall
        why = ""
        if not ok and starved_s is not None and starved_s > 36:
            why = "(client starved) "
        elif not ok and host_stall >= 15:
            why = "(host stalled %.0fs) " % host_stall
        verdict = "PASS" if ok else ("INCONCLUSIVE" if why else "FAIL")
        if not ok:
            keep_failure(run_dir, host_log, client_log)
        print("%-5s %-12s %-42s %s" % (verdict, ability, "multiplayer (listen server + client)", "" if ok else
              why +
              "; ".join(("missing " if must else "unexpected ") + rx for rx, must, good in results if not good)))
        print("      %-12s timing: %s" % ("", timing))
        sys.stdout.flush()
    print("\n%s: %s" % (mode, "ALL PASS" if fails == 0 else "%d FAILED" % fails))
    return fails


if __name__ == "__main__":
    sys.exit(main())
