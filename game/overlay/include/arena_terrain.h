#ifndef GUARD_ARENA_TERRAIN_H
#define GUARD_ARENA_TERRAIN_H
#include "global.h"
extern u8 gArenaBiome;
void ArenaTerrain_Init(void);
bool8 ArenaTerrain_ReserveProjectile(void);
void ArenaTerrain_Draw(bool8 paused,bool8 frozen);
#endif
