// Presentation-only send-out. Never changes party, PP, HP, bag or battle RNG.
#define SENDOUT_TAG 0xA7D0
#define SENDOUT_DURATION 144
static const u32 sSendoutFrames[] = INCBIN_U32("graphics/arena/sendout/burst.4bpp");
static const u16 sSendoutPalette[] = INCBIN_U16("graphics/arena/sendout/palette.gbapal");
static EWRAM_DATA struct {
    u16 age, palette[16];
    u8 active, ball[2], fx[2], bank, frame[2];
} sSendout = {};
// active, age, launches, materializations, skipped, trainer battle.
EWRAM_DATA u16 gArenaSendoutTelemetry[6] = {};
static const struct OamData sSendoutBallOam = {
    .shape=SPRITE_SHAPE(16x16),.size=SPRITE_SIZE(16x16),.priority=0
};
static const struct OamData sSendoutFxOam = {
    .shape=SPRITE_SHAPE(64x64),.size=SPRITE_SIZE(64x64),.priority=0
};

static void SendoutFinish(void)
{
    u32 i;
    for(i=0;i<2;i++)
    {
        struct ArenaBody *b=&sArena.bodies[i];
        struct Sprite *p=&gSprites[b->sprite];
        u8 id=gArenaTrainerTelemetry[i];
        p->invisible=FALSE;p->x2=p->y2=0;
        gSprites[b->shadow].invisible=FALSE;
        SetOamMatrix(p->oam.matrixNum,b->art?256:512,0,0,b->art?256:512);
        BlendPalette(OBJ_PLTT_ID(i),16,0,RGB_WHITE);
        if(sSendout.ball[i]<MAX_SPRITES)DestroySprite(&gSprites[sSendout.ball[i]]);
        if(sSendout.fx[i]<MAX_SPRITES)DestroySprite(&gSprites[sSendout.fx[i]]);
        FreeSpriteTilesByTag(SENDOUT_TAG+i);
        if(id<MAX_SPRITES)
        {
            struct Sprite *t=&gSprites[id];
            t->x2=t->y2=0;t->oam.shape=SPRITE_SHAPE(16x32);t->oam.size=SPRITE_SIZE(16x32);
            t->oam.tileNum=GetSpriteTileStartByTag(TRAINER_TAG+i);t->hFlip=!i;
            CalcCenterToCornerVec(t,t->oam.shape,t->oam.size,t->oam.affineMode);
            RefreshTrainerFacing(t);
        }
    }
    if(sSendout.bank<16)LoadPalette(sSendout.palette,OBJ_PLTT_ID(sSendout.bank),32);
    BlendPalette(16,240,0,RGB_BLACK);
    sCapture.trainerFrame=255;
    sArena.attackBuffer=sArena.dashBuffer=0;
    sSendout.active=FALSE;gArenaSendoutTelemetry[0]=0;
    sArena.hudDirty=TRUE;
}

static void SendoutInit(void)
{
    u32 i;
    memset(&sSendout,0,sizeof(sSendout));
    memset(gArenaSendoutTelemetry,0,sizeof(gArenaSendoutTelemetry));
    for(i=0;i<2;i++)sSendout.ball[i]=sSendout.fx[i]=MAX_SPRITES;
    sSendout.bank=IndexOfSpritePaletteTag(SHOT_TAG);
    if(sSendout.bank>=16||GetSpriteTileStartByTag(CAPTURE_TAG)==0xFFFF)return;
    memcpy(sSendout.palette,&gPlttBufferUnfaded[OBJ_PLTT_ID(sSendout.bank)],32);
    LoadPalette(sSendoutPalette,OBJ_PLTT_ID(sSendout.bank)+5,14);
    gArenaSendoutTelemetry[5]=(gBattleTypeFlags&BATTLE_TYPE_TRAINER)!=0;
    for(i=0;i<2;i++)
    {
        struct ArenaBody *b=&sArena.bodies[i];
        struct SpriteTemplate ball={.tileTag=CAPTURE_TAG,.paletteTag=SHOT_TAG+1,
            .oam=&sSendoutBallOam,.anims=gDummySpriteAnimTable,.images=NULL,
            .affineAnims=gDummySpriteAffineAnimTable,.callback=SpriteCallbackDummy};
        struct SpriteTemplate fx=ball;
        struct SpriteSheet sheet={sSendoutFrames,2048,SENDOUT_TAG+i};
        sSendout.frame[i]=255;
        // Wild opponents are already in their habitat; never throw their ball.
        if(i&&!gArenaSendoutTelemetry[5])continue;
        fx.tileTag=SENDOUT_TAG+i;fx.paletteTag=SHOT_TAG;fx.oam=&sSendoutFxOam;
        if(LoadSpriteSheet(&sheet)==0xFFFF)continue;
        sSendout.ball[i]=CreateSprite(&ball,0,0,0);
        sSendout.fx[i]=CreateSprite(&fx,b->x/Q,b->y/Q-8,0);
        if(sSendout.ball[i]<MAX_SPRITES)gSprites[sSendout.ball[i]].invisible=TRUE;
        if(sSendout.fx[i]<MAX_SPRITES)gSprites[sSendout.fx[i]].invisible=TRUE;
        gSprites[b->sprite].invisible=TRUE;
        gSprites[b->shadow].invisible=TRUE;
    }
    // Prime the correct idle direction before a monster becomes visible.
    for(i=0;i<2;i++)if(sArena.bodies[i].art)
    {
        struct ArenaBody *b=&sArena.bodies[i];
        ArenaSprites_Decode(&b->art->animations[ARENA_ANIM_IDLE],b->facing,0,sMonFrameTiles[i]);
        ArenaRender_Copy(sMonFrameTiles[i],(u8*)OBJ_VRAM0+GetSpriteTileStartByTag(MON_TAG+i*2)*32,2048);
        b->drawnFrame=0;b->drawnDirection=b->facing;
        gSprites[b->sprite].y=b->y/Q+8-b->art->ground[ARENA_ANIM_IDLE*8+b->facing]*MonVisualScale(i)/256;
    }
    sSendout.active=TRUE;gArenaSendoutTelemetry[0]=1;
}

static void SendoutTick(void)
{
    u32 i;
    u16 age=++sSendout.age;
    gArenaSendoutTelemetry[1]=age;
    // Start from a fresh matrix every frame, including wild opponents which
    // skip the throw animation. Presentation scaling must not accumulate.
    for(i=0;i<2;i++)
    {
        struct ArenaBody *b=&sArena.bodies[i];
        SetOamMatrix(gSprites[b->sprite].oam.matrixNum,b->art?256:512,0,0,b->art?256:512);
    }
    // Brief stage lighting, restored from the original palette, never a strobe.
    BlendPalette(16,240,age<16?age/4:age<100?4:age<144?(144-age)/11:0,RGB_BLACK);
    if(age>=SENDOUT_DURATION||(age>20&&JOY_NEW(B_BUTTON)))
    {
        if(age<SENDOUT_DURATION)gArenaSendoutTelemetry[4]=1;
        SendoutFinish();
    }
    else for(i=0;i<2;i++)
    {
        s16 t=age-(i?10:0), flight=t-18, burst=t-48;
        struct ArenaBody *b=&sArena.bodies[i];
        struct Sprite *p=&gSprites[b->sprite];
        u8 trainer=gArenaTrainerTelemetry[i];
        s16 sx=i?223:17, sy=i?49:113;
        if(i&&!gArenaSendoutTelemetry[5])continue;
        if(trainer<MAX_SPRITES)
        {
            struct Sprite *tr=&gSprites[trainer];
            tr->x2=t<18?(i?1:-1)*(t>8?2:1):t<30?(i?-3:3):0;
            tr->y2=t>8&&t<18?1:0;
            if(!i&&t>=6&&t<36)
            {
                const struct ObjectEventGraphicsInfo *info=GetObjectEventGraphicsInfo(
                    gSaveBlock2Ptr->playerGender?OBJ_EVENT_GFX_MAY_FIELD_MOVE:OBJ_EVENT_GFX_BRENDAN_FIELD_MOVE);
                u8 pose=t<18?1:t<24?2:t<30?3:4;
                tr->oam.shape=SPRITE_SHAPE(32x32);tr->oam.size=SPRITE_SIZE(32x32);
                tr->oam.tileNum=GetSpriteTileStartByTag(CAPTURE_TAG+1);tr->hFlip=FALSE;
                ArenaRender_Copy(info->images[pose].data,(u8*)OBJ_VRAM0+tr->oam.tileNum*32,512);
                CalcCenterToCornerVec(tr,tr->oam.shape,tr->oam.size,tr->oam.affineMode);
                RefreshTrainerFacing(tr);
            }
            if(!i&&t==36)
            {
                tr->oam.shape=SPRITE_SHAPE(16x32);tr->oam.size=SPRITE_SIZE(16x32);
                tr->oam.tileNum=GetSpriteTileStartByTag(TRAINER_TAG);tr->hFlip=TRUE;
                CalcCenterToCornerVec(tr,tr->oam.shape,tr->oam.size,tr->oam.affineMode);
                RefreshTrainerFacing(tr);
            }
        }
        if(t==18){PlaySE(SE_BALL_THROW);gArenaSendoutTelemetry[2]++;}
        if(sSendout.ball[i]<MAX_SPRITES)
        {
            struct Sprite *ball=&gSprites[sSendout.ball[i]];
            ball->invisible=flight<0||flight>=30;
            if(flight>=0&&flight<30)
            {
                ball->x=sx+(b->x/Q-sx)*flight/30;
                ball->y=sy+(b->y/Q+(i?12:-8)-sy)*flight/30-Sin(flight*128/30,26);
                ball->hFlip=(flight/4)&1;ball->vFlip=(flight/8)&1;
                ball->oam.matrixNum=(ball->hFlip?8:0)|(ball->vFlip?16:0);
            }
        }
        if(burst==0){PlaySE(SE_BALL_OPEN);gArenaSendoutTelemetry[3]++;}
        if(sSendout.fx[i]<MAX_SPRITES)
        {
            struct Sprite *fx=&gSprites[sSendout.fx[i]];
            u8 frame=burst>=0?burst/3:0;
            fx->y=b->y/Q+(i?12:-8)+(burst>24?Clamp((burst-24)*2,0,i?0:16):0);
            fx->invisible=burst<0||frame>=24;
            if(!fx->invisible&&frame!=sSendout.frame[i])
            {
                ArenaRender_Copy(sSendoutFrames+frame*512,(u8*)OBJ_VRAM0+fx->oam.tileNum*32,2048);
                sSendout.frame[i]=frame;
            }
        }
        p->invisible=burst<14;
        if(burst>=14)
        {
            s16 scale=burst<34?768-(burst-14)*512/20:burst<40?230:256;
            if(!b->art)scale*=2;
            SetOamMatrix(p->oam.matrixNum,scale,0,0,scale);
            p->y2=burst<34?(i?1:-1)*(34-burst)/2:burst<40?2:0;
            BlendPalette(OBJ_PLTT_ID(i),16,burst<36?Clamp(36-burst,0,16):0,RGB_WHITE);
            gSprites[b->shadow].invisible=FALSE;
        }
    }
    ArenaTerrain_Draw(FALSE,TRUE);
    MonVisualScaleApply();
    AnimateSprites();BuildOamBuffer();ArenaRender_Ready();UpdatePaletteFade();
}
