"""Copies Inter into the project and lists the sizes the scenes use, so the
editor bakes them on import. Run from the project root: python tools/add_fonts.py"""
import json
import os
import shutil
import sys
import uuid

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from deki_guid import font_variant  # noqa: E402  (the engine's deterministic GUIDs)

SRC = os.path.join(ROOT, "..", "deki-editor", "resources", "fonts")
FONTS = {
    "Inter-SemiBold.ttf": [12, 13, 18, 22],
    "Inter-Medium.ttf": [10, 11, 12],
    "Inter-Regular.ttf": [11, 12],
}

for name, sizes in FONTS.items():
    dst = os.path.join(ROOT, "assets", "fonts", name)
    shutil.copy2(os.path.join(SRC, name), dst)
    data_path = dst + ".data"
    data = json.load(open(data_path)) if os.path.exists(data_path) else {"guid": str(uuid.uuid4())}
    sizes = sorted(set(sizes) | set(data.get("fontSettings", {}).get("sizes", [])))
    data["fontSettings"] = {"firstChar": 32, "lastChar": 126, "sizes": sizes}
    data["variants"] = {}
    for s in sizes:
        g, a = font_variant(data["guid"], s)
        data["variants"][str(s)] = {"atlasGuid": a, "guid": g}
    with open(data_path, "w", newline="\n") as fp:
        json.dump(data, fp, indent=2)
        fp.write("\n")
    print(name, data["guid"], sizes)
