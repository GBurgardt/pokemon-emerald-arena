# Play Emerald Arena

Already use a ROM patcher? [Download the full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.7.0-preview.1/Emerald-Arena-0.7.0-preview.1-full.bps) and apply it to your unmodified Emerald (USA/Europe) ROM. It includes the animations and produces the same 0.7.0-preview.1 game, without extra downloads.

[Download the ZIP](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.7.0-preview.1/Emerald-Arena-0.7.0-preview.1.zip) · [Watch gameplay](https://github.com/GBurgardt/pokemon-emerald-arena#watch-gameplay)

[**How to play · 1 minute video (MP4)**](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.3.3/emerald-arena-walkthrough.mp4)
From download to your first fight, saving and continuing. GBA button names are
shown in the video; your emulator's keyboard bindings may differ.

## 1. Prepare the game on a computer

You need your own **unmodified Pokémon Emerald (USA/Europe) `.gba`** and Internet
access. Other languages, other Pokémon games and already-patched ROMs will not work.

1. Download **Emerald-Arena-0.7.0-preview.1.zip** above. Extract/unzip it first.
2. Open **Prepare-Emerald-Arena.html** in a current Chrome, Firefox or Safari.
   It is a local web page, not an app to install. Do not open `install.mjs`.
3. Click **Choose Emerald ROM** and select your original `.gba` file, not a ZIP.
4. Wait while it checks the ROM and prepares the animations. Keep the page open.
5. Click **Download game**. You will get **Emerald-Arena-0.7.0-preview.1.gba** in your downloads.

**Updating?** Back up your in-game `.sav` first. Import a copy for the new game,
or give that copy the same basename as the new ROM if your emulator requires it.
Do not reuse emulator save states across versions.

Your ROM is read locally, never uploaded or overwritten. The setup downloads
animation files from pinned public sources. The finished game works offline.

## 2. Open it in an emulator

Open **Emerald-Arena-0.7.0-preview.1.gba** using your GBA emulator's **Open / Load game**
command. [mGBA](https://mgba.io/downloads.html) is the desktop emulator used for
testing. If you already have a GBA emulator, you do not need another one.

On a phone or handheld, transfer this finished `.gba`, then import it using that
emulator's game browser. Preparing the ROM directly inside a phone's file preview
is not the supported setup path. Physical GBA hardware has not been verified.

## 3. Start your first fight

1. Start with **no save attached to this game**. Keep your existing saves backed up
   and separate; do not delete them to try this.
2. At the main menu, highlight **NEW GAME** and press the emulator button mapped
   to **SELECT**, not A. This skips the normal introduction and creates a practice team.
3. You arrive in the field with Charizard and five teammates. Press **L and R
   together** to start a practice encounter.
4. Move with the direction buttons, **A** to attack, **B + a direction** to dodge.
   **L / R** changes your selected move. **START** pauses and opens the move picker.

These are **GBA button names**, not literal keyboard keys. In your emulator's
input settings, check which keys correspond to A, B, L, R, START and SELECT.
The movement keys are usually the keyboard arrows. Bind L and R to separate keys
that you can press together. Touch controls show the GBA labels directly.

Save using the game's field menu. Practice encounters consume real HP and PP;
visit a Pokémon Center between fights. Reorder your party to try another lead.
Six additional Pokémon are in Box 1 at the PC.

## Controls

| GBA control | Action |
|---|---|
| Direction buttons | Move in eight directions |
| A | Attack; hold a direction to aim |
| B + a direction | Dodge |
| L / R | Previous / next move |
| Hold L + R in battle | Aim a Poké Ball; directions adjust the target |
| Release L or R while aiming | Throw; B cancels before release |
| START | Pause / resume; shows all four moves and PP |
| Up / right / down / left while paused | Choose move 1 / 2 / 3 / 4 |
| SELECT | Choose a teammate in supported trainer battles; classic battle in wild encounters |
| L + R in the field | Next practice opponent |

Regular saves keep their encounters and do not receive the practice party.

## Catching Pokémon

Weaken a wild Pokémon, hold **L + R**, aim with the direction buttons, then
release either shoulder to throw. **B** cancels while aiming. Combat slows
during aiming and the throw. A miss still uses one ball; a breakout resumes
the fight. A catch goes to your team, or the PC if your team is full.

This version uses regular **Poké Balls** from your bag. Buy them normally;
the optional fresh practice save starts with 20. Existing saves get no free
items. For other ball types or the full bag, press **SELECT** before throwing
to use classic battle. No capture XP is added, just like original Emerald.
There is no nickname prompt in the arena yet; the Name Rater still works.

## Something went wrong?

| What you see | What to do |
|---|---|
| The ZIP opens as a list of files | Extract it, then open `Prepare-Emerald-Arena.html`. |
| ROM rejected | Use the original unmodified USA/Europe `.gba`, not the download ZIP or a previous patched game. |
| Setup cannot fetch animations | Check your connection and retry. If it still fails, send the error text below. |
| The button does nothing in a file preview | Open the HTML in an actual desktop browser, with JavaScript enabled. |
| The normal Professor Birch introduction starts | You pressed A on NEW GAME. Restart with a separate fresh save and press SELECT instead. |
| A classic turn-based battle starts | This is the fallback for unsupported moves or encounters. The practice team has supported moves. |
| The game opens but controls do nothing | Check the emulator's input bindings and that the game is not paused. |
| Your save does not appear | Check the emulator's save location and matching ROM/save basenames. Back up before moving anything. |

[Report a problem](https://github.com/GBurgardt/pokemon-emerald-arena/issues/new?template=bug_report.yml).
Include the emulator/version, device, error message and steps. **Do not attach your ROM or save.**

## Current scope

94 animated Pokémon and 50 adapted move profiles. Supported wild and single
trainer encounters use the arena; double and link battles remain classic. Unsupported effects also fall
back to classic battles. This is not a fully rebalanced adventure.

### Pokémon

All nine Hoenn starter forms, the earlier roster, and early-route encounter
families through Slateport and Route 110, plus Route 116 and nearby caves/waters.
Evolution branches are included. [Full roster](game/overlay/tools/arena/roster.json) ·
[Exact encounter coverage](game/overlay/tools/arena/coverage.json).

Sprite coverage is not full battle compatibility. Unsupported abilities, held
items, most status conditions and movesets with no adapted damaging move stay
classic. In wild encounters, SELECT opens the classic system. In supported trainer
battles it opens the party selector. Not all Hoenn is covered.

Forest, water, cave, sand and indoor battles use matching backgrounds and
breakable objects. This is a preview: use a copy of your save. See [tested scope](PREVIEW.md).

### Moves and objects

Fire attacks, mud, rocks, bubbles, wind and stars now join the earlier moves.
Stat changes affect native battle stats; Speed changes also affect movement.
Draining moves restore HP from actual damage. False Swipe leaves one HP.
Fire attacks can cause native burns, with damage about every five active seconds.
Pause and capture freeze that timer. [All 50 move profiles](game/overlay/src/arena_moves.c).

Rocks, wood, foliage, crystals and explosive pods can break. Enemies navigate
obstacles, aim and dodge, with level-based reactions. Speed affects movement;
enemies consider distance, types and remaining PP. Attacks have a recovery time
shown below the HUD. Water Gun, Bite and Leaf Blade have distinct new effects.

[Release verification](RELEASE.md) · [Source architecture](HOW-IT-WORKS.md)
