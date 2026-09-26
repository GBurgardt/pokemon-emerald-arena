# Build

The release is already compiled. Players do not need a GBA toolchain.
For local setup without a browser, use Node 22+:

```sh
node release/install.mjs original.gba Emerald-Arena.gba
```

The installer rejects the wrong source ROM and never overwrites an existing file.

## Game source

This repository contains an overlay and patch for pret/pokeemerald.
The [landing source](site/) and [capture tools](media/capture-source/) are also included.

Run these commands from this repository:

```sh
git clone https://github.com/pret/pokeemerald.git workspace
git -C workspace checkout 5eff78649e7170a877b961ef0b3da13b81a16038
git -C workspace apply ../game/native-engine.patch
cp -R game/overlay/. workspace/
```

Set up the toolchain using that revision's `INSTALL.md`. The verified build
uses macOS arm64, agbcc `da598c1d918402c42c0c0d7128ba14567f3175e9`,
ARM binutils 2.47, cpp-15, Python 3, libpng and the host C compiler.
The wrapper uses Homebrew's `/opt/homebrew` prefix; adapt it on other platforms.

```sh
cd workspace
./tools/arena/dev.sh assets
./tools/arena/dev.sh build
./tools/arena/dev.sh release
```

`assets` downloads pinned sources and converts them without cropping. The tile
dictionary is lossless; Gyarados and Wailord use explicit 2× pixel reduction.
Keep generated sprite assets out of Git. Lab and release builds are separate.

## Checks

Public installer tests do not need a ROM:

```sh
node tools/build-installer.mjs --check
node --test tests/*.test.mjs
```

The earlier 0.6.0 release gate ran 23 suites against real ARM code in mGBA: existing
combat and capture checks, starter families, early encounters, new moves,
stat effects, burns, visuals and performance. Saves and full test evidence remain private.
Host C tests for navigation, physics, geometry and numbers are included in the
overlay; compile them with their `*_HOST` macros and sanitizers.
The 0.8.0 integrated build passed 87 checks, plus shipping-build boot and combat checks.
See [release verification](RELEASE.md) for the tested scope.

The public installer verifies every source PNG, every converted graphics block
and the final ROM hash. Release boot and local reconstruction are tested
separately. Physical GBA hardware has not been validated.

## Maintain the player package

Edit `tools/installer.html`, not the generated single-file HTML. Then run:

```sh
node tools/build-installer.mjs
node --test tests/*.test.mjs
node tools/package-release.mjs /path/outside/this/repo/new-release-directory
```

Packaging uses an explicit five-file allowlist. It refuses to overwrite an
existing archive and emits `SHA256SUMS.txt`. The original ROM is never an input
to ZIP packaging. Version 0.10.1 includes arena captures and 150 sprite sets.
