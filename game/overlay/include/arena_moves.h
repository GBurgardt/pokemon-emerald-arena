#ifndef GUARD_ARENA_MOVES_H
#define GUARD_ARENA_MOVES_H
#ifdef ARENA_MOVES_HOST
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t bool8;
#else
#include "global.h"
#endif

enum { ARENA_MOVE_NONE, ARENA_MOVE_MELEE, ARENA_MOVE_RUSH,
       ARENA_MOVE_PROJECTILE, ARENA_MOVE_CONE, ARENA_MOVE_SELF };
enum { ARENA_VIS_ARC, ARENA_VIS_RUSH, ARENA_VIS_LEER,
       ARENA_VIS_WING, ARENA_VIS_SLAM, ARENA_VIS_PECK, ARENA_VIS_CLAW,
       ARENA_VIS_BITE, ARENA_VIS_LEAF,
       ARENA_VIS_ABSORB, ARENA_VIS_WATER, ARENA_VIS_SHADE, ARENA_VIS_ACID,
       ARENA_VIS_MUD, ARENA_VIS_ROCK, ARENA_VIS_EMBER, ARENA_VIS_BUBBLE,
       ARENA_VIS_GUST, ARENA_VIS_STAR, ARENA_VIS_PSYCHIC, ARENA_VIS_SHADOW };
enum { ARENA_MOVE_CREAM, ARENA_MOVE_WHITE, ARENA_MOVE_GREEN,
       ARENA_MOVE_PURPLE, ARENA_MOVE_BLUE, ARENA_MOVE_PALETTES };

struct ArenaMoveProfile
{
    u16 move, speed;
    u8 kind, animation, windup, active, recovery, range, radius;
    u8 visual, palette, cone;
};
const struct ArenaMoveProfile *ArenaMoves_Get(u16 move);
// 0=ordinary, 1=flame, 2=solar, 3=water, 4=ice, 5=fire,
// 6=air, 7=electric, 8=shadow.
u8 ArenaMoves_Beam(u16 move);
// Q8 unit aim, pixel displacement. cone=1: 90 degree fan, cone=2: narrow fan.
bool8 ArenaMoves_InCone(s16 dx, s16 dy, s16 aimX, s16 aimY, u8 range, u8 cone);
bool8 ArenaMoves_SegmentHit(s16 x1, s16 y1, s16 x2, s16 y2,
                           s16 targetX, s16 targetY, u8 radius);
#endif
