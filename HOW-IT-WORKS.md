# How the battles work

This is a modification of the original GBA game, built from the pinned
[pret/pokeemerald decompilation](BUILD.md). It is not a video filter or a separate PC game.

1. **Enter:** a supported wild or single trainer encounter enters the arena. Unsupported encounters
   keep Emerald's classic battle system.
2. **Fight:** arena code handles movement, aiming, projectiles, enemy decisions
   and breakable objects. The native battle engine handles the supported move's
   damage and party state; the arena does not invent a second XP or save system.
3. **Leave:** HP, PP and earned experience stay with your team. Decisions that
   still need the original game use its existing flow. Save from the field menu.

## Find the implementation

- [Battle integration](game/native-engine.patch): hooks into the original engine.
- [Arena controller](game/overlay/src/realtime_arena.c): entry, combat and return flow.
- [Movement and enemy navigation](game/overlay/src/arena_navigation.c).
- [Move profiles](game/overlay/src/arena_moves.c) and [effects](game/overlay/src/arena_move_fx.c).
- [Sprite decoder](game/overlay/src/arena_sprites.c): a tile dictionary streamed into two fixed frame buffers.
- [Breakable objects and fragments](game/overlay/src/arena_physics.c).
- [Installer](release/installer.mjs): source hash, patch checksums, pinned graphics and final ROM hash.

Supported stat changes, draining moves, False Swipe and burn eligibility use
Emerald's native battle routines. The arena provides the real time timing.
Visual fragments can give up a sprite slot so they never crowd out an attack.

## Verification, not a compatibility promise

[Release notes](RELEASE.md) record the tested ROM hash and release checks.
[Build instructions](BUILD.md) separate public installer tests from the private
real-ROM acceptance suite. The public checks run in [GitHub Actions](https://github.com/GBurgardt/pokemon-emerald-arena/actions).

The installer refuses the wrong ROM and verifies the final output byte for byte.
That proves the installed file matches the tested build; it does **not** prove
every emulator, Pokémon or move works. The limits are listed in [the player guide](PLAY.md#current-scope).
