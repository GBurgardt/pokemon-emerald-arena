#ifndef GUARD_ARENA_MOVE_FX_H
#define GUARD_ARENA_MOVE_FX_H
#include "arena_moves.h"
void ArenaMoveFx_Init(void);
void ArenaMoveFx_Flame(u8 side,u16 move,s16 x,s16 y,u8 direction,u8 age,u8 reach,bool8 active);
u8 ArenaMoveFx_Palette(u8 material);
void ArenaMoveFx_Action(u8 side, const struct ArenaMoveProfile *profile,
                       s16 x, s16 y, u8 direction, u8 age, bool8 active, bool8 paused);
#define ARENA_BOLT_SLOTS 6
u8 ArenaMoveFx_CreateBolt(const struct ArenaMoveProfile *profile, s16 x, s16 y, u8 direction, u8 slot);
void ArenaMoveFx_Bolt(u8 sprite, const struct ArenaMoveProfile *profile,
                     s16 x, s16 y, u8 direction, u8 age);
#endif
