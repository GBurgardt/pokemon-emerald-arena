# Substitute and Smokescreen artwork

Current Substitute source: `substitute.html`, Claude pixel-art session 23ca6ab5-462c-4d74-b403-228abc08b61a, revision job sprite-917j5w39. Generated using actual gym and forest gameplay references, then visually reviewed and exported through Canvas. `substitute-metadata.json` describes its 16 frames; `frame-00.png` through `frame-15.png` are the native-size transparent exports. Feet anchor: (32,44). No downscaling.

Eight exact source colors are mapped by shading role to existing per-biome material palette indices. Neon particle colors 1 and 2 are never used. This avoids the old nearest-color conversion replacing cream with fluorescent green. No new OBJ palette or runtime allocation was added.

Smokescreen remains unchanged: original `source.html`, frames 16–31. Its bytes occupy the second half of `effect.4bpp`. The older combined `sheet.png`/`metadata.json` represent the first iteration, not the current doll.

Build consumes `effect.4bpp` for smoke, five `doll-*.4bpp` files for Substitute, and `palette.gbapal`. Makefile dependencies explicitly rebuild `realtime_arena.o` when these change. Run private `cover-smoke/export_redesign.py`, then `cover-smoke/pack_doll.py`. Do not run the superseded small-doll scaling script.

The runtime doll uses a 32x32 canvas, 512 bytes of OBJ VRAM instead of 2048. Its idle body pixels and feet position are unchanged; peripheral arrival/break flecks outside that canvas are cropped. Native dust still accompanies its appearance and break. The original 64x64 exports remain editable sources. This leaves room for smoke alongside larger/fallback opponent sprites. Allocation is verified by looking up the tag after loading: Emerald returns zero, not 0xFFFF, when LoadSpriteSheet fails.
