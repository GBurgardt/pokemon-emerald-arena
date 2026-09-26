// Side-wide protection retains Emerald's physical/special damage calculation.
// Five native turns become ten seconds of active arena time; menus/hitstop pause it.
#define BARRIER_TAG 0xA860
#define BARRIER_LIFE 600
static EWRAM_DATA u16 sBarrierLife[2][2]={};
static EWRAM_DATA u8 sBarrierSprite[2]={},sBarrierFrame[2]={};
EWRAM_DATA u16 gArenaBarrierTelemetry[8]={}; // casts, expiries, allocations, reserved
static const u16 sBarrierFlag[2]={SIDE_STATUS_REFLECT,SIDE_STATUS_LIGHTSCREEN};
static const struct OamData sBarrierOam={.shape=SPRITE_SHAPE(64x64),.size=SPRITE_SIZE(64x64),.priority=0};
static const u32 sBarrierFrames[]=INCBIN_U32("graphics/arena/barriers/effect.4bpp");
static const u16 sBarrierPalette[16]={RGB_BLACK,
    RGB(3,2,6),RGB(8,3,13),RGB(15,6,20),RGB(22,10,25),RGB(28,16,29),RGB(31,24,30),RGB_WHITE,
    RGB(2,5,7),RGB(3,10,13),RGB(5,17,20),RGB(8,25,26),RGB(20,30,28),RGB(31,28,15),RGB(31,31,28),RGB_BLACK};
static void BarrierReset(void)
{
    u32 side,kind;
    memset(gArenaBarrierTelemetry,0,sizeof(gArenaBarrierTelemetry));
    for(side=0;side<2;side++)
    {
        sBarrierSprite[side]=MAX_SPRITES;sBarrierFrame[side]=255;
        for(kind=0;kind<2;kind++)sBarrierLife[side][kind]=(gSideStatuses[side]&sBarrierFlag[kind])?BARRIER_LIFE:0;
    }
}
static void BarrierStart(u8 side,u16 move)
{
    u8 kind=move==MOVE_LIGHT_SCREEN;
    if(sBarrierLife[side][kind])return;
    gSideStatuses[side]|=sBarrierFlag[kind];
    if(kind){gSideTimers[side].lightscreenTimer=5;gSideTimers[side].lightscreenBattlerId=side;}
    else {gSideTimers[side].reflectTimer=5;gSideTimers[side].reflectBattlerId=side;}
    sBarrierLife[side][kind]=BARRIER_LIFE;gArenaBarrierTelemetry[side]++;
    PlaySE(SE_M_REFLECT);
}
static void BarrierTick(void)
{
    u32 side,kind;
    for(side=0;side<2;side++)for(kind=0;kind<2;kind++)
        if(sBarrierLife[side][kind]&&!--sBarrierLife[side][kind])
        {
            gSideStatuses[side]&=~sBarrierFlag[kind];
            if(kind)gSideTimers[side].lightscreenTimer=0;
            else gSideTimers[side].reflectTimer=0;
            gArenaBarrierTelemetry[2+side]++;
        }
}
static void BarrierHide(void)
{
    u32 side;
    for(side=0;side<2;side++)if(sBarrierSprite[side]<MAX_SPRITES)gSprites[sBarrierSprite[side]].invisible=TRUE;
}
static void BarrierDraw(void)
{
    u32 side;
    for(side=0;side<2;side++)
    {
        struct ArenaBody*b=&sArena.bodies[side];
        struct Sprite*p;
        u8 kind=sBarrierLife[side][1]&&(!sBarrierLife[side][0]||(sArena.frame/48)%2);
        u16 life=sBarrierLife[side][kind];
        u8 frame=kind*8+(sArena.frame/6)%8;
        if(!life||!gBattleMons[side].hp)
        {if(sBarrierSprite[side]<MAX_SPRITES)gSprites[sBarrierSprite[side]].invisible=TRUE;continue;}
        if(sBarrierSprite[side]==MAX_SPRITES)
        {
            struct SpriteSheet sheet={sBarrierFrames,2048,BARRIER_TAG+side};
            struct SpritePalette palette={sBarrierPalette,BARRIER_TAG};
            struct SpriteTemplate t=sMonTemplate;
            if(LoadSpriteSheet(&sheet)==0xFFFF)continue;
            t.tileTag=BARRIER_TAG+side;t.paletteTag=SHOT_TAG;t.oam=&sBarrierOam;
            sBarrierSprite[side]=CreateSprite(&t,0,0,0);
            if(sBarrierSprite[side]==MAX_SPRITES)continue;
            gArenaBarrierTelemetry[4+side]++;
            if(IndexOfSpritePaletteTag(BARRIER_TAG)==0xFF)LoadSpritePalette(&palette);
        }
        p=&gSprites[sBarrierSprite[side]];
        p->invisible=sArena.paused||gSprites[b->sprite].invisible||(life<60&&(life&4));
        p->x=b->x/Q;p->y=b->y/Q-8;
        p->oam.paletteNum=IndexOfSpritePaletteTag(BARRIER_TAG);
        if(p->oam.paletteNum==15&&IndexOfSpritePaletteTag(BARRIER_TAG)==0xFF)
            p->oam.paletteNum=IndexOfSpritePaletteTag(0xA743);
        if(sBarrierFrame[side]!=frame)
        {
            ArenaRender_Copy(sBarrierFrames+frame*512,(u8*)OBJ_VRAM0+p->oam.tileNum*32,2048);
            sBarrierFrame[side]=frame;
        }
    }
}
