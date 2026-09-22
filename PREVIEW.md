# Trainer battles and changing arenas

0.7.0-preview.1. Not a replacement for the stable 0.6.0 download yet.

[Download preview](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.7.0-preview.1/Emerald-Arena-0.7.0-preview.1.zip) · [Full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.7.0-preview.1/Emerald-Arena-0.7.0-preview.1-full.bps) · [Watch trailer](media/emerald-arena-trainer-biomes.mp4)

- Supported single trainer battles enter the arena. Their next Pokémon follows through the native battle flow.
- Forest, coast, cave, desert and indoor arenas follow Emerald's battle environment.
- Each environment has its own rocks, wood, plants, crystals and breakable objects.
- 94 animated Pokémon, including Mewtwo and Rayquaza. 50 adapted moves.
- SELECT opens your party during supported trainer battles. Trainer Pokémon cannot be caught.

## What still needs work

Doubles, link battles and unsupported movesets, abilities, held items or status
conditions still use classic battles. Trainer bag-item AI and a full adventure
rebalance are not finished. This does not remove every turn-based battle.
Gym badge/story callbacks and every trainer combination have not been exhaustively tested.

Back up your native `.sav` and use a copy. Do not reuse emulator save states.

## Checks

Native button-played Calvin, Rick and Roxanne fixtures completed with XP and
field return. All five map environments entered their matching arena and had
objects broken with real attacks. Lab baseline and capture checks passed.
The release build separately passed movement, attacks, capture, native saving
and cold boot with the captured party. It rejects the lab fixture mailbox.

The pinned source rebuilt to the same ROM hash. The local installer and full
BPS patch both reconstructed that hash independently. All 15 public installer
tests passed. Physical GBA hardware has not been tested.

ROM SHA-256: `f380cecd5e467a47ff9e1c2fa9671d09b2094d1c4696a50c84f11600f0e8ca15`.
No ROM or save is distributed.

## The trailer

46 seconds of native gameplay recorded in mGBA, edited in Remotion with a
continuous Emerald music bed. Matchups and locations were prepared in the
development lab; they are not a continuous story playthrough. No added combat
effects or forced damage. All ten captured takes replayed to their recorded
game state, and the separate sound-effects pass matched every video frame.

The backgrounds and object sheets were generated with GPT Image 2 using the
existing arena as reference. Final PNGs and prompts are in
[graphics/arena](game/overlay/graphics/arena). Pokémon animation credits remain
in [CREDITS.md](CREDITS.md).
