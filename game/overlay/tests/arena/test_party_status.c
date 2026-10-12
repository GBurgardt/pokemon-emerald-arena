#include "arena_party_status.h"
#include <assert.h>
#include <stdio.h>

// Independent reference: table-driven, in priority order, distinct in
// structure from the implementation's if-chain. Fainted always wins; among
// the rest, exactly one bit is ever set in real play, but the reference
// still defines a total order for any combination.
static u8 reference(u16 hp, u32 statusFlags)
{
    static const struct { u32 flag; u8 state; } priority[] = {
        {ARENA_STATUS_FROZEN, ARENA_BALL_FROZEN},
        {ARENA_STATUS_ASLEEP, ARENA_BALL_ASLEEP},
        {ARENA_STATUS_PARALYZED, ARENA_BALL_PARALYZED},
        {ARENA_STATUS_BURNED, ARENA_BALL_BURNED},
        {ARENA_STATUS_POISONED, ARENA_BALL_POISONED},
    };
    unsigned i;
    if (hp == 0) return ARENA_BALL_FAINTED;
    for (i = 0; i < sizeof(priority) / sizeof(priority[0]); i++)
        if (statusFlags & priority[i].flag) return priority[i].state;
    return ARENA_BALL_NORMAL;
}

int main(void)
{
    static const u16 hpSamples[] = {0, 1, 2, 48, 999, 65535};
    unsigned hi, flags, n = 0;
    for (hi = 0; hi < sizeof(hpSamples) / sizeof(hpSamples[0]); hi++)
        for (flags = 0; flags <= 31; flags++)
        {
            u16 hp = hpSamples[hi];
            u8 got = ArenaPartyStatus_BallState(hp, flags);
            u8 want = reference(hp, flags);
            assert(got == want);
            n++;
        }
    printf("PASS: %u exact ball-state classifications, all hp samples and status flag combos\n", n);
    return 0;
}
