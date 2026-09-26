#include "global.h"
#include "realtime_arena.h"
#include "arena_capture.h"
#include "item.h"
#include "trig.h"
#include "malloc.h"
#include "pokedex.h"
#include "arena_lab.h"
#include "arena_navigation.h"
#include "arena_sprites.h"
#include "arena_feedback.h"
#include "arena_moves.h"
#include "arena_move_fx.h"
#include "arena_physics.h"
#include "arena_terrain.h"
#include "arena_render.h"
#include "arena_psychic.h"
#include "event_object_movement.h"
#include "graphics.h"
#include "constants/event_objects.h"
#include "field_weather.h"
#include "fonts.h"
#include "constants/weather.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "bg.h"
#include "data.h"
#include "decompress.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "gpu_regs.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "reshow_battle_screen.h"
#include "save.h"
#include "scanline_effect.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "util.h"
#include "constants/abilities.h"
#include "constants/battle_move_effects.h"
#include "constants/heal_locations.h"
#include "constants/items.h"
#include "constants/trainers.h"
#include "constants/hold_effects.h"
#include "constants/moves.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define Q 256
#define SHOTS_COUNT ARENA_BOLT_SLOTS
#define SHOT_TAG 0xA710
#define MON_TAG 0xA720
#define AI_REPOSITION 0
#define AI_AIM 1
#define AI_RECOVER 2
#define AI_EVADE 3
// PMDCollab rows: down, down-right, right, up-right, up, up-left, left, down-left.
#define FACE_UP 4
#define FACE_DOWN 0
#define FACE_LEFT 6
#define FACE_RIGHT 2

struct ArenaBody
{
    s32 x, y;
    s32 aimX, aimY;
    u16 cooldown, dashCooldown;
    u8 sprite, moveSlot, dash, flash, facing, moving, shadow;
    const struct ArenaSpriteSet *art;
    u32 animClock;
    u16 shotTimer, shotElapsed;
    u8 animation, drawnFrame, drawnDirection, shotFacing, shotSlot;
    s8 dashX, dashY;
    s16 attackX, attackY;
    u8 actionLife, actionAge, connected, terrainMask,manualAim;
    s16 knockX,knockY;
    u16 burnClock;
    u16 weatherClock;
    u8 statusActions;
    u16 rollAngle;
    u8 rollStage, rollDistance, flat;
};

struct ArenaShot
{
    s32 x, y, vx, vy;
    u16 move;
    u8 side, sprite, life, direction, age, elementMask;
    u8 trail[2];
};

// DidierRLopes, PR #1: original throw choreography/state.
struct ArenaToss
{
    s32 damage;
    s16 height[2];
    s16 holdX, holdY;
    s16 drawX[2], drawY[2];
    s16 scaleX[2], scaleY[2];
    u16 rotation[2];
    u8 priority[2], animation[2], direction[2], hold[2];
    u8 phase, timer, attacker, shake, fatal, space, blackout;
};
struct ArenaState
{
    struct ArenaBody bodies[2];
    struct ArenaShot shots[SHOTS_COUNT];
    MainCallback savedCB1;
    u16 frame;
    u8 active, paused, classic, resultTimer;
    u8 lastAttacker, lastTarget, hudDirty;
    u32 aiRandom;
    struct ArenaPoint goal, waypoint, observed, previous;
    u16 thinkTimer, aimTimer, goalTimer;
    u8 aiState, style, reaction, aimError, cueSprite, aimSprite;
    u8 hitstop, attackBuffer, dashBuffer;
    s8 dashRequestX, dashRequestY;
    u32 lastBlast;
    struct ArenaToss toss;
};

static EWRAM_DATA struct ArenaState sArena = {};
// Persistent environment within one encounter, including native party switches.
static EWRAM_DATA bool8 sEnvironmentInitialized=FALSE;
static EWRAM_DATA u8 sFlameReach[2]={},sFlameElementMask[2]={};
// starts, active ticks, native hit opportunities, prop hits, peak reach.
EWRAM_DATA u32 gArenaFlameTelemetry[5]={};
static bool8 PsychicBusy(u8 side);
static bool8 PsychicStart(u8 side,s32 targetX,s32 targetY);
static bool8 BattleHasPsychic(void)
{
    u32 side,slot;
    for(side=0;side<2;side++)for(slot=0;slot<MAX_MON_MOVES;slot++)
        if(gBattleMons[side].moves[slot]==MOVE_PSYCHIC)return TRUE;
    return FALSE;
}
static void PsychicReset(void);
static void TickPsychic(void);
static void DrawPsychicAuras(void);
static void ElementsInit(void);
static void ElementsResetBattle(void);
static void ElementsTick(void);
static void ElementsDraw(void);
static bool8 SpecialBusy(u8 side);
static void DiveStart(u8 side,u16 move);
static bool8 SpecialInvulnerable(u8 side);
static bool8 WarpStart(u8 side);
static bool8 TossStart(u8 side);
static void SpecialReset(void);
static void SpecialHide(void);
static void SpecialTick(void);
static void SpecialDraw(void);
static bool8 TossActive(void);
extern u32 gArenaWarpTossTelemetry[8];
static bool8 ElementReact(struct ArenaShot *shot);
static bool8 ElementMotion(u8 side,s32 dx,s32 dy);
static EWRAM_DATA u32 sMonFrameTiles[2][512] = {};
static EWRAM_DATA bool8 sDemoRequested = FALSE;
EWRAM_DATA struct RealtimeArenaTelemetry gRealtimeArenaTelemetry = {};
EWRAM_DATA struct ArenaAiTelemetry gArenaAiTelemetry = {};
EWRAM_DATA struct ArenaSpriteTelemetry gArenaSpriteTelemetry = {};
EWRAM_DATA struct ArenaCombatTelemetry gArenaCombatTelemetry = {};
EWRAM_DATA struct ArenaMoveTelemetry gArenaMoveTelemetry = {};
// Acid Defense drops, Bite interrupts, Color Change activations.
EWRAM_DATA u32 gArenaAdventureTelemetry[3] = {};
EWRAM_DATA u32 gArenaStatusTelemetry[4] = {}; // burn applications[2], pulses[2]
EWRAM_DATA struct ArenaFrameTelemetry gArenaFrameTelemetry = {};
EWRAM_DATA bool8 gRealtimeArenaRestoringFaint = FALSE;
EWRAM_DATA bool8 gRealtimeArenaQuietResult = FALSE;
EWRAM_DATA struct ArenaResultTelemetry gArenaResultTelemetry = {};
EWRAM_DATA struct ArenaIntroTelemetry gArenaIntroTelemetry = {};
EWRAM_DATA u16 gArenaRenderTelemetry[8] = {};
static EWRAM_DATA u8 sHudRecovery=0;
static EWRAM_DATA bool8 sMoveMenuReady=FALSE;

static const struct BgTemplate sArenaBgs[] =
{
    { .bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31,
      .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0 },
    { .bg = 1, .charBaseIndex = 1, .mapBaseIndex = 30,
      .screenSize = 0, .paletteMode = 1, .priority = 2, .baseTile = 0 },
    { .bg = 2, .charBaseIndex = 3, .mapBaseIndex = 29,
      .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0 }
};
static const u32 sForestTiles[] = INCBIN_U32(".arena-dev/art/forest.8bpp");
static const u16 sForestPalette[] = INCBIN_U16(".arena-dev/art/forest.gbapal");
static const u16 sForestMap[] = INCBIN_U16(".arena-dev/art/forest.bin");
static const struct WindowTemplate sArenaWindows[] =
{
    { .bg = 0, .tilemapLeft = 0, .tilemapTop = 0,
      .width = 30, .height = 2, .paletteNum = 0, .baseBlock = 1 },
    { .bg = 0, .tilemapLeft = 1, .tilemapTop = 6,
      .width = 28, .height = 9, .paletteNum = 0, .baseBlock = 91 },
    { .bg = 0, .tilemapLeft = 0, .tilemapTop = 18,
      .width = 30, .height = 2, .paletteNum = 0, .baseBlock = 343 },
    DUMMY_WIN_TEMPLATE
};
static const u16 sArenaPalette[16] =
{
    RGB(3, 6, 9), RGB(4, 9, 12), RGB(6, 13, 15), RGB(10, 20, 19),
    RGB(29, 31, 29), RGB(8, 29, 23), RGB(31, 14, 8), RGB(29, 24, 9),
    RGB(12, 15, 17), RGB(2, 3, 6), RGB(31, 31, 31),
    RGB(3, 6, 9), RGB(2, 6, 9)
};
static const u32 sShotTiles[8] =
{
    0x00022000, 0x00211200, 0x02111120, 0x21111112,
    0x21111112, 0x02111120, 0x00211200, 0x00022000
};
// Shadows share unused shot-palette entries, preserving their exact colors and
// leaving two OBJ palettes for the original trainer artwork (16-slot GBA limit).
static const u16 sPlayerShotPalette[16] = {RGB_BLACK, RGB(10,31,25), RGB_WHITE, RGB(9,14,7), RGB(7,11,6)};
static const u16 sEnemyShotPalette[16] = {RGB_BLACK, RGB(31,10,5), RGB(31,25,10)};
static const struct SpriteSheet sShotSheet = {sShotTiles, sizeof(sShotTiles), SHOT_TAG};
static const struct SpritePalette sShotPalettes[] =
{
    {sPlayerShotPalette, SHOT_TAG}, {sEnemyShotPalette, SHOT_TAG + 1}
};
static const struct OamData sShotOam =
{
    .shape = SPRITE_SHAPE(8x8), .size = SPRITE_SIZE(8x8), .priority = 0
};
static const struct SpriteTemplate sShotTemplate =
{
    .tileTag = SHOT_TAG, .paletteTag = SHOT_TAG, .oam = &sShotOam,
    .anims = gDummySpriteAnimTable, .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};
static const struct OamData sMonOam =
{
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .shape = SPRITE_SHAPE(64x64), .size = SPRITE_SIZE(64x64), .priority = 0
};
static const struct SpriteTemplate sMonTemplate =
{
    .tileTag = MON_TAG, .paletteTag = TAG_NONE, .oam = &sMonOam,
    .anims = gDummySpriteAnimTable, .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};
static const u32 sShadowTiles[] = {0x00000000, 0x33300000, 0x44433300, 0x44444330, 0x44433300, 0x33300000, 0x00000000, 0x00000000, 0x00000000, 0x00000333, 0x00333444, 0x03344444, 0x00333444, 0x00000333, 0x00000000, 0x00000000};
static const struct SpriteSheet sShadowSheet = {sShadowTiles, sizeof(sShadowTiles), SHOT_TAG + 2};
static const struct OamData sShadowOam =
{
    .shape = SPRITE_SHAPE(16x8), .size = SPRITE_SIZE(16x8), .priority = 1
};
static const struct SpriteTemplate sShadowTemplate =
{
    .tileTag = SHOT_TAG + 2, .paletteTag = SHOT_TAG, .oam = &sShadowOam,
    .anims = gDummySpriteAnimTable, .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};
static const struct OamData sAimOam =
{
    .shape = SPRITE_SHAPE(32x32), .size = SPRITE_SIZE(32x32), .priority = 1
};
static const struct SpriteTemplate sAimTemplate =
{
    .tileTag = SHOT_TAG + 3, .paletteTag = SHOT_TAG, .oam = &sAimOam,
    .anims = gDummySpriteAnimTable, .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable, .callback = SpriteCallbackDummy
};
static const u8 sTextPaused[] = _("ARROWS: PICK MOVE   START: PLAY");
static const u8 sTextPP[] = _(" PP ");
static const u8 sMoveDirections[4][7] = {_("UP"),_("RIGHT"),_("DOWN"),_("LEFT")};
static const u8 sMoveKinds[6][8] = {_("CLASSIC"),_("HIT"),_("DASH"),_("SHOT"),_("STATUS"),_("BOOST")};
#if ARENA_LAB
static const u8 sDemoName[] = _("GERMAN");
#else
static const u8 sDemoName[] = _("ARENA");
#endif

// Native encounter shortcut for the opt-in practice SAVE, not a ROM mailbox.
// Curated levels retain genuine compatible moves from each native learnset.
static const struct {u16 species;u8 level;} sPracticeRivals[] =
{
    {SPECIES_EEVEE,36}, {SPECIES_BLASTOISE,36}, {SPECIES_SCIZOR,15},
    {SPECIES_BLAZIKEN,20}, {SPECIES_DRAGONITE,30}, {SPECIES_CHARIZARD,36},
    {SPECIES_SCEPTILE,43}, {SPECIES_RAYQUAZA,60}, {SPECIES_MEWTWO,66}
};
static EWRAM_DATA u8 sPracticeRival = 0;

void RealtimeArena_PracticeTick(void)
{
    extern const u8 EventScript_ArenaPracticeBattle[];
    u32 i;
    if (!FlagGet(FLAG_ARENA_PRACTICE) || gPaletteFade.active
        || ArePlayerFieldControlsLocked() || ScriptContext_IsEnabled()
        || (JOY_HELD(L_BUTTON | R_BUTTON) != (L_BUTTON | R_BUTTON))
        || !JOY_NEW(L_BUTTON | R_BUTTON)) return;
    for (i = 0; i < gPlayerPartyCount; i++)
        if (GetMonData(&gPlayerParty[i], MON_DATA_HP) && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG)) break;
    if (i == gPlayerPartyCount) return;
    CreateScriptedWildMon(sPracticeRivals[sPracticeRival].species,
                         sPracticeRivals[sPracticeRival].level, ITEM_NONE);
    sPracticeRival = (sPracticeRival + 1) % ARRAY_COUNT(sPracticeRivals);
    ScriptContext_SetupScript(EventScript_ArenaPracticeBattle);
}

static void CB2_ArenaInit(void);
static void CB2_Arena(void);
static void ArenaExit(bool8 fainted);
static void CaptureHud(void);
static void AiChooseGoal(void);

static s32 Abs(s32 n) { return n < 0 ? -n : n; }
static s32 Clamp(s32 n, s32 lo, s32 hi) { return n < lo ? lo : n > hi ? hi : n; }
static u16 MonVisualScale(u8 side)
{
    (void)side;
    return 256;
}
static void MonVisualScaleApply(void)
{
    u32 side;
    for(side=0;side<2;side++)if(sArena.bodies[side].art&&MonVisualScale(side)!=256)
    {
        struct Sprite *p=&gSprites[sArena.bodies[side].sprite];
        struct OamMatrix *m=&gOamMatrices[p->oam.matrixNum];
        u16 scale=MonVisualScale(side);
        p->oam.affineMode=ST_OAM_AFFINE_DOUBLE;
        CalcCenterToCornerVec(p,p->oam.shape,p->oam.size,p->oam.affineMode);
        SetOamMatrix(p->oam.matrixNum,m->a*256/scale,m->b*256/scale,m->c*256/scale,m->d*256/scale);
    }
}

// Ghost types phase through the arena walls and through the objects inside
// it: leaving through one edge brings them in from the opposite one, and rocks,
// logs, bushes, crystals and pods never block their movement. Only a battler
// with the Ghost type; attacks and projectiles still interact with cover.
#define ARENA_PHASE_MARGIN 14
EWRAM_DATA struct ArenaGhostTelemetry gArenaGhostTelemetry = {};

static bool8 GhostBody(u8 side) { return IS_BATTLER_OF_TYPE(side, TYPE_GHOST); }

// Inside a wall or overlapping a solid object: drawn translucent.
static bool8 Phasing(u8 side)
{
    const struct ArenaBody *body = &sArena.bodies[side];
    return GhostBody(side) && (body->x < ARENA_MIN_X * Q || body->x > ARENA_MAX_X * Q
        || body->y < ARENA_MIN_Y * Q || body->y > ARENA_MAX_Y * Q
        || !ArenaNav_LineClear(body->x / Q, body->y / Q, body->x / Q, body->y / Q, ARENA_BODY_RADIUS));
}

// Once a ghost has pushed a body length into a wall, it reappears inside the
// opposite wall, still moving the same way. One glow marks each side.
static void PhaseWrap(u8 side, s32 dx, s32 dy)
{
    struct ArenaBody *body = &sArena.bodies[side];
    s16 exitX = body->x / Q, exitY = body->y / Q;
    bool8 crossed = FALSE;
    if (dx < 0 && body->x <= (ARENA_MIN_X - ARENA_PHASE_MARGIN) * Q)
    { body->x = (ARENA_MAX_X + ARENA_PHASE_MARGIN) * Q; crossed = TRUE; }
    else if (dx > 0 && body->x >= (ARENA_MAX_X + ARENA_PHASE_MARGIN) * Q)
    { body->x = (ARENA_MIN_X - ARENA_PHASE_MARGIN) * Q; crossed = TRUE; }
    if (dy < 0 && body->y <= (ARENA_MIN_Y - ARENA_PHASE_MARGIN) * Q)
    { body->y = (ARENA_MAX_Y + ARENA_PHASE_MARGIN) * Q; crossed = TRUE; }
    else if (dy > 0 && body->y >= (ARENA_MAX_Y + ARENA_PHASE_MARGIN) * Q)
    { body->y = (ARENA_MIN_Y - ARENA_PHASE_MARGIN) * Q; crossed = TRUE; }
    if (!crossed) return;
    gArenaGhostTelemetry.crossings[side]++;
    ArenaFeedback_CaptureGlow(exitX, exitY, TRUE);
    ArenaFeedback_CaptureGlow(body->x / Q, body->y / Q, FALSE);
    PlaySE(SE_M_TELEPORT);
}

// Real-time Rollout. The wind-up tumbles the body in place, slowly at first,
// then it rolls along its facing, gaining speed; power doubles for every
// ROLLOUT_STAGE_PX rolled (up to x8) and once more after Defense Curl. Cover
// in the way is hit with the roll's strength and rolled through if it breaks.
#define ROLLOUT_STAGE_PX 40
#define ROLLOUT_MAX_STAGE 3
#define ROLLOUT_SPIN 0x1000
// Frames a rolled-over rival stays flattened; it springs back over the last few.
#define ROLLOUT_FLAT_FRAMES 18
#define ROLLOUT_FLAT_SPRING 10
// Camera elevation used to project the roll (35 degrees): sin and cos in Q8.
#define ROLLOUT_CAM_SIN 147
#define ROLLOUT_CAM_COS 210
EWRAM_DATA u8 gArenaRolloutStage = 0;

static bool8 Rolling(u8 side)
{
    const struct ArenaBody *body = &sArena.bodies[side];
    return body->shotTimer && gBattleMons[side].moves[body->shotSlot] == MOVE_ROLLOUT;
}

// A third of full speed at launch, full speed half a second in.
static s32 RolloutSpeed(const struct ArenaBody *body, const struct ArenaMoveProfile *p)
{
    return p->speed * (85 + 171 * min(30, body->actionAge) / 30) / 256;
}

static void RolloutEnd(struct ArenaBody *body)
{
    body->shotTimer = 0;
    body->actionLife = 1;
}

#include "arena_rollout.inc"
#include "arena_double_team.inc"
#include "arena_cover.inc"
#include "arena_barrier.inc"

static bool8 SupportedMove(u16 move)
{
    return ArenaMoves_Get(move) != NULL;
}

static u8 FirstMove(u8 side, bool8 needsPP)
{
    u32 i;
    // Start on an attack when possible. Adding Growl/Screech must not change
    // a previously playable lead into an apparently harmless default action.
    for (i = 0; i < MAX_MON_MOVES; i++)
        if (SupportedMove(gBattleMons[side].moves[i]) && gBattleMoves[gBattleMons[side].moves[i]].power
            && (!needsPP || gBattleMons[side].pp[i])) return i;
    // A moveset with only adapted buffs/debuffs is not an arena encounter.
    // For example, do not turn Seedot's unadapted Bide into a harmless AI.
    return MAX_MON_MOVES;
}

static bool8 SupportedBattler(u8 side)
{
    u32 i;
    // Only tested native damage, stat changes and burns enter the arena.
    // Other status/turn/item mechanics retain the complete classic battle.
    if ((gBattleMons[side].item && GetItemHoldEffect(gBattleMons[side].item) != HOLD_EFFECT_RESTORE_HP)
        || (gBattleMons[side].status1 & ~STATUS1_BURN)
        || (gBattleMons[side].status2 & ~(STATUS2_FOCUS_ENERGY|STATUS2_DEFENSE_CURL))
        || gStatuses3[side] || gBattleMons[side].hp == 0 || FirstMove(side, TRUE) == MAX_MON_MOVES)
        return FALSE;
    switch (gBattleMons[side].ability)
    {
    case ABILITY_NONE:
    case ABILITY_OVERGROW:
    case ABILITY_BLAZE:
    case ABILITY_TORRENT:
    case ABILITY_SWARM:
    case ABILITY_RUN_AWAY:
    case ABILITY_PICKUP:
    case ABILITY_KEEN_EYE:
    case ABILITY_SHIELD_DUST:
    case ABILITY_COMPOUND_EYES:
    case ABILITY_HUSTLE:
    case ABILITY_HUGE_POWER:
    case ABILITY_PURE_POWER:
    case ABILITY_BATTLE_ARMOR:
    case ABILITY_SHELL_ARMOR:
    case ABILITY_LEVITATE:
    case ABILITY_WONDER_GUARD:
    case ABILITY_INNER_FOCUS:
    case ABILITY_INTIMIDATE:
    case ABILITY_CHLOROPHYLL: // No weather is admitted by Eligible().
    case ABILITY_INSOMNIA:   // No sleep/status actions are admitted.
    case ABILITY_GUTS:       // Native damage handles its burn interaction.
    case ABILITY_LIGHTNING_ROD: // Gen III redirection only, doubles excluded.
    case ABILITY_COLOR_CHANGE:
    case ABILITY_WATER_VEIL: // Original SetMoveEffect prevents burns.
    case ABILITY_SWIFT_SWIM: // Weather encounters remain classic.
    case ABILITY_RAIN_DISH:
    case ABILITY_DAMP: // Explosion/Selfdestruct remain classic moves.
    case ABILITY_STURDY: // Gen III OHKO prevention; OHKO moves remain classic.
    case ABILITY_ROCK_HEAD: // Recoil moves remain classic.
    case ABILITY_PLUS: // Partner-only effects; doubles remain classic.
    case ABILITY_MINUS:
    case ABILITY_THICK_FAT: // Native CalculateBaseDamage handles Fire/Ice.
    case ABILITY_EARLY_BIRD: // Sleep actions/statuses remain classic.
    case ABILITY_ILLUMINATE: // Encounter-rate ability; no in-battle effect.
    case ABILITY_HYPER_CUTTER: // ChangeStatBuffs keeps its Attack protection.
    case ABILITY_PRESSURE: // Fire charges an extra PP for opposing targeted actions.
    case ABILITY_AIR_LOCK: // Weather encounters are still excluded by Eligible().
    case ABILITY_SAND_VEIL:
    case ABILITY_CLOUD_NINE:
    case ABILITY_CLEAR_BODY: // Native ChangeStatBuffs retains rejection.
    case ABILITY_WHITE_SMOKE:
    case ABILITY_LIMBER: // Paralysis moves still unsupported.
    case ABILITY_OBLIVIOUS: // Infatuation moves still unsupported.
    case ABILITY_OWN_TEMPO: // Confusion moves still unsupported.
    case ABILITY_VITAL_SPIRIT: // Sleep moves still unsupported.
    case ABILITY_MAGMA_ARMOR: // Freeze moves still unsupported.
    case ABILITY_FLASH_FIRE:
    case ABILITY_WATER_ABSORB:
        return TRUE;
    case ABILITY_STATIC:
    case ABILITY_EFFECT_SPORE:
    case ABILITY_POISON_POINT:
    case ABILITY_CUTE_CHARM:
    case ABILITY_FLAME_BODY:
        // Do not silently suppress a contact ability. These can enter only if
        // the opposing arena moves cannot trigger it; otherwise the complete
        // encounter stays in native classic mode, including its status rolls.
        for (i = 0; i < MAX_MON_MOVES; i++)
            if (SupportedMove(gBattleMons[side ^ 1].moves[i])
                && gBattleMons[side ^ 1].pp[i]
                && (gBattleMoves[gBattleMons[side ^ 1].moves[i]].flags & FLAG_MAKES_CONTACT))
                return FALSE;
        return TRUE;
    }
    return FALSE;
}

void RealtimeArena_ResetBattle(void)
{
    ElementsResetBattle();
    sEnvironmentInitialized=FALSE;
    memset(&sArena, 0, sizeof(sArena));
    gRealtimeArenaTelemetry.active = FALSE;
    gRealtimeArenaRestoringFaint = FALSE;
    gRealtimeArenaQuietResult = FALSE;
    memset(&gArenaResultTelemetry,0,sizeof(gArenaResultTelemetry));
    memset(gArenaAdventureTelemetry,0,sizeof(gArenaAdventureTelemetry));
    memset(gArenaStatusTelemetry,0,sizeof(gArenaStatusTelemetry));
    gArenaIntroTelemetry.started=gMain.vblankCounter1;
    gArenaIntroTelemetry.elapsed=gArenaIntroTelemetry.skipped=0;
}

static bool8 Eligible(void)
{
#if ARENA_LAB
    if (gArenaLabMailbox.classic) return FALSE;
#endif
    // FIRST_BATTLE is still a 1v1 with a real starter and native return script.
    if (sArena.classic || gBattlersCount != 2 || (gBattleTypeFlags & ~(BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_IS_MASTER | BATTLE_TYPE_TRAINER))
        || gAbsentBattlerFlags
        || !SupportedBattler(0) || !SupportedBattler(1))
        return FALSE;
    return TRUE;
}

bool8 RealtimeArena_CanSkipIntro(void)
{
    u8 weather=GetCurrentWeather();
    // Active switch-in abilities/weather may execute native animation scripts.
    // Keep their complete intro until that specific path is supported and tested.
    return Eligible()&&gBattleMons[0].ability!=ABILITY_INTIMIDATE
        &&gBattleMons[1].ability!=ABILITY_INTIMIDATE
        &&(weather==WEATHER_NONE||weather==WEATHER_SUNNY);
}

bool8 RealtimeArena_TryStart(void)
{
    if(!Eligible())return FALSE;
    // Wait inside selection rather than issuing a classic ChooseAction command
    // while the intro/fade is still completing. That menu would own the controller.
    if (gBattleControllerExecFlags || gPaletteFade.active) return TRUE;
    sArena.savedCB1 = gMain.callback1;
    gMain.callback1 = NULL;
    SetMainCallback2(CB2_ArenaInit);
    return TRUE;
}

static void VBlank_Arena(void)
{
    LoadOam();
    ArenaRender_Flush();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

#define H(n) ((((n)&192)==64?15:0)|(((n)&48)==16?240:0)|(((n)&12)==4?3840:0)|(((n)&3)==1?61440:0))
static const u16 sHudRowMask[256]={
    H(0),H(1),H(2),H(3),H(4),H(5),H(6),H(7),H(8),H(9),H(10),H(11),H(12),H(13),H(14),H(15),
    H(16),H(17),H(18),H(19),H(20),H(21),H(22),H(23),H(24),H(25),H(26),H(27),H(28),H(29),H(30),H(31),
    H(32),H(33),H(34),H(35),H(36),H(37),H(38),H(39),H(40),H(41),H(42),H(43),H(44),H(45),H(46),H(47),
    H(48),H(49),H(50),H(51),H(52),H(53),H(54),H(55),H(56),H(57),H(58),H(59),H(60),H(61),H(62),H(63),
    H(64),H(65),H(66),H(67),H(68),H(69),H(70),H(71),H(72),H(73),H(74),H(75),H(76),H(77),H(78),H(79),
    H(80),H(81),H(82),H(83),H(84),H(85),H(86),H(87),H(88),H(89),H(90),H(91),H(92),H(93),H(94),H(95),
    H(96),H(97),H(98),H(99),H(100),H(101),H(102),H(103),H(104),H(105),H(106),H(107),H(108),H(109),H(110),H(111),
    H(112),H(113),H(114),H(115),H(116),H(117),H(118),H(119),H(120),H(121),H(122),H(123),H(124),H(125),H(126),H(127),
    H(128),H(129),H(130),H(131),H(132),H(133),H(134),H(135),H(136),H(137),H(138),H(139),H(140),H(141),H(142),H(143),
    H(144),H(145),H(146),H(147),H(148),H(149),H(150),H(151),H(152),H(153),H(154),H(155),H(156),H(157),H(158),H(159),
    H(160),H(161),H(162),H(163),H(164),H(165),H(166),H(167),H(168),H(169),H(170),H(171),H(172),H(173),H(174),H(175),
    H(176),H(177),H(178),H(179),H(180),H(181),H(182),H(183),H(184),H(185),H(186),H(187),H(188),H(189),H(190),H(191),
    H(192),H(193),H(194),H(195),H(196),H(197),H(198),H(199),H(200),H(201),H(202),H(203),H(204),H(205),H(206),H(207),
    H(208),H(209),H(210),H(211),H(212),H(213),H(214),H(215),H(216),H(217),H(218),H(219),H(220),H(221),H(222),H(223),
    H(224),H(225),H(226),H(227),H(228),H(229),H(230),H(231),H(232),H(233),H(234),H(235),H(236),H(237),H(238),H(239),
    H(240),H(241),H(242),H(243),H(244),H(245),H(246),H(247),H(248),H(249),H(250),H(251),H(252),H(253),H(254),H(255)
};
#undef H

static void PrintWindow(u8 window, u32 x, u32 y, const u8 *text, u8 color)
{
    // Same original Emerald glyphs, tightly bounded HUD blit. Invoking the full
    // dialogue state machine per HP/PP change stalled several hardware frames.
    // Foreground only: this strip is cleared once, shadow/background are both 9.
    u32 n=0,tiles=sArenaWindows[window].width,height=sArenaWindows[window].height*8;
    while(*text!=EOS&&n++<32)
    {
        u32 glyph=*text++,width=gFontSmallLatinGlyphWidths[glyph],row;
        const u16 *src=gFontSmallLatinGlyphs+glyph*32;
        if(x+width>tiles*8)break;
        for(row=0;row<13&&y+row<height;row++)
        {
            u32 bits=src[row<8?row:row+8],py=y+row,shift=(x&7)*4;
            u32 mask=sHudRowMask[bits>>8]|(sHudRowMask[bits&255]<<16);
            u32 ink,colorWord=0x11111111*color;
            u32 *dest=(u32*)gWindows[window].tileData+((py/8)*tiles+x/8)*8+(py&7);
            if(width<8)mask&=0xffffffff>>((8-width)*4);
            ink=mask&colorWord;
            *dest=(*dest&~(mask<<shift))|(ink<<shift);
            if(shift&&x/8<tiles-1)dest[8]=(dest[8]&~(mask>>(32-shift)))|(ink>>(32-shift));
        }
        x+=width;
    }
}

static void Print(u32 x,u32 y,const u8 *text,u8 color)
{
    PrintWindow(0,x,y,text,color);
}

static void WindowRect(u8 window,u32 x,u32 top,u32 width,u32 height,u32 color)
{
    u32 y;
    for(y=top;y<top+height;y++)
    {
        u32 at=x,left=width;
        while(left)
        {
            u32 count=min(left,8-(at&7)),shift=(at&7)*4;
            u32 mask=(0xffffffff>>((8-count)*4))<<shift;
            u32 *dest=(u32*)gWindows[window].tileData+((y/8)*sArenaWindows[window].width+at/8)*8+(y&7);
            *dest=(*dest&~mask)|(0x11111111*color&mask);
            left-=count;at+=count;
        }
    }
}

static void HudRect(u32 x,u32 top,u32 width,u32 height,u32 color)
{
    WindowRect(0,x,top,width,height,color);
}

static void PrintPopup(u32 x, u32 y, const u8 *text, u8 color)
{
    PrintWindow(1,x,y,text,color);
}

static void DrawStage(void)
{
    // Art is on BG1. Clearing an overlay never redraws terrain or erases rocks.
    // Keep the cached HUD pixels too: pause must not erase unchanged HP/names.
    ClearWindowTilemap(1);
    sMoveMenuReady=FALSE;
    CopyWindowToVram(1, COPYWIN_MAP);
}

static u8 RecoveryPips(void)
{
    const struct ArenaBody *body=&sArena.bodies[0];
    const struct ArenaMoveProfile *p=ArenaMoves_Get(gBattleMons[0].moves[body->shotSlot]);
    if(body->shotTimer || body->actionLife)return 0;
    if(!body->cooldown)return 8;
    return 8-min(8,body->cooldown*8/max(1,p?p->recovery:30));
}

#include "arena_hud.inc"

static void DrawHud(void)
{
    u32 side;
    u8 text[64];
    u8 *end;
    DrawStatusHud();
    if (sArena.paused)
    {
        if(!sMoveMenuReady)
        {
        FillWindowPixelBuffer(1, PIXEL_FILL(9));
        PrintPopup(5, 3, sTextPaused, 7);
        for(side=0;side<MAX_MON_MOVES;side++)
        {
            u16 move=gBattleMons[0].moves[side];
            const struct ArenaMoveProfile *p=ArenaMoves_Get(move);
            u8 pp=gBattleMons[0].pp[side];
            u8 color=!p?8:!pp?6:4;
            u8 y=17+side*12;
            PrintPopup(5,y,sMoveDirections[side],color);
            PrintPopup(43,y,gMoveNames[move],color);
            PrintPopup(132,y,sMoveKinds[p?p->kind:0],color);
            end=StringCopy(text,sTextPP);
            ConvertIntToDecimalStringN(end,pp,STR_CONV_MODE_LEFT_ALIGN,2);
            PrintPopup(180,y,text,color);
        }
        sMoveMenuReady=TRUE;
        }
        for(side=0;side<MAX_MON_MOVES;side++)
            WindowRect(1,0,19+side*12,3,8,side==sArena.bodies[0].moveSlot?5:9);
        PutWindowTilemap(1);
        CopyWindowToVram(1, COPYWIN_FULL);
    }
    CopyWindowToVram(0, COPYWIN_GFX);
    CaptureHud();
    sArena.hudDirty = FALSE;
}

static void LoadAimMarker(void)
{
    u32 tiles[128] = {0};
    struct SpriteSheet sheet = {tiles, sizeof(tiles), SHOT_TAG + 3};
    u32 x, y;
    // Code-native pixel art: four small corners around the target. No opaque
    // center, so the actor's animation stays completely visible.
    for (y = 0; y < 32; y++)
        for (x = 0; x < 32; x++)
            if (((y == 4 || y == 27) && ((x >= 4 && x <= 10) || (x >= 21 && x <= 27)))
                || ((x == 4 || x == 27) && ((y >= 4 && y <= 10) || (y >= 21 && y <= 27))))
            {
                u32 tile = (y / 8) * 4 + x / 8;
                tiles[tile * 8 + y % 8] |= 1 << ((x % 8) * 4);
            }
    LoadSpriteSheet(&sheet);
}

#define TRAINER_TAG 0xA7A0
static void RefreshTrainerFacing(struct Sprite *sprite)
{
    sprite->oam.matrixNum=(sprite->oam.matrixNum&7)|(sprite->hFlip?8:0);
}
// Read-only test evidence: sprite ids, shadow ids, palette slots, graphics ids.
EWRAM_DATA u8 gArenaTrainerTelemetry[8] = {};
static const struct OamData sTrainerOam =
{
    .shape = SPRITE_SHAPE(16x32), .size = SPRITE_SIZE(16x32), .priority = 1
};

static u8 OpponentGraphics(void)
{
    u8 pic = gTrainers[gTrainerBattleOpponent_A].trainerPic;
    if (pic >= TRAINER_PIC_ELITE_FOUR_SIDNEY && pic <= TRAINER_PIC_LEADER_WINONA)
        return OBJ_EVENT_GFX_SIDNEY + pic - TRAINER_PIC_ELITE_FOUR_SIDNEY;
    switch (pic)
    {
    case TRAINER_PIC_HIKER: return OBJ_EVENT_GFX_HIKER;
    case TRAINER_PIC_BUG_CATCHER: return OBJ_EVENT_GFX_BUG_CATCHER;
    case TRAINER_PIC_SWIMMER_M: return OBJ_EVENT_GFX_SWIMMER_M;
    case TRAINER_PIC_SWIMMER_F: return OBJ_EVENT_GFX_SWIMMER_F;
    case TRAINER_PIC_LEADER_JUAN: return OBJ_EVENT_GFX_JUAN;
    case TRAINER_PIC_CHAMPION_WALLACE: return OBJ_EVENT_GFX_WALLACE;
    case TRAINER_PIC_STEVEN: return OBJ_EVENT_GFX_STEVEN;
    case TRAINER_PIC_MAY: return OBJ_EVENT_GFX_MAY_NORMAL;
    case TRAINER_PIC_BRENDAN: return OBJ_EVENT_GFX_BRENDAN_NORMAL;
    default: return OBJ_EVENT_GFX_BOY_1;
    }
}

static void CreateSidelineTrainers(void)
{
    u32 i;
    memset(gArenaTrainerTelemetry, MAX_SPRITES, sizeof(gArenaTrainerTelemetry));
    for (i = 0; i < ((gBattleTypeFlags & BATTLE_TYPE_TRAINER)?2:1); i++)
    {
        bool8 female = (gSaveBlock2Ptr->playerGender != 0) ^ (i != 0);
        u8 graphicsId = i && (gBattleTypeFlags & BATTLE_TYPE_TRAINER) ? OpponentGraphics()
            : female ? OBJ_EVENT_GFX_MAY_NORMAL : OBJ_EVENT_GFX_BRENDAN_NORMAL;
        const struct ObjectEventGraphicsInfo *info = GetObjectEventGraphicsInfo(graphicsId);
        struct SpriteSheet sheet = {info->images[2].data, 256, TRAINER_TAG + i};
        struct SpritePalette pal = {ArenaObjectPalette(info->paletteTag), TRAINER_TAG + i};
        struct SpriteTemplate template =
        {
            .tileTag = TRAINER_TAG + i, .paletteTag = TRAINER_TAG + i,
            .oam = &sTrainerOam, .anims = gDummySpriteAnimTable,
            .images = NULL, .affineAnims = gDummySpriteAffineAnimTable,
            .callback = SpriteCallbackDummy
        };
        u8 id, shadow, palette;
        s16 x = i ? 232 : 8;
        s16 y = i ? 50 : 118;
        // Wild opponents remain wild. The opposite character is a spectator,
        // not a trainer battle. Keep both on the grass edge, behind combatants.
        if (LoadSpriteSheet(&sheet) == 0xFFFF) continue;
        palette = LoadSpritePalette(&pal);
        if (palette == 0xFF) { FreeSpriteTilesByTag(TRAINER_TAG + i); continue; }
        id = CreateSprite(&template, x, y, 12);
        if (id == MAX_SPRITES)
        {
            FreeSpriteTilesByTag(TRAINER_TAG + i);
            FreeSpritePaletteByTag(TRAINER_TAG + i);
            continue;
        }
        // The source side pose faces west; left trainer must face east.
        gSprites[id].hFlip = !i;
        RefreshTrainerFacing(&gSprites[id]);
        shadow = CreateSprite(&sShadowTemplate, x, y + 13, 13);
        gArenaTrainerTelemetry[i] = id;
        gArenaTrainerTelemetry[2+i] = shadow;
        gArenaTrainerTelemetry[4+i] = palette;
        gArenaTrainerTelemetry[6+i] = graphicsId;
    }
}

#include "arena_capture.inc"
#include "arena_biomes.inc"
#include "arena_toss.inc"
#include "arena_sendout.inc"

static void CB2_ArenaInit(void)
{
    u32 i;
    gRealtimeArenaQuietResult = FALSE;
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    ScanlineEffect_Stop();
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_FORCED_BLANK);
    FreeAllWindowBuffers();
    ResetTasks();
    ResetSpriteData();
    DecoyReset();
    CoverReset();
    BarrierReset();
    SpecialReset();
    ArenaRender_Reset();
    FreeAllSpritePalettes();
    gReservedSpritePaletteCount = 2;
    ResetPaletteFade();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sArenaBgs, ARRAY_COUNT(sArenaBgs));
    InitWindows(sArenaWindows);
    sHudCached=FALSE;
    DeactivateAllTextPrinters();
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_OBJ);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(12,4));
    SetGpuReg(REG_OFFSET_BLDY, 0);
    LoadArenaBiome();
    LoadBgTiles(2, sSpaceTiles, sizeof(sSpaceTiles), 192);
    LoadBgTilemap(2, sSpaceMap, sizeof(sSpaceMap), 0);
    LoadPalette(sArenaPalette, 0, sizeof(sArenaPalette));
    DrawStage();
    ArenaNav_Init();
    if(!sEnvironmentInitialized)
    {
        ArenaPhysics_Init();
        sEnvironmentInitialized=TRUE;
    }
    else
    {
        // A party change must not respawn broken cover or erase wet ground.
        for(i=0;i<ARENA_OBSTACLES;i++)
        {
            gArenaProps[i].reserved=0;
            ArenaNav_SetObstacle(i,!gArenaProps[i].broken);
        }
    }
    LoadSpriteSheet(&sShotSheet);
    LoadSpritePalette(&sShotPalettes[0]);
    LoadSpritePalette(&sShotPalettes[1]);
    LoadSpriteSheet(&sShadowSheet);
    LoadAimMarker();
    ArenaFeedback_Init();
    ArenaMoveFx_Init();
    ArenaTerrain_Init();
    ElementsInit();
    memset(sFlameReach,0,sizeof(sFlameReach));
    memset(sFlameElementMask,0,sizeof(sFlameElementMask));
    memset(gArenaFlameTelemetry,0,sizeof(gArenaFlameTelemetry));
    PsychicReset();
    ArenaPsychicFx_Init(BattleHasPsychic());
    memset(sArena.bodies, 0, sizeof(sArena.bodies));
    memset(sArena.shots, 0, sizeof(sArena.shots));
    memset(&gArenaSpriteTelemetry, 0, sizeof(gArenaSpriteTelemetry));
    memset(&gArenaCombatTelemetry, 0, sizeof(gArenaCombatTelemetry));
    memset(&gArenaMoveTelemetry, 0, sizeof(gArenaMoveTelemetry));
    memset(&gArenaFrameTelemetry,0,sizeof(gArenaFrameTelemetry));
    memset(&gArenaGhostTelemetry,0,sizeof(gArenaGhostTelemetry));
    memset(gArenaRenderTelemetry,0,sizeof(gArenaRenderTelemetry));
    gArenaFrameTelemetry.lastVBlank=gMain.vblankCounter1;
    sArena.lastBlast=0;
    RollBallInit();
    for (i = 0; i < 2; i++)
    {
        struct ArenaBody *body = &sArena.bodies[i];
        struct SpriteTemplate template = sMonTemplate;
        struct SpriteSheet sheet;
        u32 front;
        body->art = ArenaSprites_Get(gBattleMons[i].species);
        gArenaSpriteTelemetry.pmd[i] = body->art != NULL;
        if (body->art)
        {
            ArenaSprites_Decode(&body->art->animations[ARENA_ANIM_IDLE], 0, 0, sMonFrameTiles[i]);
            sheet.data = sMonFrameTiles[i];
            sheet.size = 2048; sheet.tag = MON_TAG + i * 2;
            LoadSpriteSheet(&sheet);
            LoadPalette(body->art->palette, OBJ_PLTT_ID(i), PLTT_SIZE_4BPP);
            template.tileTag = MON_TAG + i * 2;
        }
        else
        {
            // Other species retain the native fallback until their art is imported.
            for (front = 0; front < 2; front++)
            {
                LoadSpecialPokePic(front ? &gMonFrontPicTable[gBattleMons[i].species] : &gMonBackPicTable[gBattleMons[i].species],
                    gDecompressionBuffer, gBattleMons[i].species, gBattleMons[i].personality, front);
                sheet.data = gDecompressionBuffer; sheet.size = 2048; sheet.tag = MON_TAG + i * 2 + front;
                LoadSpriteSheet(&sheet);
            }
            LoadCompressedPalette(GetMonSpritePalFromSpeciesAndPersonality(gBattleMons[i].species,
                gBattleMons[i].otId, gBattleMons[i].personality), OBJ_PLTT_ID(i), PLTT_SIZE_4BPP);
            template.tileTag = MON_TAG + i * 2 + (i ? 1 : 0);
        }
        body->x = 120 * Q;
        body->y = (i ? ARENA_MIN_Y : ARENA_MAX_Y) * Q;
        body->facing = i ? FACE_DOWN : FACE_UP;
        body->moveSlot = FirstMove(i, TRUE);
        body->cooldown = i ? 30 : 15;
        body->sprite = CreateSprite(&template, body->x / Q, body->y / Q, 0);
        body->shadow = CreateSprite(&sShadowTemplate, body->x / Q, body->y / Q + 8, 10);
        gSprites[body->sprite].oam.paletteNum = i;
        gSprites[body->sprite].affineAnimPaused = TRUE;
        gSprites[body->sprite].affineAnimBeginning = FALSE;
        SetOamMatrix(gSprites[body->sprite].oam.matrixNum, body->art ? 0x100 : 0x200, 0, 0, body->art ? 0x100 : 0x200);
        body->drawnFrame = body->drawnDirection = 255;
    }
    sArena.cueSprite = CreateSprite(&sShotTemplate, 120, 32, 0);
    gSprites[sArena.cueSprite].oam.paletteNum = IndexOfSpritePaletteTag(SHOT_TAG + 1);
    gSprites[sArena.cueSprite].invisible = TRUE;
    sArena.aimSprite = CreateSprite(&sAimTemplate, 120, 32, 1);
    gSprites[sArena.aimSprite].invisible = TRUE;
    CreateSidelineTrainers();
    CaptureInit();
    sArena.aiRandom = gBattleMons[1].personality ^ 0xA12E7A11;
    sArena.style = gBattleMons[1].personality % 3;
    sArena.reaction = Clamp(21 - gBattleMons[1].level / 2, 8, 21);
    sArena.aimError = Clamp(12 - gBattleMons[1].level / 4, 3, 12);
    sArena.thinkTimer = 1; sArena.aimTimer = 0; sArena.goalTimer = 0;
    sArena.aiState = AI_REPOSITION;
    sArena.observed.x = sArena.previous.x = 120;
    sArena.observed.y = sArena.previous.y = ARENA_MAX_Y;
    sArena.goal.x = sArena.waypoint.x = 120;
    sArena.goal.y = sArena.waypoint.y = ARENA_MIN_Y;
    memset(&gArenaAiTelemetry, 0, sizeof(gArenaAiTelemetry));
    gArenaAiTelemetry.level = gBattleMons[1].level;
    gArenaAiTelemetry.reaction = sArena.reaction;
    gArenaAiTelemetry.aimError = sArena.aimError;
    gArenaAiTelemetry.style = sArena.style;
    // Prime the initial route while the screen is forced blank. The first
    // playable frame should not pay for graph search plus sprite initialization.
    AiChooseGoal();sArena.thinkTimer=sArena.reaction;
    gArenaAiTelemetry.decisions++;
    sArena.frame = 0;
    sArena.active = TRUE;
    gRealtimeArenaQuietResult = FALSE;
    gArenaIntroTelemetry.elapsed=gMain.vblankCounter1-gArenaIntroTelemetry.started;
    sArena.paused = FALSE;
    sArena.resultTimer = 0;
    sArena.hitstop = sArena.attackBuffer = sArena.dashBuffer = 0;
    gRealtimeArenaTelemetry.active = TRUE;
    gRealtimeArenaTelemetry.paused = FALSE;
    gRealtimeArenaTelemetry.entries++;
    gRealtimeArenaTelemetry.hpBefore = gBattleMons[0].hp;
    gRealtimeArenaTelemetry.expBefore = GetMonData(&gPlayerParty[gBattlerPartyIndexes[0]], MON_DATA_EXP);
    DrawHud();
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
    SendoutInit();
    MonVisualScaleApply();
    AnimateSprites();BuildOamBuffer();
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    ShowBg(0);
    ShowBg(1);
    SetVBlankCallback(VBlank_Arena);
    SetMainCallback2(CB2_Arena);
}

static u16 Speed(u8 side)
{
    // Q8 pixels per hardware frame; no render-rate-dependent clock.
    // Compress the high end on a 240px arena, preserving native Speed ordering.
    // 1.0..1.94 pixels/frame instead of reaching 3.23 at high levels.
    u32 speed=gBattleMons[side].speed;
    u8 stage=gBattleMons[side].statStages[STAT_SPEED];
    speed=speed*gStatStageRatios[stage][0]/gStatStageRatios[stage][1];
    return 256 + Clamp(speed, 1, 120) * 2;
}

static u8 Facing(s32 dx, s32 dy)
{
    if (Abs(dx) * 2 < Abs(dy)) return dy < 0 ? FACE_UP : FACE_DOWN;
    if (Abs(dy) * 2 < Abs(dx)) return dx < 0 ? FACE_LEFT : FACE_RIGHT;
    return dy < 0 ? (dx < 0 ? 5 : 3) : (dx < 0 ? 7 : 1);
}

static void MoveDelta(u8 side, s32 dx, s32 dy)
{
    struct ArenaBody *body = &sArena.bodies[side];
    struct ArenaBody *other = &sArena.bodies[side ^ 1];
    bool8 ghost = GhostBody(side);
    // A rolling body runs over the other one instead of being held off it, and
    // a body left overlapping (a roll that ended on top of it) may step away.
    bool8 over = Rolling(side) && body->actionLife;
    // A ghost may step into the wall band and through cover; see PhaseWrap.
    s32 margin = ghost ? ARENA_PHASE_MARGIN * Q : 0;
    s32 x, y;
    if(ElementMotion(side,dx,dy))return;
    body->moving = dx || dy;
    if (dx || dy) body->facing = Facing(dx, dy);
    x = Clamp(body->x + dx, ARENA_MIN_X * Q - margin, ARENA_MAX_X * Q + margin);
    y = Clamp(body->y + dy, ARENA_MIN_Y * Q - margin, ARENA_MAX_Y * Q + margin);
    // Axis-separated sliding. Even the fastest dash step is smaller than any
    // solid obstacle; the swept test also prevents corner cutting.
    if ((ghost || ArenaNav_LineClear(body->x / Q, body->y / Q, x / Q, body->y / Q, ARENA_BODY_RADIUS))
        && CoverCanStep(x/Q,body->y/Q)
        && (over || Abs(x - other->x) >= 22 * Q || Abs(body->y - other->y) >= 22 * Q
            || Abs(x - other->x) > Abs(body->x - other->x))) body->x = x;
    else if (dx) gArenaAiTelemetry.wallBlocks[side]++;
    if ((ghost || ArenaNav_LineClear(body->x / Q, body->y / Q, body->x / Q, y / Q, ARENA_BODY_RADIUS))
        && CoverCanStep(body->x/Q,y/Q)
        && (over || Abs(body->x - other->x) >= 22 * Q || Abs(y - other->y) >= 22 * Q
            || Abs(y - other->y) > Abs(body->y - other->y))) body->y = y;
    else if (dy) gArenaAiTelemetry.wallBlocks[side]++;
    if (ghost) PhaseWrap(side, dx, dy);
}

static void Move(u8 side, s32 dx, s32 dy, s32 speed)
{
    if (dx && dy) speed = speed * 181 / 256;
    MoveDelta(side, dx * speed, dy * speed);
}

static void Fire(u8 side, u8 slot, s32 targetX, s32 targetY)
{
    struct ArenaBody *body = &sArena.bodies[side];
    const struct ArenaMoveProfile *profile;
    u32 i;
    s32 dx, dy, len;
    u8 pp;
    // The selected move is captured at BeginShot. Changing L/R during its
    // animation changes the NEXT attack, never this attack's cost or power.
    if (slot >= MAX_MON_MOVES || !gBattleMons[side].pp[slot]) return;
    profile = ArenaMoves_Get(gBattleMons[side].moves[slot]);
    if (!profile) return;
    dx = body->attackX; dy = body->attackY;
    len = max(Abs(dx), Abs(dy)) + min(Abs(dx), Abs(dy)) / 2;
    if (!len) { dx = Q; len = Q; }
    body->attackX = dx * Q / len; body->attackY = dy * Q / len;
    if(ArenaMoves_Beam(profile->move))
    {
        static const s16 dirs[8][2]={{0,256},{181,181},{256,0},{181,-181},{0,-256},{-181,-181},{-256,0},{-181,181}};
        body->attackX=dirs[body->shotFacing][0];body->attackY=dirs[body->shotFacing][1];
        sFlameElementMask[side]=sFlameReach[side]=0;
        gArenaFlameTelemetry[0]++;
    }
    if (profile->kind == ARENA_MOVE_PROJECTILE
        && !(profile->move==MOVE_PSYCHIC
             && PsychicStart(side,targetX,targetY)))
    {
        for (i = 0; i < SHOTS_COUNT; i++) if (!sArena.shots[i].life) break;
        if (i == SHOTS_COUNT) return;
        if(!ArenaTerrain_ReserveProjectile())return;
        sArena.shots[i].sprite = ArenaMoveFx_CreateBolt(profile,body->x/Q,body->y/Q,body->shotFacing,i);
        if (sArena.shots[i].sprite == MAX_SPRITES) return;
        sArena.shots[i].x = body->x; sArena.shots[i].y = body->y;
        sArena.shots[i].vx = dx * profile->speed / len;
        sArena.shots[i].vy = dy * profile->speed / len;
        sArena.shots[i].side = side; sArena.shots[i].move = profile->move;
        sArena.shots[i].life = profile->range * Q / profile->speed;
        sArena.shots[i].direction = body->shotFacing; sArena.shots[i].age = 0;
        sArena.shots[i].elementMask=0;
        // Decorative water stream segments share the head's tiles. They have
        // no hitboxes and can never multiply native damage or PP costs.
        {
            u32 t;
            for(t=0;t<2;t++)
            {
                sArena.shots[i].trail[t]=MAX_SPRITES;
                if(profile->move==MOVE_WATER_GUN||profile->move==MOVE_SURF||profile->move==MOVE_ICY_WIND||profile->move==MOVE_SHOCK_WAVE)
                {
                    u8 sprite=ArenaMoveFx_CreateBolt(profile,body->x/Q,body->y/Q,body->shotFacing,i);
                    sArena.shots[i].trail[t]=sprite;
                    if(sprite!=MAX_SPRITES)gSprites[sprite].invisible=TRUE;
                }
            }
        }
    }
    body->actionLife = profile->active;
    body->actionAge = 0; body->connected = FALSE; body->terrainMask=0;
    // Pressure applies on commitment, even if the projectile misses or hits
    // cover. Self-targeted buffs do not target the opponent. Clamp the last PP.
    pp = gBattleMons[side].pp[slot];
    if (gBattleMons[side ^ 1].ability == ABILITY_PRESSURE
        && gBattleMoves[profile->move].target != MOVE_TARGET_USER && pp > 1)
        pp--;
    gBattleMons[side].pp[slot] = --pp;
    SetMonData(side ? &gEnemyParty[gBattlerPartyIndexes[side]] : &gPlayerParty[gBattlerPartyIndexes[side]],
        MON_DATA_PP1 + slot, &pp);
    body->cooldown = profile->recovery + (side ? Clamp(8-gBattleMons[1].level/8,2,8) : 0);
    gRealtimeArenaTelemetry.shots[side]++;
    gArenaCombatTelemetry.lastMove[side] = profile->move;
    if(!gBattleMoves[profile->move].power && body->statusActions<255)body->statusActions++;
    sArena.hudDirty = TRUE;
    PlaySE(profile->move == MOVE_ROLLOUT ? SE_M_TAKE_DOWN
        : profile->move == MOVE_PSYCHIC ? SE_M_PSYBEAM
        : profile->move == MOVE_SHADOW_BALL ? SE_M_PSYBEAM2
        : profile->move == MOVE_DRAGON_CLAW ? SE_M_SCRATCH
        : profile->move == MOVE_CRUNCH ? SE_M_BITE
        : profile->move == MOVE_WATER_GUN || profile->move == MOVE_SURF ? SE_M_BUBBLE_BEAM
        : profile->move == MOVE_ICY_WIND ? SE_M_ICY_WIND
        : profile->move == MOVE_SHOCK_WAVE ? SE_M_THUNDERBOLT
        : profile->move == MOVE_FLAMETHROWER ? SE_M_FLAMETHROWER
        : profile->visual == ARENA_VIS_EMBER ? SE_M_EMBER
        : profile->move == MOVE_ROCK_THROW ? SE_M_ROCK_THROW
        : profile->move == MOVE_BUBBLE ? SE_M_BUBBLE
        : profile->move == MOVE_GUST ? SE_M_GUST
        : profile->move == MOVE_BITE ? SE_M_BITE
        : profile->move == MOVE_LEAF_BLADE ? SE_M_RAZOR_WIND
        : profile->move == MOVE_WING_ATTACK ? SE_M_WING_ATTACK
        : profile->move == MOVE_SCRATCH ? SE_M_SCRATCH
        : profile->kind == ARENA_MOVE_RUSH ? SE_M_SWIFT
        : profile->kind == ARENA_MOVE_CONE ? SE_M_LEER
        : profile->move == MOVE_ABSORB || profile->move == MOVE_MEGA_DRAIN ? SE_M_ABSORB : SE_BALL);
}

static void BeginShot(u8 side, s32 targetX, s32 targetY)
{
    struct ArenaBody *body = &sArena.bodies[side];
    const struct ArenaMoveProfile *profile = ArenaMoves_Get(gBattleMons[side].moves[body->moveSlot]);
    s32 dx,dy,len;
    if (!profile || body->shotTimer || PsychicBusy(side) || SpecialBusy(side) || !gBattleMons[side].pp[body->moveSlot]) return;
    body->shotSlot = body->moveSlot;
    body->aimX = targetX; body->aimY = targetY;
    body->shotFacing = Facing(targetX - body->x, targetY - body->y);
    dx=targetX-body->x;dy=targetY-body->y;
    len=max(Abs(dx),Abs(dy))+min(Abs(dx),Abs(dy))/2;
    if(!len){dy=-Q;len=Q;}
    body->attackX=dx*Q/len;body->attackY=dy*Q/len;
    body->shotElapsed = 0;
    body->shotTimer = profile->windup + profile->active + 8;
    body->animation = profile->animation;
    body->animClock = 0;
    body->drawnFrame = 255;
    body->rollAngle = 0; body->rollStage = 0; body->rollDistance = 0;
    if (profile->move == MOVE_ROLLOUT) RollBallBegin(side);
}

static void TickPendingShots(void)
{
    u32 side;
    for (side = 0; side < 2; side++)
    {
        struct ArenaBody *body = &sArena.bodies[side];
        const struct ArenaMoveProfile *profile;
        if (!body->shotTimer) continue;
        profile = ArenaMoves_Get(gBattleMons[side].moves[body->shotSlot]);
        // Presentation maps the profile's release to the source HitFrame.
        if (body->shotElapsed == profile->windup)
            Fire(side, body->shotSlot, body->aimX, body->aimY);
        body->shotElapsed++;
        body->shotTimer--;
    }
}

static void BufferPlayerActions(void)
{
    struct ArenaBody *body = &sArena.bodies[0];
    s32 dx = (JOY_HELD(DPAD_RIGHT) != 0) - (JOY_HELD(DPAD_LEFT) != 0);
    s32 dy = (JOY_HELD(DPAD_DOWN) != 0) - (JOY_HELD(DPAD_UP) != 0);
    if(sCapture.state==ARENA_CAPTURE_AIM)
    {sArena.attackBuffer=sArena.dashBuffer=0;return;}
    // Brief taps made just before recovery ends survive for six active frames.
    // This also catches input during the two-frame impact stop.
    if (JOY_NEW(A_BUTTON)) sArena.attackBuffer = 6;
    else if (sArena.attackBuffer) sArena.attackBuffer--;
    if (JOY_NEW(B_BUTTON) && (dx || dy))
    {
        sArena.dashBuffer = 6;
        sArena.dashRequestX = dx; sArena.dashRequestY = dy;
    }
    else if (sArena.dashBuffer) sArena.dashBuffer--;
    if (JOY_NEW(L_BUTTON | R_BUTTON) && JOY_HELD(L_BUTTON|R_BUTTON)!=(L_BUTTON|R_BUTTON))
    {
        u32 i;
        for (i = 1; i <= MAX_MON_MOVES; i++)
        {
            u8 slot = (body->moveSlot + (JOY_NEW(L_BUTTON) ? MAX_MON_MOVES - i : i)) % MAX_MON_MOVES;
            if (SupportedMove(gBattleMons[0].moves[slot])) { body->moveSlot = slot; break; }
        }
    }
}

static void TickPlayer(void)
{
    struct ArenaBody *body = &sArena.bodies[0];
    s32 dx = (JOY_HELD(DPAD_RIGHT) != 0) - (JOY_HELD(DPAD_LEFT) != 0);
    s32 dy = (JOY_HELD(DPAD_DOWN) != 0) - (JOY_HELD(DPAD_UP) != 0);
    if(SpecialBusy(0))return;
    if (body->dashCooldown) body->dashCooldown--;
    if (sArena.dashBuffer && !body->dashCooldown && !body->actionLife)
    {
        body->dash = 10; body->dashCooldown = 75;
        body->dashX = sArena.dashRequestX; body->dashY = sArena.dashRequestY;
        sArena.dashBuffer = 0;
        if (body->shotTimer && body->shotElapsed <= ArenaMoves_Get(gBattleMons[0].moves[body->shotSlot])->windup)
            gArenaCombatTelemetry.cancelled[0]++;
        body->shotTimer = 0;
        gRealtimeArenaTelemetry.dodges++;
        PlaySE(SE_M_DOUBLE_TEAM);
    }
    if (body->dash) { dx = body->dashX; dy = body->dashY; }
    if (!body->actionLife)
        Move(0, dx, dy, Speed(0) * (body->dash ? 3 : body->shotTimer ? 1 : 2) / (body->dash ? 1 : 2));
    if (body->moving && !(sArena.frame % (body->dash ? 3 : 14)))
        ArenaFeedback_Dust(body->x/Q, body->y/Q, body->dash != 0);
    if (body->dash) body->dash--;
    // Recovery follows the committed animation, not the start of its hitbox.
    // Changing slots cannot reset this shared recovery.
    if (body->cooldown && !body->shotTimer && !body->actionLife) body->cooldown--;
    if (sCapture.state==ARENA_CAPTURE_IDLE && (JOY_HELD(A_BUTTON) || sArena.attackBuffer) && !body->cooldown && !body->dash && !body->shotTimer)
    {
        // A alone aims at the rival. D-pad + A gives explicit directional
        // aim, including destructible cover. This is still the same four moves.
        body->manualAim=dx||dy;
        // Rollout goes where the body is pointing, not at the rival.
        if(!dx&&!dy&&(gBattleMons[0].moves[body->moveSlot]==MOVE_ROLLOUT
            ||SmokeBlocks(body->x/Q,body->y/Q,sArena.bodies[1].x/Q,sArena.bodies[1].y/Q)))
        {
            static const s8 facing[8][2]={{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};
            dx=facing[body->facing][0];dy=facing[body->facing][1];
            body->manualAim=TRUE;
        }
        BeginShot(0,dx||dy?body->x+dx*200*Q:DecoyAim(1,FALSE),
                    dx||dy?body->y+dy*200*Q:DecoyAim(1,TRUE));
        sArena.attackBuffer = 0;
    }
}

static u16 AiRandom(void)
{
    // Isolated deterministic stream: AI thinking never consumes the native
    // accuracy/critical-hit/damage RNG, nor reads future player input.
    sArena.aiRandom = sArena.aiRandom * 1664525 + 1013904223;
    return sArena.aiRandom >> 16;
}

static void AiRoute(void)
{
    struct ArenaBody *body = &sArena.bodies[1];
    // A ghost opponent walks straight through cover rather than around it.
    if (GhostBody(1)) { sArena.waypoint = sArena.goal; return; }
    if (ArenaNav_NextWaypoint(body->x / Q, body->y / Q, sArena.goal.x, sArena.goal.y, &sArena.waypoint))
        gArenaAiTelemetry.paths++;
}

static void AiChooseGoal(void)
{
    static const s16 offsets[8][2] =
    {
        {256,0}, {181,181}, {0,256}, {-181,181},
        {-256,0}, {-181,-181}, {0,-256}, {181,-181}
    };
    struct ArenaBody *body = &sArena.bodies[1];
    const struct ArenaMoveProfile *p = ArenaMoves_Get(gBattleMons[1].moves[body->moveSlot]);
    s32 preferred = p->kind == ARENA_MOVE_MELEE ? 24 : p->kind == ARENA_MOVE_RUSH ? p->range-22
        : p->move == MOVE_FLAMETHROWER ? 76 : p->kind == ARENA_MOVE_CONE ? 40 : 70+sArena.style*8;
    s32 best = 0x7FFFFFFF;
    u32 i;
    if (p->kind == ARENA_MOVE_PROJECTILE && gBattleMons[1].hp * 3 < gBattleMons[1].maxHP) preferred += 12;
    for (i = 0; i < 8 + ARENA_CORNERS; i++)
    {
        struct ArenaPoint p;
        s32 score;
        // Prefer the eight tactical positions around the observed opponent.
        // Only scan all obstacle corners when those positions are occluded.
        // This bounds routine thinking cost without removing the fallback.
        if(i==8&&best!=0x7FFFFFFF)break;
        if (i < 8)
        {
            p.x = Clamp(sArena.observed.x + offsets[i][0] * preferred / Q, ARENA_MIN_X, ARENA_MAX_X);
            p.y = Clamp(sArena.observed.y + offsets[i][1] * preferred / Q, ARENA_MIN_Y, ARENA_MAX_Y);
        }
        else p = ArenaNav_Corner(i - 8);
        if (!ArenaNav_CanStand(p.x, p.y)
            || !ArenaNav_LineClear(p.x, p.y, sArena.observed.x, sArena.observed.y, 2)) continue;
        score = Abs(ArenaNav_Distance(p.x,p.y,sArena.observed.x,sArena.observed.y) - preferred) * 3
            + ArenaNav_Distance(body->x/Q,body->y/Q,p.x,p.y) + AiRandom() % 24;
        if (score < best) { best = score; sArena.goal = p; }
    }
    sArena.goalTimer = 55;
    AiRoute();
}

static bool8 AiTryEvade(void)
{
    struct ArenaBody *body = &sArena.bodies[1];
    u32 i;
    // Only visible incoming projectiles at a perception tick. Novices often
    // miss the opportunity; high levels still have a reaction delay.
    for (i = 0; i < SHOTS_COUNT; i++)
    {
        struct ArenaShot *shot = &sArena.shots[i];
        s32 dx, dy, t, px, py, len, sign;
        struct ArenaPoint goal;
        if (!shot->life || shot->side != 0
            || SmokeBlocks(body->x/Q,body->y/Q,shot->x/Q,shot->y/Q)
            || !ArenaNav_LineClear(body->x/Q,body->y/Q,shot->x/Q,shot->y/Q,2)) continue;
        dx = shot->x - body->x; dy = shot->y - body->y;
        t = -(dx * shot->vx + dy * shot->vy) / (shot->vx * shot->vx + shot->vy * shot->vy);
        if (t < 1 || t > 22) continue;
        px = (dx + shot->vx * t) / Q; py = (dy + shot->vy * t) / Q;
        if (px * px + py * py > 256 || AiRandom() % 100 >= min(75, 15 + gBattleMons[1].level)) continue;
        len = max(Abs(shot->vx), Abs(shot->vy));
        for (sign = -1; sign <= 1; sign += 2)
        {
            goal.x = body->x/Q + sign * -shot->vy * 26 / len;
            goal.y = body->y/Q + sign * shot->vx * 26 / len;
            if (ArenaNav_CanStand(goal.x,goal.y)
                && ArenaNav_LineClear(body->x/Q,body->y/Q,goal.x,goal.y,ARENA_BODY_RADIUS))
            {
                sArena.goal = sArena.waypoint = goal;
                sArena.aiState = AI_EVADE;
                sArena.goalTimer = sArena.reaction;
                gArenaAiTelemetry.dodges++;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void AiBeginAim(void)
{
    struct ArenaBody *body = &sArena.bodies[1];
    s32 travel = ArenaNav_Distance(body->x/Q,body->y/Q,sArena.observed.x,sArena.observed.y) * 2 / 5;
    s32 prediction = min(30+gBattleMons[1].level*2, 80);
    s32 errorX = AiRandom() % (sArena.aimError * 2 + 1) - sArena.aimError;
    s32 errorY = AiRandom() % (sArena.aimError * 2 + 1) - sArena.aimError;
    body->aimX = (sArena.observed.x + errorX
        + (sArena.observed.x - sArena.previous.x) * travel * prediction / (sArena.reaction * 100)) * Q;
    body->aimY = (sArena.observed.y + errorY
        + (sArena.observed.y - sArena.previous.y) * travel * prediction / (sArena.reaction * 100)) * Q;
    sArena.aimTimer = Clamp(20 - gBattleMons[1].level / 6, 12, 20);
    sArena.aiState = AI_AIM;
    // This target is locked for the entire visible wind-up.
}

static s32 AiTypeValue(u16 move)
{
    u32 i;
    u8 type=gBattleMoves[move].type;
    const struct BattlePokemon *target=&gBattleMons[0];
    s32 value=100;
    if(target->ability==ABILITY_LEVITATE && type==TYPE_GROUND)return 0;
    for(i=0;i<sizeof(gTypeEffectiveness);i+=3)
    {
        u8 attack=TYPE_EFFECT_ATK_TYPE(i),defense=TYPE_EFFECT_DEF_TYPE(i);
        if(attack==TYPE_ENDTABLE)break;
        if(attack==TYPE_FORESIGHT)
        {
            if(target->status2&STATUS2_FORESIGHT)break;
            continue;
        }
        if(attack==type && (defense==target->types[0] || defense==target->types[1]))
            value=value*TYPE_EFFECT_MULTIPLIER(i)/10;
    }
    if(target->ability==ABILITY_WONDER_GUARD && value<=100)return 0;
    if(type==gBattleMons[1].types[0] || type==gBattleMons[1].types[1])value=value*3/2;
    return value;
}

static void AiChooseMove(s32 distance)
{
    u32 i;
    s32 best=-100000;
    bool8 hasPhysical=FALSE,hasAttack=FALSE;
    for(i=0;i<MAX_MON_MOVES;i++)
    {
        u16 move=gBattleMons[1].moves[i];
        if(SupportedMove(move) && gBattleMons[1].pp[i] && gBattleMoves[move].power
            && gBattleMoves[move].type<TYPE_MYSTERY && AiTypeValue(move))hasPhysical=TRUE;
        if(SupportedMove(move) && gBattleMons[1].pp[i] && gBattleMoves[move].power)hasAttack=TRUE;
    }
    for(i=0;i<MAX_MON_MOVES;i++)
    {
        u16 move=gBattleMons[1].moves[i];
        const struct ArenaMoveProfile *p=ArenaMoves_Get(move);
        s32 score;
        if(!p||!gBattleMons[1].pp[i])continue;
        // Pure estimate from the native type chart. Never call TypeCalc here:
        // its damage globals belong to the actual hit transaction.
        score=gBattleMoves[move].power*AiTypeValue(move)/50+AiRandom()%25;
        if(move==MOVE_NIGHT_SHADE||move==MOVE_SEISMIC_TOSS)score=AiTypeValue(move)?gBattleMons[1].level*2:-1000;
        if(move==MOVE_ROLLOUT&&AiTypeValue(move))score+=45; // the roll's ramp is worth more than its listed power
        else if(gBattleMoves[move].power && !AiTypeValue(move))score=-1000;
        if((move==MOVE_ABSORB||move==MOVE_MEGA_DRAIN||move==MOVE_GIGA_DRAIN||move==MOVE_LEECH_LIFE) && AiTypeValue(move)
            && gBattleMons[1].hp*2<gBattleMons[1].maxHP)score+=90;
        if(!gBattleMoves[move].power)
        {
            u8 stat=STAT_ATK;bool8 self;
            s8 change=RealtimeArena_StatChange(move,&stat,&self);
            bool8 useful=change && (change>0?gBattleMons[1].statStages[stat]<8:gBattleMons[0].statStages[stat]>4);
            if((move==MOVE_LEER||move==MOVE_TAIL_WHIP||move==MOVE_SCREECH||move==MOVE_HOWL)&&!hasPhysical)useful=FALSE;
            if(move==MOVE_FOCUS_ENERGY)useful=hasPhysical&&!(gBattleMons[1].status2&STATUS2_FOCUS_ENERGY);
            if(move==MOVE_DOUBLE_TEAM)useful=!sDecoys[1].life;
            if(move==MOVE_SUBSTITUTE)useful=!sCover[1].hp&&gBattleMons[1].hp>gBattleMons[1].maxHP/2;
            if(move==MOVE_SMOKESCREEN)useful=!sSmoke[1].life&&distance<100;
            if(move==MOVE_TELEPORT)useful=distance<60;
            if(move==MOVE_REFLECT)useful=!sBarrierLife[1][0];
            if(move==MOVE_LIGHT_SCREEN)useful=!sBarrierLife[1][1];
            // One setup action, then pressure. Repeated buffs should not turn
            // early wild encounters into several seconds of waiting around.
            if(hasAttack && sArena.bodies[1].statusActions)useful=FALSE;
            score=useful?85+AiRandom()%25:-1000;
        }
        if(p->kind!=ARENA_MOVE_SELF && distance>p->range)score-=(distance-p->range)*2;
        if(gBattleMons[1].pp[i]<=2)score-=12;
        if(score>best){best=score;sArena.bodies[1].moveSlot=i;}
    }
}

static void TickEnemy(void)
{
    struct ArenaBody *body = &sArena.bodies[1];
    s32 dx, dy, len;
    body->moving = FALSE;
    if(SpecialBusy(1))return;
    if (body->cooldown && !body->shotTimer && !body->actionLife) body->cooldown--;
    if (sArena.goalTimer) sArena.goalTimer--;
    if (body->shotTimer) return;
    if (sArena.aiState == AI_AIM)
    {
        if (--sArena.aimTimer == 0)
        {
            if (ArenaNav_LineClear(body->x/Q,body->y/Q,body->aimX/Q,body->aimY/Q,2))
                BeginShot(1, body->aimX, body->aimY);
            sArena.aiState = AI_RECOVER;
            sArena.thinkTimer = 1;
        }
        return;
    }
    if (--sArena.thinkTimer == 0)
    {
        s32 distance;
        sArena.thinkTimer = sArena.reaction;
        sArena.previous = sArena.observed;
        if(!SmokeBlocks(body->x/Q,body->y/Q,sArena.bodies[0].x/Q,sArena.bodies[0].y/Q))
        {sArena.observed.x = sArena.bodies[0].x / Q;sArena.observed.y = sArena.bodies[0].y / Q;}
        else gArenaCoverTelemetry[5]++;
        if(sDecoys[0].life && sDecoys[0].age>=24 && !SmokeBlocks(body->x/Q,body->y/Q,sDecoys[0].x,sDecoys[0].y))
        {
            sArena.observed.x=sDecoys[0].x;
            sArena.observed.y=sDecoys[0].y;
            gArenaDecoyTelemetry[4]++;
        }
        gArenaAiTelemetry.decisions++;
        distance = ArenaNav_Distance(body->x/Q,body->y/Q,sArena.observed.x,sArena.observed.y);
        if(!body->cooldown)AiChooseMove(distance);
        if (!AiTryEvade())
        {
            const struct ArenaMoveProfile *p=ArenaMoves_Get(gBattleMons[1].moves[body->moveSlot]);
            if(p->kind==ARENA_MOVE_SELF && !body->cooldown && gBattleMons[1].pp[body->moveSlot])
            {
                if(p->move==MOVE_SUBSTITUTE||p->move==MOVE_SMOKESCREEN)
                    BeginShot(1,sArena.observed.x*Q,sArena.observed.y*Q);
                else BeginShot(1,body->x,body->y-Q);
                sArena.aiState=AI_RECOVER;return;
            }
            if (!body->cooldown && gBattleMons[1].pp[body->moveSlot]
                && distance < p->range && distance > 8
                && ArenaNav_LineClear(body->x/Q,body->y/Q,sArena.observed.x,sArena.observed.y,2))
            {
                AiBeginAim(); return;
            }
            sArena.aiState = body->cooldown ? AI_RECOVER : AI_REPOSITION;
            if (!sArena.goalTimer || distance < 38
                || ArenaNav_Distance(body->x/Q,body->y/Q,sArena.goal.x,sArena.goal.y) < 5
                || ArenaNav_Distance(sArena.previous.x,sArena.previous.y,sArena.observed.x,sArena.observed.y) > 18)
                AiChooseGoal();
            else AiRoute();
        }
    }
    if (ArenaNav_Distance(body->x/Q,body->y/Q,sArena.waypoint.x,sArena.waypoint.y) < 3
        && ArenaNav_Distance(body->x/Q,body->y/Q,sArena.goal.x,sArena.goal.y) >= 4) AiRoute();
    dx = sArena.waypoint.x * Q - body->x;
    dy = sArena.waypoint.y * Q - body->y;
    len = max(Abs(dx), Abs(dy)) + min(Abs(dx), Abs(dy)) / 2;
    if (len > Q) MoveDelta(1, dx * Speed(1) / len, dy * Speed(1) / len);
}

static void ApplyResolvedHit(u8 side,u16 move,s32 damage);
static void ApplyMoveHit(u8 side,u16 move)
{
    const struct ArenaMoveProfile *profile=ArenaMoves_Get(move);
    u8 targetSide=profile->kind==ARENA_MOVE_SELF?side:side^1;
    struct ArenaBody *target=&sArena.bodies[targetSide];
    s32 damage;
    if(move==MOVE_SUBSTITUTE){if(!CoverStart(side))ArenaFeedback_Wall(target->x/Q,target->y/Q);return;}
    if(move==MOVE_SMOKESCREEN){SmokeStart(side);return;}
    if(move==MOVE_REFLECT||move==MOVE_LIGHT_SCREEN){BarrierStart(side,move);return;}
    if(move!=MOVE_TELEPORT&&move!=MOVE_DOUBLE_TEAM&&SpecialInvulnerable(targetSide))
    {gArenaWarpTossTelemetry[7]++;return;}
    if(move==MOVE_TELEPORT){WarpStart(side);return;}
    if(move==MOVE_DOUBLE_TEAM)
    {
        if(!DecoyStart(side))ArenaFeedback_Impact(side,target->x/Q,target->y/Q,0,ARENA_FEEDBACK_MISS);
        return;
    }
    if(!gBattleMoves[move].power)
    {
        u8 stat=STAT_ATK;bool8 self;
        s8 delta=RealtimeArena_StatChange(move,&stat,&self);
        bool8 worked=RealtimeArena_ResolveStatMove(side,targetSide,move);
        if(worked)gArenaMoveTelemetry.statChanges[side]++;
        ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,stat|(delta>0?256:0),
            worked?(move==MOVE_FOCUS_ENERGY?ARENA_FEEDBACK_CRIT:ARENA_FEEDBACK_STAT):ARENA_FEEDBACK_MISS);
        return;
    }
    damage=RealtimeArena_ResolveDamage(side,targetSide,move);
    ApplyResolvedHit(side,move,damage);
}

// Also used by Seismic Toss: accuracy is decided at the grab, HP at landing.
static void ApplyResolvedHit(u8 side,u16 move,s32 damage)
{
    u8 targetSide=side^1;
    struct ArenaBody *target=&sArena.bodies[targetSide];
    if(damage>0)
    {
        u16 hp=gBattleMons[targetSide].hp;
        u16 dealt=min(damage,hp);
        ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,dealt,ARENA_FEEDBACK_DAMAGE);
        sArena.hitstop=damage>=8?3:2;
        hp-=dealt;
        gBattleMons[targetSide].hp=hp;
        SetMonData(targetSide?&gEnemyParty[gBattlerPartyIndexes[targetSide]]:
                   &gPlayerParty[gBattlerPartyIndexes[targetSide]],MON_DATA_HP,&hp);
        if(move==MOVE_ABSORB || move==MOVE_MEGA_DRAIN || move==MOVE_GIGA_DRAIN || move==MOVE_LEECH_LIFE)
        {
            u16 healing=min(RealtimeArena_DrainAmount(dealt),gBattleMons[side].maxHP-gBattleMons[side].hp);
            u16 healedHp=gBattleMons[side].hp+healing;
            gBattleMons[side].hp=healedHp;
            SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:
                       &gPlayerParty[gBattlerPartyIndexes[side]],MON_DATA_HP,&healedHp);
            gArenaMoveTelemetry.healed[side]+=healing;
            ArenaFeedback_Drain(side,sArena.bodies[side].x/Q,sArena.bodies[side].y/Q,
                                target->x/Q,target->y/Q,healing);
        }
        target->flash=8;
        target->knockX=sArena.bodies[side].attackX*(move==MOVE_QUICK_ATTACK?4:2);
        target->knockY=sArena.bodies[side].attackY*(move==MOVE_QUICK_ATTACK?4:2);
        gRealtimeArenaTelemetry.hits[side]++;
        gRealtimeArenaTelemetry.lastDamage=damage;
        if(hp || move==MOVE_SUPERPOWER || move==MOVE_METEOR_MASH)
        {
            u8 effects=RealtimeArena_ResolveSecondary(side,targetSide,move);
            if(effects&1){gArenaMoveTelemetry.statChanges[side]++;gArenaAdventureTelemetry[0]++;}
            if(effects&4)gArenaAdventureTelemetry[2]++;
            if(effects&8)
            {
                target->burnClock=0;gArenaStatusTelemetry[targetSide]++;
                ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,ARENA_FEEDBACK_BURN);
            }
            if(effects&2)
            {
                gArenaAdventureTelemetry[1]++;
                // A committed hit is not refunded. Cancel only the pending
                // action and add recovery; already emitted projectiles live on.
                if(target->shotTimer && target->shotElapsed<=ArenaMoves_Get(gBattleMons[targetSide].moves[target->shotSlot])->windup)
                    gArenaCombatTelemetry.cancelled[targetSide]++;
                target->shotTimer=0;target->actionLife=0;
                target->cooldown=max(target->cooldown,18);
            }
        }
        sArena.hudDirty=TRUE;
        PlaySE(move==MOVE_ABSORB?SE_M_ABSORB_2:SE_M_COMET_PUNCH);
        if(!hp)
        {
            // Let a committed beam finish its visual release after a KO.
            // Damage/PP are already resolved; no further hits or input run.
            sArena.resultTimer=ArenaMoves_Beam(move)?max(12,sArena.bodies[side].actionLife+8):12;
            sArena.lastAttacker=side;sArena.lastTarget=targetSide;
            gArenaResultTelemetry.started=gMain.vblankCounter1;
        }
    }
    else if(move==MOVE_FALSE_SWIPE && !(gMoveResultFlags&MOVE_RESULT_NO_EFFECT))
    {
        // The native damage clamp legitimately returns zero at 1 HP. This
        // contact is not an accuracy miss and must never faint the target.
        gRealtimeArenaTelemetry.hits[side]++;
        gRealtimeArenaTelemetry.lastDamage=0;
        ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,ARENA_FEEDBACK_DAMAGE);
    }
    else
    {
        gRealtimeArenaTelemetry.misses++;
        ArenaFeedback_Impact(targetSide,target->x/Q,target->y/Q,0,
            (gMoveResultFlags&MOVE_RESULT_DOESNT_AFFECT_FOE)?ARENA_FEEDBACK_IMMUNE:ARENA_FEEDBACK_MISS);
    }
}

#include "arena_psychic.inc"
#include "arena_elements.inc"
#include "arena_warp_toss.inc"

static void DrawPsychicAuras(void)
{
    u32 side,i,n=0;
    for(i=0;i<3;i++)if(sRockShadows[i]!=MAX_SPRITES)gSprites[sRockShadows[i]].invisible=TRUE;
    for(i=0;i<ARENA_OBSTACLES&&n<3;i++)
        if(gArenaPsychicRocks[i].state)
        {
            const struct ArenaPsychicRock *r=&gArenaPsychicRocks[i];
            if(sRockShadows[n]!=MAX_SPRITES)
            {struct Sprite *s=&gSprites[sRockShadows[n]];s->x=r->x/Q;s->y=r->y/Q+8;s->invisible=sArena.paused;}
            n++;
        }
    for(side=0;side<2;side++)
        if(sPsychic[side].active)
            ArenaMoveFx_Psychic(side,sArena.bodies[side].x/Q,sArena.bodies[side].y/Q-8,
                min(23,sPsychic[side].age/4),!sArena.paused);
}

static void HitTerrain(u8 index,const struct ArenaMoveProfile*p,s16 ix,s16 iy)
{
    u32 before=gArenaPhysicsTelemetry.broken;
    ArenaPhysics_Hit(index,ix,iy,p->kind==ARENA_MOVE_PROJECTILE?1:2);
    if(before!=gArenaPhysicsTelemetry.broken)sArena.hitstop=3;
}

static void TickActions(void)
{
    u32 side;
    for(side=0;side<2&&!sArena.resultTimer;side++)
    {
        struct ArenaBody *body=&sArena.bodies[side],*target=&sArena.bodies[side^1];
        const struct ArenaMoveProfile *p;
        s16 oldX=body->x/Q,oldY=body->y/Q;
        bool8 hit=FALSE;
        if(!body->actionLife)continue;
        p=ArenaMoves_Get(gBattleMons[side].moves[body->shotSlot]);
        if(p->move==MOVE_DIG||p->move==MOVE_FLY)
        {
            if(!body->connected){body->connected=TRUE;DiveStart(side,p->move);}
            continue;
        }
        if(p->move==MOVE_SEISMIC_TOSS)
        {
            if(!body->connected){body->connected=TRUE;TossStart(side);}
            if(body->actionLife){body->actionAge++;body->actionLife--;}
            continue;
        }
        if(p->kind==ARENA_MOVE_SELF)
        {
            if(!body->connected){body->connected=TRUE;ApplyMoveHit(side,p->move);}
        }
        else if(p->kind==ARENA_MOVE_RUSH)
        {
            bool8 rollout=p->move==MOVE_ROLLOUT;
            s32 speed=rollout?RolloutSpeed(body,p):p->speed;
            s32 dx=body->attackX*speed/Q,dy=body->attackY*speed/Q;
            s16 obstacle=ArenaNav_FirstObstacle(oldX,oldY,(body->x+dx)/Q,(body->y+dy)/Q,ARENA_BODY_RADIUS);
            // A ghost's rush passes through cover instead of slamming into it.
            if(obstacle>=0 && !GhostBody(side))
            {
                if(rollout)
                {
                    // A fast roll smashes through cover; a slow one stops at it.
                    u32 before=gArenaPhysicsTelemetry.broken;
                    ArenaPhysics_Hit(obstacle,body->attackX*3,body->attackY*3,2+body->rollStage);
                    if(before!=gArenaPhysicsTelemetry.broken)sArena.hitstop=3;
                    else RolloutEnd(body);
                }
                else
                {
                    body->actionLife=1;
                    HitTerrain(obstacle,p,body->attackX*3,body->attackY*3);
                }
                gArenaMoveTelemetry.rushWalls[side]++;
                ArenaFeedback_Wall(oldX,oldY);PlaySE(SE_WALL_HIT);
            }
            MoveDelta(side,dx,dy);
            if(rollout)
            {
                s32 moved=Abs(body->x/Q-oldX)+Abs(body->y/Q-oldY);
                if(!moved && body->actionLife>1)RolloutEnd(body); // the wall
                body->rollDistance=min(255,body->rollDistance+moved);
                if(RollBallReady(side))body->rollAngle+=moved*sBall[side].step;
                body->rollStage=min(ROLLOUT_MAX_STAGE,body->rollDistance/ROLLOUT_STAGE_PX);
                if(!(body->actionAge%2))ArenaFeedback_Dust(body->x/Q,body->y/Q,TRUE);
            }
            else if(!(body->actionAge%3))ArenaFeedback_Dust(body->x/Q,body->y/Q,TRUE);
            hit=ArenaMoves_SegmentHit(oldX,oldY,body->x/Q,body->y/Q,target->x/Q,target->y/Q,p->radius);
        }
        else if(ArenaMoves_Beam(p->move))
        {
            u32 j;
            u8 reach=min(p->range,min(20+body->actionAge*7,body->actionLife*11));
            s16 obstacle;
            // A sustained, aim-locked cone. Cover stops the jet until it breaks.
            // One PP and one native damage resolution, NOT damage every frame.
            obstacle=ArenaNav_FirstObstacle(oldX,oldY,oldX+body->attackX*reach/Q,oldY+body->attackY*reach/Q,1);
            if(obstacle>=0)
            {
                const struct ArenaRect *r=&gArenaObstacles[obstacle];
                s16 nearX=body->attackX>0?r->left-1:r->right+1;
                s16 nearY=body->attackY>0?r->top-1:r->bottom+1;
                s16 alongX=body->attackX?(nearX-oldX)*Q/body->attackX:0;
                s16 alongY=body->attackY?(nearY-oldY)*Q/body->attackY:0;
                // Eight aim directions let us intersect the AABB analytically;
                // avoid thirty full visibility scans on every GBA frame.
                reach=min(reach,max(4,max(alongX,alongY)));
                if(!(body->terrainMask&(1<<obstacle)))
                {
                    body->terrainMask|=1<<obstacle;
                    ArenaPhysics_Hit(obstacle,body->attackX*4,body->attackY*4,3);
                    ArenaFeedback_Embers(oldX+body->attackX*reach/Q,oldY+body->attackY*reach/Q,body->attackX,body->attackY);
                    gArenaFlameTelemetry[3]++;
                }
            }
            sFlameReach[side]=reach;gArenaFlameTelemetry[1]++;
            gArenaFlameTelemetry[4]=max(gArenaFlameTelemetry[4],reach);
            for(j=0;j<ARENA_OBSTACLES;j++)if(!(body->terrainMask&(1<<j))&&ArenaNav_IsSolid(j))
            {
                const struct ArenaRect *r=&gArenaObstacles[j];
                s16 x=(r->left+r->right)/2,y=(r->top+r->bottom)/2;
                if(ArenaMoves_InCone(x-oldX,y-oldY,body->attackX,body->attackY,reach,p->cone)
                    &&ArenaNav_FirstObstacle(oldX,oldY,x,y,0)==j)
                {
                    body->terrainMask|=1<<j;
                    ArenaPhysics_Hit(j,body->attackX*4,body->attackY*4,3);
                    ArenaFeedback_Embers(x,y,body->attackX,body->attackY);gArenaFlameTelemetry[3]++;
                }
            }
            for(j=0;j<ELEMENT_COUNT;j++)
            {
                struct ArenaSurface *surface=&sSurfaces[j];
                if(surface->kind&&ArenaMoves_InCone(surface->x-oldX,surface->y-oldY,body->attackX,body->attackY,reach+8,p->cone)
                    &&ArenaNav_LineClear(oldX,oldY,surface->x,surface->y,0))
                {
                    struct ArenaShot heat={0};
                    heat.move=(p->move==MOVE_AURORA_BEAM?MOVE_ICY_WIND:p->move==MOVE_HYDRO_PUMP?MOVE_SURF:(p->move==MOVE_FLAMETHROWER||p->move==MOVE_FIRE_BLAST||p->move==MOVE_SACRED_FIRE)?MOVE_FLAMETHROWER:MOVE_NONE);heat.x=surface->x*Q;heat.y=surface->y*Q;
                    heat.elementMask=sFlameElementMask[side];ElementReact(&heat);
                    sFlameElementMask[side]=heat.elementMask;
                }
            }
            if(!(body->actionAge%9))ArenaFeedback_Embers(oldX+body->attackX*reach/Q,oldY+body->attackY*reach/Q-8,body->attackX,body->attackY);
            hit=(ArenaMoves_Beam(p->move)==1||body->actionAge>=8)
                &&ArenaMoves_InCone((target->x-body->x)/Q,(target->y-body->y)/Q,body->attackX,body->attackY,reach,p->cone);
        }
        else if(p->kind==ARENA_MOVE_MELEE||p->kind==ARENA_MOVE_CONE)
        {
            u8 range=p->kind==ARENA_MOVE_CONE?min(p->range,24+body->actionAge*4):p->range;
            u32 i;
            if(p->kind==ARENA_MOVE_MELEE)
                for(i=0;i<ARENA_OBSTACLES;i++)
                {
                    const struct ArenaRect*r=&gArenaObstacles[i];
                    s16 x=Clamp(oldX,r->left,r->right),y=Clamp(oldY,r->top,r->bottom);
                    if(!(body->terrainMask&(1<<i))&&ArenaNav_IsSolid(i)
                        &&ArenaMoves_InCone(x-oldX,y-oldY,body->attackX,body->attackY,range,p->cone)
                        &&ArenaNav_FirstObstacle(oldX,oldY,x,y,0)==i)
                    {
                        body->terrainMask|=1<<i;
                        HitTerrain(i,p,body->attackX*3,body->attackY*3);
                    }
                }
            hit=ArenaMoves_InCone((target->x-body->x)/Q,(target->y-body->y)/Q,
                                  body->attackX,body->attackY,range,p->cone);
        }
        if(!body->connected && gBattleMoves[p->move].power)
        {
            if(p->kind==ARENA_MOVE_RUSH)
            {if(CoverSegment(side,p->move,oldX,oldY,body->x/Q,body->y/Q,p->radius))body->connected=TRUE;}
            else if(p->kind==ARENA_MOVE_CONE||p->kind==ARENA_MOVE_MELEE)
            {
                u8 range=ArenaMoves_Beam(p->move)?sFlameReach[side]:p->kind==ARENA_MOVE_CONE?min(p->range,24+body->actionAge*4):p->range;
                if(CoverCone(side,p,range,hit))body->connected=TRUE;
            }
        }
        if(!body->connected && gBattleMoves[p->move].power && sDecoys[side^1].life)
        {
            struct ArenaDecoy*d=&sDecoys[side^1];
            bool8 decoyHit=FALSE;
            if(p->kind==ARENA_MOVE_RUSH)
                decoyHit=DecoySegment(side^1,oldX,oldY,body->x/Q,body->y/Q,p->radius);
            else if(d->age>=24 && (p->kind==ARENA_MOVE_CONE||p->kind==ARENA_MOVE_MELEE))
            {
                u8 range=ArenaMoves_Beam(p->move)?sFlameReach[side]:p->kind==ARENA_MOVE_CONE?min(p->range,24+body->actionAge*4):p->range;
                if(ArenaMoves_InCone(d->x-body->x/Q,d->y-body->y/Q,body->attackX,body->attackY,range,p->cone)
                    &&ArenaNav_LineClear(body->x/Q,body->y/Q,d->x,d->y,2)
                    &&(!hit||target->dash||ArenaNav_Distance(oldX,oldY,d->x,d->y)<ArenaNav_Distance(oldX,oldY,target->x/Q,target->y/Q)))
                {DecoyBreak(side^1);decoyHit=TRUE;}
            }
            if(decoyHit)body->connected=TRUE;
        }
        if(hit&&!body->connected&&!target->dash
            &&ArenaNav_LineClear(body->x/Q,body->y/Q,target->x/Q,target->y/Q,2))
        {
            body->connected=TRUE;
            if(p->move==MOVE_FLAMETHROWER)gArenaFlameTelemetry[2]++;
            gArenaRolloutStage=body->rollStage;
            ApplyMoveHit(side,p->move);
            if(p->move==MOVE_ROLLOUT)
            {
                // Rolled over: the rival is flattened where it stands, not
                // shoved ahead of the ball, and the roll carries on past it.
                target->flat=ROLLOUT_FLAT_FRAMES;
                target->knockX=target->knockY=0;
                sArena.hitstop=max(sArena.hitstop,4);
            }
        }
        body->actionAge++;body->actionLife--;
    }
}

static void DestroyShot(struct ArenaShot *shot)
{
    u32 t;
    DestroySprite(&gSprites[shot->sprite]);
    for(t=0;t<2;t++)
        if(shot->trail[t]!=MAX_SPRITES)DestroySprite(&gSprites[shot->trail[t]]);
    shot->life=0;
}

static void TickShots(void)
{
    u32 i;
    for (i = 0; i < SHOTS_COUNT && !sArena.resultTimer; i++)
    {
        struct ArenaShot *shot = &sArena.shots[i];
        struct ArenaBody *target;
        const struct ArenaMoveProfile *p;
        s16 oldX,oldY;
        s16 obstacle;
        if (!shot->life) continue;
        p=ArenaMoves_Get(shot->move);
        oldX=shot->x/Q;oldY=shot->y/Q;
        target = &sArena.bodies[shot->side ^ 1];
        obstacle=ArenaNav_FirstObstacle(shot->x/Q,shot->y/Q,(shot->x+shot->vx)/Q,(shot->y+shot->vy)/Q,2);
        if (obstacle>=0)
        {
            gArenaAiTelemetry.blockedShots[shot->side]++;
            ArenaFeedback_Wall(shot->x/Q,shot->y/Q);
            PlaySE(SE_WALL_HIT);
            HitTerrain(obstacle,p,shot->vx,shot->vy);
            DestroyShot(shot);
            continue;
        }
        shot->x += shot->vx; shot->y += shot->vy;
        if(ElementReact(shot)){DestroyShot(shot);continue;}
        if(CoverSegment(shot->side,shot->move,oldX,oldY,shot->x/Q,shot->y/Q,p->radius))
        {DestroyShot(shot);continue;}
        if(DecoySegment(shot->side^1,oldX,oldY,shot->x/Q,shot->y/Q,p->radius))
        {DestroyShot(shot);continue;}
        if (ArenaMoves_SegmentHit(oldX,oldY,shot->x/Q,shot->y/Q,target->x/Q,target->y/Q,p->radius) && !target->dash)
        {
            if(SpecialInvulnerable(shot->side^1))gArenaWarpTossTelemetry[7]++;
            else{ApplyMoveHit(shot->side,shot->move);shot->life = 1;}
        }
        if (shot->x < 3 * Q || shot->x > 237 * Q || shot->y < 19 * Q || shot->y > 157 * Q) shot->life = 1;
        if (--shot->life == 0) DestroyShot(shot);
        else
        {
            u32 t;
            ArenaMoveFx_Bolt(shot->sprite,p,shot->x/Q,shot->y/Q,shot->direction,++shot->age);
            for(t=0;t<2;t++)
                if(shot->trail[t]!=MAX_SPRITES)
                {
                    u8 delay=(t+1)*2;
                    gSprites[shot->trail[t]].invisible=shot->age<delay;
                    ArenaMoveFx_Bolt(shot->trail[t],p,(shot->x-shot->vx*delay)/Q,
                        (shot->y-shot->vy*delay)/Q,shot->direction,shot->age);
                }
        }
    }
}

static void TickPhysics(void)
{
    u32 side;
    ArenaPhysics_Update();
    for(side=0;side<2;side++)
    {
        struct ArenaBody *body=&sArena.bodies[side];
        if(SpecialBusy(side))continue;
        if(sArena.lastBlast!=gArenaPhysicsTelemetry.blastSerial)
        {
            s32 dx=body->x/Q-gArenaPhysicsTelemetry.blastX;
            s32 dy=body->y/Q-gArenaPhysicsTelemetry.blastY;
            s32 len=max(1,max(Abs(dx),Abs(dy))+min(Abs(dx),Abs(dy))/2);
            if(dx*dx+dy*dy<46*46){body->knockX=dx*1200/len;body->knockY=dy*1200/len;}
        }
        if(body->knockX||body->knockY)
        {
            MoveDelta(side,body->knockX,body->knockY);
            body->knockX=body->knockX*180/256;body->knockY=body->knockY*180/256;
            if(Abs(body->knockX)<8)body->knockX=0;
            if(Abs(body->knockY)<8)body->knockY=0;
        }
    }
    sArena.lastBlast=gArenaPhysicsTelemetry.blastSerial;
}

static void TickBurn(void)
{
    u32 side;
    // One native Gen III 1/8-max-HP burn pulse per 300 active gameplay ticks.
    // Pausing, capture animation and hitstop do not advance this clock.
    for(side=0;side<2&&!sArena.resultTimer;side++)
    {
        struct ArenaBody *body=&sArena.bodies[side];
        if((gBattleMons[side].status1&STATUS1_BURN) && gBattleMons[side].hp)
        {
            if(++body->burnClock==300)
            {
                u16 damage=min(gBattleMons[side].hp,max(1,gBattleMons[side].maxHP/8));
                body->burnClock=0;gBattleMons[side].hp-=damage;
                SetMonData(side?&gEnemyParty[gBattlerPartyIndexes[side]]:&gPlayerParty[gBattlerPartyIndexes[side]],
                           MON_DATA_HP,&gBattleMons[side].hp);
                gArenaStatusTelemetry[side+2]++;sArena.hudDirty=TRUE;
                ArenaFeedback_Impact(side,body->x/Q,body->y/Q,damage,ARENA_FEEDBACK_DAMAGE);
                body->flash=5;
                if(!gBattleMons[side].hp)
                {
                    sArena.resultTimer=12;sArena.lastAttacker=side^1;sArena.lastTarget=side;
                    gArenaResultTelemetry.started=gMain.vblankCounter1;
                }
            }
        }
        else body->burnClock=0;
    }
}

static void CB2_Arena(void)
{
    u32 i;
    bool8 frozen = FALSE;
    u32 stamp=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228,phaseStamp=stamp,now;
    u32 gap=gMain.vblankCounter1-gArenaFrameTelemetry.lastVBlank;
    if(sSendout.active){gArenaFrameTelemetry.lastVBlank=gMain.vblankCounter1;SendoutTick();return;}
    gArenaFrameTelemetry.lastVBlank=gMain.vblankCounter1;
    gArenaFrameTelemetry.updates++;
    if(gap>gArenaFrameTelemetry.maxGap)gArenaFrameTelemetry.maxGap=gap;
    if(gap>1)gArenaFrameTelemetry.missedVBlanks+=gap-1;
    if (JOY_NEW(SELECT_BUTTON) && !SpecialBusy(0) && !sArena.resultTimer && sCapture.state==ARENA_CAPTURE_IDLE) { ArenaExit(FALSE); return; }
    if (JOY_NEW(START_BUTTON) && !sArena.resultTimer && sCapture.state==ARENA_CAPTURE_IDLE)
    {
        sArena.paused ^= TRUE;
        DrawStage();
        sArena.hudDirty = TRUE;
    }
    if(!SpecialBusy(0)&&!SpecialBusy(1))CaptureInput();
    if(sArena.paused)
    {
        u16 keys=gMain.newKeys&DPAD_ANY;
        u8 slot=keys==DPAD_UP?0:keys==DPAD_RIGHT?1:keys==DPAD_DOWN?2:keys==DPAD_LEFT?3:MAX_MON_MOVES;
        if(slot<MAX_MON_MOVES && SupportedMove(gBattleMons[0].moves[slot]))
        {
            sArena.bodies[0].moveSlot=slot;
            sArena.hudDirty=TRUE;
        }
        // Menu input never queues an attack or dodge for the resumed battle.
        sArena.attackBuffer=sArena.dashBuffer=0;
    }
    if (!sArena.paused)
    {
        sArena.frame++;
        gRealtimeArenaTelemetry.frames++;
        if (sArena.resultTimer)
        {
            SpecialTick();
            for(i=0;i<2;i++)if(sArena.bodies[i].actionLife)
            {
                sArena.bodies[i].actionAge++;sArena.bodies[i].actionLife--;
                sFlameReach[i]=min(sFlameReach[i],sArena.bodies[i].actionLife*11);
            }
            if (--sArena.resultTimer == 0) { ArenaExit(TRUE); return; }
        }
        else
        {
            BufferPlayerActions();
            if(TossActive()){SpecialTick();TickShots();frozen=TRUE;}
            else if(CaptureLocked() || ((sCapture.state==ARENA_CAPTURE_AIM || sCapture.state==ARENA_CAPTURE_THROW) && sArena.frame%3)) frozen=TRUE;
            else if (sArena.hitstop) { sArena.hitstop--; frozen = TRUE; }
            else
            {
                DecoyTick();
                CoverTick();
                BarrierTick();
                SpecialTick();
                TickPhysics();
                now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
                gArenaFrameTelemetry.scanlines[0]=now-phaseStamp;phaseStamp=now;
                if(sCapture.state!=ARENA_CAPTURE_AIM)TickPlayer();
                TickEnemy();
                now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
                gArenaFrameTelemetry.scanlines[1]=now-phaseStamp;phaseStamp=now;
                TickPendingShots(); TickActions(); TickShots(); TickPsychic();
                if(!sArena.resultTimer)ElementsTick();
                TickBurn();
                for (i = 0; i < 2 && !sArena.resultTimer; i++)
                {
                    struct BattlePokemon *mon = &gBattleMons[i];
                    struct ArenaBody *body = &sArena.bodies[i];
                    if (++body->weatherClock >= 300)
                    {
                        u16 damage = 0;
                        body->weatherClock = 0;
                        if (gBattleMons[0].ability == ABILITY_AIR_LOCK || gBattleMons[1].ability == ABILITY_AIR_LOCK
                            || gBattleMons[0].ability == ABILITY_CLOUD_NINE || gBattleMons[1].ability == ABILITY_CLOUD_NINE) continue;
                        if ((gBattleWeather & B_WEATHER_SANDSTORM) && !IS_BATTLER_OF_TYPE(i, TYPE_ROCK)
                            && !IS_BATTLER_OF_TYPE(i, TYPE_GROUND)
                            && !IS_BATTLER_OF_TYPE(i, TYPE_STEEL) && mon->ability != ABILITY_SAND_VEIL)
                            damage = max(1, mon->maxHP / 16);
                        if ((gBattleWeather & B_WEATHER_HAIL) && !IS_BATTLER_OF_TYPE(i, TYPE_ICE))
                            damage = max(1, mon->maxHP / 16);
                        if ((gBattleWeather & B_WEATHER_RAIN) && mon->ability == ABILITY_RAIN_DISH)
                            mon->hp = min(mon->maxHP, mon->hp + max(1, mon->maxHP / 16));
                        damage = min(damage, mon->hp);
                        mon->hp -= damage;
                        SetMonData(i ? &gEnemyParty[gBattlerPartyIndexes[i]] : &gPlayerParty[gBattlerPartyIndexes[i]], MON_DATA_HP, &mon->hp);
                        if (damage) ArenaFeedback_Impact(i, body->x/Q, body->y/Q, damage, ARENA_FEEDBACK_DAMAGE);
                        sArena.hudDirty = TRUE;
                        if (!mon->hp)
                        {
                            sArena.resultTimer=12; sArena.lastAttacker=i^1; sArena.lastTarget=i;
                            gArenaResultTelemetry.started=gMain.vblankCounter1;
                        }
                    }
                }
                // Oran/Sitrus use their original Gen III HP thresholds and
                // values. Consume once, persist to the actual party, and never
                // resurrect a fainted Pokemon. No turn menu is needed.
                for (i = 0; i < 2; i++)
                {
                    struct BattlePokemon *mon = &gBattleMons[i];
                    if (mon->hp && mon->hp <= mon->maxHP / 2
                        && GetItemHoldEffect(mon->item) == HOLD_EFFECT_RESTORE_HP)
                    {
                        u16 item = mon->item;
                        struct Pokemon *party = i ? &gEnemyParty[gBattlerPartyIndexes[i]] : &gPlayerParty[gBattlerPartyIndexes[i]];
                        mon->hp = min(mon->maxHP, mon->hp + GetItemHoldEffectParam(item));
                        gBattleStruct->usedHeldItems[i] = item;
                        mon->item = ITEM_NONE;
                        SetMonData(party, MON_DATA_HELD_ITEM, &mon->item);
                        SetMonData(party, MON_DATA_HP, &mon->hp);
                        sArena.hudDirty = TRUE;
                        PlaySE(SE_M_MORNING_SUN);
                    }
                }
            }
        }
    }
    CaptureTick();
    if(!sArena.active)return;
    frozen |= CaptureLocked();
    frozen |= sArena.hitstop != 0;
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    gArenaFrameTelemetry.scanlines[2]=now-phaseStamp;phaseStamp=now;
    for (i = 0; i < 2; i++)
    {
        struct ArenaBody *body = &sArena.bodies[i];
        const struct ArenaMoveProfile *profile=ArenaMoves_Get(gBattleMons[i].moves[body->shotTimer?body->shotSlot:body->moveSlot]);
        struct Sprite *sprite = &gSprites[body->sprite];
        sprite->x = body->x / Q;
        sprite->y = body->y / Q;
        gSprites[body->shadow].x = sprite->x;
        gSprites[body->shadow].y = body->y / Q + 8;
        gSprites[body->shadow].invisible = gBattleMons[i].hp == 0;
        // A ghost inside a wall is drawn semi-transparent (the HUD's blend
        // settings already list OBJ and the stage) with a pale spectral tint.
        gArenaGhostTelemetry.phasing[i] = Phasing(i);
        sprite->oam.objMode = gArenaGhostTelemetry.phasing[i] ? ST_OAM_OBJ_BLEND : ST_OAM_OBJ_NORMAL;
        if (!sArena.paused && !frozen)
            sprite->x2 = i && sArena.aiState == AI_AIM ? ((sArena.frame & 2) ? 1 : -1) : 0;
        if (body->art)
        {
            u8 animation = PsychicBusy(i) ? ARENA_ANIM_SPECIAL : body->shotTimer ? profile->animation : body->moving ? ARENA_ANIM_WALK : ARENA_ANIM_IDLE;
            u8 direction = body->shotTimer || PsychicBusy(i) ? body->shotFacing : body->facing;
            const struct ArenaSpriteAnimation *anim;
            u8 frame;
            u16 tick=body->animClock/Q;
            bool8 holdPose=FALSE;
            if (!RollBallVisible(i)) RollBallHide(i);
            if(TossActive())holdPose=TossPose(i,&animation,&direction);
            anim=&body->art->animations[animation];
            if (body->animation != animation)
            {
                body->animation = animation; body->animClock = 0; body->drawnFrame = 255;
            }
            if(body->shotTimer)
                tick=body->shotElapsed<profile->windup?body->shotElapsed*anim->hitTick/profile->windup:
                    anim->hitTick+(body->shotElapsed-profile->windup)*(anim->totalTicks-anim->hitTick-1)/(profile->active+8);
            if(PsychicBusy(i))tick=anim->hitTick;
            if(holdPose)tick=anim->hitTick;
            sprite->y2=PsychicBusy(i)?-3-Sin(sPsychic[i].age*4,2):0;
            frame = ArenaSprites_Frame(anim,tick);
            sprite->y += 8-body->art->ground[animation*8+direction]*MonVisualScale(i)/256;
            if (!RollBallVisible(i) && (body->drawnFrame != frame || body->drawnDirection != direction))
            {
                ArenaSprites_Decode(anim, direction, frame, sMonFrameTiles[i]);
                ArenaRender_Copy(sMonFrameTiles[i],
                    (u8 *)OBJ_VRAM0 + GetSpriteTileStartByTag(MON_TAG + i * 2) * 32, 2048);
                body->drawnFrame = frame; body->drawnDirection = direction;
                gArenaSpriteTelemetry.uploads[i]++;
            }
            if (!sArena.paused && !sArena.resultTimer && !frozen)
                body->animClock += animation == ARENA_ANIM_WALK ? Clamp(Speed(i) * Q / 220, 128, 512) : Q;
            if (RollBallVisible(i))
            {
                // Curled into a ball: its own frames stand in for the pose.
                RollBallShow(i, sprite);
                if (!sArena.paused && !frozen) RollBallTick(i);
            }
            else if (Rolling(i) && !TossActive())
            {
                // Roll like a ball along the heading, seen from the arena's
                // front-elevated camera: one rotation about the ground axis
                // perpendicular to the travel direction, projected to the
                // screen. Sideways travel turns like a wheel with the top
                // moving toward the heading; up/down travel tumbles forward;
                // diagonals blend the two. Spin-up in place, then flat out.
                s32 ux = body->attackX, uy = body->attackY, ct, st, v, m00, m01, m10, m11, spin, cross, det;
                if (!body->actionLife && body->shotElapsed && !sArena.paused && !frozen) RollBallTick(i);
                if (!sArena.paused && !frozen)
                {
                    u16 rate = body->actionLife ? ROLLOUT_SPIN
                        : ROLLOUT_SPIN * min(body->shotElapsed, profile->windup) / max(1, profile->windup);
                    body->rollAngle += rate;
                    if (!body->actionLife && !(sArena.frame % 6)) ArenaFeedback_Dust(body->x / Q, body->y / Q, FALSE);
                }
                ct = Cos(body->rollAngle >> 8, 256); st = Sin(body->rollAngle >> 8, 256);
                if (ct > -40 && ct < 40) ct = ct < 0 ? -40 : 40;
                v = 256 - ct;
                spin = ux * ROLLOUT_CAM_COS / 256 * st / 256;
                cross = v * ux / 256 * uy / 256 * ROLLOUT_CAM_SIN / 256;
                m00 = ct + v * uy * uy / 65536;
                m01 = -spin - cross;
                m10 = spin - cross;
                m11 = ct + v * (ux * ux / 256) * (ROLLOUT_CAM_SIN * ROLLOUT_CAM_SIN / 256) / 65536;
                det = (m00 * m11 - m01 * m10) / 256;
                if (det > -16 && det < 16) det = det < 0 ? -16 : 16;
                // The hardware wants the inverse map (screen to texture).
                SetOamMatrix(sprite->oam.matrixNum, m11 * 256 / det, -m01 * 256 / det, -m10 * 256 / det, m00 * 256 / det);
                // Keep the pose's centre where it stands: the hardware turns
                // about the canvas centre, so shift by what that moves it.
                {
                    s32 px = sBall[i].pivotX, py = sBall[i].pivotY;
                    sprite->x += px - ((m00 * px + m01 * py) >> 8);
                    sprite->y += py - ((m10 * px + m11 * py) >> 8);
                }
            }
            else if (body->rollAngle)
            {
                body->rollAngle = 0;
                SetOamMatrix(sprite->oam.matrixNum, 0x100, 0, 0, 0x100);
            }
            else if (body->flat && !TossActive())
            {
                // Flattened under a Rollout: wide and low, pressed onto the
                // ground beneath the ball, then springing back up.
                s32 f = body->flat >= ROLLOUT_FLAT_SPRING ? 256 : body->flat * 256 / ROLLOUT_FLAT_SPRING;
                SetOamMatrix(sprite->oam.matrixNum, 65536 / (256 + 72 * f / 256), 0, 0, 65536 / (256 - 150 * f / 256));
                sprite->y += 8 * f / 256;
                if (!sArena.paused && !frozen && !--body->flat)
                    SetOamMatrix(sprite->oam.matrixNum, 0x100, 0, 0, 0x100);
            }
            sprite->subpriority = body->flat ? 1 : 0;
            gArenaSpriteTelemetry.animation[i] = animation;
            gArenaSpriteTelemetry.frame[i] = frame;
            gArenaSpriteTelemetry.direction[i] = direction;
        }
        else
        {
            if (!sArena.paused && !frozen)
                sprite->y2 = body->moving && (sArena.frame & 8) ? -1 : 0;
            sprite->oam.tileNum = GetSpriteTileStartByTag(MON_TAG + i * 2 + (body->facing < 3 || body->facing > 5));
            SetOamMatrix(sprite->oam.matrixNum, body->facing > 0 && body->facing < 4 ? -0x200 : 0x200, 0, 0, 0x200);
        }
        // A short palette flash keeps the original pose visible, unlike
        // alternating invisible frames. Dust now communicates the dash.
        sprite->invisible = sArena.resultTimer && !gBattleMons[i].hp;
        gSprites[body->shadow].invisible = sprite->invisible;
        if (body->flash) BlendPalette(OBJ_PLTT_ID(i), 16, body->flash + 3, RGB_WHITE);
        else BlendPalette(OBJ_PLTT_ID(i), 16, gArenaGhostTelemetry.phasing[i] ? 7 : 0, RGB(22, 26, 31));
        if (body->flash && !sArena.paused && !frozen) body->flash--;
        gRealtimeArenaTelemetry.x[i] = sprite->x;
        gRealtimeArenaTelemetry.y[i] = body->y / Q;
        gRealtimeArenaTelemetry.speed[i] = Speed(i);
        gArenaAiTelemetry.facing[i] = body->facing;
        gArenaCombatTelemetry.cooldown[i] = body->cooldown;
        gArenaCombatTelemetry.windup[i] = body->shotTimer;
        gArenaCombatTelemetry.dashCooldown[i] = body->dashCooldown;
        gArenaCombatTelemetry.pendingSlot[i] = body->shotTimer ? body->shotSlot : 255;
        gArenaMoveTelemetry.kind[i]=profile->kind;
        gArenaMoveTelemetry.phase[i]=body->actionLife?2:body->shotTimer&&body->shotElapsed<=profile->windup?1:body->cooldown?3:0;
        gArenaMoveTelemetry.range[i]=profile->range;
        gArenaMoveTelemetry.active[i]=body->actionLife;
        gArenaMoveTelemetry.actionMove[i]=profile->move;
        ArenaMoveFx_Action(i,profile,body->x/Q,body->y/Q,body->shotFacing,body->actionAge,
            body->actionLife!=0&&profile->move!=MOVE_DOUBLE_TEAM&&profile->move!=MOVE_TELEPORT&&profile->move!=MOVE_SEISMIC_TOSS&&profile->move!=MOVE_DIG&&profile->move!=MOVE_FLY&&profile->move!=MOVE_SUBSTITUTE&&profile->move!=MOVE_SMOKESCREEN,sArena.paused);
        ArenaMoveFx_Flame(i,profile->move,body->x/Q,body->y/Q-8,body->shotFacing,body->actionAge,sFlameReach[i],
            ArenaMoves_Beam(profile->move)&&body->actionLife&&!sArena.paused&&gBattleMons[i].hp);
    }
    {
        u8 warmth=0;
        for(i=0;i<2;i++)
        {
            struct ArenaBody *b=&sArena.bodies[i];
            if(b->actionLife&&gBattleMons[i].moves[b->shotSlot]==MOVE_FLAMETHROWER)
                warmth=max(warmth,b->actionAge<8?4:b->actionLife<10?1:2);
        }
        // Brief warm light on the scenery only, never the HUD or HP palettes.
        BlendPalette(16,240,sArena.paused?0:warmth,RGB(31,16,4));
    }
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    gArenaFrameTelemetry.scanlines[3]=now-phaseStamp;phaseStamp=now;
    gArenaCombatTelemetry.aimBlocked = !ArenaNav_LineClear(sArena.bodies[0].x/Q,
        sArena.bodies[0].y/Q, DecoyAim(1,FALSE)/Q, DecoyAim(1,TRUE)/Q, 2);
    gArenaCombatTelemetry.aimVisible = !sArena.paused && !sArena.resultTimer
        && sCapture.state==ARENA_CAPTURE_IDLE
        && !(sArena.bodies[0].shotTimer?sArena.bodies[0].manualAim:JOY_HELD(DPAD_ANY))
        && (JOY_HELD(A_BUTTON) || sArena.bodies[0].shotTimer);
    gSprites[sArena.aimSprite].x = DecoyAim(1,FALSE) / Q;
    gSprites[sArena.aimSprite].y = DecoyAim(1,TRUE) / Q;
    gSprites[sArena.aimSprite].invisible = !gArenaCombatTelemetry.aimVisible;
    gSprites[sArena.aimSprite].oam.paletteNum = IndexOfSpritePaletteTag(SHOT_TAG
        + (gArenaCombatTelemetry.aimBlocked ? 1 : sArena.bodies[0].cooldown ? 2 : 0));
    gSprites[sArena.cueSprite].x = sArena.bodies[1].x / Q;
    gSprites[sArena.cueSprite].y = sArena.bodies[1].y / Q + 14;
    gSprites[sArena.cueSprite].invisible = sArena.aiState != AI_AIM || sArena.resultTimer;
    DecoyDraw();
    SpecialDraw();
    MonVisualScaleApply();
    CoverDraw();
    BarrierDraw();
    CaptureDraw();
    gArenaAiTelemetry.state = sArena.aiState;
    gArenaAiTelemetry.goalX = sArena.goal.x; gArenaAiTelemetry.goalY = sArena.goal.y;
    gArenaAiTelemetry.waypointX = sArena.waypoint.x; gArenaAiTelemetry.waypointY = sArena.waypoint.y;
    gRealtimeArenaTelemetry.paused = sArena.paused;
    gRealtimeArenaTelemetry.selectedMove = sArena.bodies[0].moveSlot;
    ArenaFeedback_Update(sArena.paused, frozen && !CaptureLocked());
    ArenaTerrain_Draw(sArena.paused,frozen);
    ElementsDraw();
    DrawPsychicAuras();
    ArenaPsychicFx_Draw(sArena.paused,frozen);
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    gArenaRenderTelemetry[0]=now-phaseStamp;
    gArenaFeedbackTelemetry.hitstop = sArena.hitstop;
    gArenaFeedbackTelemetry.attackBuffer = sArena.attackBuffer;
    gArenaFeedbackTelemetry.dashBuffer = sArena.dashBuffer;
    if (sArena.hudDirty || sHudRecovery!=RecoveryPips() || JOY_NEW(START_BUTTON | L_BUTTON | R_BUTTON)) DrawHud();
    gArenaRenderTelemetry[1]=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228-now;
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    AnimateSprites();
    BuildOamBuffer();
    // BG0's paused picker must cover actors and attack sprites. Change only
    // this frame's OAM, so normal priorities return automatically on resume.
    if(sArena.paused)
        for(i=0;i<128;i++)
            if(gMain.oamBuffer[i].priority==0)gMain.oamBuffer[i].priority=1;
    TossHideOam();
    ArenaRender_Ready();
    gArenaRenderTelemetry[2]=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228-now;
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    UpdatePaletteFade();
    gArenaRenderTelemetry[3]=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228-now;
    for(i=0;i<4;i++)if(gArenaRenderTelemetry[i]>gArenaRenderTelemetry[4+i])gArenaRenderTelemetry[4+i]=gArenaRenderTelemetry[i];
    now=gMain.vblankCounter1*228+(REG_VCOUNT+68)%228;
    gArenaFrameTelemetry.scanlines[4]=now-phaseStamp;
    gArenaFrameTelemetry.scanlines[5]=now-stamp;
    for(i=0;i<6;i++)if(gArenaFrameTelemetry.scanlines[i]>gArenaFrameTelemetry.peak[i])
        gArenaFrameTelemetry.peak[i]=gArenaFrameTelemetry.scanlines[i];
}

static void ArenaExit(bool8 fainted)
{
    u32 i;
    CaptureReset();
    DecoyHide();
    CoverHide();
    BarrierHide();
    SpecialHide();
    // Finish the ORIGINAL battle scripts behind the arena image. No classic
    // scene reconstruction just to say "fainted" and animate an EXP bar.
    // Interactive choices explicitly restore the native UI if needed.
    RollBallExit();
    if(fainted)
    {
        sArena.active=FALSE;sArena.classic=FALSE;
        gRealtimeArenaTelemetry.active=FALSE;
        gRealtimeArenaTelemetry.exits++;
        gRealtimeArenaTelemetry.lastExitFainted=TRUE;
        gRealtimeArenaRestoringFaint=TRUE;
        gRealtimeArenaQuietResult=TRUE;
        gBattlerAttacker=sArena.lastAttacker;gBattlerTarget=sArena.lastTarget;
        ArenaFeedback_Destroy();
        for(i=0;i<2;i++)ArenaMoveFx_Action(i,NULL,0,0,0,0,FALSE,TRUE);
        gSprites[sArena.aimSprite].invisible=TRUE;
        gSprites[sArena.cueSprite].invisible=TRUE;
        BuildOamBuffer();ArenaRender_Ready();
        gMain.callback1=sArena.savedCB1;
        RealtimeArena_ResumeBattle(TRUE);
        SetMainCallback2(BattleMainCB2);
        return;
    }
    SetVBlankCallback(NULL);
    ArenaFeedback_Destroy();
    for (i = 0; i < 2; i++)
    {
        struct Sprite *sprite = &gSprites[sArena.bodies[i].sprite];
        FreeOamMatrix(sprite->oam.matrixNum);
        DestroySprite(sprite);
        FreeSpriteTilesByTag(MON_TAG + i * 2);
        FreeSpriteTilesByTag(MON_TAG + i * 2 + 1);
    }
    FreeAllWindowBuffers();
    sArena.active = FALSE;
    // SELECT opens the original party selector in trainer arenas, then
    // returns here after the switch. It never enables a turn-based attack.
    sArena.classic = !(gBattleTypeFlags & BATTLE_TYPE_TRAINER);
    gRealtimeArenaTelemetry.active = FALSE;
    gRealtimeArenaTelemetry.exits++;
    gRealtimeArenaTelemetry.lastExitFainted = FALSE;
    gRealtimeArenaRestoringFaint = FALSE;
    gMain.callback1 = sArena.savedCB1;
    RealtimeArena_ResumeBattle(FALSE);
    ReshowBattleScreenAfterMenu();
}

bool8 RealtimeArena_RequestDemo(void)
{
    // A new, disposable save only. Never reset an existing player's progress.
    if (gSaveFileStatus != SAVE_STATUS_EMPTY) return FALSE;
    sDemoRequested = TRUE;
    return TRUE;
}

bool8 RealtimeArena_SetupDemo(void)
{
#if !ARENA_LAB
    static const struct {u16 species;u8 level;} team[] = {
        {SPECIES_CHARIZARD,36}, {SPECIES_BLASTOISE,36}, {SPECIES_EEVEE,25},
        {SPECIES_DRAGONITE,30}, {SPECIES_SCIZOR,15}, {SPECIES_BLAZIKEN,20}
    };
    static const struct {u16 species;u8 level;} box[] = {
        {SPECIES_TREECKO,11}, {SPECIES_POOCHYENA,12}, {SPECIES_BULBASAUR,12},
        {SPECIES_SQUIRTLE,15}, {SPECIES_GROVYLE,16}, {SPECIES_SCEPTILE,43}
    };
    u32 i;
#endif
    if (!sDemoRequested) return FALSE;
    sDemoRequested = FALSE;
    StringCopy(gSaveBlock2Ptr->playerName, sDemoName);
    gSaveBlock2Ptr->playerGender = MALE;
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_OLDALE_TOWN);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_OLDALE_TOWN);
    gSaveBlock1Ptr->location.warpId = -1;
    gSaveBlock1Ptr->location.x = 10;
    gSaveBlock1Ptr->location.y = 17;
    gSaveBlock1Ptr->pos.x = 10;
    gSaveBlock1Ptr->pos.y = 17;
    SetLastHealLocationWarp(HEAL_LOCATION_OLDALE_TOWN);
    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_POKEDEX_GET);
    FlagSet(FLAG_ADVENTURE_STARTED);
    FlagSet(FLAG_SYS_B_DASH);
    AddBagItem(ITEM_POKE_BALL,20);
    FlagSet(FLAG_HIDE_ROUTE_101_BIRCH_STARTERS_BAG);
    FlagSet(FLAG_HIDE_ROUTE_101_BIRCH_ZIGZAGOON_BATTLE);
    FlagSet(FLAG_HIDE_ROUTE_101_ZIGZAGOON);
    FlagSet(FLAG_HIDE_ROUTE_101_BIRCH);
    VarSet(VAR_ROUTE101_STATE, 3);
#if ARENA_LAB
    CreateMon(&gPlayerParty[0], SPECIES_TREECKO, 6, 20, TRUE, 0, OT_ID_PLAYER_ID, 0);
    CreateMon(&gPlayerParty[1], SPECIES_MUDKIP, 5, 20, TRUE, 0, OT_ID_PLAYER_ID, 0);
    gPlayerPartyCount = 2;
#else
    // A native NEW GAME preset, never an imported save or a fixture mailbox.
    // RequestDemo already refuses when any existing save is present.
    FlagSet(FLAG_ARENA_PRACTICE);
    gPlayerPartyCount = ARRAY_COUNT(team);
    for (i = 0; i < ARRAY_COUNT(team); i++)
        CreateMon(&gPlayerParty[i], team[i].species, team[i].level,
                  20, TRUE, i * 2, OT_ID_PLAYER_ID, 0);
    for (i = 0; i < ARRAY_COUNT(box); i++)
        CreateBoxMonAt(0, i, box[i].species, box[i].level,
                       20, TRUE, i * 2, OT_ID_PLAYER_ID, 0);
#endif
    return TRUE;
}

void RealtimeArena_DemoFieldCallback(void)
{
    FieldCB_WarpExitFadeFromBlack();
}
