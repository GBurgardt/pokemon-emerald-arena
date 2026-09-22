# Credits

- [pret/pokeemerald](https://github.com/pret/pokeemerald):
  Pokémon Emerald decompilation and reconstruction.
- [PMDCollab/SpriteCollab](https://github.com/PMDCollab/SpriteCollab):
  animation sources, metadata and individual credits for the 94 Pokémon.
  Pin: `d25607ff4746957df10bdb78db090887cd94f1f8`.
  The installer manifest preserves each species' credit file. The original
  sheets credit CHUNSOFT and individual PMDCollab contributors. The web installer downloads and converts these locally.
  The optional full BPS patch includes the converted animation data so it can
  be applied without a separate animation download. Original sprite sheets are
  not bundled. Conversion includes frame selection, palette reduction and GBA
  tile packing. Gyarados and Wailord are reduced 2× to fit; other sprites keep
  their original size. Pose aliases are explicit in the [catalog](game/overlay/tools/arena/roster.json).
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
