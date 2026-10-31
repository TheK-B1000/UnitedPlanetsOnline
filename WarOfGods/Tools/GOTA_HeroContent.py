# Gods of the Arena — hero roster content (transcribed from the approved hero cards in
# Docs/References/Heroes). Only what the cards state. Empty fields = not stated (never invented).
# Consumed by GOTA_BuildMenuUI.py.

ABILITY = lambda name, desc: (name, desc)  # noqa: E731

HEROES = [
    dict(id="Ayla", order=0, name="Ayla", title="DESERT STORM",
         quote="The storm within never stops. It only chooses a direction.",
         faction="UNITED PLANETS AUTHORITY", cls="ASCENDANT CLASS", spec="MOBILITY • ELEMENTAL • PRECISION",
         stats=(8, 6, 9, 8),
         bio="Ayla is a fusion of human, machine, and the divine. Born human and transformed through advanced cybernetics, "
             "she commands metal itself — turning the battlefield into her weapon. A survivor of empires and a defender "
             "of free will, Ayla fights with unmatched precision, speed, and control.",
         abilities=[("Iron Sense", "Detect metal, reveal enemies and weak points."),
                    ("Thousand Blades", "Control metal fragments."),
                    ("Cybernetic Overdrive", "Increase speed, strength and reflexes."),
                    ("Living Arsenal", "Shape metal weapons and defenses."),
                    ("Iron Dominion", "Control all metal in the arena.")],
         weapon="Dual Cybernetic Gauntlets", accent=(0.35, 0.65, 1.0)),
    dict(id="Drakon", order=1, name="Drakon", title="WARFALL TITAN",
         quote="Strength is the honest truth that outlives every lie.",
         faction="", cls="", spec="", stats=(9, 9, 6, 7),
         bio="Drakon is a Warfall Titan, forged in the fires of broken worlds. He believes strength is a form of justice and "
             "that true freedom is earned through struggle. On the Arena, he fights not for glory, but to test the limits of his own will.",
         abilities=[("Dragon Roar", "Area control. Knock back enemies."),
                    ("Titan Charge", "High movement. Breaks guard."),
                    ("Molten Strike", "Burst damage. Ignites enemies."),
                    ("Unchained Fury", "Ultimate. Massive area damage.")],
         weapon="Two-Handed Titan Axe", accent=(1.0, 0.35, 0.12)),
    dict(id="Lysara", order=2, name="Lysara", title="ASTRAL SOVEREIGN",
         quote="Balance is not peace. Balance is power.",
         faction="", cls="", spec="", stats=(7, 8, 8, 9),
         bio="Lysara is an Astral Sovereign, a wielder of celestial arts and keeper of cosmic balance. She commands the forces "
             "of light, gravity, and time, fighting with elegance and precision. In the Arena, she does not seek to dominate, "
             "but to bring clarity to chaos.",
         abilities=[("Astral Orbit", "Control space and distance."),
                    ("Gravity Well", "Pull enemies and disrupt."),
                    ("Celestial Step", "Teleport and counter."),
                    ("Starfall", "Rain celestial energy."),
                    ("Orb of Equilibrium", "Ultimate. Balance all forces.")],
         weapon="Celestial Twin Rings", accent=(0.95, 0.75, 0.35)),
    dict(id="Kairos", order=3, name="Kairos", title="CHRONOS WARDEN",
         quote="Time is a weapon, and I decide how it flows.",
         faction="", cls="", spec="", stats=(8, 8, 9, 9),
         bio="Kairos is a Chronos Warden, a master of temporal control and precision. He bends the flow of battle, striking "
             "before his enemies can react. To Kairos, time is not a constraint, but a battlefield. In the Arena, he fights to "
             "maintain the order of the cosmos and prevent its collapse.",
         abilities=[("Time Lash", "Area control. Slow enemies."),
                    ("Chrono Step", "Instant dash. Afterimage."),
                    ("Temporal Prison", "Trap and suspend enemies."),
                    ("Time Fracture", "Burst damage. Split timeline."),
                    ("Aeon Strike", "Ultimate. Stop time briefly.")],
         weapon="Chrono Blade", accent=(0.3, 0.55, 1.0)),
    dict(id="Zephyr", order=4, name="Zephyr", title="WIND REBELLION",
         quote="Freedom moves faster than fear.",
         faction="", cls="", spec="", stats=(8, 6, 10, 8),
         bio="Zephyr is a Wind Warden, a master of speed, aerial combat, and battlefield control. He fights with fluid precision, "
             "turning air into a weapon and using movement to overwhelm his enemies. A free spirit who refuses to be bound, "
             "Zephyr believes that true strength lies in freedom — the ability to choose your own path.",
         abilities=[("Gale Rush", "High-speed dash attack."),
                    ("Wind Blades", "Ranged slashes of compressed air."),
                    ("Updraft", "Launch upward and reposition."),
                    ("Storm Cage", "Area control. Traps enemies."),
                    ("Freedom's Flight", "Ultimate. Unstoppable speed.")],
         weapon="Twin Wind Blades", accent=(0.6, 0.8, 1.0)),
    dict(id="Varoth", order=5, name="Varoth", title="INFERNO COLOSSUS",
         quote="Creation burns. And from the ashes, stronger worlds rise.",
         faction="", cls="", spec="", stats=(10, 9, 6, 8),
         bio="Varoth is an Inferno Colossus, a living embodiment of controlled destruction. Once a guardian of dying worlds, he "
             "now fights in the Arena to test the strength of those who claim to shape the future. Varoth believes that true "
             "progress is born from hardship, and that only through fire can weakness be destroyed and a stronger order emerge.",
         abilities=[("Inferno Slam", "Area damage. Launch enemies."),
                    ("Molten Spikes", "Create pillars of fire."),
                    ("Ashen Charge", "High-impact rush attack."),
                    ("World Burn", "Burning area. Damage over time."),
                    ("Colossus Form", "Increased power, defense, and area damage.")],
         weapon="Inferno Titan Blade", accent=(1.0, 0.25, 0.08)),
    dict(id="Nyx", order=6, name="Nyx", title="SHADOW WEAVER",
         quote="The dark isn't the absence of light. It's where I find my power.",
         faction="UNITED PLANETS AUTHORITY", cls="ASCENDANT CLASS", spec="INTELLIGENCE • STEALTH • CONTROL",
         stats=(8, 7, 9, 10),
         bio="Nyx is a master of shadows, illusion, and mind. She moves unseen, strikes without warning, and bends reality to "
             "her will. Her power grows in the dark, where she can manipulate shadows, control minds, and unravel her enemies from within.",
         abilities=[("Shadow Step", "Short-range teleportation."),
                    ("Mind Spike", "Stuns and disrupts."),
                    ("Void Shield", "Absorbs damage and negates control."),
                    ("Illusion", "Creates decoys and confusion."),
                    ("Soul Drain", "Steals energy and weakens.")],
         weapon="Shadow Blades", accent=(0.65, 0.3, 1.0)),
    dict(id="Orion", order=7, name="Orion", title="STARFORGE SENTINEL",
         quote="I defend what can be more than this.",
         faction="", cls="", spec="", stats=(8, 9, 8, 8),
         bio="Orion is a Starforge Sentinel, a warrior who protects the balance between worlds. Blending advanced battlecraft, "
             "energy manipulation, and gravitational control, he stands as a shield for the United Planets, fighting to ensure "
             "a tomorrow where all peoples can choose their own path.",
         abilities=[("Gravity Slam", "Pull and crush enemies in an area."),
                    ("Starward Dash", "High-speed dash with gravitational trail."),
                    ("Orbital Guard", "Create a gravitational shield that absorbs damage."),
                    ("Astral Spear", "Launch condensed gravity projectiles."),
                    ("Nova Field", "Create a vortex that pulls, stuns, and destabilizes enemies.")],
         weapon="Gravity Lance", accent=(0.3, 0.55, 1.0)),
]

# Prototype bodies (free Epic Paragon assets) worn over the BP_Player combat frame through a runtime
# IK retargeter (Tools/GOTA_BuildParagonRigs.py, UGOTAHeroBodyComponent). These are PROTOTYPE bodies,
# not final god designs, and do not change combat. A hero stays IN_DEVELOPMENT until a full duel
# (selection, spawn, combat, animation, damage, match end) has been verified with it.
# Paragon licence: use in Unreal projects is allowed; never use the Paragon name to market the game.
PARAGON_RIGS = "/Game/Heroes/Paragon/Rigs"
BODIES = {
    "Drakon": dict(mesh="/Game/ParagonGreystone/Characters/Heroes/Greystone/Meshes/Greystone",
                   retargeter=PARAGON_RIGS + "/RTG_GOTA_Manny_Greystone", scale=1.0,
                   note="Paragon: Greystone (prototype body)", status="IN_DEVELOPMENT"),
    "Nyx": dict(mesh="/Game/ParagonCountess/Characters/Heroes/Countess/Meshes/SM_Countess",
                retargeter=PARAGON_RIGS + "/RTG_GOTA_Manny_Countess", scale=1.0,
                note="Paragon: Countess (prototype body)", status="IN_DEVELOPMENT"),
    "Varoth": dict(mesh="/Game/ParagonSevarog/Characters/Heroes/Sevarog/Meshes/Sevarog",
                   retargeter=PARAGON_RIGS + "/RTG_GOTA_Manny_Sevarog", scale=1.0,
                   note="Paragon: Sevarog (prototype body)", status="IN_DEVELOPMENT"),
}
IN_DEVELOPMENT_REASON = ("In development: prototype body over the shared combat frame. Not yet verified "
                         "through a full match, so it cannot be selected.")

PREVIEW_REASON = ("No gameplay character exists for this hero yet (no mesh, animation or combat setup in the project). "
                  "Preview only — it will not be substituted with another fighter.")

# The one hero that maps to a real, validated gameplay pawn today. Clearly labelled as such.
PROTOTYPE = dict(id="UPAWarden", order=100, name="UPA Warden", title="PROTOTYPE COMBAT FRAME",
                 quote="", faction="UNITED PLANETS AUTHORITY", cls="WARDEN", spec="",
                 stats=(0, 0, 0, 0),
                 bio="The current playable combat character (BP_Player). Use it to play Trial of Champions while the "
                     "roster heroes' gameplay characters are being built. Card statistics are not defined for this frame.",
                 abilities=[], weapon="Standard Warden kit", accent=(0.78, 0.55, 0.22))
