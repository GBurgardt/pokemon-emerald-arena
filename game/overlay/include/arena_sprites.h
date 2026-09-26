#ifndef GUARD_ARENA_SPRITES_H
#define GUARD_ARENA_SPRITES_H
#ifdef ARENA_SPRITES_HOST
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint8_t bool8;
typedef int8_t s8;
#else
#include "global.h"
#endif
#define ARENA_ANIM_IDLE 0
#define ARENA_ANIM_WALK 1
#define ARENA_ANIM_SHOOT 2
#define ARENA_ANIM_ATTACK 3
#define ARENA_ANIM_SPECIAL 4

struct ArenaSpriteAnimation
{
    const u8 *tiles;
    const u8 *durations;
    u16 frames, totalTicks, hitTick;
};
struct ArenaSpriteSet
{
    u16 species;
    const u16 *palette;
    struct ArenaSpriteAnimation animations[5];
    const s8 *ground; // Median opaque foot line per pose/direction, from canvas centre.
};
const struct ArenaSpriteSet *ArenaSprites_Get(u16 species);
u8 ArenaSprites_Frame(const struct ArenaSpriteAnimation *anim, u16 tick);
bool8 ArenaSprites_Decode(const struct ArenaSpriteAnimation *anim, u8 direction, u8 frame, u32 *destination);
#endif
