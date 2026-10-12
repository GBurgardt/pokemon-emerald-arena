#include "arena_party_status.h"

u8 ArenaPartyStatus_BallState(u16 hp, u32 statusFlags)
{
    if (!hp) return ARENA_BALL_FAINTED;
    if (statusFlags & ARENA_STATUS_FROZEN) return ARENA_BALL_FROZEN;
    if (statusFlags & ARENA_STATUS_ASLEEP) return ARENA_BALL_ASLEEP;
    if (statusFlags & ARENA_STATUS_PARALYZED) return ARENA_BALL_PARALYZED;
    if (statusFlags & ARENA_STATUS_BURNED) return ARENA_BALL_BURNED;
    if (statusFlags & ARENA_STATUS_POISONED) return ARENA_BALL_POISONED;
    return ARENA_BALL_NORMAL;
}
