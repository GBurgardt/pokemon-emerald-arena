#!/usr/bin/env python3
"""Pack the reviewed 64px signature frames into deterministic GBA 4bpp tiles.

Only original effect art is handled here. PMD sprites use the separate importer.
Palette entries are assigned in arena_move_fx.c; no terrain palette is changed.
"""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def pack(folder, count, first, shades):
    frames = []
    for i in range(count):
        im = Image.open(folder / f"frame-{i:02}.png").convert("RGBA")
        assert im.size == (64, 64)
        assert set(im.getchannel("A").getdata()) <= {0, 255}
        frames.append(im)
    colors = sorted({p[:3] for im in frames for p in im.getdata() if p[3]},
                    key=lambda c: sum(c))
    mapping = {c: first + round(i * (shades - 1) / max(1, len(colors) - 1))
               for i, c in enumerate(colors)}
    data = bytearray()
    for im in frames:
        for ty in range(0, 64, 8):
            for tx in range(0, 64, 8):
                for y in range(8):
                    for x in range(0, 8, 2):
                        a = im.getpixel((tx + x, ty + y))
                        b = im.getpixel((tx + x + 1, ty + y))
                        data.append((mapping[a[:3]] if a[3] else 0)
                                    | ((mapping[b[:3]] if b[3] else 0) << 4))
    assert len(data) == count * 2048
    (folder / "forest.4bpp").write_bytes(data)
    print(folder.name, len(data))


if __name__ == "__main__":
    pack(ROOT / "graphics/arena/signatures150", 48, 5, 7)
    pack(ROOT / "graphics/arena/signatures150-extra", 16, 12, 4)
