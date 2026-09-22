#!/usr/bin/env bash
set -euo pipefail
arena_root="$(cd -- "$(dirname -- "$0")/../.." && pwd)"
arena_local="$arena_root/.arena-dev"
mkdir -p "$arena_local"
cd "$arena_root"
export LC_ALL=C
build_background() {
  mkdir -p .arena-dev/art
  if [[ ! -f .arena-dev/art/props.4bpp || ! -f .arena-dev/art/prop-palettes.gbapal || tools/arena/pack_props.c -nt .arena-dev/art/props.4bpp || graphics/arena/forest-props-sunburst-v1.png -nt .arena-dev/art/props.4bpp ]]; then
    cc -std=c11 -O2 -Wall -Wextra -Werror -I/opt/homebrew/include tools/arena/pack_props.c -L/opt/homebrew/lib -lpng -o .arena-dev/art/pack-props
    .arena-dev/art/pack-props graphics/arena/forest-props-sunburst-v1.png .arena-dev/art/props.4bpp .arena-dev/art/pieces.4bpp .arena-dev/art/blast.4bpp .arena-dev/art/prop-palettes.gbapal
  fi
  if [[ ! -f .arena-dev/art/actions.4bpp || tools/arena/pack_move_fx.c -nt .arena-dev/art/actions.4bpp || graphics/arena/cc0/trace_01.png -nt .arena-dev/art/actions.4bpp ]]; then
    cc -std=c11 -O2 -Wall -Wextra -Werror -I/opt/homebrew/include tools/arena/pack_move_fx.c -L/opt/homebrew/lib -lpng -o .arena-dev/art/pack-move-fx
    .arena-dev/art/pack-move-fx .arena-dev/art/actions.4bpp .arena-dev/art/bolts.4bpp
  fi
  if [[ ! -f .arena-dev/art/forest.8bpp || graphics/arena/forest-clearing-v2-clean.png -nt .arena-dev/art/forest.8bpp || tools/arena/pack_background.c -nt .arena-dev/art/forest.8bpp ]]; then
    cc -std=c11 -O2 -Wall -Wextra -Werror -I/opt/homebrew/include tools/arena/pack_background.c -L/opt/homebrew/lib -lpng -o .arena-dev/art/pack-background
    .arena-dev/art/pack-background graphics/arena/forest-clearing-v2-clean.png .arena-dev/art/forest.8bpp .arena-dev/art/forest.gbapal .arena-dev/art/forest.bin
  fi
  for biome in coast cave desert gym; do
    if [[ ! -f .arena-dev/art/$biome-props.4bpp || graphics/arena/props-$biome-v2.png -nt .arena-dev/art/$biome-props.4bpp || tools/arena/pack_props.c -nt .arena-dev/art/$biome-props.4bpp ]]; then
      cc -std=c11 -O2 -Wall -Wextra -Werror -I/opt/homebrew/include tools/arena/pack_props.c -L/opt/homebrew/lib -lpng -o .arena-dev/art/pack-props
      .arena-dev/art/pack-props graphics/arena/props-$biome-v2.png .arena-dev/art/$biome-props.4bpp .arena-dev/art/$biome-pieces.4bpp .arena-dev/art/$biome-blast.4bpp .arena-dev/art/$biome-prop-palettes.gbapal
    fi
    if [[ ! -f .arena-dev/art/$biome.8bpp || graphics/arena/biome-$biome-v1.png -nt .arena-dev/art/$biome.8bpp || tools/arena/pack_background.c -nt .arena-dev/art/$biome.8bpp ]]; then
      cc -std=c11 -O2 -Wall -Wextra -Werror -I/opt/homebrew/include tools/arena/pack_background.c -L/opt/homebrew/lib -lpng -o .arena-dev/art/pack-background
      .arena-dev/art/pack-background graphics/arena/biome-$biome-v1.png .arena-dev/art/$biome.8bpp .arena-dev/art/$biome.gbapal .arena-dev/art/$biome.bin
    fi
  done
}
case "${1:-help}" in
  assets)
    python3 tools/arena/import_sprites.py
    build_background
    ;;
  build)
    mkdir -p "$arena_root/.arena-dev"
    test -f .arena-dev/pmd/sprites.inc || { printf '%s\n' 'Import sprite assets first: ./tools/arena/dev.sh assets'; exit 1; }
    build_background
    /usr/bin/time -p make -j4 CPP=/opt/homebrew/bin/cpp-15 ARENA_LAB=1 FILE_NAME=arena_lab BUILD_DIR=build-lab > "$arena_root/.arena-dev/build.log" 2>&1 || { tail -60 "$arena_root/.arena-dev/build.log"; exit 1; }
    tail -12 "$arena_root/.arena-dev/build.log"
    ;;
  runner)
    cc -std=c11 -O2 -DENABLE_VFS -DENABLE_DIRECTORIES -DENABLE_DEBUGGERS -DM_CORE_GBA -DM_CORE_GB \
      -I "$arena_local/mgba/include" -I "$arena_local/mgba/build/include" -I /opt/homebrew/include \
      tests/arena/runner.c -L "$arena_local/mgba/build" -Wl,-rpath,"$arena_local/mgba/build" \
      -lmgba -L /opt/homebrew/lib -lpng -lSDL2 -o "$arena_local/arena-runner"
    ;;
  play)
    exec python3 tools/arena/lab.py play
    ;;
  test)
    exec python3 tests/arena/test_lab.py
    ;;
  release)
    build_background
    /usr/bin/time -p make -j4 CPP=/opt/homebrew/bin/cpp-15 ARENA_LAB=0 FILE_NAME=pokeemerald BUILD_DIR=build > "$arena_local/release-build.log" 2>&1 || { tail -60 "$arena_local/release-build.log"; exit 1; }
    tail -12 "$arena_local/release-build.log"
    ;;
  *)
    printf '%s\n' 'dev.sh assets|build|runner|test|play|release' 'python3 tools/arena/lab.py boot|status|battle|press|wait|snapshot|restore|save|screenshot'
    ;;
esac
