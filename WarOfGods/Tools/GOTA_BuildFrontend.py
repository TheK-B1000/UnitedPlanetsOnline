# White Tiger Studios — Gods of the Arena
# Editor Python: create WBP_GOTA_* assets, dress L_GOTA_MainMenu, place cameras.
# Run: UnrealEditor-Cmd.exe WarOfGods.uproject -ExecutePythonScript=".../GOTA_BuildFrontend.py"

import unreal

UI_PATH = "/Game/MainMenu/UI"
MAP_PATH = "/Game/MainMenu/L_GOTA_MainMenu"
GOLD = unreal.LinearColor(0.85, 0.65, 0.22, 1.0)
STONE = unreal.LinearColor(0.04, 0.035, 0.03, 0.92)
CREAM = unreal.LinearColor(0.92, 0.88, 0.78, 1.0)


def log(msg: str):
    unreal.log(f"[GOTA Frontend] {msg}")


def ensure_dir(path: str):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def load_class(path: str):
    cls = unreal.load_class(None, path)
    if not cls:
        raise RuntimeError(f"Missing class: {path}")
    return cls


def create_widget_bp(name: str, parent_class_path: str):
    ensure_dir(UI_PATH)
    asset_path = f"{UI_PATH}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        log(f"Exists, reloading: {asset_path}")
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", load_class(parent_class_path))
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(name, UI_PATH, unreal.WidgetBlueprint, factory)
    if not asset:
        raise RuntimeError(f"Failed to create {name}")
    unreal.EditorAssetLibrary.save_asset(asset_path)
    log(f"Created {asset_path}")
    return asset


def make_brush(color: unreal.LinearColor, tint=None):
    brush = unreal.SlateBrush()
    brush.set_editor_property("tint_color", unreal.SlateColor(color if tint is None else tint))
    brush.set_editor_property("draw_as", unreal.SlateBrushDrawType.BOX)
    return brush


def set_slot_anchors(slot, min_x, min_y, max_x, max_y, off_l=0, off_t=0, off_r=0, off_b=0):
    slot.set_anchors(unreal.Anchors(min_x, min_y, max_x, max_y))
    slot.set_offsets(unreal.Margin(off_l, off_t, off_r, off_b))
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))


def build_menu_button_tree(wbp):
    """Build BindWidgetOptional: Button_Root + Text_Label."""
    tree = wbp.widget_tree
    root = tree.construct_widget(unreal.SizeBox, "RootSize")
    root.set_editor_property("width_override", 420.0)
    root.set_editor_property("height_override", 56.0)
    tree.root_widget = root

    button = tree.construct_widget(unreal.Button, "Button_Root")
    style = unreal.ButtonStyle()
    normal = unreal.SlateBrush()
    normal.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(0, 0, 0, 0)))
    hovered = unreal.SlateBrush()
    hovered.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(0.85, 0.65, 0.22, 0.15)))
    style.set_editor_property("normal", normal)
    style.set_editor_property("hovered", hovered)
    style.set_editor_property("pressed", hovered)
    button.set_editor_property("widget_style", style)
    root.add_child(button)

    text = tree.construct_widget(unreal.TextBlock, "Text_Label")
    text.set_text(unreal.Text("BUTTON"))
    text.set_editor_property("color_and_opacity", unreal.SlateColor(CREAM))
    font = text.get_editor_property("font")
    font.size = 28
    text.set_editor_property("font", font)
    button.add_child(text)
    return wbp


def add_named_button(tree, parent_box, name: str, label: str):
    """Add a nested WBP_GOTA_MenuButton instance if available, else a labeled button row."""
    btn_class = unreal.load_class(None, "/Game/MainMenu/UI/WBP_GOTA_MenuButton.WBP_GOTA_MenuButton_C")
    if btn_class:
        widget = tree.construct_widget(btn_class, name)
        parent_box.add_child(widget)
        return widget

    # Fallback plain button (still named for BindWidgetOptional on parent C++ expecting UGOTAMenuButtonWidget — needs WBP class)
    size = tree.construct_widget(unreal.SizeBox, name + "_Size")
    size.set_editor_property("height_override", 52.0)
    parent_box.add_child(size)
    button = tree.construct_widget(unreal.Button, name)
    size.add_child(button)
    text = tree.construct_widget(unreal.TextBlock, name + "_Text")
    text.set_text(unreal.Text(label))
    text.set_editor_property("color_and_opacity", unreal.SlateColor(CREAM))
    button.add_child(text)
    return button


def build_screen_shell(wbp, title: str, button_specs):
    """
    Full-screen translucent left rail + title.
    button_specs: list of (bind_name, label)
    """
    tree = wbp.widget_tree
    canvas = tree.construct_widget(unreal.CanvasPanel, "RootCanvas")
    tree.root_widget = canvas

    # Dim vignette for readability over cinematic scene
    dim = tree.construct_widget(unreal.Image, "Img_Vignette")
    dim_brush = unreal.SlateBrush()
    dim_brush.set_editor_property("tint_color", unreal.SlateColor(unreal.LinearColor(0.0, 0.0, 0.0, 0.35)))
    dim.set_editor_property("brush", dim_brush)
    dim_slot = canvas.add_child_to_canvas(dim)
    set_slot_anchors(dim_slot, 0, 0, 1, 1)

    # Left stone panel
    panel = tree.construct_widget(unreal.Border, "Border_LeftPanel")
    panel_brush = unreal.SlateBrush()
    panel_brush.set_editor_property("tint_color", unreal.SlateColor(STONE))
    panel.set_editor_property("background", panel_brush)
    panel.set_editor_property("padding", unreal.Margin(36, 48, 28, 48))
    panel_slot = canvas.add_child_to_canvas(panel)
    set_slot_anchors(panel_slot, 0.0, 0.08, 0.0, 0.92, 48, 0, 520, 0)

    vbox = tree.construct_widget(unreal.VerticalBox, "VBox_Nav")
    panel.set_content(vbox)

    title_text = tree.construct_widget(unreal.TextBlock, "Text_Title")
    title_text.set_text(unreal.Text(title))
    title_text.set_editor_property("color_and_opacity", unreal.SlateColor(GOLD))
    font = title_text.get_editor_property("font")
    font.size = 42
    title_text.set_editor_property("font", font)
    vbox.add_child_to_vertical_box(title_text)

    spacer = tree.construct_widget(unreal.Spacer, "Spacer_Title")
    spacer.set_editor_property("size", unreal.Vector2D(1, 28))
    vbox.add_child_to_vertical_box(spacer)

    for bind_name, label in button_specs:
        add_named_button(tree, vbox, bind_name, label)
        row_space = tree.construct_widget(unreal.Spacer, bind_name + "_Sp")
        row_space.set_editor_property("size", unreal.Vector2D(1, 8))
        vbox.add_child_to_vertical_box(row_space)

    # Brand mark top-center
    brand = tree.construct_widget(unreal.TextBlock, "Text_Brand")
    brand.set_text(unreal.Text("GODS OF THE ARENA"))
    brand.set_editor_property("color_and_opacity", unreal.SlateColor(GOLD))
    brand.set_justification(unreal.TextJustify.CENTER)
    bfont = brand.get_editor_property("font")
    bfont.size = 36
    brand.set_editor_property("font", bfont)
    brand_slot = canvas.add_child_to_canvas(brand)
    set_slot_anchors(brand_slot, 0.5, 0.0, 0.5, 0.0, -320, 36, 320, 90)

    return wbp


def build_frontend_root(wbp, screen_classes):
    tree = wbp.widget_tree
    overlay = tree.construct_widget(unreal.Overlay, "RootOverlay")
    tree.root_widget = overlay
    for name, class_path in screen_classes:
        cls = unreal.load_class(None, class_path)
        if not cls:
            log(f"WARN missing screen class {class_path}")
            continue
        child = tree.construct_widget(cls, name)
        slot = overlay.add_child_to_overlay(child)
        slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
    return wbp


def spawn_mesh(mesh_path, loc, rot=None, scale=None, label=None):
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if not mesh:
        log(f"Missing mesh {mesh_path}")
        return None
    loc = unreal.Vector(*loc)
    rot = unreal.Rotator(*(rot or (0, 0, 0)))
    scale = unreal.Vector(*(scale or (1, 1, 1)))
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
    if not actor:
        return None
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(scale)
    if label:
        actor.set_actor_label(label)
    return actor


def spawn_sk(mesh_path, anim_path, loc, rot=None, scale=None, label="Warrior"):
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if not mesh:
        log(f"Missing skel {mesh_path}")
        return None
    loc = unreal.Vector(*loc)
    rot = unreal.Rotator(*(rot or (0, 0, 0)))
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkeletalMeshActor, loc, rot)
    if not actor:
        return None
    actor.skeletal_mesh_component.set_skeletal_mesh_asset(mesh)
    if scale:
        actor.set_actor_scale3d(unreal.Vector(*scale))
    anim = unreal.EditorAssetLibrary.load_asset(anim_path) if anim_path else None
    if anim:
        actor.skeletal_mesh_component.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
        actor.skeletal_mesh_component.override_animation_data(anim, True, True, 0.0, 1.0)
    actor.set_actor_label(label)
    return actor


def spawn_cine_camera(loc, rot, label, focal=35.0, aperture=2.8):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.CineCameraActor, unreal.Vector(*loc), unreal.Rotator(*rot)
    )
    if not actor:
        return None
    actor.set_actor_label(label)
    cine = actor.get_cine_camera_component()
    if cine:
        filmback = cine.get_editor_property("filmback")
        # Keep defaults; set focus settings lightly
        cine.set_editor_property("current_focal_length", focal)
        cine.set_editor_property("current_aperture", aperture)
    return actor


def spawn_point_light(loc, intensity=8000.0, color=(1.0, 0.45, 0.15), radius=1200.0, label="BrazierLight"):
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PointLight, unreal.Vector(*loc), unreal.Rotator(0, 0, 0)
    )
    if not actor:
        return None
    actor.set_actor_label(label)
    comp = actor.point_light_component
    comp.set_intensity(intensity)
    comp.set_light_color(unreal.LinearColor(color[0], color[1], color[2], 1.0))
    comp.set_attenuation_radius(radius)
    comp.set_editor_property("use_temperature", True)
    comp.set_editor_property("temperature", 2200.0)
    return actor


def spawn_emitter(ps_path, loc, scale=None, label="FX"):
    ps = unreal.EditorAssetLibrary.load_asset(ps_path)
    if not ps:
        return None
    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.Emitter, unreal.Vector(*loc), unreal.Rotator(0, 0, 0)
    )
    if not actor:
        # Niagara-only projects may not spawn legacy Emitter; try ParticleSystemComponent via Actor
        return None
    # Soft set template if property exists
    try:
        actor.set_editor_property("particle_system_component", actor.particle_system_component)
        actor.particle_system_component.set_template(ps)
    except Exception:
        pass
    if scale:
        actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_actor_label(label)
    return actor


def clear_gota_actors():
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        label = actor.get_actor_label()
        if label.startswith("GOTA_"):
            unreal.EditorLevelLibrary.destroy_actor(actor)


def dress_level():
    log("Loading L_GOTA_MainMenu…")
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)
    clear_gota_actors()

    # Lighting
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-35, -40, 0)
    )
    if sun:
        sun.set_actor_label("GOTA_Sun")
        sun.directional_light_component.set_intensity(8.0)
        sun.directional_light_component.set_light_color(unreal.LinearColor(1.0, 0.72, 0.45, 1.0))

    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0, 0, 400), unreal.Rotator(0, 0, 0)
    )
    if sky:
        sky.set_actor_label("GOTA_SkyLight")
        sky.sky_light_component.set_intensity(1.5)

    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
    )
    if fog:
        fog.set_actor_label("GOTA_Fog")
        fog.component.set_fog_density(0.025)
        fog.component.set_fog_height_falloff(0.15)
        fog.component.set_fog_inscattering_color(unreal.LinearColor(0.35, 0.22, 0.12, 1.0))

    # Ground / balcony
    spawn_mesh("/Game/StarterContent/Architecture/Floor_400x400", (0, 0, 0), scale=(8, 8, 1), label="GOTA_BalconyFloor")
    for i, x in enumerate([-600, -200, 200, 600]):
        spawn_mesh("/Game/StarterContent/Architecture/Pillar_50x500", (x, -200, 0), scale=(1.5, 1.5, 1.2), label=f"GOTA_Pillar_{i}")

    # Statues / rocks midground
    spawn_mesh("/Game/StarterContent/Props/SM_Statue", (1200, 1800, 0), rot=(0, -30, 0), scale=(6, 6, 6), label="GOTA_GodStatue_L")
    spawn_mesh("/Game/StarterContent/Props/SM_Statue", (-1400, 2200, 0), rot=(0, 25, 0), scale=(7, 7, 7), label="GOTA_GodStatue_R")
    spawn_mesh("/Game/StarterContent/Props/SM_Rock", (400, 900, 0), scale=(3, 3, 2), label="GOTA_Rock_01")
    spawn_mesh("/Game/StarterContent/Props/SM_Stairs", (0, -50, 0), rot=(0, 0, 0), scale=(2, 2, 1.5), label="GOTA_Stairs")

    # Volcanic / cliff silhouettes
    spawn_mesh("/Game/Geometry/SM_Lava_Cliff_L_01", (2500, 3200, -200), rot=(0, -50, 0), scale=(2, 2, 2), label="GOTA_LavaCliff_R")
    spawn_mesh("/Game/Geometry/SM_Lava_Cliff_L_02", (-2800, 3000, -150), rot=(0, 40, 0), scale=(2.2, 2.2, 2.2), label="GOTA_LavaCliff_L")
    spawn_mesh("/Game/Geometry/SM_Lava_RockMound_S_01", (1800, 800, 0), scale=(3, 3, 3), label="GOTA_Mound_01")

    # Coliseum suggestion — ring of walls
    for i in range(12):
        ang = i * 30.0
        rad = 3500.0
        import math
        x = math.cos(math.radians(ang)) * rad
        y = math.sin(math.radians(ang)) * rad + 4500.0
        spawn_mesh(
            "/Game/StarterContent/Architecture/Wall_400x400",
            (x, y, 0),
            rot=(0, ang + 90.0, 0),
            scale=(3, 1, 4),
            label=f"GOTA_ColiseumWall_{i}",
        )

    # Braziers = lamps + fire lights + particles
    for i, pos in enumerate([(-350, -150, 120), (350, -150, 120), (-500, 200, 80), (500, 200, 80)]):
        spawn_mesh("/Game/StarterContent/Props/SM_Lamp_Wall", pos, scale=(2, 2, 2), label=f"GOTA_Brazier_{i}")
        spawn_point_light((pos[0], pos[1], pos[2] + 40), intensity=12000.0, label=f"GOTA_BrazierLight_{i}")
        spawn_emitter("/Game/StarterContent/Particles/P_Fire", (pos[0], pos[1], pos[2] + 30), scale=(1.5, 1.5, 1.5), label=f"GOTA_Fire_{i}")

    spawn_emitter("/Game/StarterContent/Particles/P_Ambient_Dust", (0, 500, 200), scale=(3, 3, 3), label="GOTA_Dust")
    spawn_emitter("/Game/StarterContent/Particles/P_Smoke", (2200, 2800, 100), scale=(4, 4, 4), label="GOTA_VolcanoSmoke")

    # Foreground warrior
    spawn_sk(
        "/Game/BossyEnemy/SkeletalMesh/SK_Mannequin_UE4_WithWeapon",
        "/Game/BossyEnemy/Animations/Boss_Idle",
        loc=(-80, -320, 10),
        rot=(0, 15, 0),
        scale=(1.15, 1.15, 1.15),
        label="GOTA_Warrior",
    )

    # Champion hall markers (multiplayer camera look-at)
    for i, x in enumerate([-450, -150, 150, 450]):
        spawn_mesh(
            "/Game/LevelPrototyping/Meshes/SM_Cylinder",
            (x, 400, 0),
            scale=(0.8, 0.8, 0.05),
            label=f"GOTA_ChampionPad_{i+1}",
        )

    # Cameras
    cam_main = spawn_cine_camera((-220, -780, 180), (-8, 20, 0), "GOTA_Cam_MainMenu", focal=28.0)
    cam_settings = spawn_cine_camera((420, -100, 140), (-5, -110, 0), "GOTA_Cam_Settings", focal=40.0)
    cam_mp = spawn_cine_camera((0, -200, 220), (-12, 0, 0), "GOTA_Cam_Multiplayer", focal=32.0)

    # Camera director
    director = unreal.EditorLevelLibrary.spawn_actor_from_class(
        load_class("/Script/WarOfGods.GOTAMenuCameraDirector"),
        unreal.Vector(0, 0, 50),
        unreal.Rotator(0, 0, 0),
    )
    if director:
        director.set_actor_label("GOTA_MenuCameraDirector")
        shots = []
        for screen_enum, cam, blend in (
            (unreal.GOTAMenuScreen.MAIN_MENU, cam_main, 1.4),
            (unreal.GOTAMenuScreen.SETTINGS, cam_settings, 1.2),
            (unreal.GOTAMenuScreen.MULTIPLAYER, cam_mp, 1.3),
            (unreal.GOTAMenuScreen.LOBBY, cam_mp, 0.8),
        ):
            if not cam:
                continue
            shot = unreal.GOTAMenuCameraShot()
            shot.set_editor_property("screen", screen_enum)
            shot.set_editor_property("camera_actor", cam)
            shot.set_editor_property("blend_time", blend)
            shots.append(shot)
        director.set_editor_property("shots", shots)
        director.set_editor_property("startup_screen", unreal.GOTAMenuScreen.MAIN_MENU)

    unreal.EditorLevelLibrary.save_current_level()
    log("Level dressed and saved.")


def main():
    log("=== Building cinematic frontend assets ===")
    ensure_dir(UI_PATH)

    # 1) Shared button first
    btn = create_widget_bp("WBP_GOTA_MenuButton", "/Script/WarOfGods.GOTAMenuButtonWidget")
    build_menu_button_tree(btn)
    unreal.EditorAssetLibrary.save_asset(f"{UI_PATH}/WBP_GOTA_MenuButton")

    # 2) Screens
    screens = [
        ("WBP_GOTA_MainMenu", "/Script/WarOfGods.GOTAMainMenuWidget", "MAIN MENU", [
            ("Btn_Continue", "CONTINUE"),
            ("Btn_NewStory", "STORY MODE"),
            ("Btn_PvEArena", "PVE ARENA"),
            ("Btn_Multiplayer", "MULTIPLAYER"),
            ("Btn_Settings", "SETTINGS"),
            ("Btn_Extras", "EXTRAS"),
            ("Btn_Quit", "QUIT"),
        ]),
        ("WBP_GOTA_Settings", "/Script/WarOfGods.GOTASettingsWidget", "SETTINGS", [
            ("Btn_Apply", "APPLY"),
            ("Btn_Cancel", "CANCEL"),
            ("Btn_Defaults", "RESTORE DEFAULTS"),
            ("Btn_Back", "BACK"),
        ]),
        ("WBP_GOTA_Multiplayer", "/Script/WarOfGods.GOTAMultiplayerWidget", "MULTIPLAYER", [
            ("Btn_HostGame", "HOST GAME"),
            ("Btn_JoinGame", "JOIN GAME"),
            ("Btn_FindSessions", "FIND SESSIONS"),
            ("Btn_InviteFriends", "INVITE FRIENDS"),
            ("Btn_OpenLobby", "CHAMPION HALL"),
            ("Btn_Back", "BACK"),
        ]),
        ("WBP_GOTA_Lobby", "/Script/WarOfGods.GOTALobbyWidget", "CHAMPION HALL", [
            ("Btn_Ready", "READY"),
            ("Btn_StartMatch", "START MATCH"),
            ("Btn_Leave", "LEAVE LOBBY"),
            ("Btn_SelectArena", "SELECT ARENA"),
            ("Btn_Back", "BACK"),
        ]),
        ("WBP_GOTA_ArenaSelection", "/Script/WarOfGods.GOTAArenaSelectionWidget", "ARENAS", [
            ("Btn_Confirm", "ENTER ARENA"),
            ("Btn_Back", "BACK"),
        ]),
        ("WBP_GOTA_StoryMode", "/Script/WarOfGods.GOTAStoryMenuWidget", "STORY MODE", [
            ("Btn_NewGameSolo", "NEW GAME — SOLO"),
            ("Btn_NewGameCoop", "NEW GAME — CO-OP"),
            ("Btn_Continue", "CONTINUE"),
            ("Btn_Back", "BACK"),
        ]),
    ]

    for name, parent, title, buttons in screens:
        wbp = create_widget_bp(name, parent)
        build_screen_shell(wbp, title, buttons)
        unreal.EditorAssetLibrary.save_asset(f"{UI_PATH}/{name}")

    # 3) Root shell hosting all screens
    root = create_widget_bp("WBP_GOTA_FrontendRoot", "/Script/UMG.UserWidget")
    build_frontend_root(root, [
        ("MainMenu", f"{UI_PATH}/WBP_GOTA_MainMenu.WBP_GOTA_MainMenu_C"),
        ("StoryMode", f"{UI_PATH}/WBP_GOTA_StoryMode.WBP_GOTA_StoryMode_C"),
        ("Settings", f"{UI_PATH}/WBP_GOTA_Settings.WBP_GOTA_Settings_C"),
        ("Multiplayer", f"{UI_PATH}/WBP_GOTA_Multiplayer.WBP_GOTA_Multiplayer_C"),
        ("Lobby", f"{UI_PATH}/WBP_GOTA_Lobby.WBP_GOTA_Lobby_C"),
        ("ArenaSelection", f"{UI_PATH}/WBP_GOTA_ArenaSelection.WBP_GOTA_ArenaSelection_C"),
    ])
    unreal.EditorAssetLibrary.save_asset(f"{UI_PATH}/WBP_GOTA_FrontendRoot")

    # 4) Dress cinematic map
    dress_level()

    unreal.EditorAssetLibrary.save_directory("/Game/MainMenu")
    log("=== Frontend build complete ===")


if __name__ == "__main__":
    main()
