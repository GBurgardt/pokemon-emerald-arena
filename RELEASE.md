# Emerald Arena 0.10.2

[Download](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.2/Emerald-Arena-0.10.2.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.2/Emerald-Arena-0.10.2-full.bps)

Fixes a black rectangle during Reflect and Light Screen. Their projectile-style
visual ID was also passed to the generic action renderer, reading beyond its
tile atlas. Dedicated barriers remain; the generic renderer now rejects the
wrong atlas and bounds-checks its frame reads.

The Starmie showcase was re-recorded and all 480 frames checked. Earlier takes
are unchanged. Barrier mechanics, PP and expiry tests still pass.
ROM SHA-256: `3abaa1c525cc2f2bd0782fb81862313b0a4809f9fdc1519ebdf4d195899bafa7`.

# Emerald Arena 0.10.1

[Download](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.1/Emerald-Arena-0.10.1.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.1/Emerald-Arena-0.10.1-full.bps) · [Re-recorded gameplay](media/emerald-arena-150.mp4)

Fixes the blocky Lugia, Ho-Oh and Salamence sprites from 0.10.0. They were
reduced by half during import and enlarged again at runtime, losing detail.
Salamence now uses original-resolution pixels. Lugia uses 3/4 and Ho-Oh 7/8
of the original size, with transparent margins removed per frame and no
runtime enlargement. No opaque source area is cropped. Other sprites remain unchanged.

Source artwork and credits are unchanged. All 54,216 frames pass the decoder;
native combat, trainer encounters and capture/save regressions pass.
ROM SHA-256: `64d4aed4beb4511483dfe9f655ef9f243fb188d74649a7d4af613789e144a8ad`.

# Emerald Arena 0.10.0

[Download](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.0/Emerald-Arena-0.10.0.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.0/Emerald-Arena-0.10.0-full.bps) · [Gameplay](media/emerald-arena-150.mp4)

- 50 more animated Pokémon, bringing the roster to 150. Larger Lugia, Ho-Oh and Salamence; pose-aware ground alignment across the roster.
- New effects for Aeroblast, Sacred Fire, Dragon Claw, Shock Wave and more.
- Reflect and Light Screen protect against physical and special damage for ten active seconds. Menus pause their timers; native damage rules still apply.
- Both Pokémon levels are visible. Wild battles no longer show a second trainer; May and Brendan use their matching sprites.
- Didier Lopes' [Rollout PR #4](https://github.com/GBurgardt/pokemon-emerald-arena/pull/4) is merged, retaining his original commits. Integration fixes a tile-renderer boundary read and allocates its graphics only when needed.

Back up your in-game save before updating. Do not reuse emulator save states.
Unsupported battles still fall back to classic combat. [Scope and limits](PLAY.md#current-scope).

### Verification

All 54,216 animation frames across 150 Pokémon decode correctly. Native combat,
capture, trainer progression, five environments, move picker and result-flow
regressions passed. Barrier tests cover PP, independent expiry, pause and native
damage reduction: the same physical hit changed from 20 to 11 with Reflect,
and a special hit from 50 to 27 with Light Screen. Shipping cold boot and combat
passed without the lab mailbox. Not full-adventure or physical-GBA validation.

Release ROM SHA-256: `0809eb6c377cb848c456162352f2f97bbba5ec8d91807430b7373646718fc511`.

The clean public-source build, installer and full BPS patch reconstruct that
same file byte for byte. Installer and player-journey checks pass, as do the
21 Substitute/Smokescreen and 16 Ghost integration checks.

### Rollout

- Rollout joins the arena as a real-time move: the user tumbles in place,
  slowly at first, then rolls in the direction it is facing, gaining speed and
  leaving a dust trail, until it hits the rival, a wall or an object. Power
  doubles for every 40 px rolled, up to eight times the base, and once more
  after Defense Curl, like the original turn ramp. Once it launches, the
  Pokémon curls into a ball drawn in its own colours, with bands that turn
  along the heading as it rolls. The ball rolls right over the rival,
  flattening it for a moment, and a fast roll smashes through breakable
  objects and keeps going. Geodude, Graveler, Golem, Marill,
  Azumarill and Wailmer learn it.

# Emerald Arena 0.9.0

[Download](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.9.0/Emerald-Arena-0.9.0.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.9.0/Emerald-Arena-0.9.0-full.bps) · [Gameplay](media/emerald-arena-cover-smoke.mp4)

- Substitute creates movable, breakable cover at a quarter of your maximum HP.
- Smokescreen briefly blocks sight and enemy tracking.
- Gengar and Shelgon bring the animated roster to 100 Pokémon.
- Ghost phasing contributed by [Didier Lopes in PR #2](https://github.com/GBurgardt/pokemon-emerald-arena/pull/2). His original commit is retained.

Back up your in-game save before updating. Do not reuse emulator save states.

### Verification

21 Substitute/Smokescreen checks and 16 Ghost checks passed in the integrated ROM. Native lab, combat, capture, five environments, trainer victory/XP and move-picker checks passed too. All 36,344 animation frames across 100 Pokémon pass the C decoder.

The clean public-source build, installer and full BPS reconstruction match byte for byte. All 16 installer and four landing checks pass. The shipping ROM cold-boots without a test mailbox; native encounter, send-out, movement, PP and party checksums were verified. This is not full-adventure or physical-GBA validation.

Release ROM SHA-256: `8af469970aaf60fb711dd22caac2af80a238ce0e48859969f22e22152fd05226`.

### Ghost movement

- Ghost types phase through the arena walls and through the objects inside
  it: walking or dodging out through any edge brings them back in from the
  opposite edge, and rocks, logs, bushes, crystals and pods never block them.
  They are drawn translucent and tinted while inside a wall or an object, with
  a glow at both sides of an edge crossing. Attacks and projectiles still
  interact with cover as before. This is a type rule, so it covers every
  Ghost-type battler on either side; a ghost opponent routes straight through cover.
- Gengar joins the animated roster, so a Ghost type with real moves is
  available for trying this.

# Emerald Arena 0.8.0

[Download](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.8.0/Emerald-Arena-0.8.0.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.8.0/Emerald-Arena-0.8.0-full.bps) · [Watch the four moves](media/emerald-arena-four-moves.mp4)

- Dig, Fly and Teleport join the arena. Hide underground, dive from above or blink away from attacks.
- Didier Lopes' Seismic Toss from [PR #1](https://github.com/GBurgardt/pokemon-emerald-arena/pull/1), including the space-orbit finisher and persistent crater.
- Double Team decoys, psychic rock throws, larger flames and water/ice interactions.
- Trainer send-outs and single trainer battles in five environments, with matching breakable objects.
- 98 animated Pokémon. Double, link and unsupported encounters still use classic battles.

Back up your in-game save before updating. Do not reuse emulator save states.

### Verification

87 checks passed on the integrated lab build, including the four moves, combat regression and palette restoration in all five environments. Didier's original PR was also built and tested separately (8 checks). The shipping build cold-boots without the test mailbox and passes native encounter, send-out, movement, attack and party-checksum checks. This is not a full-adventure or physical-GBA validation.

The clean source rebuild, web/CLI installer and full BPS reconstruction all produce the same release. All 16 installer checks pass. Sprite registration is preserved without cropping opaque pixels.

Release ROM SHA-256: `8eefc295d6de396e5a1726d4909b582cf89b053917c74fe6bb66cf72827fbd24`.

## Seismic Toss contribution notes

- Seismic Toss joins the arena as a throw: a connected grab carries the target
  straight up off the screen, then the attacker dives back down head first
  with the target held beneath it and slams it into the ground, leaving a
  crater for the rest of the fight. When the throw will knock the target out,
  the leap cuts to the anime's view of the Earth from space: the pair circles
  the globe once and dives back for the slam. Damage is the native level-based
  script, resolved at the grab so misses and Ghost immunity whiff instead of
  playing the full throw. Wild Pokémon with the move use it too.

# Emerald Arena 0.6.0

[Download](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.6.0/Emerald-Arena-0.6.0.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.6.0/Emerald-Arena-0.6.0-full.bps) · [Controls](PLAY.md)

- All three Hoenn starters and their evolutions have directional animations.
- 92 animated Pokémon, covering early routes, nearby caves and their evolution families.
- 44 adapted moves. Fire, mud, rocks, bubbles, wind and stars have new effects.
- Native stat changes, draining moves, False Swipe, Focus Energy and burns.
- Poké Ball captures from 0.5.0 are included.
- Cosmetic fragments no longer crowd out real attack sprites.

Sprite coverage is not full battle compatibility. Unsupported moves, abilities
and encounters still use classic battles. Trainers, doubles and link battles
are unchanged. [Exact scope](PLAY.md#current-scope).

Back up your in-game `.sav` before updating and import a copy for the new ROM.
Do not reuse emulator save states across versions.

### Verification

All 23 real-ROM suites passed, plus host C checks and visual inspection.
Release-only controls verified capture, native saving and cold boot with the
same party. All 33,168 animation frames decode correctly; the original 7,864
frames remain byte-identical. Gyarados and Wailord use explicit 2× pixel reduction.

The clean public source rebuild, installer and two independent BPS decoders
match the release hash below. All 15 installer checks and four landing checks
passed. Physical GBA hardware has not been tested.

The reported Team Aqua gate was checked on a separate seven-badge save. The
original Slateport submarine event opened the passage, including after save/reboot.
No story bug was reproduced or patched; the reporter's exact save was unavailable.

Release ROM SHA-256: `47b2b0442cd619a4428ff5be95f5b914160aebdc0815a711fe6144a76e837b2c`.

## Previous: 0.5.0

[Download](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.5.0/Emerald-Arena-0.5.0.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.5.0/Emerald-Arena-0.5.0-full.bps) · [Watch a catch](media/emerald-arena-pokeballs.mp4)

Weaken the Pokémon, aim and throw. If you catch it, it joins your team.

- Hold L + R to aim, use directions to adjust, release to throw. B cancels.
- Uses regular Poké Balls from your bag. Misses consume a ball; escapes resume combat.
- Native catch rules, team/PC storage, Pokédex and saves. No capture XP.
- Trainer throw animation, ball arc, shakes, breakouts and original capture music.

Other ball types remain available through SELECT and the classic bag.
No arena nickname prompt yet. Trainer, double and unsupported battles stay classic.
Back up your in-game save before updating; do not reuse emulator save states.

### Verification

21 real-game capture checks and all 16 existing regression/performance suites
passed. Catches to the team and PC survived native saving and cold boot.
The release replay also verified capture, the party screen, saving and continuing.
The public source rebuild, installer and two independent BPS decoders produce
the same release hash. Installer and landing checks passed.
Physical GBA hardware has not been tested.

Release ROM SHA-256: `c3f9dc6d48a4c670a0e50e8a93dace6e646b16432ee68521cbc6cf06029770f1`.

## Previous: 0.4.0

[Download](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.4.0/Emerald-Arena-0.4.0.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.4.0/Emerald-Arena-0.4.0-full.bps) · [Watch the update](media/emerald-arena-combat-update.mp4)

- New Water Gun, Bite and Leaf Blade effects, with distinct attack sounds.
- Speed-based movement, readable enemy wind-ups and smarter move choices.
- Shared attack recovery, shown on the HUD. Switching moves cannot skip it.
- START opens a paused move picker with all four moves, PP and move roles.
- 22 animated Pokémon, 17 move profiles and trainers at the arena sidelines.

Native damage, PP, experience and saves remain in use. Trainer, double, link
and unsupported battles stay classic. This is not a full adventure rebalance.

Back up your in-game save before updating. Import a copy for the new ROM;
do not carry emulator save states between releases. [Setup and controls](PLAY.md).

### Verification

The private mGBA acceptance run passed 16 suites, covering combat, effects,
objects, result flow, adventure integration, trainers, tactics, move selection
and performance. Host geometry, navigation, physics and numeric checks also passed.
The new video was captured from the release build, including the return to the field.
Original GBA hardware has not been tested.

The public source overlay was rebuilt from a clean pinned pret checkout.
That build, the local installer and both BPS readers all reproduced the same
release hash. Public installer checks and landing checks passed too.

Release ROM SHA-256: `c312ada6abc53e16474c54171235f8d7fff94c81d936fe39295834fd58e6d0c2`.

## Previous: 0.3.3 web setup fixes

## Optional full patch

[Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.3.3/Emerald-Arena-0.3.1-full.bps): apply directly to your unmodified Emerald (USA/Europe) ROM with a BPS patcher. The converted PMD animations are included, with attribution in [CREDITS.md](CREDITS.md). No separate animation download is needed. No original ROM is distributed.

The output is byte-for-byte identical to game 0.3.1. Packaging was checked with Floating IPS and an independent JavaScript BPS reader; both reproduce the release SHA-256 below. This is not a new game build or a new full gameplay test run. The existing web installer is unchanged.

Maintainers can reproduce this artifact with `tools/package-full-patch.mjs`, using the private original ROM and verified game release. Never upload either ROM or the private verification output.

[Download setup 0.3.3](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.3.3/Emerald-Arena-0.3.3.zip).

The hosted and downloaded preparers now use the same source. Setup shows a
visible loading message, browser guidance and a ZIP alternative even if scripts
cannot start. Animation downloads time out with a retry message instead of
waiting indefinitely. The game and save filename are unchanged from 0.3.1.

## Previous setup 0.3.2

[**Download Emerald Arena — ZIP**](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.3.2/Emerald-Arena-0.3.2.zip)
· [Installation guide](https://github.com/GBurgardt/pokemon-emerald-arena/blob/main/PLAY.md)
· [Watch gameplay](https://github.com/GBurgardt/pokemon-emerald-arena#watch-gameplay)

**Setup update only. The game is identical to 0.3.1. Existing players do not need to update.**

- Extract the ZIP, open `Prepare-Emerald-Arena.html`, choose your original Emerald
  ROM, then click **Download game**. Open the resulting `.gba` in your emulator.
- Clear next steps for starting the practice team and first fight.
- Browser compatibility checks, visible preparation status and recoverable errors.
- A short player guide, troubleshooting and a structured bug report form.
- `SHA256SUMS.txt` checks the downloaded ZIP. The installer also checks your source
  ROM, every downloaded animation and the final game automatically.

The output remains `Emerald-Arena-0.3.1.gba` to preserve the existing save filename.
Keep backups of your saves. No original ROM, save or sprite sheets are included.

## Game release 0.3.1

### Setup 0.3.2 checks — September 14, 2026

- Ten public installer and player-journey tests pass.
- Real browser preparation with the original ROM: final SHA-256 matches below.
- Fresh release boot: SELECT creates the six-member practice party with Charizard.
- L+R starts a native practice encounter; movement and an attack verified in mGBA.
- No game binary changes. The 211-check acceptance result below belongs to the
  original 0.3.1 game release; it is not a newly claimed full-suite run.

### Original game release notes

Pokémon Emerald with real-time battles. Twelve animated Pokémon, ten move
profiles and breakable objects. Game UI, installer and documentation in English.

**Download the ZIP, open Prepare-Emerald-Arena.html, and choose your Emerald ROM.**
Requires your own unmodified Pokémon Emerald (USA/Europe).

Open the result in a GBA emulator. With no existing save, press SELECT on
NEW GAME for the practice team. Press L+R in the field to fight.

Charizard, Blastoise, Eevee, Dragonite, Scizor and Blaziken start in the party.
Six more Pokémon are in the PC. Save from the field menu to keep your progress.
Moves and effects not yet adapted use classic battles.

## Verification

- 211 real-game checks pass on the matching English lab build.
- Fresh release boot, native practice encounters and classic Run verified.
- Public installer tested with all 47 pinned source downloads.
- Reconstructed output matches the release byte for byte.
- BPS checksums, PNG conversion and wrong-ROM rejection tested separately.

Release SHA-256: `d357b8648b955529cc60e127164dfa75491508be61f87a7e747a14c53b024890`.
Source SHA-1: `f3ae088181bf583e55daf962a92bb46f4f1d07b7`.
Lab SHA-256: `5495d6e7772c1e5ea61f02dc3fc02d5f9ac2e2838e1bfcc32c3dcc03652147ec`.

This ZIP contains no ROM, save or sprite sheets.
[Source and controls](https://github.com/GBurgardt/pokemon-emerald-arena).
