// Private arena presentation. Included by realtime_arena.c after its helpers.
// Native Pokemon, bag, capture RNG and save systems remain authoritative.
#define CAPTURE_TAG 0xA7B0
#define CAPTURE_FLIGHT 24
struct CaptureState {
    u8 state, sprite, shadow, marker, shakes, message, ready, manual;
    u8 trainerFrame, hudMessage, hudVisible, hudReady, heldLatch;
    u16 hudStock;
    u16 age, notice;
    s16 aimX, aimY, startX, startY, velocityX, velocityY;
    s32 previousX, previousY;
};
static EWRAM_DATA struct CaptureState sCapture = {};
EWRAM_DATA struct ArenaCaptureTelemetry gArenaCaptureTelemetry = {};
static const u8 sCaptureHint[] = _("L+R: AIM BALL   RELEASE: THROW");
static const u8 sCaptureAim[] = _("RELEASE: THROW   B: CANCEL");
static const u8 sCaptureMiss[] = _("MISSED!");
static const u8 sCaptureBreak[] = _("BROKE FREE!");
static const u8 sCaptureParty[] = _("CAUGHT!  ADDED TO YOUR PARTY");
static const u8 sCapturePC[] = _("CAUGHT!  SENT TO YOUR PC");
static const u8 sCaptureEmpty[] = _("NO POKé BALLS");
static const u8 sCaptureFull[] = _("PARTY AND PC ARE FULL");
static const u8 sCaptureWait[] = _("...");
static const u8 sCaptureCount[] = _("BALLS: ");
static const u8 *const sCaptureMessages[] = {
    sCaptureHint, sCaptureAim, sCaptureMiss, sCaptureBreak,
    sCaptureParty, sCapturePC, sCaptureEmpty, sCaptureFull, sCaptureWait
};
static const struct OamData sCaptureOam = {
    .affineMode=ST_OAM_AFFINE_NORMAL,
    .shape=SPRITE_SHAPE(16x16), .size=SPRITE_SIZE(16x16), .priority=0
};
static void DestroyShot(struct ArenaShot *shot);

static void CaptureMessage(u8 message, u16 frames)
{
    sCapture.message=message;sCapture.notice=frames;
    sArena.hudDirty=TRUE;
}

static void CaptureHud(void)
{
    u8 text[20];
    bool8 visible=sCapture.notice && !sArena.paused;
    u16 stock=CountTotalItemQuantityInBag(ITEM_POKE_BALL);
    if(sCapture.hudReady && sCapture.hudVisible==visible
        && (!visible || (sCapture.hudMessage==sCapture.message && sCapture.hudStock==stock)))return;
    sCapture.hudReady=TRUE;sCapture.hudVisible=visible;
    sCapture.hudMessage=sCapture.message;sCapture.hudStock=stock;
    if (!visible) ClearWindowTilemap(2);
    else
    {
        CpuFastFill(0x99999999,gWindows[2].tileData,30*2*32);
        PrintWindow(2,5,1,sCaptureMessages[sCapture.message],4);
        ConvertIntToDecimalStringN(StringCopy(text,sCaptureCount),
            CountTotalItemQuantityInBag(ITEM_POKE_BALL),STR_CONV_MODE_LEFT_ALIGN,3);
        PrintWindow(2,185,1,text,7);
        PutWindowTilemap(2);
    }
    CopyWindowToVram(2,COPYWIN_FULL);
}

static void CaptureSetState(u8 state)
{
    sCapture.state=state;sCapture.age=0;
    gArenaCaptureTelemetry.state=state;
}

static void CaptureInit(void)
{
    // Original Emerald ball pixels, losslessly index-remapped into UNUSED
    // colors of the enemy cue palette. All 16 GBA OBJ banks are already used.
    u32 tiles[96];
    u16 palette[16];
    u32 i;
    u8 *pixels=(u8*)tiles;
    u8 pal=IndexOfSpritePaletteTag(SHOT_TAG+1);
    struct SpriteSheet sheet={tiles,sizeof(tiles),CAPTURE_TAG};
    struct SpriteTemplate template={
        .tileTag=CAPTURE_TAG,.paletteTag=SHOT_TAG+1,.oam=&sCaptureOam,
        .anims=gDummySpriteAnimTable,.images=NULL,
        .affineAnims=gDummySpriteAffineAnimTable,.callback=SpriteCallbackDummy
    };
    memset(&sCapture,0,sizeof(sCapture));
    memset(&gArenaCaptureTelemetry,0,sizeof(gArenaCaptureTelemetry));
    sCapture.sprite=sCapture.shadow=sCapture.marker=MAX_SPRITES;
    sCapture.trainerFrame=255;
    // A practice encounter may start while the shoulders are still held.
    // Require a fresh press then; never spend a ball just for entering battle.
    sCapture.heldLatch=JOY_HELD(L_BUTTON|R_BUTTON)==(L_BUTTON|R_BUTTON);
    LZDecompressWram(gBallGfx_Poke,tiles);
    LZDecompressWram(gBallPal_Poke,palette);
    for(i=0;i<sizeof(tiles);i++)
    {
        u8 a=pixels[i]&15,b=pixels[i]>>4;
        // The native asset uses exactly indices 0..7. Preserve transparency.
        if(a>7||b>7)return;
        pixels[i]=(a?a+2:0)|((b?b+2:0)<<4);
    }
    if(pal==0xFF || LoadSpriteSheet(&sheet)==0xFFFF)return;
    LoadPalette(palette+1,OBJ_PLTT_ID(pal)+3,14);
    sCapture.sprite=CreateSprite(&template,8,110,0);
    sCapture.shadow=CreateSprite(&sShadowTemplate,8,118,10);
    sCapture.marker=CreateSprite(&sAimTemplate,120,40,0);
    sCapture.ready=sCapture.sprite!=MAX_SPRITES&&sCapture.shadow!=MAX_SPRITES&&sCapture.marker!=MAX_SPRITES
        && !(gBattleTypeFlags & (BATTLE_TYPE_FIRST_BATTLE | BATTLE_TYPE_TRAINER));
    sCapture.previousX=sArena.bodies[1].x;sCapture.previousY=sArena.bodies[1].y;
    if(sCapture.sprite!=MAX_SPRITES)
    {
        gSprites[sCapture.sprite].invisible=TRUE;
        gSprites[sCapture.sprite].affineAnimPaused=TRUE;
        gSprites[sCapture.sprite].affineAnimBeginning=FALSE;
        SetOamMatrix(gSprites[sCapture.sprite].oam.matrixNum,256,0,0,256);
    }
    if(sCapture.shadow!=MAX_SPRITES)gSprites[sCapture.shadow].invisible=TRUE;
    if(sCapture.marker!=MAX_SPRITES)gSprites[sCapture.marker].invisible=TRUE;
    gArenaCaptureTelemetry.ballSprite=sCapture.sprite;
    gArenaCaptureTelemetry.balls=CountTotalItemQuantityInBag(ITEM_POKE_BALL);
    {
        const struct ObjectEventGraphicsInfo *info=GetObjectEventGraphicsInfo(
            gSaveBlock2Ptr->playerGender?OBJ_EVENT_GFX_MAY_FIELD_MOVE:OBJ_EVENT_GFX_BRENDAN_FIELD_MOVE);
        struct SpriteSheet trainerSheet={info->images[0].data,512,CAPTURE_TAG+1};
        LoadSpriteSheet(&trainerSheet);
    }
    if(sCapture.ready)CaptureMessage(0,180);
}

static bool8 CaptureLocked(void)
{
    return sCapture.state>=ARENA_CAPTURE_ABSORB && sCapture.state<=ARENA_CAPTURE_CAUGHT;
}

static void CaptureReset(void)
{
    struct Sprite *enemy=&gSprites[sArena.bodies[1].sprite];
    u16 scale=sArena.bodies[1].art?256:512;
    enemy->x2=enemy->y2=0;
    SetOamMatrix(enemy->oam.matrixNum,scale,0,0,scale);
    if(sCapture.sprite!=MAX_SPRITES)gSprites[sCapture.sprite].invisible=TRUE;
    if(sCapture.sprite!=MAX_SPRITES)gSprites[sCapture.sprite].x2=gSprites[sCapture.sprite].y2=0;
    if(sCapture.shadow!=MAX_SPRITES)gSprites[sCapture.shadow].invisible=TRUE;
    if(sCapture.marker!=MAX_SPRITES)gSprites[sCapture.marker].invisible=TRUE;
    if(gArenaTrainerTelemetry[0]!=MAX_SPRITES)gSprites[gArenaTrainerTelemetry[0]].x2=0;
    CaptureSetState(ARENA_CAPTURE_IDLE);
    sArena.attackBuffer=sArena.dashBuffer=0;
    sArena.bodies[0].moving=FALSE;
}

static bool8 CaptureCanThrow(void)
{
    if(!CheckBagHasItem(ITEM_POKE_BALL,1))
    {gArenaCaptureTelemetry.empty++;CaptureMessage(6,90);return FALSE;}
    if(IsPlayerPartyAndPokemonStorageFull())
    {gArenaCaptureTelemetry.full++;CaptureMessage(7,90);return FALSE;}
    return TRUE;
}

static void CaptureInput(void)
{
    struct ArenaBody *enemy=&sArena.bodies[1],*player=&sArena.bodies[0];
    bool8 held=JOY_HELD(L_BUTTON|R_BUTTON)==(L_BUTTON|R_BUTTON);
    if(!held)sCapture.heldLatch=FALSE;
    if(sCapture.notice && sCapture.state==ARENA_CAPTURE_IDLE && !sArena.paused)
        if(!--sCapture.notice)sArena.hudDirty=TRUE;
    if(sArena.paused || sArena.resultTimer || !sCapture.ready)return;
    if(sCapture.state==ARENA_CAPTURE_IDLE && held && !sCapture.heldLatch
        && !player->shotTimer && !player->actionLife && !player->dash)
    {
        sCapture.heldLatch=TRUE;
        if(!CaptureCanThrow())return;
        CaptureSetState(ARENA_CAPTURE_AIM);
        sCapture.aimX=enemy->x/Q;sCapture.aimY=enemy->y/Q;
        sCapture.manual=FALSE;
        sArena.attackBuffer=sArena.dashBuffer=0;player->moving=FALSE;
        CaptureMessage(1,1);
        PlaySE(SE_SELECT);
    }
    if(sCapture.state!=ARENA_CAPTURE_AIM)return;
    if(JOY_NEW(B_BUTTON|START_BUTTON|SELECT_BUTTON))
    {
        gArenaCaptureTelemetry.cancelled++;CaptureReset();CaptureMessage(0,90);return;
    }
    if(held)
    {
        s16 dx=(JOY_HELD(DPAD_RIGHT)!=0)-(JOY_HELD(DPAD_LEFT)!=0);
        s16 dy=(JOY_HELD(DPAD_DOWN)!=0)-(JOY_HELD(DPAD_UP)!=0);
        if(dx||dy)sCapture.manual=TRUE;
        if(!sCapture.manual){sCapture.aimX=enemy->x/Q;sCapture.aimY=enemy->y/Q;}
        sCapture.aimX=Clamp(sCapture.aimX+dx*2,ARENA_MIN_X,ARENA_MAX_X);
        sCapture.aimY=Clamp(sCapture.aimY+dy*2,ARENA_MIN_Y,ARENA_MAX_Y);
        return;
    }
    if(!CaptureCanThrow() || !RemoveBagItem(ITEM_POKE_BALL,1)){CaptureReset();return;}
    gArenaCaptureTelemetry.throws++;
    gArenaCaptureTelemetry.balls=CountTotalItemQuantityInBag(ITEM_POKE_BALL);
    gLastUsedItem=ITEM_POKE_BALL;
    if(gBattleResults.catchAttempts[ITEM_POKE_BALL-ITEM_ULTRA_BALL]<255)
        gBattleResults.catchAttempts[ITEM_POKE_BALL-ITEM_ULTRA_BALL]++;
    // Small assist only near the actual target; never bend a deliberate miss.
    if(ArenaNav_Distance(sCapture.aimX,sCapture.aimY,enemy->x/Q,enemy->y/Q)<18)
    {
        // Flight retains aiming's 1/3 cadence: 24 video frames, 8 world ticks.
        sCapture.aimX=Clamp(enemy->x/Q+Clamp(sCapture.velocityX*8/Q,-16,16),ARENA_MIN_X,ARENA_MAX_X);
        sCapture.aimY=Clamp(enemy->y/Q+Clamp(sCapture.velocityY*8/Q,-16,16),ARENA_MIN_Y,ARENA_MAX_Y);
    }
    sCapture.startX=14;sCapture.startY=108;
    CaptureSetState(ARENA_CAPTURE_THROW);
    CaptureMessage(8,1);
    PlaySE(SE_BALL_THROW);
}

static bool8 CaptureAward(void)
{
    struct Pokemon *mon=&gEnemyParty[gBattlerPartyIndexes[1]];
    u8 result;
    u16 ball=ITEM_POKE_BALL;
    // HP and status are native battle values, never a test-written reward.
    SetMonData(mon,MON_DATA_HP,&gBattleMons[1].hp);
    SetMonData(mon,MON_DATA_STATUS,&gBattleMons[1].status1);
    SetMonData(mon,MON_DATA_POKEBALL,&ball);
    result=GiveMonToPlayer(mon);
    if(result==MON_CANT_GIVE)return FALSE;
    gArenaCaptureTelemetry.destination=result;
    gBattleResults.caughtMonSpecies=GetMonData(mon,MON_DATA_SPECIES,NULL);
    GetMonData(mon,MON_DATA_NICKNAME,gBattleResults.caughtMonNick);
    gBattleResults.caughtMonBall=ITEM_POKE_BALL;
    if(!GetSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[1].species),FLAG_GET_CAUGHT))
        HandleSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[1].species),FLAG_SET_CAUGHT,gBattleMons[1].personality);
    IncrementGameStat(GAME_STAT_POKEMON_CAPTURES);
    gArenaCaptureTelemetry.caught++;
    CaptureMessage(result==MON_GIVEN_TO_PARTY?4:5,1);
    return TRUE;
}

static void CaptureTick(void)
{
    struct ArenaBody *enemy=&sArena.bodies[1];
    struct Sprite *ball;
    s32 t;
    u32 i;
    if(!sCapture.ready)return;
    ball=&gSprites[sCapture.sprite];
    if(enemy->x!=sCapture.previousX || enemy->y!=sCapture.previousY)
    {
        sCapture.velocityX=Clamp(enemy->x-sCapture.previousX,-2*Q,2*Q);
        sCapture.velocityY=Clamp(enemy->y-sCapture.previousY,-2*Q,2*Q);
    }
    sCapture.previousX=enemy->x;sCapture.previousY=enemy->y;
    if(sCapture.state==ARENA_CAPTURE_IDLE || sCapture.state==ARENA_CAPTURE_AIM || sArena.paused)return;
    if(sArena.resultTimer){CaptureReset();sCapture.notice=0;sArena.hudDirty=TRUE;return;}
    t=++sCapture.age;
    switch(sCapture.state)
    {
    case ARENA_CAPTURE_THROW:
        ball->x=sCapture.startX+(sCapture.aimX-sCapture.startX)*t/CAPTURE_FLIGHT;
        ball->y=sCapture.startY+(sCapture.aimY-sCapture.startY)*t/CAPTURE_FLIGHT;
        ball->y2=-8*t/CAPTURE_FLIGHT-4*26*t*(CAPTURE_FLIGHT-t)/(CAPTURE_FLIGHT*CAPTURE_FLIGHT);
        if(t==CAPTURE_FLIGHT)
        {
            if(ArenaNav_Distance(sCapture.aimX,sCapture.aimY,enemy->x/Q,enemy->y/Q)>22
                || ArenaNav_FirstObstacle(sCapture.aimX,sCapture.aimY,sCapture.aimX,sCapture.aimY,2)>=0)
            {
                gArenaCaptureTelemetry.misses++;CaptureSetState(ARENA_CAPTURE_MISS);CaptureMessage(2,75);PlaySE(SE_BALL_BOUNCE_1);
            }
            else
            {
                gArenaCaptureTelemetry.hits++;
                gArenaCaptureTelemetry.species=gBattleMons[1].species;
                gArenaCaptureTelemetry.hp=gBattleMons[1].hp;
                sCapture.shakes=CalculateBallCatchShakes(CalculateBallCatchOdds(1,ITEM_POKE_BALL),ITEM_POKE_BALL);
                gArenaCaptureTelemetry.shakes=sCapture.shakes;
                CaptureSetState(ARENA_CAPTURE_ABSORB);
                PlaySE(SE_BALL_OPEN);
                // No old projectile may hit a newly released Pokemon.
                for(i=0;i<SHOTS_COUNT;i++)if(sArena.shots[i].life)DestroyShot(&sArena.shots[i]);
                for(i=0;i<2;i++)
                {
                    sArena.bodies[i].shotTimer=sArena.bodies[i].actionLife=0;
                    sArena.bodies[i].moving=FALSE;
                    sArena.bodies[i].knockX=sArena.bodies[i].knockY=0;
                }
                sArena.hitstop=sArena.attackBuffer=sArena.dashBuffer=0;
            }
        }
        break;
    case ARENA_CAPTURE_ABSORB:
        if(t==1 || t==9)ArenaFeedback_CaptureGlow(ball->x,ball->y-8,TRUE);
        if(t==8)PlaySE(SE_BALL_TRADE);
        if(t>=24){CaptureSetState(ARENA_CAPTURE_DROP);PlaySE(SE_BALL);}
        break;
    case ARENA_CAPTURE_DROP:
        ball->y2=t<=16?-8+8*t/16:-4*(t-16)*(24-t)/16;
        if(t==16)PlaySE(SE_BALL_BOUNCE_1);
        if(t>=24){ball->y2=0;CaptureSetState(ARENA_CAPTURE_SHAKE);PlaySE(SE_BALL_BOUNCE_2);}
        break;
    case ARENA_CAPTURE_SHAKE:
        if(t==20 || t==54 || t==88)
        {
            u8 n=(t-20)/34;
            if(n>=sCapture.shakes)
            {CaptureSetState(ARENA_CAPTURE_BREAK);gArenaCaptureTelemetry.escaped++;PlaySE(SE_BALL_OPEN);CaptureMessage(3,90);}
            else PlaySE(SE_BALL);
        }
        if(t==122)
        {
            if(sCapture.shakes==4 && CaptureAward())
            {CaptureSetState(ARENA_CAPTURE_CAUGHT);PlaySE(SE_RG_BALL_CLICK);}
            else
            {CaptureSetState(ARENA_CAPTURE_BREAK);gArenaCaptureTelemetry.escaped++;PlaySE(SE_BALL_OPEN);CaptureMessage(3,90);}
        }
        break;
    case ARENA_CAPTURE_BREAK:
        if(t==1)ArenaFeedback_CaptureGlow(ball->x,ball->y-8,FALSE);
        if(t>=22)
        {
            CaptureReset();sArena.bodies[0].cooldown=max(12,sArena.bodies[0].cooldown);
            sArena.bodies[1].cooldown=max(30,sArena.bodies[1].cooldown);
            sArena.aiState=AI_RECOVER;sArena.aimTimer=0;
        }
        break;
    case ARENA_CAPTURE_CAUGHT:
        if(t==12)PlayBGM(MUS_CAUGHT);
        if(t==12 || t==18 || t==24)ArenaFeedback_Capture(ball->x,ball->y);
        if(t>=100)
        {
            sArena.active=FALSE;gRealtimeArenaTelemetry.active=FALSE;
            gRealtimeArenaTelemetry.exits++;
            gRealtimeArenaQuietResult=TRUE;
            gArenaResultTelemetry.started=gMain.vblankCounter1;
            gMain.callback1=sArena.savedCB1;
            RealtimeArena_FinishCapture();SetMainCallback2(BattleMainCB2);
        }
        break;
    case ARENA_CAPTURE_MISS:
        ball->x2=t/3;ball->y2=-4*6*t*(24-t)/(24*24);
        if(t>=24){ball->x2=ball->y2=0;CaptureReset();}
        break;
    }
    gArenaCaptureTelemetry.age=sCapture.age;
}

static void CaptureTrainer(void)
{
    u8 id=gArenaTrainerTelemetry[0];
    u8 frame=sCapture.state==ARENA_CAPTURE_AIM?1:
        sCapture.state==ARENA_CAPTURE_THROW?(sCapture.age<6?2:sCapture.age<12?3:4):255;
    struct Sprite *sprite;
    const struct ObjectEventGraphicsInfo *info;
    if(id==MAX_SPRITES || GetSpriteTileStartByTag(CAPTURE_TAG+1)==0xFFFF)return;
    sprite=&gSprites[id];
    if(frame==255)
    {
        sprite->oam.shape=SPRITE_SHAPE(16x32);sprite->oam.size=SPRITE_SIZE(16x32);
        sprite->oam.tileNum=GetSpriteTileStartByTag(TRAINER_TAG);
        sprite->hFlip=TRUE;sprite->x2=0;
    }
    else
    {
        sprite->oam.shape=SPRITE_SHAPE(32x32);sprite->oam.size=SPRITE_SIZE(32x32);
        sprite->oam.tileNum=GetSpriteTileStartByTag(CAPTURE_TAG+1);
        sprite->hFlip=FALSE;sprite->x2=4;
        if(frame!=sCapture.trainerFrame)
        {
            info=GetObjectEventGraphicsInfo(gSaveBlock2Ptr->playerGender?OBJ_EVENT_GFX_MAY_FIELD_MOVE:OBJ_EVENT_GFX_BRENDAN_FIELD_MOVE);
            ArenaRender_Copy(info->images[frame].data,(u8*)OBJ_VRAM0+sprite->oam.tileNum*32,512);
        }
    }
    CalcCenterToCornerVec(sprite,sprite->oam.shape,sprite->oam.size,sprite->oam.affineMode);
    sCapture.trainerFrame=frame;
}

static void CaptureDraw(void)
{
    struct Sprite *ball,*enemy;
    u32 t=sCapture.age;
    u8 state=sCapture.state;
    u16 scale=sArena.bodies[1].art?256:512;
    s16 tilt=0;
    if(!sCapture.ready)return;
    ball=&gSprites[sCapture.sprite];enemy=&gSprites[sArena.bodies[1].sprite];
    ball->invisible=state<ARENA_CAPTURE_THROW;
    if(state==ARENA_CAPTURE_ABSORB || state==ARENA_CAPTURE_BREAK)ball->y2=-8;
    ball->oam.tileNum=GetSpriteTileStartByTag(CAPTURE_TAG)+((state==ARENA_CAPTURE_ABSORB||state==ARENA_CAPTURE_BREAK)?8:0);
    gSprites[sCapture.shadow].invisible=ball->invisible;
    gSprites[sCapture.shadow].x=ball->x+ball->x2;
    gSprites[sCapture.shadow].y=ball->y+6;
    gSprites[sCapture.marker].invisible=state!=ARENA_CAPTURE_AIM;
    gSprites[sCapture.marker].x=sCapture.aimX;gSprites[sCapture.marker].y=sCapture.aimY;
    gSprites[sCapture.marker].oam.paletteNum=IndexOfSpritePaletteTag(SHOT_TAG+
        (ArenaNav_Distance(sCapture.aimX,sCapture.aimY,sArena.bodies[1].x/Q,sArena.bodies[1].y/Q)>22?1:0));
    CaptureTrainer();
    if(state==ARENA_CAPTURE_ABSORB || state==ARENA_CAPTURE_BREAK)
    {
        u16 amount=state==ARENA_CAPTURE_ABSORB?min(24,t):24-min(24,t);
        u16 size=scale+amount*80;
        enemy->x2=(sCapture.aimX-enemy->x)*amount/24;
        enemy->y2=(sCapture.aimY-8-enemy->y)*amount/24;
        SetOamMatrix(enemy->oam.matrixNum,size,0,0,size);
        BlendPalette(OBJ_PLTT_ID(1),16,min(16,amount),RGB(31,8,10));
        gSprites[sArena.bodies[1].shadow].invisible=amount>8;
    }
    if(state>=ARENA_CAPTURE_DROP && state<=ARENA_CAPTURE_CAUGHT && state!=ARENA_CAPTURE_BREAK)
        enemy->invisible=gSprites[sArena.bodies[1].shadow].invisible=TRUE;
    if(state==ARENA_CAPTURE_SHAKE && t>=20 && t<104)
    {
        u32 local=(t-20)%34;
        if(local<16){tilt=local<4?-60:local<12?60:-60;ball->x2=tilt/20;}
        else ball->x2=0;
    }
    else if(state!=ARENA_CAPTURE_MISS)ball->x2=0;
    SetOamMatrix(ball->oam.matrixNum,256,tilt,-tilt,256);
    if(state==ARENA_CAPTURE_THROW)
        SetOamMatrix(ball->oam.matrixNum,Cos(t*8,256),Sin(t*8,256),-Sin(t*8,256),Cos(t*8,256));
    if(CaptureLocked())gSprites[sArena.cueSprite].invisible=TRUE;
    gArenaCaptureTelemetry.aimX=sCapture.aimX;gArenaCaptureTelemetry.aimY=sCapture.aimY;
    gArenaCaptureTelemetry.ballX=ball->x;gArenaCaptureTelemetry.ballY=ball->y+ball->y2;
}
