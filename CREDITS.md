# Credits

- [Didier Lopes](https://github.com/DidierRLopes): accelerating Rollout, its palette-based rolling form, obstacle breaking and opponent squash in [PR #4](https://github.com/GBurgardt/pokemon-emerald-arena/pull/4). Original commits retained.
- [@Jahusek](https://x.com/Jahusek/status/2103395525265375697): real-time Rollout suggestion. [@GajoeDraws](https://x.com/GajoeDraws/status/2103555185398169731): continuing through the opponent.
- [@p_Itzo](https://x.com/p_Itzo/status/2103642975259807761): Reflect and Light Screen. [@MatheusLynar](https://x.com/MatheusLynar): level display and trainer-identity feedback.

- [Didier Lopes](https://github.com/DidierRLopes): Ghost phasing and Gengar in [PR #2](https://github.com/GBurgardt/pokemon-emerald-arena/pull/2), alongside his earlier Seismic Toss contribution. Original commits retained.
- [@Modmaster22](https://x.com/Modmaster22/status/2103410909779517678): movable Substitute idea. [@JuniorMc22](https://x.com/JuniorMc22/status/2103344165358813579): Smokescreen visibility idea.

- [pret/pokeemerald](https://github.com/pret/pokeemerald):
  Pokémon Emerald decompilation and reconstruction.
- [PMDCollab/SpriteCollab](https://github.com/PMDCollab/SpriteCollab):
  animation sources, metadata and individual credits for the 150 Pokémon.
  Pin: `d25607ff4746957df10bdb78db090887cd94f1f8`.
  The installer manifest preserves each species' credit file. The original
  sheets credit CHUNSOFT and individual PMDCollab contributors. The web installer downloads and converts these locally.
  The optional full BPS patch includes the converted animation data so it can
  be applied without a separate animation download. Original sprite sheets are
  not bundled. Conversion includes frame selection, palette reduction and GBA
  tile packing. Large source sheets use explicit 2× sampling where required;
  Lugia and Ho-Oh use explicit 3/4 and 7/8 import ratios; Salamence keeps its
  original pixels. Transparent frame margins are removed without cropping
  opaque artwork. None of these three is enlarged at runtime.
  Pose aliases and sampling are explicit in the [catalog](game/overlay/tools/arena/roster.json).
  These PMD resources remain their owners' material, not
  original art by this project or assets licensed by this project's code license.
- Nintendo, Game Freak, Creatures, The Pokémon Company and Chunsoft:
  Pokémon and the associated original material belong to their owners.
- [mGBA](https://github.com/mgba-emu/mgba): lab and test emulator, MPL-2.0.
- [Floating IPS](https://github.com/Sir-Walrus/Flips):
  BPS delta creation during release preparation. Its binary and source are
  not bundled with the installer.
- [Kenney Particle Pack 1.1](https://kenney.nl/assets/particle-pack): CC0 trace
  used for Leaf Blade. [License and conversion notes](game/overlay/graphics/arena/cc0/README.md).
- Arena background and props: images generated for this project, with the
  actual background used as a reference for props before GBA conversion.
- Arena code, AI, physics, integration, tools and installer: German Burgardt,
  with AI coding assistance.

Independent, unofficial experiment. Requires your own ROM.
The new code's license does not grant rights to Pokémon or third-party assets.
See the terms and credits of each linked project.
# Seismic Toss contribution

Didier Lopes ([DidierRLopes](https://github.com/DidierRLopes), [@didier_lopes](https://x.com/didier_lopes)) built the Seismic Toss grab, ascent, space orbit, dive and crater in [PR #1](https://github.com/GBurgardt/pokemon-emerald-arena/pull/1). His original implementation and procedural art generator are retained, with compatibility changes for the newer arena, effects and biomes.

Dig and Fly suggested by [@MatheusLynar](https://x.com/MatheusLynar). Teleport suggested by [@Modmaster22](https://x.com/Modmaster22).
