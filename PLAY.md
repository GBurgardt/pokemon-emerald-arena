# Play Emerald Arena

Already use a ROM patcher? [Download the full BPS patch](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.0/Emerald-Arena-0.10.0-full.bps) and apply it to your unmodified Emerald (USA/Europe) ROM. It includes the animations and produces the same 0.10.0 game, without extra downloads.

[Download the ZIP](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.10.0/Emerald-Arena-0.10.0.zip) · [Watch gameplay](https://github.com/GBurgardt/pokemon-emerald-arena#watch-gameplay)

[**How to play · 1 minute video (MP4)**](https://github.com/GBurgardt/pokemon-emerald-arena/releases/download/v0.3.3/emerald-arena-walkthrough.mp4)
From download to your first fight, saving and continuing. GBA button names are
shown in the video; your emulator's keyboard bindings may differ.

## 1. Prepare the game on a computer

You need your own **unmodified Pokémon Emerald (USA/Europe) `.gba`** and Internet
access. Other languages, other Pokémon games and already-patched ROMs will not work.

1. Download **Emerald-Arena-0.10.0.zip** above. Extract/unzip it first.
2. Open **Prepare-Emerald-Arena.html** in a current Chrome, Firefox or Safari.
   It is a local web page, not an app to install. Do not open `install.mjs`.
3. Click **Choose Emerald ROM** and select your original `.gba` file, not a ZIP.
4. Wait while it checks the ROM and prepares the animations. Keep the page open.
5. Click **Download game**. You will get **Emerald-Arena-0.10.0.gba** in your downloads.

**Updating?** Back up your in-game `.sav` first. Import a copy for the new game,
or give that copy the same basename as the new ROM if your emulator requires it.
Do not reuse emulator save states across versions.

Your ROM is read locally, never uploaded or overwritten. The setup downloads
animation files from pinned public sources. The finished game works offline.

## 2. Open it in an emulator

Open **Emerald-Arena-0.10.0.gba** using your GBA emulator's **Open / Load game**
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
| SELECT | Switch to classic battle |
| L + R in the field | Next practice opponent |

Ghost types can walk or dodge out through any arena edge and come back in from
the opposite one, and they pass straight through rocks and other objects; they
are drawn translucent while inside a wall or an object. Attacks still hit cover.

Substitute costs a quarter of your maximum HP and creates a movable shield that
breaks under attacks. Smokescreen blocks sight briefly; the enemy cannot track you through it.

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

150 animated Pokémon. Wild and single trainer arena encounters;
double and link battles remain classic. Unsupported effects also fall
back to classic battles. This is not a fully rebalanced adventure.

### Pokémon

All nine Hoenn starter forms, the earlier roster, and early-route encounter
families through Slateport and Route 110, plus Route 116 and nearby caves/waters.
Evolution branches are included. [Full roster](game/overlay/tools/arena/roster.json) ·
[Exact encounter coverage](game/overlay/tools/arena/coverage.json).

Sprite coverage is not full battle compatibility. Unsupported abilities, held
items, most status conditions and movesets with no adapted damaging move stay
classic. Use SELECT for moves not available in the arena. Not all Hoenn is covered.

### Moves and objects

Fire attacks, mud, rocks, bubbles, wind and stars now join the earlier moves.
Stat changes affect native battle stats; Speed changes also affect movement.
Draining moves restore HP from actual damage. False Swipe leaves one HP.
Fire attacks can cause native burns, with damage about every five active seconds.
Pause and capture freeze that timer. [All adapted move profiles](game/overlay/src/arena_moves.c).

Reflect reduces physical damage; Light Screen reduces special damage. Each
lasts ten active seconds. Both can coexist, and menus pause their timers.
Critical hits bypass them, following Emerald's original rules.

Rollout is a real-time roll. Press A and the user spins up in place, then
rolls the way it is facing (or the way you hold), faster and faster, until it
hits something, curled into a ball in its own colours. It rolls right over
the rival, flattening it for a moment.
The farther it rolls, the harder it hits, and Defense Curl beforehand doubles
that; a fast roll breaks objects in its way.

Seismic Toss is a throw. Connect the grab up close and the attacker carries the
target straight up off the screen, dives back down with it and slams it into
the ground, leaving a crater. A throw that knocks the target out first circles
the Earth from space, as in the anime. It deals the user's level in damage, as
in the original game, and cannot touch Ghost types.

Dig hides the Pokémon underground before its strike. Fly leaves a moving shadow
before the dive. Teleport blinks away from danger. Double Team creates a decoy
that breaks when hit. Water leaves pools, ice freezes them and fire melts them.

Rocks, wood, foliage, crystals and explosive pods can break. Enemies navigate
obstacles, aim and dodge, with level-based reactions. Speed affects movement;
enemies consider distance, types and remaining PP. Attacks have a recovery time
shown below the HUD. Water Gun, Bite and Leaf Blade have distinct new effects.

[Release verification](RELEASE.md) · [Source architecture](HOW-IT-WORKS.md)
