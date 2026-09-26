#ifndef GUARD_REALTIME_ARENA_H
#define GUARD_REALTIME_ARENA_H

// One native wild-battle presentation. Party/save data stay owned by Emerald.
void RealtimeArena_ResetBattle(void);
bool8 RealtimeArena_TryStart(void);
bool8 RealtimeArena_CanSkipIntro(void);
s32 RealtimeArena_ResolveDamage(u8 attacker, u8 target, u16 move);
u8 RealtimeArena_ResolveSecondary(u8 attacker, u8 target, u16 move);
bool8 RealtimeArena_ResolveLeer(u8 attacker, u8 target);
s8 RealtimeArena_StatChange(u16 move, u8 *stat, bool8 *self);
bool8 RealtimeArena_ResolveStatMove(u8 attacker, u8 target, u16 move);
u16 RealtimeArena_DrainAmount(u16 hpDealt);
void RealtimeArena_ResumeBattle(bool8 fainted);
void RealtimeArena_ShowResultUi(void);
bool8 RealtimeArena_RequestDemo(void);
bool8 RealtimeArena_SetupDemo(void);
void RealtimeArena_DemoFieldCallback(void);
void RealtimeArena_PracticeTick(void);

// Read-only telemetry for tests of the actual ROM, never a second party.
struct RealtimeArenaTelemetry
{
    u32 entries;
    u32 frames;
    u32 shots[2];
    u32 hits[2];
    u32 misses;
    u32 dodges;
    u32 exits;
    u32 expBefore;
    u16 hpBefore;
    u16 lastDamage;
    s16 x[2];
    s16 y[2];
    u16 speed[2];
    u8 active;
    u8 paused;
    u8 lastExitFainted;
    u8 selectedMove;
};
extern struct RealtimeArenaTelemetry gRealtimeArenaTelemetry;
struct ArenaAiTelemetry
{
    u32 decisions, paths, dodges;
    u32 blockedShots[2], wallBlocks[2];
    u8 level, reaction, aimError, style, state, facing[2], reserved;
    s16 goalX, goalY, waypointX, waypointY;
};
extern struct ArenaAiTelemetry gArenaAiTelemetry;
struct ArenaSpriteTelemetry
{
    u8 pmd[2], animation[2], frame[2], direction[2];
    u32 uploads[2];
};
extern struct ArenaSpriteTelemetry gArenaSpriteTelemetry;
struct ArenaCombatTelemetry
{
    u32 cancelled[2];
    u16 lastMove[2], cooldown[2], windup[2], dashCooldown[2];
    u8 pendingSlot[2], aimBlocked, aimVisible;
};
extern struct ArenaCombatTelemetry gArenaCombatTelemetry;
struct ArenaMoveTelemetry
{
    u32 healed[2], statChanges[2], rushWalls[2];
    u8 kind[2], phase[2], range[2], active[2];
    u16 actionMove[2];
};
extern struct ArenaMoveTelemetry gArenaMoveTelemetry;
struct ArenaFrameTelemetry {u32 updates,missedVBlanks,maxGap,lastVBlank;u16 scanlines[6],peak[6];};
extern struct ArenaFrameTelemetry gArenaFrameTelemetry;
struct ArenaIntroTelemetry {u32 started,elapsed,skipped;};
extern struct ArenaIntroTelemetry gArenaIntroTelemetry;
extern bool8 gRealtimeArenaRestoringFaint;
extern bool8 gRealtimeArenaQuietResult;
struct ArenaResultTelemetry {u32 started,elapsed,uiFallbacks,textsSkipped,animationsSkipped,expUpdates;};
extern struct ArenaResultTelemetry gArenaResultTelemetry;
// Ghost types phasing through the arena walls: wall crossings per side and
// whether each body is currently drawn inside a wall.
struct ArenaGhostTelemetry {u32 crossings[2]; u8 phasing[2];};
extern struct ArenaGhostTelemetry gArenaGhostTelemetry;
// Real-time Rollout: how far the current roll has travelled, as a power stage
// (0..3) read by the native damage adapter when the hit resolves.
extern u8 gArenaRolloutStage;

#endif
