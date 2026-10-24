"""Draws the demo's UI sprites: rounded surfaces, circles, icons, the orb and the gear.

Everything is drawn white (or white shading) and antialiased, so a scene tints
it to any colour with SpriteComponent.tintColor. Run from the project root:

    python tools/make_ui_sprites.py

Needs Pillow. Icons come from Tabler Icons (MIT), the font the editor ships in
deki-editor/resources/fonts/tabler-icons.ttf.
"""
import json
import math
import os
import uuid

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPRITES = os.path.join(ROOT, "assets", "sprites")
TABLER = os.path.join(ROOT, "..", "deki-editor", "resources", "fonts", "tabler-icons.ttf")
SS = 8  # supersampling

ICONS = {
    "meadow": 0xEF01,     # tree
    "ui": 0xEA03,         # adjustments
    "effects": 0xF6D7,    # sparkles
    "logic": 0xF5D4,      # binary-tree
    "gps": 0xEAE8,        # map-pin
    "lora": 0xF094,       # antenna
    "trackball": 0xEFB1,  # circle-dot
    "keyboard": 0xEBD6,   # keyboard
    "back": 0xEA60,       # chevron-left
    "gear": 0xEB20,       # settings
}


def write_data(png, nine_slice=None):
    """Keeps an existing sidecar's GUID; makes a new one for a new sprite."""
    path = png + ".data"
    data = {}
    if os.path.exists(path):
        with open(path) as fp:
            data = json.load(fp)
    data.setdefault("guid", str(uuid.uuid4()))
    if nine_slice:
        data.setdefault("formatVersion", 1)
        data.setdefault("settings", {})["nine_slice"] = list(nine_slice)
    with open(path, "w", newline="\n") as fp:
        json.dump(data, fp, indent=2)
        fp.write("\n")


def save(img, rel, nine_slice=None):
    path = os.path.join(SPRITES, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path)
    write_data(path, nine_slice)
    print("wrote", rel, img.size)


def rounded(w, h, r, fill=255, stroke=0):
    """White rounded rectangle (or just its outline) as an RGBA sprite."""
    m = Image.new("L", (w * SS, h * SS), 0)
    d = ImageDraw.Draw(m)
    d.rounded_rectangle((0, 0, w * SS - 1, h * SS - 1), r * SS, fill=fill)
    if stroke:
        inset = stroke * SS
        d.rounded_rectangle((inset, inset, w * SS - 1 - inset, h * SS - 1 - inset), max(0, r - stroke) * SS, fill=0)
    a = m.resize((w, h), Image.LANCZOS)
    img = Image.new("RGBA", (w, h), (255, 255, 255, 0))
    img.putalpha(a)
    return img


def circle(d):
    return rounded(d, d, d / 2)


def icon(code, box, stroke_boost=0):
    """A Tabler glyph centred in a box x box white sprite."""
    size = box * SS
    font = ImageFont.truetype(TABLER, int(size * 0.92))
    m = Image.new("L", (size * 2, size * 2), 0)
    d = ImageDraw.Draw(m)
    d.text((size // 2, size // 2), chr(code), font=font, fill=255)
    if stroke_boost:
        m = m.filter(ImageFilter.MaxFilter(stroke_boost * 2 + 1))
    bbox = m.getbbox()
    glyph = m.crop(bbox)
    # Tabler glyphs sit inside a padded em square; fill the box instead.
    fit = size * 0.9 / max(glyph.size)
    glyph = glyph.resize((max(1, round(glyph.width * fit)), max(1, round(glyph.height * fit))), Image.LANCZOS)
    canvas = Image.new("L", (size, size), 0)
    canvas.paste(glyph, ((size - glyph.width) // 2, (size - glyph.height) // 2))
    a = canvas.resize((box, box), Image.LANCZOS)
    img = Image.new("RGBA", (box, box), (255, 255, 255, 0))
    img.putalpha(a)
    return img


def orb(d):
    """A soft, lit sphere in greys, so a tint gives it its colour."""
    n = d * SS
    img = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    px = img.load()
    c = (n - 1) / 2
    r = n / 2 - SS * 0.5
    lx, ly = -0.45, -0.55  # light from the upper left
    for y in range(n):
        for x in range(n):
            dx, dy = (x - c) / r, (y - c) / r
            q = dx * dx + dy * dy
            if q > 1:
                continue
            dz = math.sqrt(1 - q)
            diffuse = max(0.0, (dx * lx + dy * ly + dz * 0.7) / 1.0)
            shade = 0.62 + 0.38 * min(1.0, diffuse)
            spec = max(0.0, 1 - math.hypot(dx - lx * 0.75, dy - ly * 0.75) / 0.32) ** 2
            rim = max(0.0, (math.sqrt(q) - 0.82) / 0.18) * 0.25
            v = min(1.0, shade + spec * 0.55 + rim)
            g = int(255 * v)
            px[x, y] = (g, g, g, 255)
    return img.resize((d, d), Image.LANCZOS)


def spark(d, core, edge):
    """A soft glowing dot for particles: a pale core, a coloured glow, then nothing.
    Particles have no tint of their own here, so the colour lives in the sprite."""
    n = d * SS
    img = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    px = img.load()
    c = (n - 1) / 2
    for y in range(n):
        for x in range(n):
            r = math.hypot(x - c, y - c) / (n / 2)
            if r < 1:
                a = 1.0 if r < 0.3 else ((1 - r) / 0.7) ** 1.2
                t = min(1.0, r / 0.6)
                col = tuple(int(core[i] + (edge[i] - core[i]) * t) for i in range(3))
                px[x, y] = col + (int(255 * a),)
    return img.resize((d, d), Image.LANCZOS)


def pointer():
    """The on-screen pointer for trackball boards: a white arrowhead with a dark
    rim, its tip at the top-left pixel, where the pointer is."""
    w, h = 13, 15
    pts = [(0.7, 0.7), (12.0, 7.4), (6.9, 8.4), (4.9, 14.0)]
    big = [(x * SS, y * SS) for x, y in pts]
    rim = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(rim).polygon(big, fill=255)
    rim = rim.filter(ImageFilter.MaxFilter(SS + 1))
    body = Image.new("L", (w * SS, h * SS), 0)
    ImageDraw.Draw(body).polygon(big, fill=255)
    img = Image.new("RGBA", (w * SS, h * SS), (12, 14, 20, 0))
    img.putalpha(rim)
    white = Image.new("RGBA", img.size, (255, 255, 255, 255))
    img = Image.composite(white, img, body)
    return img.resize((w, h), Image.LANCZOS)


def main():
    # Surfaces. 9-slice borders equal the corner radius plus a pixel.
    save(rounded(40, 40, 10), "ui/surface.png", (11, 11, 11, 11))
    save(rounded(40, 40, 10, stroke=1), "ui/outline.png", (11, 11, 11, 11))
    # The button sprite every scene already uses: a pill-ish rounded rect.
    save(rounded(24, 24, 7), "ui/button.png", (8, 8, 8, 8))
    for d in (24, 34):
        save(circle(d), f"ui/circle{d}.png")
    # Icons: launcher tiles at 18 px, the back chevron at 14.
    for name, code in ICONS.items():
        if name == "back":
            save(icon(code, 14), "icons/back.png")
        elif name != "gear":
            save(icon(code, 18), f"icons/{name}.png")
    # The demo's own sprites, redrawn in place (same GUIDs).
    save(orb(32), "orb.png")
    save(spark(16, (255, 246, 214), (255, 110, 176)), "spark_warm.png")
    save(spark(16, (232, 251, 255), (59, 160, 255)), "spark_cool.png")
    save(pointer(), "ui/cursor.png")
    save(icon(ICONS["gear"], 40), "gear.png")


if __name__ == "__main__":
    main()
