#!/usr/bin/env python3
"""Fetch pinned originals; compile their pixels/timing into GBA assets.

Original dimensions are preserved except explicit integer scale entries in the
catalog (Gyarados/Wailord: 2x nearest-neighbor reduction). Constant frame_offset
entries register asymmetrically padded originals across all poses. Nothing is cropped.

No ROMs and no generated image artwork. All downloaded/compiled data is private
and ignored by Git. Authorship and source hashes accompany every import.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import subprocess
import urllib.request
import urllib.error
import time
import struct
import statistics
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.arena-dev/pmd'
PIN = 'd25607ff4746957df10bdb78db090887cd94f1f8'
BASE = f'https://raw.githubusercontent.com/PMDCollab/SpriteCollab/{PIN}/'
MOTIONS = ['Idle', 'Walk', 'Shoot', 'Attack', 'SpAttack']


def fetch(relative):
    path = OUT / 'source' / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists():
        for attempt in range(3):
            try:
                with urllib.request.urlopen(BASE + relative, timeout=30) as response:
                    data = response.read(4_000_001)
                break
            except urllib.error.URLError:
                if attempt == 2: raise
                time.sleep(attempt + 1)
        if len(data) > 4_000_000: raise ValueError('Unexpectedly large sprite resource')
        path.write_bytes(data)
    return path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fetch-only', action='store_true')
    parser.add_argument('--catalog', type=Path, default=ROOT/'tools/arena/roster.json')
    args = parser.parse_args()
    license_path = fetch('LICENSE.md')
    catalog = json.loads(args.catalog.read_text())
    assert len({m['species'] for m in catalog}) == len(catalog), 'Duplicate species'
    assert len({m['dex'] for m in catalog}) == len(catalog), 'Duplicate source dex'
    def collect(mon):
        dex, species = mon['dex'], mon['species']
        scale=mon.get('scale',1)
        assert scale in (1,2), 'Only documented integer sampling is supported'
        assert len(dex) == 4 and dex.isdigit()
        assert species.replace('_', '').isalpha() and species.isupper()
        folder = f'sprite/{dex}/'
        xml = fetch(folder + 'AnimData.xml')
        credits = fetch(folder + 'credits.txt')
        nodes = {n.findtext('Name'): n for n in ET.parse(xml).findall('./Anims/Anim')}
        animations = []
        for motion in MOTIONS:
            # Some originals have no dedicated special pose. The fallback is
            # explicit in the provenance manifest, never a fabricated frame.
            # Large original lunges extend outside a 64-pixel OBJ. Explicit
            # per-species substitutions preserve intact pixels; arena movement
            # supplies displacement. Every alias is recorded in the manifest.
            actual = mon.get('poses', {}).get(motion, motion if motion in nodes else 'Shoot')
            seen = set()
            while nodes[actual].findtext('CopyOf'):
                if actual in seen: raise ValueError('Cyclic animation alias')
                seen.add(actual)
                actual = nodes[actual].findtext('CopyOf')
            node = nodes[actual]
            png = fetch(folder + actual + '-Anim.png')
            durations = [int(d.text) for d in node.findall('./Durations/Duration')]
            assert durations and all(0 < d < 128 for d in durations)
            animations.append(dict(name=motion, actual=actual, png=str(png),
                width=int(node.findtext('FrameWidth')), height=int(node.findtext('FrameHeight')),
                durations=durations, hit_frame=int(node.findtext('HitFrame', str(len(durations)//2) if motion == 'Attack' else '0')),
                scale=scale,frame_offset=mon.get('frame_offset',[0,0]),sha256=hashlib.sha256(png.read_bytes()).hexdigest()))
        print('Fetched/cached ' + species + ': 5 poses, 8 directions', flush=True)
        return dict(dex=dex,species=species,credits=credits.read_text(),animations=animations,
                    fixture_level=mon['level'], xml_sha256=hashlib.sha256(xml.read_bytes()).hexdigest(),
                    credits_sha256=hashlib.sha256(credits.read_bytes()).hexdigest())
    with ThreadPoolExecutor(max_workers=4) as pool:
        bundles = list(pool.map(collect, catalog))
    manifest = dict(repo='PMDCollab/SpriteCollab', commit=PIN, sprite_format='tile-dictionary-v1', bundles=bundles,
        license_file=str(license_path), note='Credits preserved separately for each species. '
        'Repository CC BY-NC terms do not relicense Nintendo/Chunsoft originals. Private prototype, not redistributed.')
    (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    if args.fetch_only:
        print(json.dumps(manifest, indent=2))
        return
    packer = OUT / 'pack-sprites'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',
        '-I/opt/homebrew/include',str(ROOT/'tools/arena/pack_sprites.c'),
        '-L/opt/homebrew/lib','-lpng','-o',str(packer)],check=True)
    lines = ['// Generated by import_sprites.py; source artwork stays outside Git.']
    table = []
    for bundle in bundles:
        dex, species = bundle['dex'], bundle['species']
        palette = OUT / (dex + '.gbapal')
        a0=bundle['animations'][0]
        offset=a0['frame_offset']
        assert len(offset)==2 and all(isinstance(x,int) and abs(x)<=16 for x in offset)
        argv = [str(packer), str(palette), ','.join(map(str,[a0['scale'],*offset]))]
        for a in bundle['animations']:
            a['binary'] = str(OUT / (dex + '-' + a['name'] + '.4bpp'))
            argv += [a['binary'], a['png'], str(a['width']), str(a['height']), str(len(a['durations']))]
        subprocess.run(argv,check=True)
        lines.append(f'static const u16 sPmdPal{dex}[] = INCBIN_U16(".arena-dev/pmd/{dex}.gbapal");')
        entries = []
        grounds = []
        shared_tiles = {}
        for a in bundle['animations']:
            ident = dex + a['name']
            # CopyOf poses share ROM pixels, not additional RAM/VRAM slots.
            tile_ident = shared_tiles.get(a['sha256'])
            if tile_ident is None:
                tile_ident = ident
                shared_tiles[a['sha256']] = tile_ident
                lines.append(f'static const u32 sPmdTiles{ident}[] = INCBIN_U32(".arena-dev/pmd/{dex}-{a["name"]}.4bpp");')
            # PMDCollab's format defines durations in 1/60 s, like arena ticks.
            durations = a['durations']
            lines.append(f'static const u8 sPmdTimes{ident}[] = {{{", ".join(map(str,durations))}}};')
            hit = sum(durations[:a['hit_frame']])
            entries.append(f'{{(const u8 *)sPmdTiles{tile_ident}, sPmdTimes{ident}, {len(durations)}, {sum(durations)}, {hit}}}')
            packed=Path(a['binary']).read_bytes()
            base=struct.unpack_from('<I',packed)[0]
            for direction in range(8):
                bottoms=[]
                for frame in range(len(durations)):
                    ids=struct.unpack_from('<64H',packed,8+(direction*len(durations)+frame)*128)
                    bottom=32
                    for y in range(63,-1,-1):
                        if any(any(packed[base+ids[(y//8)*8+x]*32+(y%8)*4:base+ids[(y//8)*8+x]*32+(y%8)*4+4]) for x in range(8)):
                            bottom=y+1;break
                    bottoms.append(bottom-32)
                grounds.append(int(statistics.median(bottoms)))
        lines.append(f'static const s8 sPmdGround{dex}[] = {{{", ".join(map(str,grounds))}}};')
        table.append(f'{{SPECIES_{species}, sPmdPal{dex}, {{{", ".join(entries)}}}, sPmdGround{dex}}}')
    lines.append('static const struct ArenaSpriteSet sPmdSets[] = {\n' + ',\n'.join(table) + '\n};')
    header = OUT / 'sprites.inc'
    content = '\n'.join(lines) + '\n'
    if not header.exists() or header.read_text() != content: header.write_text(content)
    manifest['bundles'] = bundles
    (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Imported {len(bundles)} Pokemon: 5 poses, 8 directions; original timing; no clipping; explicit scale in manifest.')


if __name__ == '__main__': main()
