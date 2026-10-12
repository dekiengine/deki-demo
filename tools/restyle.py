"""Gives the demo scenes their look: a flat dark theme, Inter type, rounded
surfaces, an icon launcher and a shared header on every screen.

Run from the project root after tools/make_ui_sprites.py and tools/add_fonts.py:

    python tools/restyle.py            # restyles every scene
    python tools/restyle.py menu ui    # only these

It always starts from the scenes as they were before the restyle (BASE), so
it can be run again and again while tuning. Object ids that components
reference are kept.
"""
import copy
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCENES = os.path.join(ROOT, "assets", "scenes")
BASE = "0a2359f"  # the scenes before this restyle

# ---------------------------------------------------------------------------
# Design tokens
# ---------------------------------------------------------------------------
BG = "#0B0E16FF"
SURFACE = "#151A26FF"
SURFACE_HI = "#1D2333FF"
SURFACE_LO = "#10131CFF"
SURFACE_OFF = "#12151DFF"
TEXT = "#EEF1F7FF"
MUTED = "#8A93A6FF"
FAINT = "#566074FF"
ACCENT = "#3BC3FFFF"
INK = "#07121AFF"  # text on a bright fill

CATEGORY = {
    "Meadow": "#34D399",
    "UI": "#3BC3FF",
    "Effects": "#F472B6",
    "Logic": "#A78BFA",
    "GPS": "#FBBF24",
    "LoRa": "#FB923C",
    "Trackball": "#2DD4BF",
    "Keyboard": "#60A5FA",
}

PX = 1 / 16  # metres per pixel at the project's 16 px per metre


def px(v):
    return round(v * PX, 4)


def alpha(hex6, a):
    return f"{hex6[:7]}{a:02X}"


# ---------------------------------------------------------------------------
# Assets
# ---------------------------------------------------------------------------
def guid_of(rel):
    with open(os.path.join(ROOT, "assets", rel + ".data")) as fp:
        return json.load(fp)["guid"]


SPR = {k: guid_of(v) for k, v in {
    "surface": "sprites/ui/surface.png",
    "outline": "sprites/ui/outline.png",
    "button": "sprites/ui/button.png",
    "circle24": "sprites/ui/circle24.png",
    "circle34": "sprites/ui/circle34.png",
    "back": "sprites/icons/back.png",
    "spark_warm": "sprites/spark_warm.png",
    "spark_cool": "sprites/spark_cool.png",
    "orb": "sprites/orb.png",
}.items()}
for name in CATEGORY:
    SPR["icon_" + name] = guid_of(f"sprites/icons/{name.lower()}.png")


def font(face, size):
    with open(os.path.join(ROOT, "assets", "fonts", face + ".ttf.data")) as fp:
        data = json.load(fp)
    variant = data["variants"].get(str(size))
    if not variant:
        raise SystemExit(f"{face} is not baked at {size}: add it in tools/add_fonts.py")
    return {"guid": variant["guid"], "source": data["guid"]}


SEMI, MED, REG = "Inter-SemiBold", "Inter-Medium", "Inter-Regular"


# ---------------------------------------------------------------------------
# Component builders
# ---------------------------------------------------------------------------
def text_props(text, face, size, color, w, h, align="Left", valign="CapCenter", order=20):
    return {
        "align": align, "alphaMode": "Blend", "color": color, "decorationColor": "#000000FF",
        "font": font(face, size), "fontSize": size, "height": h, "ignoreClip": False,
        "pixelScale": 1, "pixelSnap": True, "sortingOrder": order, "text": text,
        "verticalAlign": valign, "width": w,
    }


def text_comp(*args, **kw):
    return {"properties": text_props(*args, **kw), "type": "Deki2D::TextComponent"}


def sprite_comp(sprite, w, h, tint="#FFFFFFFF", mode="Normal", order=5):
    return {"properties": {
        "alphaMode": "Blend", "flipHorizontal": False, "flipVertical": False, "height": h,
        "ignoreClip": False, "pixelSnap": True, "renderMode": mode, "sortingOrder": order,
        "sprite": sprite, "tintColor": tint, "width": w,
    }, "type": "Deki2D::SpriteComponent"}


def anchor_comp(where, ox, oy):
    return {"properties": {"anchor": where, "offsetX": ox, "offsetY": oy}, "type": "Deki::ScreenAnchor"}


def make(oid, name, comps, x=0.0, y=0.0, children=None):
    return {"active": True, "children": children or [], "components": comps, "id": oid, "name": name,
            "transform": {"rotation": 0.0, "scaleX": 1.0, "scaleY": 1.0, "x": x, "y": y}}


# ---------------------------------------------------------------------------
# Scene access
# ---------------------------------------------------------------------------
class Scene:
    def __init__(self, name):
        self.name = name
        raw = subprocess.run(["git", "-C", ROOT, "show", f"{BASE}:assets/scenes/{name}.scene"],
                             capture_output=True, text=True, encoding="utf-8", check=True).stdout
        self.data = json.loads(raw)
        self.root = self.data["objects"][0]
        self.next_id = max(self._ids(self.root)) + 1

    def _ids(self, o):
        yield o["id"]
        for c in o.get("children", []):
            yield from self._ids(c)

    def new_id(self):
        self.next_id += 1
        return self.next_id - 1

    def find(self, name, parent=None):
        def walk(o):
            if o["name"] == name:
                return o
            for c in o.get("children", []):
                hit = walk(c)
                if hit:
                    return hit
            return None
        hit = walk(parent or self.root)
        if not hit:
            raise KeyError(f"{self.name}: no object {name}")
        return hit

    def remove(self, name):
        def walk(o):
            o["children"] = [c for c in o.get("children", []) if c["name"] != name]
            for c in o["children"]:
                walk(c)
        walk(self.root)

    def add(self, obj, parent=None, index=None):
        kids = (parent or self.root)["children"]
        kids.insert(len(kids) if index is None else index, obj)
        return obj

    def save(self):
        path = os.path.join(SCENES, self.name + ".scene")
        with open(path, "w", encoding="utf-8", newline="\n") as fp:
            json.dump(self.data, fp, indent=2, ensure_ascii=False)
            fp.write("\n")
        print("restyled", self.name)


def comp(obj, short):
    for c in obj["components"]:
        if c["type"].split("::")[-1] == short:
            return c["properties"]
    raise KeyError(f"{obj['name']} has no {short}")


def drop(obj, short):
    obj["components"] = [c for c in obj["components"] if c["type"].split("::")[-1] != short]


def place(obj, x, y):
    obj["transform"]["x"], obj["transform"]["y"] = x, y


def set_text(obj, text=None, face=MED, size=12, color=TEXT, w=None, h=None, align=None, valign="CapCenter",
             order=None):
    p = comp(obj, "TextComponent")
    keep = {k: p[k] for k in ("width", "height", "align", "sortingOrder")}
    p.update(text_props(p["text"] if text is None else text, face, size, color,
                        keep["width"] if w is None else w, keep["height"] if h is None else h,
                        keep["align"] if align is None else align, valign,
                        keep["sortingOrder"] if order is None else order))


def style_button(obj, w, h, normal, hover, pressed, disabled, sprite="button"):
    s = comp(obj, "SpriteComponent")
    s.update(sprite_comp(SPR[sprite], w, h, normal, "NineSlice", s["sortingOrder"])["properties"])
    b = comp(obj, "ButtonStyleComponent")
    b.update(normalColor=normal, hoveredColor=hover, pressedColor=pressed, disabledColor=disabled)
    c = comp(obj, "InputCollider")
    c.update(width=w, height=h)


def lighten(hex8, amount):
    r, g, b = (int(hex8[i:i + 2], 16) for i in (1, 3, 5))
    f = (lambda v: min(255, int(v + (255 - v) * amount))) if amount > 0 else (lambda v: int(v * (1 + amount)))
    return f"#{f(r):02X}{f(g):02X}{f(b):02X}{hex8[7:9] or 'FF'}"


def fill_button(obj, color, w, h, label_color=INK, size=12):
    style_button(obj, w, h, color, lighten(color, 0.18), lighten(color, -0.22), SURFACE_OFF)
    for child in obj["children"]:
        if any(c["type"].endswith("TextComponent") for c in child["components"]):
            set_text(child, face=SEMI, size=13 if size >= 13 else 12, color=label_color, w=w, h=h, align="Center")


# ---------------------------------------------------------------------------
# Shared chrome: background, header, back button, FPS readout, pointer
# ---------------------------------------------------------------------------
def chrome(sc, title, subtitle, back=True):
    comp(sc.find("Camera"), "CameraComponent")["clearColor"] = BG
    # A flat fill: the old dithered gradient becomes one solid colour, which
    # also gives the Scene View the screen's background in edit mode.
    g = comp(sc.find("Background"), "GradientComponent")
    g.update(ditherMode="None", stopCount=2, stop1Color=BG, stop1Position=0.0, stop2Color=BG,
             stop2Position=1.0, width=48.0, height=24.0)

    left = 14 if not back else 44
    t = sc.find("Title")
    set_text(t, title, SEMI, 18, TEXT, w=px(240), h=px(22), align="Left")
    comp(t, "ScreenAnchor").update(anchor="TopLeft", offsetX=px(left + 120), offsetY=-px(19))
    st = sc.find("Subtitle")
    set_text(st, subtitle, REG, 11, MUTED, w=px(240), h=px(14), align="Left")
    comp(st, "ScreenAnchor").update(anchor="TopLeft", offsetX=px(left + 120), offsetY=-px(35))

    if back:
        b = sc.find("Back")
        s = comp(b, "SpriteComponent")
        s.update(sprite_comp(SPR["circle24"], px(26), px(26), SURFACE_HI, "Normal", 30)["properties"])
        comp(b, "ButtonStyleComponent").update(normalColor=SURFACE_HI, hoveredColor="#2A3247FF",
                                                pressedColor=SURFACE_LO, disabledColor=SURFACE_OFF)
        comp(b, "InputCollider").update(width=px(36), height=px(36))
        comp(b, "ScreenAnchor").update(anchor="TopLeft", offsetX=px(25), offsetY=-px(27))
        label = b["children"][0]
        label["name"] = "Icon"
        label["components"] = [sprite_comp(SPR["back"], px(14), px(14), TEXT, "Normal", 31)]
        place(label, -px(1), 0.0)

    fps = sc.find("Fps")
    set_text(fps, None, MED, 10, FAINT, w=px(110), h=px(12), align="Right", order=900)
    comp(fps, "ScreenAnchor").update(anchor="TopRight", offsetX=-px(14 + 55), offsetY=-px(20))


def card(sc, name, x, y, w, h, tint=SURFACE, order=2, parent=None, index=None, border=True):
    """A rounded surface with a faint hairline, centred at (x, y) metres."""
    kids = []
    if border:
        kids.append(make(sc.new_id(), "Border", [sprite_comp(SPR["outline"], w, h, "#FFFFFF14", "NineSlice", order + 1)]))
    return sc.add(make(sc.new_id(), name, [sprite_comp(SPR["surface"], w, h, tint, "NineSlice", order)], x, y, kids),
                  parent, index)


# ---------------------------------------------------------------------------
# Scenes
# ---------------------------------------------------------------------------
def menu():
    sc = Scene("menu")
    chrome(sc, "Deki demo", "One project, every board", back=False)
    set_text(sc.find("Title"), face=SEMI, size=22, color=TEXT)

    # 4 x 2 launcher. Tiles 68 x 74 px with 8 px gaps, 12 px margins.
    order = ["Meadow", "UI", "Effects", "Logic", "GPS", "LoRa", "Trackball", "Keyboard"]
    tw, th, gap = 68, 74, 8
    xs = [-160 + 12 + tw / 2 + i * (tw + gap) for i in range(4)]
    ys = [120 - 62 - th / 2, 120 - 62 - th - gap - th / 2]
    for i, name in enumerate(order):
        tile = sc.find(name)
        place(tile, px(xs[i % 4]), px(ys[i // 4]))
        style_button(tile, px(tw), px(th), SURFACE, SURFACE_HI, SURFACE_LO, SURFACE_OFF, sprite="surface")
        colour = CATEGORY[name]
        label = tile["children"][0]
        set_text(label, name, SEMI, 12, TEXT, w=px(tw), h=px(14), align="Center", order=8)
        place(label, 0.0, -px(21))
        tile["children"] += [
            make(sc.new_id(), "Bubble", [sprite_comp(SPR["circle34"], px(34), px(34), alpha(colour, 0x2A), "Normal", 6)],
                 0.0, px(9)),
            make(sc.new_id(), "Icon", [sprite_comp(SPR["icon_" + name], px(18), px(18), colour + "FF", "Normal", 7)],
                 0.0, px(9)),
        ]
    sc.save()


def ui():
    sc = Scene("ui")
    chrome(sc, "UI toolkit", "Buttons, a roller and type")
    content_y = -20  # px, the middle of the area under the header

    # Buttons: a column of pills on the left.
    bw, bh = 116, 26
    buttons = sc.find("Buttons")
    place(buttons, px(-160 + 12 + bw / 2), 0.0)
    rows = {"Primary": ACCENT, "Accept": "#34D399FF", "Danger": "#F2667AFF", "Disabled": None}
    for i, (name, colour) in enumerate(rows.items()):
        b = sc.find(name)
        place(b, 0.0, px(content_y + 48 - i * (bh + 6)))
        if colour:
            fill_button(b, colour, px(bw), px(bh), INK if name != "Danger" else "#FFFFFFFF")
        else:
            style_button(b, px(bw), px(bh), SURFACE, SURFACE, SURFACE, SURFACE)
            set_text(b["children"][0], face=SEMI, size=12, color=FAINT, w=px(bw), h=px(bh), align="Center")

    # Roller in the middle, as a card with a highlighted row.
    roller = sc.find("Roller")
    place(roller, px(-160 + 12 + bw + 10 + 36), px(content_y))
    rc = comp(roller, "RollerComponent")
    rc.update(selectedColor=TEXT, normalColor=FAINT, selectedFontSize=13, normalFontSize=12,
              itemHeight=px(22), selectedItemHeight=px(30))
    # The roller drives its first "Clip" child; a stray second copy only drew
    # an empty box over it, so it goes.
    clips = [c for c in roller["children"] if c["name"] == "Clip"]
    roller["children"] = [c for c in roller["children"] if c["name"] != "Clip" or c is clips[0]]
    clip = clips[0]
    comp(clip, "ClipComponent").update(width=px(72), height=px(74))
    comp(roller, "InputCollider").update(width=px(72), height=px(74))
    for child in clip["children"]:
        if child["name"] == "Background":
            comp(child, "SpriteComponent").update(sprite=SPR["surface"], tintColor=SURFACE, renderMode="NineSlice",
                                                  width=px(72), height=px(74))
        elif child["name"] == "Selection":
            comp(child, "SpriteComponent").update(sprite=SPR["button"], tintColor=alpha("#3BC3FF", 0x2C),
                                                  renderMode="NineSlice", width=px(64), height=px(26))
        elif child["name"].startswith("TextRow"):
            set_text(child, face=SEMI, size=12, color=FAINT, w=px(72), h=px(22), align="Center")
    card(sc, "RollerFrame", px(-160 + 12 + bw + 10 + 36), px(content_y), px(72), px(74), tint="#00000000",
         order=40)

    # Type: alignment and size on the right, on a card.
    tx = 160 - 12 - 44
    group = sc.find("Text")
    place(group, px(tx), px(content_y))
    card(sc, "TypeCard", px(tx), px(content_y), px(88), px(122))
    lines = [("Left", 40, MED, 12, MUTED), ("Center", 20, MED, 12, MUTED), ("Right", 0, MED, 12, MUTED),
             ("Big", -32, SEMI, 22, ACCENT)]
    for name, y, face, size, colour in lines:
        o = sc.find(name)
        place(o, 0.0, px(y))
        set_text(o, "Centre" if name == "Center" else None, face, size, colour, w=px(72), h=px(28 if name == "Big" else 14))
    sc.save()


def effects():
    sc = Scene("effects")
    chrome(sc, "Effects", "Particles and tweens")
    cy, ch, cw = -22, 160, 92
    for i, x in enumerate((-100, 0, 100)):
        card(sc, f"Card{i}", px(x), px(cy), px(cw), px(ch))
    for name, x in (("SparksLabel", -100), ("FountainLabel", 100)):
        o = sc.find(name)
        place(o, px(x), px(cy - ch / 2 + 14))
        set_text(o, None, MED, 11, MUTED, w=px(cw), h=px(14), align="Center")
    tw = sc.add(make(sc.new_id(), "TweensLabel", [text_comp("Tweens", MED, 11, MUTED, px(cw), px(14), "Center")],
                     0.0, px(cy - ch / 2 + 14)))
    place(sc.find("Sparks"), px(-100), px(cy + 24))
    place(sc.find("Fountain"), px(100), px(cy - ch / 2 + 34))
    comp(sc.find("Sparks"), "ParticleEmitterComponent")["sprite"] = SPR["spark_warm"]
    comp(sc.find("Fountain"), "ParticleEmitterComponent")["sprite"] = SPR["spark_cool"]

    gear = sc.find("Gear")
    place(gear, 0.0, px(cy + 44))
    comp(gear, "SpriteComponent").update(tintColor="#A78BFAFF", width=px(40), height=px(40))
    pulse = sc.find("Pulse")
    place(pulse, 0.0, px(cy + 2))
    comp(pulse, "SpriteComponent").update(sprite=SPR["orb"], tintColor="#3BC3FFFF", width=px(22), height=px(22))
    slide = sc.find("Slide")
    place(slide, px(-20), px(cy - 34))
    comp(slide, "SpriteComponent").update(sprite=SPR["circle24"], tintColor="#F472B6FF", width=px(10), height=px(10))
    tween = comp(slide, "TweenComponent")
    tween["endValue"] = [px(40), 0.0, 0.0]
    sc.save()


def logic():
    sc = Scene("logic")
    chrome(sc, "Logic", "Five orbs, one state machine")
    card(sc, "Stage", 0.0, px(-20), px(296), px(150))
    colours = ["#3BC3FF", "#A78BFA", "#F472B6", "#FBBF24", "#34D399"]
    for i in range(5):
        o = sc.find(f"Orb{i + 1}")
        s = comp(o, "SpriteComponent")
        s.update(sprite=SPR["orb"], tintColor=colours[i] + "FF")
    cap = sc.find("Caption")
    place(cap, 0.0, px(-20 - 75 + 16))
    set_text(cap, "One graph, five phases", MED, 11, MUTED, w=px(280), h=px(14), align="Center")
    sc.save()


def info_screen(name, title, subtitle):
    sc = Scene(name)
    chrome(sc, title, subtitle)
    return sc


def gps():
    sc = info_screen("gps", "GPS", "Where on Earth")
    box = sc.find("Box")
    place(box, 0.0, px(-20))
    comp(box, "SpriteComponent").update(sprite=SPR["surface"], tintColor=SURFACE, width=px(296), height=px(150))
    sc.add(make(sc.new_id(), "StatusLabel", [text_comp("Status", MED, 11, MUTED, px(120), px(14))],
                px(-160 + 26 + 60), px(-20 + 75 - 18)))
    status = sc.find("Status")
    place(status, px(160 - 26 - 100), px(-20 + 75 - 18))
    set_text(status, None, SEMI, 12, TEXT, w=px(200), h=px(14), align="Right")
    details = sc.find("Details")
    place(details, 0.0, px(-20 - 14))
    set_text(details, None, MED, 12, TEXT, w=px(270), h=px(90), align="Left", valign="Top")
    sc.save()


def lora():
    sc = info_screen("lora", "LoRa", "868 MHz pings")
    card(sc, "StatusCard", 0.0, px(120 - 52 - 22), px(296), px(44))
    status = sc.find("Status")
    place(status, 0.0, px(120 - 52 - 22))
    set_text(status, None, MED, 12, TEXT, w=px(270), h=px(36), align="Left", valign="Middle")
    box = sc.find("HeardBox")
    place(box, 0.0, px(-30))
    comp(box, "SpriteComponent").update(sprite=SPR["surface"], tintColor=SURFACE, width=px(296), height=px(80))
    heard = sc.find("Heard")
    place(heard, 0.0, px(-30))
    set_text(heard, None, MED, 11, "#7EE2B8FF", w=px(270), h=px(66), align="Left", valign="Top")
    ping = sc.find("Ping")
    fill_button(ping, ACCENT, px(76), px(26))
    comp(ping, "ScreenAnchor").update(anchor="BottomRight", offsetX=-px(12 + 38), offsetY=px(10 + 13))
    sc.save()


def trackball():
    sc = info_screen("trackball", "Trackball", "Point and click the orb")
    field = sc.find("Field")
    place(field, 0.0, px(-6))
    comp(field, "SpriteComponent").update(sprite=SPR["surface"], tintColor=SURFACE, width=px(296), height=px(120))
    target = sc.find("Target")
    place(target, 0.0, px(-6))
    comp(target, "SpriteComponent").update(sprite=SPR["orb"], width=px(22), height=px(22))
    readout = sc.find("Readout")
    place(readout, 0.0, px(-120 + 24))
    set_text(readout, None, MED, 11, MUTED, w=px(296), h=px(32), align="Center", valign="Middle")
    sc.save()


def keyboard():
    sc = info_screen("keyboard", "Keyboard", "Type something")
    box = sc.find("Box")
    place(box, 0.0, px(-6))
    comp(box, "SpriteComponent").update(sprite=SPR["surface"], tintColor=SURFACE, width=px(296), height=px(108))
    typed = sc.find("Typed")
    place(typed, 0.0, px(-6))
    set_text(typed, None, MED, 12, TEXT, w=px(272), h=px(92), align="Left", valign="Top")
    last = sc.find("LastKey")
    place(last, px(-160 + 14 + 70), px(-120 + 25))
    set_text(last, None, MED, 11, MUTED, w=px(140), h=px(14), align="Left")
    for name, ox in (("LightUp", 12 + 14), ("LightDown", 12 + 28 + 6 + 14)):
        b = sc.find(name)
        fill_button(b, SURFACE_HI, px(28), px(26), TEXT)
        comp(b, "ScreenAnchor").update(anchor="BottomRight", offsetX=-px(ox), offsetY=px(12 + 13))
    level = sc.find("LightLevel")
    set_text(level, None, MED, 11, MUTED, w=px(100), h=px(14), align="Right")
    comp(level, "ScreenAnchor").update(anchor="BottomRight", offsetX=-px(12 + 28 + 6 + 28 + 10 + 50), offsetY=px(25))
    sc.save()


def meadow():
    sc = Scene("meadow")
    sky = comp(sc.find("Sky"), "GradientComponent")
    sky["ditherMode"] = "None"
    b = sc.find("Back")
    s = comp(b, "SpriteComponent")
    s.update(sprite_comp(SPR["circle24"], px(26), px(26), "#0B0E16B8", "Normal", 30)["properties"])
    comp(b, "ButtonStyleComponent").update(normalColor="#0B0E16B8", hoveredColor="#1D2333D0",
                                           pressedColor="#0B0E16FF", disabledColor="#0B0E1680")
    comp(b, "InputCollider").update(width=px(36), height=px(36))
    comp(b, "ScreenAnchor").update(anchor="TopLeft", offsetX=px(25), offsetY=-px(27))
    label = b["children"][0]
    label["name"] = "Icon"
    label["components"] = [sprite_comp(SPR["back"], px(14), px(14), TEXT, "Normal", 31)]
    place(label, -px(1), 0.0)
    # The hearts move right of the back button.
    for i in range(1, 4):
        a = comp(sc.find(f"Heart{i}"), "ScreenAnchor")
        a["offsetX"] = a["offsetX"] + px(34)
    score = sc.find("Score")
    set_text(score, None, SEMI, 13, TEXT)
    fps = sc.find("Fps")
    set_text(fps, None, MED, 10, "#FFFFFFB0", w=px(110), h=px(12), align="Right", order=900)
    comp(fps, "ScreenAnchor").update(anchor="BottomRight", offsetX=-px(10 + 55), offsetY=px(10))
    sc.save()


ALL = {"menu": menu, "ui": ui, "effects": effects, "logic": logic, "gps": gps, "lora": lora,
       "trackball": trackball, "keyboard": keyboard, "meadow": meadow}

if __name__ == "__main__":
    for name in sys.argv[1:] or ALL:
        ALL[name]()
