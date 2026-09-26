# Real gameplay

## 150 Pokémon · 0.10.2

[Watch / download with sound](emerald-arena-150.mp4)

47 seconds: walking up to Drake, trainer send-outs, Lugia's Aeroblast,
Ho-Oh's Sacred Fire, Salamence's Dragon Claw, Raikou's Shock Wave,
Golem's Rollout and Starmie's Light Screen followed by Hydro Pump.

Six real mGBA captures from the integrated lab build. Teams and legal moves
were prepared for filming; combat damage, PP and enemy AI run normally.
Dialogue is shortened and separate battles are cut together. Continuous
Emerald music is mixed with game effects. No added combat graphics.
This is a curated showcase, not an uncut adventure or hardware benchmark.
Re-recorded after fixing the large-sprite import; the final Starmie take also
includes the 0.10.2 barrier-rendering fix. Combat inputs return to open
space instead of pushing into a corner between attacks.

## Poké Balls 0.5.0

[Watch / download with sound](emerald-arena-pokeballs.mp4)

22 seconds from the release build. Charizard weakens Blastoise, two throws
break out, the third catches it, then the native party screen shows the result.
Recorded in mGBA with original game audio. No forced catch or capture XP.

## Combat update 0.4.0

[Watch / download with sound](emerald-arena-combat-update.mp4) · [GIF](emerald-arena-combat-update.gif)

16 seconds from release 0.4.0: Blastoise versus Eevee, the move picker,
Water Gun, Bite, dodges and breakable objects. A continuous battle ending
back in the field, at normal speed with the game's own audio.
Recorded in mGBA with timed button inputs. No added combat effects.

MP4 SHA-256: `88f80afe6b6d7daa09363bc01c9b03ba0437db234067b1ceffb80018e3dc542b`.

## AYN Thor

[Watch / download the handheld video](emerald-arena-ayn-thor.mp4)

16 seconds recorded by Germán, playing on an AYN Thor with physical controls.
The GBA ROM runs in an emulator on the handheld, not on original GBA hardware.
Full recording with sound, converted to MP4 for playback. No cuts or added effects.

## Full gameplay

[Watch / download the full video with sound](emerald-arena-full-gameplay.mp4)

2:26 around Lilycove and Route 121. Wild battles, switching Pokémon, healing
and saving. Recorded before the 0.4.0 combat update; kept here as a longer look at the adventure.

The team was prepared on a copy of an advanced save. Normal speed, original
game audio. Only the return trip to the Pokémon Center is shortened.

MP4 SHA-256: `1432488bc580d7cf98614d5393d033dd67dcb6db2a2d26042ee34a6823a0df6c`.

## Quick look

[Watch / download the 17-second video](emerald-arena-17s.mp4) ·
[Download the looping GIF](emerald-arena-17s.gif)

Two seconds of Route 103's tall grass and Emerald's native encounter transition,
then the complete original fight. The field intro is a separate recording from
the same public release, using the demo's L+R practice encounter. A short section
of its transition is omitted to keep the opening to two seconds; this is an edit,
not one continuous field-to-battle take. No generated gameplay or extra effects.

- MP4: 17.000 seconds, 960 × 640, 60 fps, H.264 / AAC, 1.98 MB.
- GIF: 17.000 seconds, 480 × 320, 20 fps, looping, 4.23 MB.
- All 900 battle video frames and all 300 battle GIF frames match the originals
  exactly after decoding. Video packets are copied; GIF image data and source
  palettes are preserved. Native audio is joined and encoded to AAC.
- Intro controls: [intro-inputs.txt](capture-source/intro-inputs.txt).
- Lossless GIF joiner: [concat_gif.py](capture-source/concat_gif.py).
- Verification: [verify_media.py](capture-source/verify_media.py).

MP4 SHA-256: `45d81fca963066195a14018ace9e94687154f231ae9f7286f868f761cf0cbfd4`.
GIF SHA-256: `e22e9027aa2bb6089a47bb2800fba1c1ca0c774a2c2b1dbc3a4eece1840cc2de`.

## Original battle-only clip

[Preserved 15-second video](emerald-arena-15s.mp4) ·
[Preserved 15-second GIF](emerald-arena-15s.gif)

Charizard versus Blastoise, both level 36. A continuous take from the public
0.3.1 release: seven dodges, six objects destroyed, a chain explosion and hits
on both Pokémon. The video includes the game's own music and sound effects.
The GIF has no audio.

This is real ARM code running in mGBA. The route was rehearsed with timed GBA
buttons. No HP, PP, damage, enemy behavior or outcome was changed for the take.
The recorded core's final party, battlers and physics matched the rehearsal.

- MP4: 15.000 seconds, 960 × 640, H.264 / AAC, 1.72 MB.
- GIF: 15.000 seconds, 480 × 320, looping, 2.66 MB.
- Source: 896 GBA frames at the native clock, stereo PCM at 65,536 Hz.
  Pixel-nearest scaling; no interpolated animation or generated gameplay.
- Recorder: [capture.c](capture-source/capture.c).
- Exact input sequence: [inputs.txt](capture-source/inputs.txt).

Replaying the exact take also needs its matching private emulator snapshot.
That snapshot and its ROM are not published. Start the demo normally to play
these Pokémon yourself.

Release ROM SHA-256: `d357b8648b955529cc60e127164dfa75491508be61f87a7e747a14c53b024890`.
MP4 SHA-256: `c26fa29e1f65f437649167493231d2cbe9accfdcb940fc8eb1ef3952611422db`.
GIF SHA-256: `1ec74faa603fada8802cb62ff0bc7aa83b98be2f8357cd98766bed6f590ca4e4`.

Original game, music and character credits: [CREDITS.md](../CREDITS.md).
# Substitute and Smokescreen

[Watch the 23-second clip](emerald-arena-cover-smoke.mp4). Walk into Drake's room, enter the original trainer encounter, use Substitute and Smokescreen, then finish Shelgon with Flamethrower. This shows one opponent, not a full Elite Four victory.

Recorded from the native game with a disposable prepared team. Music and effects are game audio. The capture predates Ghost phasing; release 0.9.0 includes it alongside these moves.
