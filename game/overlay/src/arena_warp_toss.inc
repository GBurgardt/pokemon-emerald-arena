// Native moves; visuals never own HP, party data or rewards.
#define WARP_FX_TAG 0xA820
struct ArenaWarp {s16 ox,oy,nx,ny;u8 age,active;};
struct ArenaDive {s16 ox,oy,sx,sy;u16 age;u8 active,fly;};
static EWRAM_DATA struct ArenaDive sDive[2]={};
// dig starts, fly starts, underground/air evades, impacts, misses, safe-position rejects
EWRAM_DATA u32 gArenaDiveTelemetry[6]={};
static EWRAM_DATA struct ArenaWarp sWarp[2]={};
static EWRAM_DATA u8 sSpecialShade=0;
static EWRAM_DATA struct {s16 x,y;u8 sprite,frame,life,kind;} sWarpFx[2]={};
// teleports, rejected destinations, grabs, missed grabs, impacts, finishing hits,
// allocation failures, damage avoided during teleport.
EWRAM_DATA u32 gArenaWarpTossTelemetry[8]={};
static const u32 sWarpTossFrames[]=INCBIN_U32("graphics/arena/warp-toss/effect.4bpp");
static const u16 sWarpPalette[]=INCBIN_U16("graphics/arena/psychic/palette.gbapal");
static const struct OamData sWarpFxOam={
    .affineMode=ST_OAM_AFFINE_DOUBLE,.shape=SPRITE_SHAPE(64x64),
    .size=SPRITE_SIZE(64x64),.priority=1
};

static bool8 SpecialBusy(u8 side){return TossActive()||sWarp[side].active||sDive[side].active;}
static bool8 SpecialInvulnerable(u8 side)
{
    if(sDive[side].active&&sDive[side].age>=10&&sDive[side].age<(sDive[side].fly?120:92))
    {gArenaDiveTelemetry[2]++;return TRUE;}
    return TossAirborne(side)||(sWarp[side].active&&sWarp[side].age<12);
}
static void SpecialReset(void)
{
    u32 i;
    memset(sWarp,0,sizeof(sWarp));memset(&sArena.toss,0,sizeof(sArena.toss));
    memset(sDive,0,sizeof(sDive));
    sSpecialShade=0;
    for(i=0;i<2;i++){sWarpFx[i].sprite=MAX_SPRITES;sWarpFx[i].frame=255;sWarpFx[i].life=0;}
}
static void SpecialHide(void)
{
    u32 i;
    for(i=0;i<2;i++)if(sWarpFx[i].sprite<MAX_SPRITES)gSprites[sWarpFx[i].sprite].invisible=TRUE;
    if(TossActive())TossEnd();
    sWarp[0].active=sWarp[1].active=FALSE;
    sDive[0].active=sDive[1].active=FALSE;
    BlendPalette(16,240,0,RGB_BLACK);sSpecialShade=0;
    SetGpuReg(REG_OFFSET_BG1HOFS,0);SetGpuReg(REG_OFFSET_BG1VOFS,0);
}
static void SpecialFx(u8 side,u8 kind,s16 x,s16 y)
{
    if(sWarpFx[side].sprite==MAX_SPRITES)
    {
        struct SpriteSheet sheet={sWarpTossFrames,2048,WARP_FX_TAG+side};
        struct SpriteTemplate t=sMonTemplate;
        t.tileTag=WARP_FX_TAG+side;t.paletteTag=SHOT_TAG;t.oam=&sWarpFxOam;
        if(LoadSpriteSheet(&sheet)!=0xFFFF&&ArenaTerrain_ReserveProjectile())
            sWarpFx[side].sprite=CreateSprite(&t,x,y,0);
        if(sWarpFx[side].sprite==MAX_SPRITES){gArenaWarpTossTelemetry[6]++;return;}
        gSprites[sWarpFx[side].sprite].animPaused=TRUE;
        gSprites[sWarpFx[side].sprite].affineAnimPaused=TRUE;
        gSprites[sWarpFx[side].sprite].affineAnimBeginning=FALSE;
    }
    sWarpFx[side].kind=kind;sWarpFx[side].x=x;sWarpFx[side].y=y;
    sWarpFx[side].frame=255;sWarpFx[side].life=kind>=3?200:kind==1?180:48;
    if(!kind)LoadPalette(sWarpPalette+1,OBJ_PLTT_ID(IndexOfSpritePaletteTag(SHOT_TAG))+5,16);
}
static bool8 WarpStart(u8 side)
{
    struct ArenaBody*b=&sArena.bodies[side],*other=&sArena.bodies[side^1];
    struct ArenaWarp*w=&sWarp[side];
    s32 dx=b->attackX,dy=b->attackY; s16 d;
    if(!side&&!b->manualAim){dx=-dx;dy=-dy;}
    if(side){dx=-dx;dy=-dy;}
    w->ox=b->x/Q;w->oy=b->y/Q;
    for(d=68;d>=24;d-=4)
    {
        s16 nx=w->ox+dx*d/Q,ny=w->oy+dy*d/Q;
        // A teleport may cross cover, but can never end inside it or a body.
        if(ArenaNav_CanStand(nx,ny)&&ArenaNav_Distance(nx,ny,other->x/Q,other->y/Q)>=24)
        {w->nx=nx;w->ny=ny;break;}
    }
    if(d<24){gArenaWarpTossTelemetry[1]++;return FALSE;}
    w->age=0;w->active=TRUE;b->dash=0;b->knockX=b->knockY=0;
    SpecialFx(side,0,w->ox,w->oy-8);gArenaWarpTossTelemetry[0]++;
    PlaySE(SE_M_TELEPORT);return TRUE;
}
static bool8 TossStart(u8 side)
{
    struct ArenaBody*b=&sArena.bodies[side],*t=&sArena.bodies[side^1];
    s16 tx=DecoyAim(side^1,FALSE)/Q,ty=DecoyAim(side^1,TRUE)/Q;
    if(TossActive()||sDive[side^1].active||SpecialInvulnerable(side^1)||t->dash
        ||!ArenaMoves_InCone(tx-b->x/Q,ty-b->y/Q,b->attackX,b->attackY,48,1)
        ||!ArenaNav_LineClear(b->x/Q,b->y/Q,tx,ty,2))
    {gArenaWarpTossTelemetry[3]++;ArenaFeedback_Wall(b->x/Q,b->y/Q);return FALSE;}
    if(sDecoys[side^1].life&&sDecoys[side^1].age>=24)
    {DecoyBreak(side^1);gArenaWarpTossTelemetry[3]++;return FALSE;}
    if(CoverSegment(side,MOVE_SEISMIC_TOSS,b->x/Q,b->y/Q,t->x/Q,t->y/Q,8))return FALSE;
    TossBegin(side);
    return TossActive();
}
#include "arena_dig_fly.inc"

static void SpecialTick(void)
{
    u32 i;
    for(i=0;i<2;i++)
    {
        struct ArenaWarp*w=&sWarp[i];
        // Leave the most recent crater until another special effect uses
        // this side's bounded sprite bank, or the battle ends.
        if(sWarpFx[i].life && !(sWarpFx[i].kind==1&&sWarpFx[i].life==1))sWarpFx[i].life--;
        if(!w->active)continue;
        w->age++;
        if(w->age==8)
        {
            sArena.bodies[i].x=w->nx*Q;sArena.bodies[i].y=w->ny*Q;
            sWarpFx[i].x=w->nx;sWarpFx[i].y=w->ny-8;
            sArena.thinkTimer=1;sArena.goalTimer=0;
        }
        if(w->age==24)w->active=FALSE;
    }
    if(TossActive())TossTick();
    else DiveTick();
}
static void SpecialDraw(void)
{
    u32 side;
    s16 shake=0;
    for(side=0;side<2;side++)
    {
        struct ArenaWarp*w=&sWarp[side];
        struct Sprite*mon=&gSprites[sArena.bodies[side].sprite];
        u8 id=sWarpFx[side].sprite,f=0;
        // Undo the throw's head-first rotation on every ordinary frame;
        // a Rollout in progress owns the matrix for its tumble.
        if(sArena.bodies[side].art&&!Rolling(side)&&!sArena.bodies[side].flat)SetOamMatrix(mon->oam.matrixNum,256,0,0,256);
        if(w->active)
        {
            mon->invisible=w->age>=5&&w->age<10;
            if(w->age<5)mon->x2=Sin(w->age*48,3);
            gSprites[sArena.bodies[side].shadow].invisible=mon->invisible;
        }
        if(id==MAX_SPRITES)continue;
        gSprites[id].invisible=!sWarpFx[side].life;
        if(!sWarpFx[side].life)continue;
        if(sWarpFx[side].kind==0)f=w->active?(w->age<8?w->age:8+min(7,(w->age-8)/2)):15;
        if(sWarpFx[side].kind==1)f=16+min(15,(180-sWarpFx[side].life)/3);
        if(sWarpFx[side].kind==1&&sWarpFx[side].life>168)shake=(sWarpFx[side].life&2)?2:-2;
        if(sWarpFx[side].kind==2)f=32+((48-sWarpFx[side].life)/4)%8;
        if(sWarpFx[side].kind>=3)f=DiveFrame(side);
        gSprites[id].x=sWarpFx[side].x;gSprites[id].y=sWarpFx[side].y-(sWarpFx[side].kind==1?8:0);
        gSprites[id].oam.paletteNum=IndexOfSpritePaletteTag(sWarpFx[side].kind?0xA790:SHOT_TAG);
        gSprites[id].oam.priority=sWarpFx[side].kind==1?1:0;
        {
            s16 scale=sWarpFx[side].kind==2?128:sWarpFx[side].kind==1?208:256;
            SetOamMatrix(gSprites[id].oam.matrixNum,scale,0,0,scale);
        }
        if(sWarpFx[side].frame!=f)
        {
            const u32*frames=sWarpFx[side].kind>=3?sDiveFrames:sWarpTossFrames;
            ArenaRender_Copy(frames+f*512,(u8*)OBJ_VRAM0+GetSpriteTileStartByTag(WARP_FX_TAG+side)*32,2048);
            sWarpFx[side].frame=f;
        }
    }
    SetGpuReg(REG_OFFSET_BG1HOFS,shake);SetGpuReg(REG_OFFSET_BG1VOFS,shake/2);
    DiveDraw();
    if(TossActive())
        for(side=0;side<2;side++)
        {
            struct Sprite *mon=&gSprites[sArena.bodies[side].sprite];
            if(TossPresent(side,mon))mon->invisible=TRUE;
            TossAffine(side,mon);
        }
    TossDraw();
}
