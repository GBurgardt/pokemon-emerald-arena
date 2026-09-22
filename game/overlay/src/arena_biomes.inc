// Selection follows Emerald's actual tile/map environment, not the opponent.
// All backgrounds share one tilemap and VRAM layout; extra art lives in ROM.
static const u32 scoastTiles[] = INCBIN_U32(".arena-dev/art/coast.8bpp");
static const u16 scoastPalette[] = INCBIN_U16(".arena-dev/art/coast.gbapal");
static const u32 scaveTiles[] = INCBIN_U32(".arena-dev/art/cave.8bpp");
static const u16 scavePalette[] = INCBIN_U16(".arena-dev/art/cave.gbapal");
static const u32 sdesertTiles[] = INCBIN_U32(".arena-dev/art/desert.8bpp");
static const u16 sdesertPalette[] = INCBIN_U16(".arena-dev/art/desert.gbapal");
static const u32 sgymTiles[] = INCBIN_U32(".arena-dev/art/gym.8bpp");
static const u16 sgymPalette[] = INCBIN_U16(".arena-dev/art/gym.gbapal");

EWRAM_DATA u8 gArenaBiome = 0;
static void LoadArenaBiome(void)
{
    const u32 *tiles = sForestTiles;
    const u16 *palette = sForestPalette;
    gArenaBiome = 0;
    switch (gBattleEnvironment)
    {
    case BATTLE_ENVIRONMENT_UNDERWATER:
    case BATTLE_ENVIRONMENT_WATER:
    case BATTLE_ENVIRONMENT_POND:
        gArenaBiome = 1; tiles = scoastTiles; palette = scoastPalette; break;
    case BATTLE_ENVIRONMENT_CAVE:
    case BATTLE_ENVIRONMENT_MOUNTAIN:
        gArenaBiome = 2; tiles = scaveTiles; palette = scavePalette; break;
    case BATTLE_ENVIRONMENT_SAND:
        gArenaBiome = 3; tiles = sdesertTiles; palette = sdesertPalette; break;
    case BATTLE_ENVIRONMENT_BUILDING:
        gArenaBiome = 4; tiles = sgymTiles; palette = sgymPalette; break;
    }
    LoadPalette(palette, 0, sizeof(sForestPalette));
    LoadBgTiles(1, tiles, sizeof(sForestTiles), 0);
    LoadBgTilemap(1, sForestMap, sizeof(sForestMap), 0);
}
