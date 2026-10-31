# Gods of the Arena — UPG frontend builder (Editor Python, UE 5.8).
#
# Authors REAL Unreal assets (idempotent — safe to re-run):
#   /Game/MainMenu/UI/Textures, /Fonts        imported art + OFL fonts
#   /Game/Audio/Mix, /Game/Audio/UI           sound classes, original UI cues + ambience
#   /Game/MainMenu/Data/DA_GOTA_MenuContent   copy, theme, lore, chapters, audio refs
#   /Game/MainMenu/UI/WBP_GOTA_*              components, screens, modal, root
#   /Game/UI/Duel/WBP_GOTA_DuelHUD|Results    Trial of Champions HUD + results
#   L_GOTA_MainMenu WorldSettings → GOTAFrontendGameMode
#   BP_WarOfGods_PlayerController → GOTAPlayerCameraManager (look/shake settings)
#
# Run headless:
#   UnrealEditor-Cmd.exe WarOfGods.uproject -run=pythonscript -script="<abs>/Tools/GOTA_BuildMenuUI.py"
# Prereqs: Tools/GOTA_BuildMenuArt.py and Tools/GOTA_BuildMenuAudio.py outputs exist.
#
# Widget trees are built through UGOTAEditorUIBuilderLibrary (C++, editor-only) because Python
# cannot reach WidgetBlueprint.WidgetTree. The result is ordinary UMG, editable in the Designer.

import os
import unreal

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ART_SRC = os.path.join(PROJECT, "Tools", "MenuArtSource")
AUDIO_SRC = os.path.join(PROJECT, "Tools", "MenuAudioSource")
FONT_SRC = os.path.join(PROJECT, "Tools", "MenuFonts")

UI = "/Game/MainMenu/UI"
TEX = UI + "/Textures"
FONTS = UI + "/Fonts"
DATA = "/Game/MainMenu/Data"
MIX = "/Game/Audio/Mix"
AUDIO_UI = "/Game/Audio/UI"
DUEL_UI = "/Game/UI/Duel"
MENU_MAP = "/Game/MainMenu/L_GOTA_MainMenu"

B = unreal.GOTAEditorUIBuilderLibrary
EAL = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LOG_LINES = []


def log(msg):
    LOG_LINES.append(msg)
    unreal.log("[GOTA MenuUI] " + msg)


def ensure_dir(path):
    if not EAL.does_directory_exist(path):
        EAL.make_directory(path)


# ============================================================ design tokens
def lc(r, g, b, a=1.0):
    return unreal.LinearColor(r, g, b, a)


NAVY = lc(0.006, 0.009, 0.022, 1)
SURFACE = lc(0.010, 0.014, 0.030, 0.78)
SURFACE_SOFT = lc(0.010, 0.014, 0.030, 0.55)
GOLD = lc(0.78, 0.55, 0.22, 1)
GOLD_BRIGHT = lc(1.0, 0.82, 0.48, 1)
TEXT = lc(0.93, 0.90, 0.84, 1)
MUTED = lc(0.56, 0.60, 0.68, 1)
BLUE = lc(0.25, 0.55, 1.0, 1)
CLEAR = lc(0, 0, 0, 0)

FONT_DISPLAY = None  # set after import
FONT_BODY = None


# ============================================================ import
def import_files(files, dest, replace=True):
    tasks = []
    for f in files:
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", f)
        t.set_editor_property("destination_path", dest)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", replace)
        t.set_editor_property("save", True)
        tasks.append(t)
    TOOLS.import_asset_tasks(tasks)
    out = []
    for t in tasks:
        paths = t.get_editor_property("imported_object_paths")
        out.extend(paths)
    return out


def import_textures():
    ensure_dir(TEX)
    files = [os.path.join(ART_SRC, f) for f in sorted(os.listdir(ART_SRC)) if f.endswith(".png") and not f.startswith("_")]
    import_files(files, TEX)
    for f in files:
        name = os.path.splitext(os.path.basename(f))[0]
        tex = EAL.load_asset(TEX + "/" + name)
        if not tex:
            log("WARN texture import failed: " + name)
            continue
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("never_stream", True)
        EAL.save_asset(TEX + "/" + name)
    log("Imported %d textures" % len(files))


def tex(name):
    asset = EAL.load_asset(TEX + "/" + name)
    if not asset:
        raise RuntimeError("Missing texture " + name)
    return asset


def import_fonts():
    global FONT_DISPLAY, FONT_BODY
    ensure_dir(FONTS)
    faces = {}
    for fname in ["Cinzel_400", "Cinzel_600", "Cinzel_700", "Montserrat_400", "Montserrat_500", "Montserrat_600"]:
        path = FONTS + "/" + fname
        if not EAL.does_asset_exist(path):
            import_files([os.path.join(FONT_SRC, fname + ".ttf")], FONTS)
        face = EAL.load_asset(path)
        if not face:
            raise RuntimeError("Font face missing: " + path)
        faces[fname] = face
    FONT_DISPLAY = B.create_composite_font(FONTS, "F_GOTA_Cinzel", {
        "Regular": faces["Cinzel_400"], "SemiBold": faces["Cinzel_600"], "Bold": faces["Cinzel_700"]})
    FONT_BODY = B.create_composite_font(FONTS, "F_GOTA_Montserrat", {
        "Regular": faces["Montserrat_400"], "Medium": faces["Montserrat_500"], "SemiBold": faces["Montserrat_600"]})
    EAL.save_asset(FONTS + "/F_GOTA_Cinzel")
    EAL.save_asset(FONTS + "/F_GOTA_Montserrat")
    log("Fonts ready: Cinzel (display) + Montserrat (body), SIL OFL 1.1")


def import_audio():
    ensure_dir(MIX)
    ensure_dir(AUDIO_UI)
    for name in ["SC_GOTA_Music", "SC_GOTA_UI"]:
        if not EAL.does_asset_exist(MIX + "/" + name):
            TOOLS.create_asset(name, MIX, unreal.SoundClass, unreal.SoundClassFactory())
            EAL.save_asset(MIX + "/" + name)
    files = [os.path.join(AUDIO_SRC, f) for f in sorted(os.listdir(AUDIO_SRC)) if f.endswith(".wav")]
    import_files(files, AUDIO_UI)
    amb = EAL.load_asset(AUDIO_UI + "/S_GOTA_Menu_Ambience")
    if amb:
        amb.set_editor_property("looping", True)
        EAL.save_asset(AUDIO_UI + "/S_GOTA_Menu_Ambience")
    log("Audio: %d original cues imported; sound classes SC_GOTA_Music / SC_GOTA_UI" % len(files))


# ============================================================ widget helpers
def font(face, size, typeface="Regular", spacing=0):
    f = unreal.SlateFontInfo()
    f.set_editor_property("font_object", face)
    f.set_editor_property("typeface_font_name", typeface)
    f.set_editor_property("size", size)
    f.set_editor_property("letter_spacing", spacing)
    return f


def slate_vec(x, y):
    vec_type = getattr(unreal, "DeprecateSlateVector2D", None)
    if vec_type is None:
        return unreal.Vector2D(x, y)
    v = vec_type()
    v.set_editor_property("x", float(x))
    v.set_editor_property("y", float(y))
    return v


def brush_tex(texture, w=None, h=None, draw=unreal.SlateBrushDrawType.IMAGE, tint=None, margin=None):
    b = unreal.SlateBrush()
    b.set_editor_property("resource_object", texture)
    if w and h:
        b.set_editor_property("image_size", slate_vec(w, h))
    b.set_editor_property("draw_as", draw)
    if margin is not None:
        b.set_editor_property("margin", unreal.Margin(margin, margin, margin, margin))
    b.set_editor_property("tint_color", unreal.SlateColor(tint or lc(1, 1, 1, 1)))
    return b


def brush_color(color, draw=unreal.SlateBrushDrawType.BOX):
    b = unreal.SlateBrush()
    b.set_editor_property("draw_as", draw)
    b.set_editor_property("tint_color", unreal.SlateColor(color))
    return b


def brush_none():
    b = unreal.SlateBrush()
    b.set_editor_property("draw_as", unreal.SlateBrushDrawType.NO_DRAW_TYPE)
    return b


class Tree:
    """Thin wrapper over UGOTAEditorUIBuilderLibrary for one widget blueprint."""

    def __init__(self, wbp):
        self.wbp = wbp
        B.reset_widget_tree(wbp)

    def add(self, cls, name, parent=None):
        w = B.add_widget(self.wbp, cls, name, parent if parent else "None")
        if not w:
            raise RuntimeError("add_widget failed: %s (%s) under %s in %s" % (name, cls, parent, self.wbp.get_name()))
        return w

    # canvas child helper: anchors (min x,y,max x,y), offsets (l,t,r,b), alignment
    def canvas(self, w, anchors, offsets, align=(0, 0), z=0, autosize=False):
        slot = w.get_editor_property("slot")
        a = unreal.Anchors()
        a.set_editor_property("minimum", unreal.Vector2D(anchors[0], anchors[1]))
        a.set_editor_property("maximum", unreal.Vector2D(anchors[2], anchors[3]))
        slot.set_anchors(a)
        slot.set_offsets(unreal.Margin(*offsets))
        slot.set_alignment(unreal.Vector2D(*align))
        slot.set_z_order(z)
        slot.set_auto_size(autosize)
        return w

    def text(self, name, parent, value, face, size, color, typeface="Regular", spacing=0, justify=None, wrap=False):
        t = self.add(unreal.TextBlock, name, parent)
        t.set_text(unreal.Text(value))
        t.set_editor_property("font", font(face, size, typeface, spacing))
        t.set_color_and_opacity(unreal.SlateColor(color))
        if justify is not None:
            t.set_editor_property("justification", justify)
        if wrap:
            if hasattr(t, "set_auto_wrap_text"):
                t.set_auto_wrap_text(True)
            else:
                t.set_editor_property("auto_wrap_text", True)
        t.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        return t

    def image(self, name, parent, brush, color=None, hit=False):
        i = self.add(unreal.Image, name, parent)
        i.set_brush(brush)
        if color is not None:
            i.set_color_and_opacity(color)
        if not hit:
            i.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        return i

    def sizebox(self, name, parent, w=None, h=None, min_w=None):
        s = self.add(unreal.SizeBox, name, parent)
        if w:
            s.set_width_override(w)
        if h:
            s.set_height_override(h)
        if min_w:
            s.set_min_desired_width(min_w)
        return s


def hslot(w, padding=(0, 0, 0, 0), fill=False, h=unreal.HorizontalAlignment.H_ALIGN_FILL, v=unreal.VerticalAlignment.V_ALIGN_CENTER, value=1.0):
    slot = w.get_editor_property("slot")
    slot.set_padding(unreal.Margin(*padding))
    size = unreal.SlateChildSize()
    size.set_editor_property("size_rule", unreal.SlateSizeRule.FILL if fill else unreal.SlateSizeRule.AUTOMATIC)
    size.set_editor_property("value", value)
    slot.set_size(size)
    slot.set_horizontal_alignment(h)
    slot.set_vertical_alignment(v)
    return w


def vslot(w, padding=(0, 0, 0, 0), fill=False, h=unreal.HorizontalAlignment.H_ALIGN_FILL, v=unreal.VerticalAlignment.V_ALIGN_FILL):
    return hslot(w, padding, fill, h, v)


def oslot(w, h=unreal.HorizontalAlignment.H_ALIGN_FILL, v=unreal.VerticalAlignment.V_ALIGN_FILL, padding=(0, 0, 0, 0)):
    slot = w.get_editor_property("slot")
    slot.set_horizontal_alignment(h)
    slot.set_vertical_alignment(v)
    slot.set_padding(unreal.Margin(*padding))
    return w


def bslot(w, h=unreal.HorizontalAlignment.H_ALIGN_FILL, v=unreal.VerticalAlignment.V_ALIGN_FILL, padding=(0, 0, 0, 0)):
    # content slots of Border / Button
    slot = w.get_editor_property("slot")
    if hasattr(slot, "set_horizontal_alignment"):
        slot.set_horizontal_alignment(h)
        slot.set_vertical_alignment(v)
    if hasattr(slot, "set_padding"):
        slot.set_padding(unreal.Margin(*padding))
    return w


def invisible_button_style():
    style = unreal.ButtonStyle()
    for state in ["normal", "hovered", "pressed", "disabled"]:
        style.set_editor_property(state, brush_none())
    style.set_editor_property("normal_padding", unreal.Margin(0, 0, 0, 0))
    style.set_editor_property("pressed_padding", unreal.Margin(0, 0, 0, 0))
    return style


def make_wbp(path, name, parent_class):
    ensure_dir(path)
    full = path + "/" + name
    if EAL.does_asset_exist(full):
        return EAL.load_asset(full)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = TOOLS.create_asset(name, path, unreal.WidgetBlueprint, factory)
    if not asset:
        raise RuntimeError("Could not create " + full)
    return asset


def finish(wbp, path):
    ok = B.compile_widget_blueprint(wbp)
    EAL.save_asset(path)
    log(("OK   " if ok else "FAIL ") + path)
    if not ok:
        raise RuntimeError("Compile failed: " + path)
    return wbp


def wbp_class(path):
    name = path.split("/")[-1]
    cls = unreal.load_class(None, "%s.%s_C" % (path, name))
    if not cls:
        raise RuntimeError("Missing class for " + path)
    return cls


def frame_brush(alpha=1.0):
    return brush_tex(tex("T_GOTA_Frame"), 64, 64, unreal.SlateBrushDrawType.BORDER, lc(1, 1, 1, alpha), 0.125)


# ============================================================ components
def build_button_shell(t, root_size, label_font, sub_font, icon_size=None, art=False, bg=None, label_justify=None):
    """Shared anatomy: Overlay[bg, art, shade, selection, frame, Button_Root[content]]."""
    sb = t.sizebox("Root", None, *root_size)
    ov = t.add(unreal.Overlay, "Layers", "Root")
    if art:
        ov.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS)
    if bg is not None:
        bgi = t.image("Image_Background", "Layers", brush_color(bg))
        oslot(bgi)
    if art:
        scale = t.add(unreal.ScaleBox, "ArtScale", "Layers")
        scale.set_stretch(unreal.Stretch.SCALE_TO_FILL)
        scale.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
        oslot(scale)
        a = t.image("Image_Art", "ArtScale", brush_tex(tex("T_GOTA_Card_Story"), 590, 330))
        shade = t.image("Image_Shade", "Layers", brush_tex(tex("T_GOTA_Grad_V"), 4, 16), lc(0, 0, 0, 0.92))
        oslot(shade)
    # Stretch-only images get tiny desired sizes so they never widen their button.
    sel = t.image("Image_Selection", "Layers", brush_tex(tex("T_GOTA_Grad_Selection"), 16, 4), lc(0.78, 0.55, 0.22, 0))
    oslot(sel)
    fr = t.add(unreal.Border, "Border_Frame", "Layers")
    fr.set_brush(frame_brush())
    fr.set_brush_color(lc(0.78, 0.55, 0.22, 0))
    fr.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    oslot(fr)
    btn = t.add(unreal.Button, "Button_Root", "Layers")
    btn.set_style(invisible_button_style())
    oslot(btn)
    return btn


def build_nav_button():
    path = UI + "/WBP_GOTA_NavButton"
    wbp = make_wbp(UI, "WBP_GOTA_NavButton", unreal.GOTAMenuButtonWidget)
    t = Tree(wbp)
    btn = build_button_shell(t, (400, 74), None, None)
    row = t.add(unreal.HorizontalBox, "Row", "Button_Root")
    bslot(row, padding=(16, 6, 12, 6))
    icon_box = t.sizebox("IconBox", "Row", 40, 40)
    hslot(icon_box, padding=(0, 0, 18, 0))
    t.image("Image_Icon", "IconBox", brush_tex(tex("T_GOTA_Glyph_Diamond"), 40, 40))
    col = t.add(unreal.VerticalBox, "Texts", "Row")
    hslot(col, fill=True)
    l = t.text("Text_Label", "Texts", "LABEL", FONT_DISPLAY, 23, TEXT, "SemiBold", 90)
    vslot(l)
    s = t.text("Text_Subtitle", "Texts", "SUBTITLE", FONT_BODY, 10, MUTED, "Medium", 220)
    vslot(s, padding=(1, 1, 0, 0))
    gen_cdo_defaults(wbp, style=unreal.GOTAButtonStyle.NAV_ROW)
    return finish(wbp, path)


def build_feature_card():
    path = UI + "/WBP_GOTA_FeatureCard"
    wbp = make_wbp(UI, "WBP_GOTA_FeatureCard", unreal.GOTAMenuButtonWidget)
    t = Tree(wbp)
    build_button_shell(t, (None, 196), None, None, art=True)
    stack = t.add(unreal.VerticalBox, "Bottom", "Button_Root")
    bslot(stack, v=unreal.VerticalAlignment.V_ALIGN_BOTTOM, padding=(18, 0, 14, 14))
    row = t.add(unreal.HorizontalBox, "Row", "Bottom")
    icon_box = t.sizebox("IconBox", "Row", 38, 38)
    hslot(icon_box, padding=(0, 0, 14, 0))
    t.image("Image_Icon", "IconBox", brush_tex(tex("T_GOTA_Glyph_Diamond"), 38, 38))
    col = t.add(unreal.VerticalBox, "Texts", "Row")
    hslot(col, fill=True)
    vslot(t.text("Text_Label", "Texts", "CARD", FONT_DISPLAY, 22, TEXT, "SemiBold", 110))
    vslot(t.text("Text_Subtitle", "Texts", "SUBTITLE", FONT_BODY, 9, MUTED, "Medium", 200), padding=(1, 0, 0, 0))
    gen_cdo_defaults(wbp, style=unreal.GOTAButtonStyle.FEATURE_CARD, card=True)
    return finish(wbp, path)


def build_action_button():
    path = UI + "/WBP_GOTA_ActionButton"
    wbp = make_wbp(UI, "WBP_GOTA_ActionButton", unreal.GOTAMenuButtonWidget)
    t = Tree(wbp)
    build_button_shell(t, (None, 54), None, None, bg=SURFACE)
    root = wbp  # noqa
    sb = B.find_widget(wbp, "Root")
    sb.set_min_desired_width(200)
    box = t.add(unreal.VerticalBox, "Texts", "Button_Root")
    bslot(box, h=unreal.HorizontalAlignment.H_ALIGN_CENTER, v=unreal.VerticalAlignment.V_ALIGN_CENTER, padding=(26, 4, 26, 4))
    vslot(t.text("Text_Label", "Texts", "ACTION", FONT_DISPLAY, 17, TEXT, "SemiBold", 140, unreal.TextJustify.CENTER), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    sub = t.text("Text_Subtitle", "Texts", "", FONT_BODY, 9, MUTED, "Medium", 150, unreal.TextJustify.CENTER)
    vslot(sub, h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    gen_cdo_defaults(wbp, style=unreal.GOTAButtonStyle.ACTION)
    return finish(wbp, path)


def build_list_entry():
    path = UI + "/WBP_GOTA_ListEntry"
    wbp = make_wbp(UI, "WBP_GOTA_ListEntry", unreal.GOTAMenuButtonWidget)
    t = Tree(wbp)
    build_button_shell(t, (None, 62), None, None, bg=SURFACE_SOFT)
    box = t.add(unreal.VerticalBox, "Texts", "Button_Root")
    bslot(box, v=unreal.VerticalAlignment.V_ALIGN_CENTER, padding=(20, 4, 16, 4))
    vslot(t.text("Text_Label", "Texts", "ENTRY", FONT_DISPLAY, 17, TEXT, "SemiBold", 80))
    vslot(t.text("Text_Subtitle", "Texts", "DETAIL", FONT_BODY, 10, MUTED, "Medium", 120), padding=(1, 1, 0, 0))
    gen_cdo_defaults(wbp, style=unreal.GOTAButtonStyle.LIST_ENTRY)
    return finish(wbp, path)


def build_icon_button():
    path = UI + "/WBP_GOTA_IconButton"
    wbp = make_wbp(UI, "WBP_GOTA_IconButton", unreal.GOTAMenuButtonWidget)
    t = Tree(wbp)
    build_button_shell(t, (58, 58), None, None, bg=SURFACE)
    box = t.sizebox("IconBox", "Button_Root", 28, 28)
    bslot(box, h=unreal.HorizontalAlignment.H_ALIGN_CENTER, v=unreal.VerticalAlignment.V_ALIGN_CENTER)
    t.image("Image_Icon", "IconBox", brush_tex(tex("T_GOTA_Icon_Plus"), 28, 28))
    gen_cdo_defaults(wbp, style=unreal.GOTAButtonStyle.ACTION)
    return finish(wbp, path)


def gen_cdo_defaults(wbp, style, card=False):
    # Compile first so the generated class exists, then set class defaults.
    B.compile_widget_blueprint(wbp)
    cdo = unreal.get_default_object(wbp.generated_class())
    cdo.set_editor_property("style", style)
    cdo.set_editor_property("use_card_content", card)


def build_setting_row():
    path = UI + "/WBP_GOTA_SettingRow"
    wbp = make_wbp(UI, "WBP_GOTA_SettingRow", unreal.GOTASettingRowWidget)
    t = Tree(wbp)
    t.sizebox("Root", None, None, 54)
    ov = t.add(unreal.Overlay, "Layers", "Root")
    oslot(t.image("Image_Background", "Layers", brush_color(SURFACE_SOFT)))
    fr = t.add(unreal.Border, "Border_Frame", "Layers")
    fr.set_brush(frame_brush())
    fr.set_brush_color(lc(0.78, 0.55, 0.22, 0.12))
    fr.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    oslot(fr)
    btn = t.add(unreal.Button, "Button_Row", "Layers")
    btn.set_style(invisible_button_style())
    oslot(btn)
    row = t.add(unreal.HorizontalBox, "Row", "Button_Row")
    bslot(row, padding=(22, 0, 14, 0))
    hslot(t.text("Text_Label", "Row", "SETTING", FONT_DISPLAY, 16, TEXT, "SemiBold", 90), fill=True)
    prev = t.add(unreal.Button, "Btn_Prev", "Row")
    prev.set_style(invisible_button_style())
    prev.set_editor_property("is_focusable", False)
    hslot(prev, padding=(0, 0, 6, 0))
    oslot_text = t.text("Text_Prev", "Btn_Prev", "<", FONT_DISPLAY, 18, GOLD, "Bold")
    val_box = t.sizebox("ValueBox", "Row", 210, None)
    hslot(val_box)
    t.text("Text_Value", "ValueBox", "VALUE", FONT_BODY, 13, MUTED, "SemiBold", 120, unreal.TextJustify.CENTER)
    nxt = t.add(unreal.Button, "Btn_Next", "Row")
    nxt.set_style(invisible_button_style())
    nxt.set_editor_property("is_focusable", False)
    hslot(nxt, padding=(6, 0, 14, 0))
    t.text("Text_Next", "Btn_Next", ">", FONT_DISPLAY, 18, GOLD, "Bold")
    slider_box = t.sizebox("SliderBox", "Row", 220, None)
    hslot(slider_box)
    sl = t.add(unreal.Slider, "Slider_Value", "SliderBox")
    sl.set_editor_property("is_focusable", False)
    sl.set_slider_bar_color(lc(0.25, 0.27, 0.32, 1))
    sl.set_slider_handle_color(GOLD_BRIGHT)
    return finish(wbp, path)


# ============================================================ screens
def screen_header(t, parent, title, subtitle, name="Header"):
    box = t.add(unreal.VerticalBox, name, parent)
    t.canvas(box, (0, 0, 0, 0), (84, 64, 900, 120), autosize=True)
    vslot(t.text(name + "_Kicker", name, "UNITED PLANETS AUTHORITY", FONT_BODY, 11, GOLD, "SemiBold", 400))
    vslot(t.text(name + "_Title", name, title, FONT_DISPLAY, 46, TEXT, "Bold", 60), padding=(0, 2, 0, 0))
    line = t.sizebox(name + "_LineBox", name, 420, 2)
    vslot(line, padding=(0, 6, 0, 8), h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    t.image(name + "_Line", name + "_LineBox", brush_tex(tex("T_GOTA_Grad_Selection"), 512, 8), GOLD)
    vslot(t.text(name + "_Sub", name, subtitle, FONT_BODY, 14, MUTED, "Medium", 40))
    return box


def screen_base(name, parent_cls, title, subtitle):
    path = UI + "/" + name
    wbp = make_wbp(UI, name, parent_cls)
    t = Tree(wbp)
    t.add(unreal.CanvasPanel, "Canvas")
    shade = t.image("Image_Shade", "Canvas", brush_tex(tex("T_GOTA_Grad_H"), 256, 8), lc(0.004, 0.006, 0.016, 0.93))
    t.canvas(shade, (0, 0, 0, 1), (0, 0, 1250, 0))
    full = t.image("Image_Dim", "Canvas", brush_color(lc(0.004, 0.006, 0.016, 0.35)))
    t.canvas(full, (0, 0, 1, 1), (0, 0, 0, 0))
    screen_header(t, "Canvas", title, subtitle)
    return path, wbp, t


def panel(t, name, parent, padding=(28, 24, 28, 24), color=SURFACE):
    outer = t.add(unreal.Border, name, parent)
    outer.set_brush(brush_color(color))
    outer.set_padding(unreal.Margin(*padding))
    return outer


def back_button(t, parent="Canvas", name="Btn_Back"):
    b = t.add(wbp_class(UI + "/WBP_GOTA_ActionButton"), name, parent)
    b.set_editor_property("label", unreal.Text("BACK"))
    t.canvas(b, (0, 1, 0, 1), (84, -60, 220, 54), (0, 1), autosize=True)
    return b


def nav(t, name, parent, label, sub):
    b = t.add(wbp_class(UI + "/WBP_GOTA_NavButton"), name, parent)
    b.set_editor_property("label", unreal.Text(label))
    b.set_editor_property("subtitle", unreal.Text(sub))
    return b


def action(t, name, parent, label):
    b = t.add(wbp_class(UI + "/WBP_GOTA_ActionButton"), name, parent)
    b.set_editor_property("label", unreal.Text(label))
    return b


def build_main_menu():
    path = UI + "/WBP_GOTA_MainMenu"
    wbp = make_wbp(UI, "WBP_GOTA_MainMenu", unreal.GOTAMainMenuWidget)
    t = Tree(wbp)
    t.add(unreal.CanvasPanel, "Canvas")

    # readability shades (left nav column + bottom card band)
    shade = t.image("Image_LeftShade", "Canvas", brush_tex(tex("T_GOTA_Grad_H"), 256, 8), lc(0.004, 0.006, 0.016, 0.72))
    t.canvas(shade, (0, 0, 0, 1), (0, 0, 640, 0))
    bshade = t.image("Image_BottomShade", "Canvas", brush_tex(tex("T_GOTA_Grad_V"), 8, 256), lc(0.004, 0.006, 0.016, 0.85))
    t.canvas(bshade, (0, 1, 1, 1), (0, -330, 0, 330), (0, 0))

    # Region A — UPG identity
    ident = t.add(unreal.HorizontalBox, "Identity", "Canvas")
    t.canvas(ident, (0, 0, 0, 0), (52, 14, 420, 100), autosize=True)
    ib = t.sizebox("InsigniaBox", "Identity", 96, 96)
    hslot(ib, padding=(0, 0, 16, 0))
    t.image("Image_Insignia", "InsigniaBox", brush_tex(tex("T_GOTA_UPG_Insignia"), 96, 96))
    it = t.add(unreal.VerticalBox, "IdentityText", "Identity")
    hslot(it, v=unreal.VerticalAlignment.V_ALIGN_CENTER)
    vslot(t.text("Text_FactionName", "IdentityText", "UNITED PLANETS\nOF GALAXIES", FONT_DISPLAY, 19, TEXT, "Regular", 380))
    vslot(t.text("Text_FactionTagline", "IdentityText", "EARTH  •  CAPITAL  •  UPG", FONT_BODY, 10, GOLD, "SemiBold", 420), padding=(2, 6, 0, 0))

    # Region C — navigation
    navbox = t.add(unreal.VerticalBox, "Nav", "Canvas")
    t.canvas(navbox, (0, 0, 0, 0), (50, 166, 410, 600), autosize=True)
    for name, label, sub, icon in [
        ("Btn_Continue", "CONTINUE", "RETURN TO YOUR MISSION", "T_GOTA_Icon_Continue"),
        ("Btn_NewStory", "STORY MODE", "A GALAXY OF CHOICES", "T_GOTA_Icon_Story"),
        ("Btn_PvEArena", "ARENA MODE", "FIGHT. SURVIVE. ASCEND.", "T_GOTA_Icon_Arena"),
        ("Btn_Multiplayer", "MULTIPLAYER", "PLAY WITH ALLIES", "T_GOTA_Icon_Multiplayer"),
        ("Btn_Settings", "SETTINGS", "CUSTOMIZE YOUR EXPERIENCE", "T_GOTA_Icon_Settings"),
        ("Btn_Extras", "EXTRAS", "LORE, CREDITS AND MORE", "T_GOTA_Icon_Extras"),
        ("Btn_Quit", "QUIT", "EXIT TO DESKTOP", "T_GOTA_Icon_Quit"),
    ]:
        b = nav(t, name, "Nav", label, sub)
        vslot(b, padding=(0, 0, 0, 12), h=unreal.HorizontalAlignment.H_ALIGN_LEFT)

    # Region B — title logotype + editable subtitle
    title = t.add(unreal.VerticalBox, "Title", "Canvas")
    t.canvas(title, (0.5, 0, 0.5, 0), (10, 34, 620, 470), (0.5, 0), autosize=True)
    lb = t.sizebox("LogoBox", "Title", 452, 370)
    vslot(lb, h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    t.image("Image_Logo", "LogoBox", brush_tex(tex("T_GOTA_Logo_GodsOfTheArena"), 452, 370))
    vslot(t.text("Text_GameSubtitle", "Title", "UNITED PLANETS OF GALAXIES", FONT_DISPLAY, 18, TEXT, "Regular", 520, unreal.TextJustify.CENTER),
          padding=(0, 6, 0, 0), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    orn = t.add(unreal.HorizontalBox, "Ornament", "Title")
    vslot(orn, padding=(0, 10, 0, 0), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    for i, (w, h) in enumerate([(120, 1), (10, 10), (18, 18), (10, 10), (120, 1)]):
        bx = t.sizebox("Orn%d" % i, "Ornament", w, h)
        hslot(bx, padding=(6, 0, 6, 0))
        if h == 1:
            t.image("OrnImg%d" % i, "Orn%d" % i, brush_color(GOLD))
        else:
            t.image("OrnImg%d" % i, "Orn%d" % i, brush_tex(tex("T_GOTA_Glyph_Diamond"), w, h), GOLD)

    # Region D — profile
    prof = panel(t, "ProfilePanel", "Canvas", (8, 8, 18, 8), lc(0.010, 0.014, 0.030, 0.82))
    t.canvas(prof, (1, 0, 1, 0), (-100, 22, 362, 74), (1, 0))
    prow = t.add(unreal.HorizontalBox, "ProfileRow", "ProfilePanel")
    pb = t.sizebox("PortraitBox", "ProfileRow", 58, 58)
    hslot(pb, padding=(0, 0, 14, 0))
    pf = t.add(unreal.Border, "PortraitFrame", "PortraitBox")
    pf.set_brush(frame_brush())
    pf.set_brush_color(GOLD)
    pf.set_padding(unreal.Margin(2, 2, 2, 2))
    t.image("Image_Portrait", "PortraitFrame", brush_tex(tex("T_GOTA_Portrait_Warden"), 54, 54))
    pt = t.add(unreal.VerticalBox, "ProfileText", "ProfileRow")
    hslot(pt, fill=True)
    nb = t.sizebox("ProfileNameBox", "ProfileText", 200, None)
    vslot(nb, h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    pn = t.text("Text_ProfileName", "ProfileNameBox", "WARDEN", FONT_DISPLAY, 17, TEXT, "SemiBold", 80)
    pn.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS)
    try:
        pn.set_editor_property("text_overflow_policy", unreal.TextOverflowPolicy.ELLIPSIS)
    except Exception:
        pass
    vslot(t.text("Text_ProfileRank", "ProfileText", "AUTHORITY WARDEN", FONT_BODY, 10, MUTED, "SemiBold", 200), padding=(0, 2, 0, 4))
    bar = t.add(unreal.ProgressBar, "Bar_ProfileProgress", "ProfileText")
    bar.set_fill_color_and_opacity(BLUE)
    bar.set_percent(0.0)
    fb = t.add(wbp_class(UI + "/WBP_GOTA_IconButton"), "Btn_Friends", "ProfileRow")
    fb.set_editor_property("icon_override", tex("T_GOTA_Icon_Friends"))
    hslot(fb, padding=(12, 0, 0, 0))
    plus = t.add(wbp_class(UI + "/WBP_GOTA_IconButton"), "Btn_Plus", "Canvas")
    t.canvas(plus, (1, 0, 1, 0), (-28, 22, 66, 74), (1, 0))

    # Region E — UPG / Earth panel + rotating institutional messages
    upg = panel(t, "UPGPanel", "Canvas", (16, 8, 8, 8), lc(0.010, 0.014, 0.030, 0.78))
    t.canvas(upg, (1, 0, 1, 0), (-30, 140, 280, 88), (1, 0))
    urow = t.add(unreal.HorizontalBox, "UPGRow", "UPGPanel")
    ut = t.add(unreal.VerticalBox, "UPGText", "UPGRow")
    hslot(ut, fill=True, v=unreal.VerticalAlignment.V_ALIGN_CENTER)
    head = t.add(unreal.HorizontalBox, "UPGHead", "UPGText")
    hb = t.sizebox("UPGMarkBox", "UPGHead", 30, 30)
    hslot(hb, padding=(0, 0, 10, 0))
    t.image("Image_UPGMark", "UPGMarkBox", brush_tex(tex("T_GOTA_UPG_Insignia"), 30, 30))
    hu = t.add(unreal.VerticalBox, "UPGHeadText", "UPGHead")
    vslot(t.text("Text_UPG", "UPGHeadText", "UPG", FONT_DISPLAY, 16, TEXT, "SemiBold", 200))
    vslot(t.text("Text_Earth", "UPGHeadText", "EARTH", FONT_DISPLAY, 11, MUTED, "Regular", 300))
    lineb = t.sizebox("UPGLine", "UPGText", 150, 1)
    vslot(lineb, padding=(0, 6, 0, 4), h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    t.image("UPGLineImg", "UPGLine", brush_color(lc(0.56, 0.6, 0.68, 0.6)))
    vslot(t.text("Text_Capital", "UPGText", "CAPITAL OF HUMANITY", FONT_BODY, 9, MUTED, "SemiBold", 200))
    eb = t.sizebox("EarthBox", "UPGRow", 96, 72)
    hslot(eb, padding=(8, 0, 0, 0))
    t.image("Image_Earth", "EarthBox", brush_tex(tex("T_GOTA_Earth_Capital"), 96, 72))

    slogan = t.text("Text_Slogan", "Canvas", "MANY WORLDS\nONE PEOPLE\nA STRONGER TOMORROW", FONT_DISPLAY, 14, TEXT, "Regular", 300, unreal.TextJustify.RIGHT)
    t.canvas(slogan, (1, 1, 1, 1), (-52, -258, 420, 80), (1, 1), autosize=True)

    # Region F — five illustrated cards
    cards = t.add(unreal.HorizontalBox, "Cards", "Canvas")
    t.canvas(cards, (0, 1, 1, 1), (58, -42, 58, 200), (0, 1))
    for i, (name, label, sub) in enumerate([
        ("Card_Story", "STORY", "EXPLORE THE UPG UNIVERSE"),
        ("Card_Arena", "ARENA", "FIGHT. SURVIVE. ASCEND."),
        ("Card_Multiplayer", "MULTIPLAYER", "PLAY WITH ALLIES"),
        ("Card_Settings", "SETTINGS", "CUSTOMIZE YOUR EXPERIENCE"),
        ("Card_Extras", "EXTRAS", "LORE, CREDITS AND MORE"),
    ]):
        c = t.add(wbp_class(UI + "/WBP_GOTA_FeatureCard"), name, "Cards")
        c.set_editor_property("label", unreal.Text(label))
        c.set_editor_property("subtitle", unreal.Text(sub))
        hslot(c, padding=(0 if i == 0 else 9, 0, 0 if i == 4 else 9, 0), fill=True, v=unreal.VerticalAlignment.V_ALIGN_FILL)

    hint = t.text("Text_InputHint", "Canvas", "ENTER  SELECT     ESC  BACK", FONT_BODY, 10, MUTED, "SemiBold", 300)
    t.canvas(hint, (0, 1, 0, 1), (60, -12, 600, 20), (0, 1), autosize=True)
    return finish(wbp, path)


def build_story():
    path, wbp, t = screen_base("WBP_GOTA_StoryMode", unreal.GOTAStoryMenuWidget, "STORY MODE", "The Sovereign Protocol")
    left = t.add(unreal.VerticalBox, "Actions", "Canvas")
    t.canvas(left, (0, 0, 0, 0), (80, 250, 420, 600), autosize=True)
    for name, label in [("Btn_Continue", "CONTINUE"), ("Btn_NewGameSolo", "NEW GAME"), ("Btn_NewGameCoop", "NEW CO-OP CAMPAIGN"), ("Btn_JoinCoop", "JOIN A CAMPAIGN")]:
        vslot(nav(t, name, "Actions", label, ""), padding=(0, 0, 0, 12), h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    info = panel(t, "SynopsisPanel", "Canvas", (34, 28, 34, 28))
    t.canvas(info, (0, 0, 0, 0), (560, 250, 820, 600))
    col = t.add(unreal.VerticalBox, "SynopsisCol", "SynopsisPanel")
    vslot(t.text("Text_CampaignTitle", "SynopsisCol", "THE SOVEREIGN PROTOCOL", FONT_DISPLAY, 28, GOLD_BRIGHT, "Bold", 60))
    vslot(t.text("Text_Synopsis", "SynopsisCol", "Synopsis", FONT_BODY, 14, TEXT, "Regular", 0, wrap=True), padding=(0, 12, 0, 18))
    vslot(t.text("ChaptersTitle", "SynopsisCol", "CHAPTERS", FONT_BODY, 11, GOLD, "SemiBold", 400), padding=(0, 0, 0, 8))
    sc = t.add(unreal.ScrollBox, "ChapterScroll", "SynopsisCol")
    vslot(sc, fill=True)
    t.add(unreal.VerticalBox, "Box_Chapters", "ChapterScroll")
    back_button(t)
    return finish(wbp, path)


def build_arena():
    path, wbp, t = screen_base("WBP_GOTA_ArenaSelection", unreal.GOTAArenaSelectionWidget, "ARENA MODE", "Fight. Survive. Ascend.")
    tabs = t.add(unreal.HorizontalBox, "Modes", "Canvas")
    t.canvas(tabs, (0, 0, 0, 0), (84, 238, 900, 60), autosize=True)
    for name, label in [("Btn_ModeFightAI", "TRIAL OF CHAMPIONS  •  FIGHT AI"), ("Btn_ModeHorde", "WRATH OF THE GODS  •  HORDE")]:
        hslot(action(t, name, "Modes", label), padding=(0, 0, 14, 0))
    lst = t.add(unreal.ScrollBox, "ArenaScroll", "Canvas")
    t.canvas(lst, (0, 0, 0, 1), (84, 320, 440, 150))
    t.add(unreal.VerticalBox, "Box_Arenas", "ArenaScroll")
    det = panel(t, "DetailPanel", "Canvas", (30, 26, 30, 26))
    t.canvas(det, (0, 0, 0, 1), (560, 320, 820, 150))
    dc = t.add(unreal.VerticalBox, "DetailCol", "DetailPanel")
    pv = t.sizebox("PreviewBox", "DetailCol", 760, 300)
    vslot(pv, padding=(0, 0, 0, 16), h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    pov = t.add(unreal.Overlay, "PreviewLayers", "PreviewBox")
    pov.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS)
    oslot(t.image("PreviewBg", "PreviewLayers", brush_color(lc(0.02, 0.025, 0.05, 1))))
    psc = t.add(unreal.ScaleBox, "PreviewScale", "PreviewLayers")
    psc.set_stretch(unreal.Stretch.SCALE_TO_FILL)
    oslot(psc)
    t.image("Image_ArenaPreview", "PreviewScale", brush_tex(tex("T_GOTA_Arena_TitansQuarry"), 760, 300))
    pfr = t.add(unreal.Border, "PreviewFrame", "PreviewLayers")
    pfr.set_brush(frame_brush())
    pfr.set_brush_color(lc(0.78, 0.55, 0.22, 0.6))
    oslot(pfr)
    vslot(t.text("Text_ArenaName", "DetailCol", "TITAN'S QUARRY", FONT_DISPLAY, 30, TEXT, "Bold", 60))
    vslot(t.text("Text_ArenaDescription", "DetailCol", "", FONT_BODY, 14, MUTED, "Regular", 0, wrap=True), padding=(0, 6, 0, 16))
    vslot(t.text("Text_ModeTitle", "DetailCol", "", FONT_BODY, 12, GOLD, "SemiBold", 300))
    vslot(t.text("Text_ModeDescription", "DetailCol", "", FONT_BODY, 14, TEXT, "Regular", 0, wrap=True), padding=(0, 6, 0, 18))
    foot = t.add(unreal.HorizontalBox, "DetailFoot", "DetailCol")
    vslot(foot)
    hslot(t.text("Text_Availability", "DetailFoot", "", FONT_BODY, 12, MUTED, "SemiBold", 150, wrap=True), fill=True)
    hslot(action(t, "Btn_Confirm", "DetailFoot", "ENTER THE ARENA"), padding=(20, 0, 0, 0))
    back_button(t)
    return finish(wbp, path)


def build_multiplayer():
    path, wbp, t = screen_base("WBP_GOTA_Multiplayer", unreal.GOTAMultiplayerWidget, "MULTIPLAYER", "Local network play  •  up to four Wardens")
    left = t.add(unreal.VerticalBox, "Actions", "Canvas")
    t.canvas(left, (0, 0, 0, 0), (80, 250, 420, 500), autosize=True)
    for name, label in [("Btn_HostGame", "HOST GAME"), ("Btn_FindSessions", "FIND GAMES"), ("Btn_JoinGame", "JOIN GAME"), ("Btn_OpenLobby", "LOBBY")]:
        vslot(nav(t, name, "Actions", label, ""), padding=(0, 0, 0, 12), h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    p = panel(t, "BrowserPanel", "Canvas", (30, 24, 30, 24))
    t.canvas(p, (0, 0, 0, 1), (560, 250, 820, 150))
    col = t.add(unreal.VerticalBox, "BrowserCol", "BrowserPanel")
    vslot(t.text("BrowserTitle", "BrowserCol", "SESSIONS ON YOUR NETWORK", FONT_BODY, 11, GOLD, "SemiBold", 400), padding=(0, 0, 0, 10))
    sc = t.add(unreal.ScrollBox, "SessionScroll", "BrowserCol")
    vslot(sc, fill=True)
    t.add(unreal.VerticalBox, "Box_Sessions", "SessionScroll")
    vslot(t.text("Text_Status", "BrowserCol", "", FONT_BODY, 13, MUTED, "Medium", 40, wrap=True), padding=(0, 14, 0, 0))
    back_button(t)
    return finish(wbp, path)


def build_lobby():
    path, wbp, t = screen_base("WBP_GOTA_Lobby", unreal.GOTALobbyWidget, "CHAMPION HALL", "Trial of Champions  •  Titan's Quarry")
    left = t.add(unreal.VerticalBox, "Actions", "Canvas")
    t.canvas(left, (0, 0, 0, 0), (80, 250, 420, 500), autosize=True)
    for name, label in [("Btn_StartMatch", "START DUEL"), ("Btn_Ready", "READY"), ("Btn_Leave", "LEAVE LOBBY")]:
        vslot(nav(t, name, "Actions", label, ""), padding=(0, 0, 0, 12), h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    p = panel(t, "RosterPanel", "Canvas", (34, 28, 34, 28))
    t.canvas(p, (0, 0, 0, 0), (560, 250, 700, 420))
    col = t.add(unreal.VerticalBox, "RosterCol", "RosterPanel")
    vslot(t.text("RosterTitle", "RosterCol", "WARDENS  (4 MAX)", FONT_BODY, 11, GOLD, "SemiBold", 400), padding=(0, 0, 0, 14))
    vslot(t.text("Text_Roster", "RosterCol", "", FONT_DISPLAY, 20, TEXT, "SemiBold", 80))
    vslot(t.text("Text_Status", "RosterCol", "", FONT_BODY, 13, MUTED, "Medium", 40, wrap=True), padding=(0, 18, 0, 0))
    back_button(t)
    return finish(wbp, path)


def build_settings():
    path, wbp, t = screen_base("WBP_GOTA_Settings", unreal.GOTASettingsWidget, "SETTINGS", "Customize your experience")
    tabs = t.add(unreal.HorizontalBox, "Tabs", "Canvas")
    t.canvas(tabs, (0, 0, 0, 0), (84, 238, 1000, 60), autosize=True)
    for name, label in [("Tab_Video", "VIDEO"), ("Tab_Audio", "AUDIO"), ("Tab_Controls", "CONTROLS"), ("Tab_Accessibility", "ACCESSIBILITY")]:
        hslot(action(t, name, "Tabs", label), padding=(0, 0, 12, 0))
    hint = t.text("TabHint", "Canvas", "LB / RB  (Q / E)  SWITCH TAB", FONT_BODY, 10, MUTED, "SemiBold", 300)
    t.canvas(hint, (0, 0, 0, 0), (88, 304, 600, 20), autosize=True)
    sc = t.add(unreal.ScrollBox, "RowScroll", "Canvas")
    t.canvas(sc, (0, 0, 0, 1), (84, 336, 980, 210))
    t.add(unreal.VerticalBox, "Box_Rows", "RowScroll")
    note = t.text("Text_TabNote", "Canvas", "", FONT_BODY, 12, MUTED, "Medium", 30, wrap=True)
    t.canvas(note, (0, 1, 0, 1), (88, -170, 960, 40), (0, 1))
    foot = t.add(unreal.HorizontalBox, "Footer", "Canvas")
    t.canvas(foot, (0, 1, 0, 1), (330, -60, 900, 54), (0, 1), autosize=True)
    for name, label in [("Btn_Apply", "APPLY"), ("Btn_Cancel", "REVERT"), ("Btn_Defaults", "DEFAULTS")]:
        hslot(action(t, name, "Footer", label), padding=(0, 0, 12, 0))
    hslot(t.text("Text_Pending", "Footer", "", FONT_BODY, 11, GOLD, "SemiBold", 300), padding=(12, 0, 0, 0))
    back_button(t)
    return finish(wbp, path)


def build_extras():
    path, wbp, t = screen_base("WBP_GOTA_Extras", unreal.GOTAExtrasWidget, "EXTRAS", "Lore, credits and more")
    lst = t.add(unreal.ScrollBox, "EntryScroll", "Canvas")
    t.canvas(lst, (0, 0, 0, 1), (84, 250, 440, 150))
    t.add(unreal.VerticalBox, "Box_Entries", "EntryScroll")
    p = panel(t, "ReaderPanel", "Canvas", (40, 32, 40, 32))
    t.canvas(p, (0, 0, 0, 1), (560, 250, 860, 150))
    col = t.add(unreal.VerticalBox, "ReaderCol", "ReaderPanel")
    vslot(t.text("Text_EntryCategory", "ReaderCol", "", FONT_BODY, 11, GOLD, "SemiBold", 400))
    vslot(t.text("Text_EntryTitle", "ReaderCol", "", FONT_DISPLAY, 30, TEXT, "Bold", 60), padding=(0, 4, 0, 14))
    sc = t.add(unreal.ScrollBox, "Scroll_Body", "ReaderCol")
    vslot(sc, fill=True)
    t.text("Text_EntryBody", "Scroll_Body", "", FONT_BODY, 15, TEXT, "Regular", 0, wrap=True)
    back_button(t)
    return finish(wbp, path)


def build_modal():
    path = UI + "/WBP_GOTA_Modal"
    wbp = make_wbp(UI, "WBP_GOTA_Modal", unreal.GOTAMenuModalWidget)
    t = Tree(wbp)
    t.add(unreal.Overlay, "Layers")
    dim = t.image("Image_Dim", "Layers", brush_color(lc(0.002, 0.003, 0.01, 0.78)), hit=True)
    oslot(dim)
    box = t.sizebox("DialogBox", "Layers", 780, None)
    oslot(box, unreal.HorizontalAlignment.H_ALIGN_CENTER, unreal.VerticalAlignment.V_ALIGN_CENTER)
    d = t.add(unreal.Overlay, "DialogLayers", "DialogBox")
    oslot(t.image("DialogBg", "DialogLayers", brush_color(lc(0.01, 0.014, 0.03, 0.97))))
    fr = t.add(unreal.Border, "DialogFrame", "DialogLayers")
    fr.set_brush(frame_brush())
    fr.set_brush_color(GOLD)
    oslot(fr)
    col = t.add(unreal.VerticalBox, "DialogCol", "DialogLayers")
    oslot(col, padding=(54, 40, 54, 40))
    ib = t.sizebox("ModalMark", "DialogCol", 56, 56)
    vslot(ib, padding=(0, 0, 0, 14), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    t.image("Image_Spinner", "ModalMark", brush_tex(tex("T_GOTA_UPG_Insignia"), 56, 56))
    vslot(t.text("Text_Title", "DialogCol", "TITLE", FONT_DISPLAY, 28, GOLD_BRIGHT, "Bold", 80, unreal.TextJustify.CENTER), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    vslot(t.text("Text_Body", "DialogCol", "Body", FONT_BODY, 15, TEXT, "Regular", 0, unreal.TextJustify.CENTER, wrap=True), padding=(0, 14, 0, 26))
    btns = t.add(unreal.HorizontalBox, "Panel_Buttons", "DialogCol")
    vslot(btns, h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    hslot(action(t, "Btn_Cancel", "Panel_Buttons", "CANCEL"), padding=(0, 0, 18, 0))
    hslot(action(t, "Btn_Confirm", "Panel_Buttons", "CONFIRM"))
    return finish(wbp, path)


def build_root():
    path = UI + "/WBP_GOTA_FrontendRoot"
    wbp = make_wbp(UI, "WBP_GOTA_FrontendRoot", unreal.GOTAFrontendRootWidget)
    t = Tree(wbp)
    t.add(unreal.Overlay, "Layers")
    sc = t.add(unreal.ScaleBox, "BackdropScale", "Layers")
    sc.set_stretch(unreal.Stretch.SCALE_TO_FILL)
    sc.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    oslot(sc)
    t.image("Image_Backdrop", "BackdropScale", brush_tex(tex("T_GOTA_Menu_Backdrop_CleanPlate"), 1920, 1081))
    entry = wbp_class(UI + "/WBP_GOTA_ListEntry")
    for name, cls_name, extra in [
        ("MainMenu", "WBP_GOTA_MainMenu", None),
        ("StoryMode", "WBP_GOTA_StoryMode", "entry_class"),
        ("ArenaSelection", "WBP_GOTA_ArenaSelection", "entry_class"),
        ("Multiplayer", "WBP_GOTA_Multiplayer", "entry_class"),
        ("Lobby", "WBP_GOTA_Lobby", None),
        ("Settings", "WBP_GOTA_Settings", "row_class"),
        ("Extras", "WBP_GOTA_Extras", "entry_class"),
        ("HeroSelect", "WBP_GOTA_HeroSelect", "tile_class"),
    ]:
        w = t.add(wbp_class(UI + "/" + cls_name), name, "Layers")
        oslot(w)
        if extra == "entry_class":
            w.set_editor_property("entry_class", entry)
        elif extra == "tile_class":
            w.set_editor_property("tile_class", wbp_class(UI + "/WBP_GOTA_HeroTile"))
        elif extra == "row_class":
            w.set_editor_property("row_class", wbp_class(UI + "/WBP_GOTA_SettingRow"))
    modal = t.add(wbp_class(UI + "/WBP_GOTA_Modal"), "Modal", "Layers")
    oslot(modal)
    return finish(wbp, path)


# ============================================================ duel HUD / results
def build_duel_hud():
    path = DUEL_UI + "/WBP_GOTA_DuelHUD"
    wbp = make_wbp(DUEL_UI, "WBP_GOTA_DuelHUD", unreal.GOTADuelHUDWidget)
    t = Tree(wbp)
    t.add(unreal.CanvasPanel, "Canvas")
    top = t.add(unreal.VerticalBox, "Top", "Canvas")
    t.canvas(top, (0.5, 0, 0.5, 0), (0, 22, 900, 140), (0.5, 0), autosize=True)
    vslot(t.text("Text_Round", "Top", "ROUND 1", FONT_BODY, 12, GOLD, "SemiBold", 400, unreal.TextJustify.CENTER), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    bars = t.add(unreal.HorizontalBox, "Bars", "Top")
    vslot(bars, padding=(0, 6, 0, 0), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)

    def fighter(prefix, name_widget, bar_widget, align_right):
        col = t.add(unreal.VerticalBox, prefix + "Col", "Bars")
        hslot(col)
        just = unreal.TextJustify.RIGHT if align_right else unreal.TextJustify.LEFT
        vslot(t.text(name_widget, prefix + "Col", "WARDEN", FONT_DISPLAY, 16, TEXT, "SemiBold", 100, just))
        bb = t.sizebox(prefix + "BarBox", prefix + "Col", 330, 12)
        vslot(bb, padding=(0, 4, 0, 0))
        b = t.add(unreal.ProgressBar, bar_widget, prefix + "BarBox")
        b.set_fill_color_and_opacity(GOLD_BRIGHT if not align_right else lc(0.75, 0.2, 0.15, 1))
        b.set_percent(1.0)
        if align_right:
            b.set_editor_property("bar_fill_type", unreal.ProgressBarFillType.RIGHT_TO_LEFT)

    hb = t.sizebox("LocalHeroBox", "Bars", 56, 56)
    hslot(hb, padding=(0, 0, 12, 0), v=unreal.VerticalAlignment.V_ALIGN_CENTER)
    t.image("Image_LocalHero", "LocalHeroBox", brush_tex(tex("T_GOTA_Portrait_Warden"), 56, 56))
    fighter("L", "Text_LocalName", "Bar_LocalHealth", False)
    score_box = t.sizebox("ScoreBox", "Bars", 160, None)
    hslot(score_box, v=unreal.VerticalAlignment.V_ALIGN_CENTER)
    t.text("Text_Score", "ScoreBox", "0  —  0", FONT_DISPLAY, 30, GOLD_BRIGHT, "Bold", 60, unreal.TextJustify.CENTER)
    fighter("R", "Text_OpponentName", "Bar_OpponentHealth", True)
    # Divine Momentum (challenger) under the local bar; champion difficulty / phase / guard under the opponent's.
    mb = t.sizebox("LMomentumBox", "LCol", 330, 6)
    vslot(mb, padding=(0, 5, 0, 0))
    m = t.add(unreal.ProgressBar, "Bar_Momentum", "LMomentumBox")
    m.set_fill_color_and_opacity(BLUE)
    m.set_percent(0.0)
    vslot(t.text("Text_Momentum", "LCol", "DIVINE MOMENTUM", FONT_BODY, 10, MUTED, "SemiBold", 200), padding=(0, 3, 0, 0))
    vslot(t.text("Text_ChampionState", "RCol", "", FONT_BODY, 10, MUTED, "SemiBold", 200, unreal.TextJustify.RIGHT), padding=(0, 5, 0, 0), h=unreal.HorizontalAlignment.H_ALIGN_RIGHT)
    vslot(t.text("Text_Timer", "Top", "90", FONT_DISPLAY, 22, TEXT, "SemiBold", 60, unreal.TextJustify.CENTER), padding=(0, 6, 0, 0), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)

    mid = t.add(unreal.VerticalBox, "Center", "Canvas")
    t.canvas(mid, (0.5, 0.36, 0.5, 0.36), (0, 0, 900, 260), (0.5, 0.5), autosize=True)
    vslot(t.text("Text_Phase", "Center", "PREPARE YOURSELF", FONT_DISPLAY, 34, GOLD_BRIGHT, "Bold", 200, unreal.TextJustify.CENTER), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    vslot(t.text("Text_Countdown", "Center", "3", FONT_DISPLAY, 110, TEXT, "Bold", 0, unreal.TextJustify.CENTER), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    # Combat callouts (PERFECT PARRY, GUARD BROKEN, SOVEREIGN SURGE ...), driven by the duel cue delegate.
    co = t.text("Text_Callout", "Canvas", "", FONT_DISPLAY, 30, GOLD_BRIGHT, "Bold", 240, unreal.TextJustify.CENTER)
    t.canvas(co, (0.5, 0.64, 0.5, 0.64), (0, 0, 1100, 60), (0.5, 0.5), autosize=True)
    hint = t.text("Text_ControlsHint", "Canvas", "LMB ATTACK   ·   RMB BLOCK (PARRY ON IMPACT)   ·   ALT + MOVE DODGE   ·   Q DIVINE COUNTER", FONT_BODY, 10, MUTED, "SemiBold", 200, unreal.TextJustify.CENTER)
    t.canvas(hint, (0.5, 1, 0.5, 1), (0, -28, 1200, 24), (0.5, 1), autosize=True)
    return finish(wbp, path)


def build_duel_results():
    path = DUEL_UI + "/WBP_GOTA_DuelResults"
    wbp = make_wbp(DUEL_UI, "WBP_GOTA_DuelResults", unreal.GOTADuelResultsWidget)
    t = Tree(wbp)
    t.add(unreal.Overlay, "Layers")
    oslot(t.image("Image_Dim", "Layers", brush_color(lc(0.002, 0.003, 0.01, 0.8)), hit=True))
    col = t.add(unreal.VerticalBox, "Col", "Layers")
    oslot(col, unreal.HorizontalAlignment.H_ALIGN_CENTER, unreal.VerticalAlignment.V_ALIGN_CENTER)
    hp = t.sizebox("HeroBox", "Col", 420, 260)
    vslot(hp, h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    hov = t.add(unreal.Overlay, "HeroLayers", "HeroBox")
    hov.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS)
    hsc = t.add(unreal.ScaleBox, "HeroScale", "HeroLayers")
    hsc.set_stretch(unreal.Stretch.SCALE_TO_FILL)
    oslot(hsc)
    t.image("Image_Hero", "HeroScale", brush_tex(tex("T_GOTA_Portrait_Warden"), 420, 260))
    hfr = t.add(unreal.Border, "HeroFrame", "HeroLayers")
    hfr.set_brush(frame_brush())
    hfr.set_brush_color(GOLD)
    oslot(hfr)
    vslot(t.text("Text_HeroName", "Col", "", FONT_DISPLAY, 18, TEXT, "SemiBold", 300, unreal.TextJustify.CENTER), padding=(0, 10, 0, 0), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    vslot(t.text("Text_Outcome", "Col", "VICTORY", FONT_DISPLAY, 84, GOLD_BRIGHT, "Bold", 160, unreal.TextJustify.CENTER), padding=(0, 10, 0, 0), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    vslot(t.text("Text_Summary", "Col", "", FONT_BODY, 14, TEXT, "SemiBold", 200, unreal.TextJustify.CENTER), padding=(0, 6, 0, 36), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    row = t.add(unreal.HorizontalBox, "Buttons", "Col")
    vslot(row, h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    hslot(action(t, "Btn_Rematch", "Buttons", "REMATCH"), padding=(0, 0, 20, 0))
    hslot(action(t, "Btn_ReturnToLobby", "Buttons", "RETURN TO MAIN MENU"))
    return finish(wbp, path)


# ============================================================ content data
def controls_text():
    lines = []
    imc = EAL.load_asset("/Game/ThirdPerson/Input/IMC_Default")
    try:
        mappings = imc.get_editor_property("mappings") if imc else []
    except Exception:
        mappings = []
    if mappings:
        groups = {}
        for m in mappings:
            act = m.get_editor_property("action")
            key = m.get_editor_property("key")
            if act and key:
                name = act.get_name().replace("IA_", "")
                groups.setdefault(name, []).append(str(key.get_editor_property("key_name")))
        for name, keys in groups.items():
            lines.append("%s:  %s" % (name.upper(), ", ".join(sorted(set(keys)))))
    lines.append("")
    lines.append("COMBAT (bound on the Warden):  Left Mouse, Right Mouse, 1, 2, Tab")
    lines.append("")
    lines.append("MENUS:  Arrow keys / D-pad / left stick to move  •  Enter / (A) to select  •  Esc / (B) to go back  •  Q / E or LB / RB to switch settings tabs")
    lines.append("")
    lines.append("Rebinding is not available yet. This list is generated from the project's input assets.")
    return "\n".join(lines)


def text(v):
    return unreal.Text(v)


def build_content_asset():
    ensure_dir(DATA)
    path = DATA + "/DA_GOTA_MenuContent"
    if EAL.does_asset_exist(path):
        da = EAL.load_asset(path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.GOTAMenuContentDefinition)
        da = TOOLS.create_asset("DA_GOTA_MenuContent", DATA, unreal.GOTAMenuContentDefinition, factory)

    dests = []
    for action_enum, label, sub, card_sub, icon, art, tip in [
        (unreal.GOTAMenuAction.CONTINUE, "CONTINUE", "RETURN TO YOUR MISSION", "", "T_GOTA_Icon_Continue", None, "Resume from your last campaign checkpoint."),
        (unreal.GOTAMenuAction.STORY, "STORY MODE", "A GALAXY OF CHOICES", "EXPLORE THE UPG UNIVERSE", "T_GOTA_Icon_Story", "T_GOTA_Card_Story", "The Sovereign Protocol campaign — solo or with up to three allies."),
        (unreal.GOTAMenuAction.ARENA, "ARENA MODE", "FIGHT. SURVIVE. ASCEND.", "FIGHT. SURVIVE. ASCEND.", "T_GOTA_Icon_Arena", "T_GOTA_Card_Arena", "Trial of Champions and Wrath of the Gods."),
        (unreal.GOTAMenuAction.MULTIPLAYER, "MULTIPLAYER", "PLAY WITH ALLIES", "PLAY WITH ALLIES", "T_GOTA_Icon_Multiplayer", "T_GOTA_Card_Multiplayer", "Host or join a LAN session for up to four Wardens."),
        (unreal.GOTAMenuAction.SETTINGS, "SETTINGS", "CUSTOMIZE YOUR EXPERIENCE", "CUSTOMIZE YOUR EXPERIENCE", "T_GOTA_Icon_Settings", "T_GOTA_Card_Settings", "Video, audio, controls and accessibility."),
        (unreal.GOTAMenuAction.EXTRAS, "EXTRAS", "LORE, CREDITS AND MORE", "LORE, CREDITS AND MORE", "T_GOTA_Icon_Extras", "T_GOTA_Card_Extras", "The UPG codex, controls guide and credits."),
        (unreal.GOTAMenuAction.QUIT, "QUIT", "EXIT TO DESKTOP", "", "T_GOTA_Icon_Quit", None, "Exit Gods of the Arena."),
    ]:
        d = unreal.GOTAMenuDestination()
        d.set_editor_property("action", action_enum)
        d.set_editor_property("label", text(label))
        d.set_editor_property("subtitle", text(sub))
        d.set_editor_property("card_subtitle", text(card_sub))
        d.set_editor_property("tooltip", text(tip))
        d.set_editor_property("icon", tex(icon))
        if art:
            d.set_editor_property("card_art", tex(art))
        dests.append(d)
    da.set_editor_property("destinations", dests)

    da.set_editor_property("faction_name", text("UNITED PLANETS OF GALAXIES"))
    da.set_editor_property("faction_tagline", text("EARTH • CAPITAL • UPG"))
    da.set_editor_property("game_subtitle", text("UNITED PLANETS OF GALAXIES"))
    da.set_editor_property("default_rank_title", text("AUTHORITY WARDEN"))
    da.set_editor_property("institutional_messages", [
        text("MANY WORLDS\nONE PEOPLE\nA STRONGER TOMORROW"),
        text("THE CHARTER BINDS\nTHE STRONG AND\nTHE WEAK ALIKE"),
        text("THE ARENA INITIATIVE\nTESTS EVERY\nASCENDANT"),
    ])

    da.set_editor_property("campaign_title", text("THE SOVEREIGN PROTOCOL"))
    da.set_editor_property("campaign_synopsis", text(
        "When a classified UPG security project begins reproducing the powers of forgotten gods, "
        "the Wardens of Aegis Team Seven are sent to the frontier to find out who is building it — and why. "
        "Their investigation will lead to a world erased from history, and to a choice over whether the future "
        "of the republic belongs to its people or to those powerful enough to control them."))
    chapters = []
    for cid, title, summary, playable in [
        ("Prologue", "PROLOGUE  •  THE FALLEN SANCTUARY", "Frontier ruins — an experimental guardian and an unknown energy source.", True),
        ("Chapter1", "I  •  QUARRY PROTOCOL", "Titan's Quarry", False),
        ("Chapter2", "II  •  VERDANT PROTOCOL", "Verdant Sanctuary", False),
        ("Chapter3", "III  •  FIRES OF SOVEREIGNTY", "Ashfall Crucible", False),
        ("Chapter4", "IV  •  THE ERASED COLONY", "Frostbound Citadel", False),
        ("Chapter5", "V  •  THE FORGOTTEN WORLDS", "Sunscar Expanse", False),
        ("Chapter6", "VI  •  THE BREAKING FLEET", "Stormbreak Coast", False),
    ]:
        c = unreal.GOTAStoryChapterInfo()
        c.set_editor_property("chapter_id", cid)
        c.set_editor_property("title", text(title))
        c.set_editor_property("summary", text(summary))
        c.set_editor_property("playable", playable)
        chapters.append(c)
    da.set_editor_property("chapters", chapters)

    previews = {}
    for arena_enum, tname in [(unreal.ArenaId.TITANS_QUARRY, "T_GOTA_Arena_TitansQuarry"), (unreal.ArenaId.ASHFALL_CRUCIBLE, "T_GOTA_Arena_AshfallCrucible")]:
        previews[arena_enum] = tex(tname)
    for arena_enum, asset in [
        (unreal.ArenaId.SUNSCAR_EXPANSE, "/Game/PhotoR_Backgrounds/Textures/Mobile/desert"),
        (unreal.ArenaId.FROSTBOUND_CITADEL, "/Game/PhotoR_Backgrounds/Textures/Mobile/Mountain_a_Winter"),
        (unreal.ArenaId.VERDANT_SANCTUARY, "/Game/PhotoR_Backgrounds/Textures/Mobile/Mountain_c_Summer"),
    ]:
        a = EAL.load_asset(asset)
        if a:
            previews[arena_enum] = a
    da.set_editor_property("arena_previews", previews)
    da.set_editor_property("horde_launch_enabled", False)
    da.set_editor_property("horde_unavailable_reason", text(
        "Wrath of the Gods is still in validation: wave spawning passes automated tests, but a full five-wave run has not been verified from the menu."))

    lore = []
    for cat, title, body in [
        ("THE REPUBLIC", "THE UNITED PLANETS OF GALAXIES",
         "Humanity's descendants govern thousands of worlds across several galaxies through the United Planets of Galaxies — a constitutional republic bound by a single Charter.\n\n"
         "An elected High Magistrate leads the executive. The House of Representatives gives every member world a voice and may check the Magistrate. Beside them sits the Galactic Council: "
         "admirals, industrialists and old families whose seats are not won at any ballot box.\n\n"
         "Citizens of the core call it the greatest civilization ever built. Citizens of the frontier are less certain whom it was built for."),
        ("THE REPUBLIC", "EARTH — CAPITAL OF HUMANITY",
         "Earth is the seat of the republic: the Charter Halls, the House chambers and the orbital rings that carry the traffic of a thousand worlds. "
         "Ancient monuments stand in the shadow of spires older than most colonies. For the core worlds, Earth is proof of what humanity can become. "
         "For the frontier, it is very far away."),
        ("THE AUTHORITY", "THE UNITED PLANETS AUTHORITY",
         "The UPA is the republic's interplanetary law-enforcement and security service. Its mandate covers Ascendant incidents, weapons trafficking, "
         "frontier stabilization, missing federal personnel and the enforcement of court orders.\n\n"
         "Its elite Ascendant-threat branch fields the Wardens: augmented operators with credentials, a chain of command and limits. "
         "Unlawful detention, falsified evidence and unauthorized executions carry consequences — even for a Warden."),
        ("THE AUTHORITY", "WARDENS OF AEGIS TEAM SEVEN",
         "Aegis Team Seven deploys in units of one to four Wardens. Their regulated augmentation redirects recovered combat energy into decisive strikes — "
         "power that answers to a badge, not a throne."),
        ("THE WORLDS", "CORE AND FRONTIER",
         "The republic is not one civilization but many. Core worlds live inside orbital networks and instant law. Frontier and low-technology worlds trade by "
         "hand, wait months for courts, and remember every promise the capital failed to keep. Both are citizens. Few agree on what that means."),
        ("ASCENDANTS", "ASCENDANT TECHNOLOGY",
         "Quantum, gravitic, biological, synthetic and stranger enhancements can raise a single person to the power of a myth. Ascendant abilities cost energy, heat "
         "and material; Ascendants can be exhausted, countered and killed. Regulating them is the defining political question of the age."),
        ("ASCENDANTS", "THE ARENA INITIATIVE",
         "The Arena Initiative exists for testing, training, sport, research and containment. Its champions are celebrated across the galaxies. "
         "Not every arena hides a secret — but some research is easiest to bury where the whole republic is watching something else."),
        ("CLASSIFIED", "PROJECT SOVEREIGN",
         "RESTRICTED — ASCENDANT SECURITY DIRECTORATE.\n\nReferences to a 'Sovereign Protocol' appear in sealed procurement records linked to arena research facilities. "
         "Purpose: not disclosed. Authorizing office: not disclosed. Warden access: denied."),
        ("GUIDE", "CONTROLS", controls_text()),
        ("CREDITS", "CREDITS AND LICENSES",
         "GODS OF THE ARENA — White Tiger Studios.\n\n"
         "Typefaces: Cinzel (Natanael Gama) and Montserrat (The Montserrat Project Authors), SIL Open Font License 1.1 — license texts in Tools/MenuFonts.\n\n"
         "Menu interface cues and ambience: original, synthesized for this project.\n"
         "Menu music: Unreal Engine Starter Content (Epic Games).\n"
         "Environment, character, animation and VFX content: Unreal Engine sample content and Fab / Marketplace packs used under their respective licenses, "
         "including Quarry Collections, Megascans, Paragon characters, Infinity Blade Effects, Realistic Starter VFX Pack Vol. 2, Fighting Animset Pro, Free Animation Library, "
         "Iceland Environment, Winter Forest Set, PhotoR Backgrounds, European Beech, Norway Maple and Human Vocalizations.\n\n"
         "Built with Unreal Engine 5.8."),
    ]:
        e = unreal.GOTALoreEntry()
        e.set_editor_property("category", text(cat))
        e.set_editor_property("title", text(title))
        e.set_editor_property("body", text(body))
        lore.append(e)
    da.set_editor_property("lore_entries", lore)

    da.set_editor_property("menu_music", EAL.load_asset("/Game/StarterContent/Audio/Starter_Music_Cue"))
    da.set_editor_property("menu_ambience", EAL.load_asset(AUDIO_UI + "/S_GOTA_Menu_Ambience"))
    da.set_editor_property("focus_cue", EAL.load_asset(AUDIO_UI + "/S_GOTA_UI_Focus"))
    da.set_editor_property("confirm_cue", EAL.load_asset(AUDIO_UI + "/S_GOTA_UI_Confirm"))
    da.set_editor_property("back_cue", EAL.load_asset(AUDIO_UI + "/S_GOTA_UI_Back"))
    da.set_editor_property("error_cue", EAL.load_asset(AUDIO_UI + "/S_GOTA_UI_Error"))
    da.set_editor_property("transition_cue", EAL.load_asset(AUDIO_UI + "/S_GOTA_UI_Transition"))
    EAL.save_asset(path)
    log("DA_GOTA_MenuContent authored (%d destinations, %d lore, %d chapters)" % (len(dests), len(lore), len(chapters)))


# ============================================================ level + controller wiring
def wire_level_and_controller():
    pc_path = "/Game/Framework/BP_WarOfGods_PlayerController"
    pc_bp = EAL.load_asset(pc_path)
    if pc_bp:
        cdo = unreal.get_default_object(pc_bp.generated_class())
        cdo.set_editor_property("player_camera_manager_class", unreal.GOTAPlayerCameraManager)
        EAL.save_asset(pc_path)
        log("BP_WarOfGods_PlayerController → GOTAPlayerCameraManager")

    world = unreal.EditorLoadingAndSavingUtils.load_map(MENU_MAP)
    marker = [a for a in unreal.EditorLevelLibrary.get_all_level_actors() if a.get_actor_label() == "GOTA_FrontendLevelMarker"]
    if not marker:
        # The seeded copy inherited MainMenu's Level Blueprint, which spawns the legacy WBP_MainMenu.
        # Clear only that inherited graph (logged node-by-node); MainMenu.umap keeps its own logic.
        removed = B.clear_level_script_graphs(world)
        m = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.Note, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
        m.set_actor_label("GOTA_FrontendLevelMarker")
        log("L_GOTA_MainMenu: removed %d inherited legacy level-script nodes" % removed)
    ws = None
    try:
        ws = world.get_world_settings()
    except Exception:
        ws = unreal.GameplayStatics.get_actor_of_class(world, unreal.WorldSettings)
    ws.modify()
    ws.set_editor_property("default_game_mode", unreal.GOTAFrontendGameMode)
    unreal.EditorLoadingAndSavingUtils.save_current_level()
    log("L_GOTA_MainMenu GameMode override → GOTAFrontendGameMode")


# ============================================================ heroes
HERO_TEX = "/Game/Heroes/Textures"
HERO_DATA = "/Game/Heroes/Data"
HERO_SRC = os.path.join(PROJECT, "Tools", "HeroArtSource")


def import_hero_textures():
    ensure_dir(HERO_TEX)
    files = [os.path.join(HERO_SRC, f) for f in sorted(os.listdir(HERO_SRC)) if f.endswith(".png")]
    import_files(files, HERO_TEX)
    for f in files:
        name = os.path.splitext(os.path.basename(f))[0]
        t = EAL.load_asset(HERO_TEX + "/" + name)
        if not t:
            continue
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        t.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        # Portraits keep mips + streaming (large); thumbnails are tiny and never stream.
        if name.endswith("_Thumbnail"):
            t.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
            t.set_editor_property("never_stream", True)
        EAL.save_asset(HERO_TEX + "/" + name)
    log("Imported %d hero textures" % len(files))


def apply_hero_body(da, HC, hero_id):
    """Prototype body + availability from GOTA_HeroContent.BODIES (also used by GOTA_BuildHeroBodies.py)."""
    b = HC.BODIES[hero_id]
    da.set_editor_property("body_mesh", unreal.load_asset(b["mesh"]))
    da.set_editor_property("body_retargeter", unreal.load_asset(b["retargeter"]))
    da.set_editor_property("body_scale", float(b["scale"]))
    da.set_editor_property("body_source_note", unreal.Text(b["note"]))
    da.set_editor_property("gameplay_pawn_class", unreal.load_class(None, "/Game/Player/BP_Player.BP_Player_C"))
    status = getattr(unreal.GOTAHeroAvailability, b["status"])
    da.set_editor_property("availability", status)
    da.set_editor_property("unavailable_reason", unreal.Text("" if b["status"] == "PLAYABLE" else HC.IN_DEVELOPMENT_REASON))


def build_hero_definitions():
    import sys
    sys.path.insert(0, os.path.join(PROJECT, "Tools"))
    import importlib
    import GOTA_HeroContent as HC
    importlib.reload(HC)
    ensure_dir(HERO_DATA)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.GOTAHeroDefinition)

    def make(entry, playable):
        name = "DA_Hero_" + entry["id"]
        path = HERO_DATA + "/" + name
        da = EAL.load_asset(path) if EAL.does_asset_exist(path) else TOOLS.create_asset(name, HERO_DATA, unreal.GOTAHeroDefinition, factory)
        da.set_editor_property("hero_id", entry["id"])
        da.set_editor_property("sort_order", entry["order"])
        da.set_editor_property("display_name", text(entry["name"]))
        da.set_editor_property("title", text(entry["title"]))
        da.set_editor_property("quote", text(entry["quote"]))
        da.set_editor_property("biography", text(entry["bio"]))
        da.set_editor_property("faction", text(entry["faction"]))
        da.set_editor_property("class_label", text(entry["cls"]))
        da.set_editor_property("specialization", text(entry["spec"]))
        da.set_editor_property("weapon_style", text(entry["weapon"]))
        stats = unreal.GOTAHeroCardStats()
        for key, value in zip(["attack", "defense", "mobility", "technique"], entry["stats"]):
            stats.set_editor_property(key, value)
        da.set_editor_property("card_stats", stats)
        abilities = []
        for aname, adesc in entry["abilities"]:
            a = unreal.GOTAHeroAbilityInfo()
            a.set_editor_property("name", text(aname))
            a.set_editor_property("description", text(adesc))
            abilities.append(a)
        da.set_editor_property("abilities", abilities)
        r, g, b = entry["accent"]
        da.set_editor_property("accent_color", lc(r, g, b, 1))
        if playable:
            da.set_editor_property("portrait", tex("T_GOTA_Portrait_Warden"))
            da.set_editor_property("thumbnail", tex("T_GOTA_Portrait_Warden"))
            da.set_editor_property("availability", unreal.GOTAHeroAvailability.PLAYABLE)
            da.set_editor_property("gameplay_pawn_class", unreal.load_class(None, "/Game/Player/BP_Player.BP_Player_C"))
            da.set_editor_property("unavailable_reason", text(""))
        else:
            da.set_editor_property("portrait", EAL.load_asset(HERO_TEX + "/T_GOTA_%s_Portrait" % entry["id"]))
            da.set_editor_property("thumbnail", EAL.load_asset(HERO_TEX + "/T_GOTA_%s_Thumbnail" % entry["id"]))
            da.set_editor_property("availability", unreal.GOTAHeroAvailability.PREVIEW_ONLY)
            da.set_editor_property("unavailable_reason", text(HC.PREVIEW_REASON))
            if entry["id"] in getattr(HC, "BODIES", {}):
                apply_hero_body(da, HC, entry["id"])
        EAL.save_asset(path)

    for entry in HC.HEROES:
        make(entry, False)
    make(HC.PROTOTYPE, True)
    log("Hero definitions: %d roster (preview-only) + 1 playable prototype" % len(HC.HEROES))


def build_hero_tile():
    path = UI + "/WBP_GOTA_HeroTile"
    wbp = make_wbp(UI, "WBP_GOTA_HeroTile", unreal.GOTAMenuButtonWidget)
    t = Tree(wbp)
    build_button_shell(t, (168, 196), None, None, art=True)
    col = t.add(unreal.VerticalBox, "Texts", "Button_Root")
    bslot(col, h=unreal.HorizontalAlignment.H_ALIGN_CENTER, v=unreal.VerticalAlignment.V_ALIGN_BOTTOM, padding=(6, 0, 6, 10))
    vslot(t.text("Text_Label", "Texts", "HERO", FONT_DISPLAY, 15, TEXT, "SemiBold", 120, unreal.TextJustify.CENTER), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    vslot(t.text("Text_Subtitle", "Texts", "", FONT_BODY, 8, MUTED, "SemiBold", 250, unreal.TextJustify.CENTER), h=unreal.HorizontalAlignment.H_ALIGN_CENTER)
    gen_cdo_defaults(wbp, style=unreal.GOTAButtonStyle.FEATURE_CARD)
    return finish(wbp, path)


def build_hero_select():
    path = UI + "/WBP_GOTA_HeroSelect"
    wbp = make_wbp(UI, "WBP_GOTA_HeroSelect", unreal.GOTAHeroSelectWidget)
    t = Tree(wbp)
    t.add(unreal.CanvasPanel, "Canvas")
    full = t.image("Image_Dim", "Canvas", brush_color(lc(0.004, 0.006, 0.016, 0.92)))
    t.canvas(full, (0, 0, 1, 1), (0, 0, 0, 0))

    # Left — ONE large character illustration (clipped) + identity block
    art = t.add(unreal.Overlay, "ArtLayers", "Canvas")
    art.set_clipping(unreal.WidgetClipping.CLIP_TO_BOUNDS)
    t.canvas(art, (0, 0, 0.67, 1), (0, 0, 0, 240))
    sc = t.add(unreal.ScaleBox, "PortraitScale", "ArtLayers")
    sc.set_stretch(unreal.Stretch.SCALE_TO_FILL)
    oslot(sc)
    t.image("Image_Portrait", "PortraitScale", brush_tex(EAL.load_asset(HERO_TEX + "/T_GOTA_Ayla_Portrait"), 1600, 1238))
    bottom = t.image("PortraitFadeB", "ArtLayers", brush_tex(tex("T_GOTA_Grad_V"), 4, 16), lc(0.004, 0.006, 0.016, 0.85))
    oslot(bottom)
    left_shade = t.image("PortraitShadeL", "ArtLayers", brush_tex(tex("T_GOTA_Grad_H"), 16, 4), lc(0.004, 0.006, 0.016, 0.7))
    oslot(left_shade, unreal.HorizontalAlignment.H_ALIGN_LEFT)
    ident = t.add(unreal.VerticalBox, "Identity", "Canvas")
    t.canvas(ident, (0, 1, 0, 1), (60, -262, 620, 300), (0, 1), autosize=True)
    vslot(t.text("Text_HeroName", "Identity", "AYLA", FONT_DISPLAY, 64, GOLD_BRIGHT, "Bold", 60))
    vslot(t.text("Text_HeroTitle", "Identity", "DESERT STORM", FONT_DISPLAY, 18, TEXT, "Regular", 520), padding=(4, 0, 0, 10))
    q = t.text("Text_Quote", "Identity", "", FONT_DISPLAY, 16, TEXT, "Regular", 40, wrap=True)
    vslot(q, padding=(4, 0, 0, 12))
    vslot(t.text("Text_Faction", "Identity", "", FONT_BODY, 11, MUTED, "SemiBold", 250), padding=(4, 0, 0, 0))

    # Right — profile panel
    pnl = panel(t, "ProfilePanel", "Canvas", (34, 28, 34, 24), lc(0.008, 0.011, 0.025, 0.94))
    t.canvas(pnl, (1, 0, 1, 1), (-40, 36, 640, 290), (1, 0))
    col = t.add(unreal.VerticalBox, "ProfileCol", "ProfilePanel")
    vslot(t.text("Text_PanelName", "ProfileCol", "AYLA", FONT_DISPLAY, 46, GOLD_BRIGHT, "Bold", 40))
    vslot(t.text("Text_PanelTitle", "ProfileCol", "DESERT STORM", FONT_DISPLAY, 15, TEXT, "Regular", 520), padding=(2, 0, 0, 14))
    for stat in ["Attack", "Defense", "Mobility", "Technique"]:
        row = t.add(unreal.HorizontalBox, "Row_" + stat, "ProfileCol")
        vslot(row, padding=(0, 3, 0, 3))
        lb = t.sizebox("Lbl_" + stat + "Box", "Row_" + stat, 150, None)
        hslot(lb)
        t.text("Lbl_" + stat, "Lbl_" + stat + "Box", stat.upper(), FONT_DISPLAY, 13, TEXT, "SemiBold", 200)
        bb = t.sizebox("Bar_" + stat + "Box", "Row_" + stat, 330, 10)
        hslot(bb, padding=(0, 0, 18, 0))
        bar = t.add(unreal.ProgressBar, "Bar_" + stat, "Bar_" + stat + "Box")
        bar.set_fill_color_and_opacity(GOLD_BRIGHT)
        hslot(t.text("Text_" + stat, "Row_" + stat, "0", FONT_DISPLAY, 16, TEXT, "SemiBold", 0))
    vslot(t.text("Text_Biography", "ProfileCol", "", FONT_BODY, 13, TEXT, "Regular", 0, wrap=True), padding=(0, 16, 0, 14))
    vslot(t.text("AbilitiesHeader", "ProfileCol", "DIVINE ABILITIES", FONT_DISPLAY, 16, GOLD, "SemiBold", 200), padding=(0, 0, 0, 6))
    vslot(t.text("Text_Abilities", "ProfileCol", "", FONT_BODY, 12, TEXT, "Medium", 0, wrap=True), padding=(0, 0, 0, 14))
    vslot(t.text("WeaponHeader", "ProfileCol", "WEAPON STYLE", FONT_DISPLAY, 16, GOLD, "SemiBold", 200))
    vslot(t.text("Text_Weapon", "ProfileCol", "", FONT_DISPLAY, 17, TEXT, "Regular", 40), padding=(0, 2, 0, 14))
    vslot(t.text("Text_Availability", "ProfileCol", "", FONT_BODY, 11, GOLD, "SemiBold", 100, wrap=True), padding=(0, 0, 0, 12))
    # Fight AI: champion difficulty on its own row so the action row never overflows the panel.
    vslot(action(t, "Btn_Difficulty", "ProfileCol", "CHAMPION: VETERAN"), padding=(0, 0, 0, 12), h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    acts = t.add(unreal.HorizontalBox, "Actions", "ProfileCol")
    vslot(acts)
    hslot(action(t, "Btn_Back", "Actions", "BACK"), padding=(0, 0, 14, 0))
    hslot(action(t, "Btn_Confirm", "Actions", "SELECT HERO"))

    # Bottom — roster carousel
    rs = t.add(unreal.ScrollBox, "RosterScroll", "Canvas")
    rs.set_editor_property("orientation", unreal.Orientation.ORIENT_HORIZONTAL)
    t.canvas(rs, (0, 1, 1, 1), (40, -24, 40, 210), (0, 1))
    box = t.add(unreal.HorizontalBox, "Box_Roster", "RosterScroll")
    return finish(wbp, path)


def build_loading_screen():
    path = UI + "/WBP_GOTA_LoadingScreen"
    wbp = make_wbp(UI, "WBP_GOTA_LoadingScreen", unreal.GOTALoadingScreenWidget)
    t = Tree(wbp)
    t.add(unreal.Overlay, "Layers")
    oslot(t.image("Bg", "Layers", brush_color(NAVY)))
    sc = t.add(unreal.ScaleBox, "ArtScale", "Layers")
    sc.set_stretch(unreal.Stretch.SCALE_TO_FILL)
    oslot(sc)
    t.image("Image_Art", "ArtScale", brush_tex(tex("T_GOTA_Menu_Backdrop_CleanPlate"), 1920, 1081))
    oslot(t.image("ShadeL", "Layers", brush_tex(tex("T_GOTA_Grad_H"), 16, 4), lc(0.004, 0.006, 0.016, 0.88)))
    oslot(t.image("ShadeB", "Layers", brush_tex(tex("T_GOTA_Grad_V"), 4, 16), lc(0.004, 0.006, 0.016, 0.9)))
    col = t.add(unreal.VerticalBox, "Info", "Layers")
    oslot(col, unreal.HorizontalAlignment.H_ALIGN_LEFT, unreal.VerticalAlignment.V_ALIGN_BOTTOM, (90, 0, 0, 120))
    vslot(t.text("Text_Kicker", "Info", "", FONT_BODY, 13, GOLD, "SemiBold", 400))
    vslot(t.text("Text_Title", "Info", "", FONT_DISPLAY, 76, GOLD_BRIGHT, "Bold", 60), padding=(0, 4, 0, 0))
    vslot(t.text("Text_Subtitle", "Info", "", FONT_DISPLAY, 20, TEXT, "Regular", 520), padding=(4, 0, 0, 16))
    bb = t.sizebox("BodyBox", "Info", 760, None)
    vslot(bb, h=unreal.HorizontalAlignment.H_ALIGN_LEFT)
    t.text("Text_Body", "BodyBox", "", FONT_BODY, 15, TEXT, "Regular", 0, wrap=True)
    tipb = t.sizebox("TipBox", "Layers", 1100, None)
    oslot(tipb, unreal.HorizontalAlignment.H_ALIGN_LEFT, unreal.VerticalAlignment.V_ALIGN_BOTTOM, (94, 0, 0, 48))
    t.text("Text_Tip", "TipBox", "", FONT_BODY, 12, MUTED, "Medium", 60, wrap=True)
    dep = t.add(unreal.HorizontalBox, "Deploying", "Layers")
    oslot(dep, unreal.HorizontalAlignment.H_ALIGN_RIGHT, unreal.VerticalAlignment.V_ALIGN_BOTTOM, (0, 0, 90, 44))
    mk = t.sizebox("MarkBox", "Deploying", 40, 40)
    hslot(mk, padding=(0, 0, 14, 0))
    t.image("Mark", "MarkBox", brush_tex(tex("T_GOTA_UPG_Insignia"), 40, 40))
    hslot(t.text("DeployText", "Deploying", "DEPLOYING", FONT_DISPLAY, 16, TEXT, "SemiBold", 500))
    return finish(wbp, path)


def main():
    log("=== GOTA UPG frontend build ===")
    only = os.environ.get("GOTA_BUILD_ONLY", "").strip()
    if only:
        # e.g. GOTA_BUILD_ONLY=build_duel_hud,build_duel_results — rebuild selected assets only.
        # Fonts are always resolved first: every widget text references the composite fonts.
        import_fonts()
        for name in [n.strip() for n in only.split(",") if n.strip()]:
            globals()[name]()
        log("=== complete (only: %s) ===" % only)
        return
    import_textures()
    import_fonts()
    import_audio()
    build_content_asset()
    import_hero_textures()
    build_hero_definitions()
    for fn in [build_nav_button, build_feature_card, build_action_button, build_list_entry, build_icon_button, build_setting_row, build_hero_tile]:
        fn()
    for fn in [build_main_menu, build_story, build_arena, build_multiplayer, build_lobby, build_settings, build_extras, build_modal, build_hero_select, build_loading_screen]:
        fn()
    build_root()
    build_duel_hud()
    build_duel_results()
    wire_level_and_controller()
    log("=== complete ===")


try:
    main()
    RESULT = "PASS"
except Exception as exc:  # noqa
    import traceback
    log("ERROR: " + str(exc))
    log(traceback.format_exc())
    RESULT = "FAIL"
finally:
    out = os.path.join(PROJECT, "Saved", "GOTA_BuildMenuUI.txt")
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(LOG_LINES) + "\nRESULT: " + RESULT + "\n")

# When launched via -ExecutePythonScript (full editor, needed for Slate-dependent imports),
# close the editor once the build finishes so automation can continue.
if os.environ.get("GOTA_QUIT_AFTER_BUILD") == "1":
    unreal.SystemLibrary.quit_editor()
