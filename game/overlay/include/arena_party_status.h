#ifndef GUARD_ARENA_PARTY_STATUS_H
#define GUARD_ARENA_PARTY_STATUS_H
#ifdef ARENA_PARTY_STATUS_HOST
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#else
#include "global.h"
#endif

// Ball indicator states for the per-side trainer party row.
#define ARENA_BALL_FAINTED 0
#define ARENA_BALL_FROZEN 1
#define ARENA_BALL_ASLEEP 2
#define ARENA_BALL_PARALYZED 3
#define ARENA_BALL_BURNED 4
#define ARENA_BALL_POISONED 5
#define ARENA_BALL_NORMAL 6

// Caller-decoded status flags, not raw STATUS1 bits -- keeps this module
// independent of pokeemerald's status encoding so it can be unit tested
// on host without the full engine. The caller maps STATUS1_FREEZE/SLEEP/
// PARALYSIS/BURN/PSN_ANY onto these before calling ArenaPartyStatus_BallState.
#define ARENA_STATUS_FROZEN (1 << 0)
#define ARENA_STATUS_ASLEEP (1 << 1)
#define ARENA_STATUS_PARALYZED (1 << 2)
#define ARENA_STATUS_BURNED (1 << 3)
#define ARENA_STATUS_POISONED (1 << 4)

u8 ArenaPartyStatus_BallState(u16 hp, u32 statusFlags);
#endif
