// Included after ApplyMoveHit: native damage/PP/save remain authoritative.
EWRAM_DATA struct ArenaPsychicRock gArenaPsychicRocks[ARENA_OBSTACLES]={};
EWRAM_DATA u32 gArenaPsychicTelemetry[8]={};
static EWRAM_DATA struct {u8 active,age,connected,manual; s16 aimX,aimY;} sPsychic[2]={};
static EWRAM_DATA u8 sRockShadows[3]={};
static bool8 PsychicBusy(u8 side){return sPsychic[side].active;}
static void PsychicReset(void)
{
    u32 i;
    memset(gArenaPsychicRocks,0,sizeof(gArenaPsychicRocks));
    memset(sPsychic,0,sizeof(sPsychic));
    memset(gArenaPsychicTelemetry,0,sizeof(gArenaPsychicTelemetry));
    for(i=0;i<3;i++)sRockShadows[i]=MAX_SPRITES;
    if(!BattleHasPsychic())return;
    for(i=0;i<3;i++)
    {sRockShadows[i]=CreateSprite(&sShadowTemplate,0,0,12);if(sRockShadows[i]!=MAX_SPRITES)gSprites[sRockShadows[i]].invisible=TRUE;}
}
static bool8 PsychicStart(u8 side,s32 targetX,s32 targetY)
{
    u32 i,n=0;
    for(i=0;i<ARENA_OBSTACLES&&n<3;i++)
    {
        const struct ArenaRect *rect=&gArenaObstacles[i];
        struct ArenaProp *p=&gArenaProps[i];
        struct ArenaPsychicRock *r=&gArenaPsychicRocks[i];
        if(p->kind!=ARENA_PROP_ROCK||p->broken||p->reserved)continue;
        r->x=(rect->left+rect->right)*Q/2;r->y=(rect->top+rect->bottom)*Q/2;
        r->state=1;r->side=side;r->order=n++;r->age=0;r->height=0;
        p->reserved=side+1;ArenaNav_SetObstacle(i,0);
        gArenaPhysicsTelemetry.solidMask&=~(1<<i);
    }
    if(!n)return FALSE; // depleted arena: original psychic projectile
    sPsychic[side].active=TRUE;sPsychic[side].age=0;sPsychic[side].connected=FALSE;
    sPsychic[side].manual=side?FALSE:sArena.bodies[side].manualAim;
    sPsychic[side].aimX=targetX/Q;sPsychic[side].aimY=targetY/Q;
    gArenaPsychicTelemetry[0]++;gArenaPsychicTelemetry[1]+=n;
    return TRUE;
}
static void PsychicImpact(u8 i)
{
    struct ArenaPsychicRock *r=&gArenaPsychicRocks[i];
    ArenaPhysics_ShatterAt(i,r->x/Q,r->y/Q,r->vx,r->vy);
    ArenaPsychicFx_Impact(r->x/Q,r->y/Q);
    ArenaFeedback_Wall(r->x/Q,r->y/Q);
    gArenaPsychicTelemetry[3]++;r->state=0;sArena.hitstop=3;
    PlaySE(SE_M_ROCK_THROW);
}
static void TickPsychic(void)
{
    u32 side,i;
    for(side=0;side<2&&!sArena.resultTimer;side++)
    {
        u8 count=0;
        struct ArenaBody *b=&sArena.bodies[side],*target=&sArena.bodies[side^1];
        if(!sPsychic[side].active)continue;
        sPsychic[side].age++;gArenaPsychicTelemetry[7]++;
        for(i=0;i<ARENA_OBSTACLES;i++)
        {
            struct ArenaPsychicRock *r=&gArenaPsychicRocks[i];
            s16 ox,oy,nx,ny,obstacle;
            s32 dx,dy,len;
            if(!r->state||r->side!=side)continue;
            count++;r->age++;
            if(r->state==1)
            {
                u8 angle=r->age*4+r->order*85;
                const struct ArenaRect *rect=&gArenaObstacles[i];
                s16 groundX=(rect->left+rect->right)/2,groundY=(rect->top+rect->bottom)/2;
                nx=Clamp(b->x/Q+Sin(angle,43),18,222);
                ny=Clamp(b->y/Q+Cos(angle,25),56,143);
                r->height=min(16,r->age);
                if(r->age<30)
                {r->x=(groundX+(nx-groundX)*r->age/30)*Q;r->y=(groundY+(ny-groundY)*r->age/30)*Q;}
                else {r->x=nx*Q;r->y=ny*Q;}
                if(r->age>=72+r->order*12)
                {
                    // Automatic aim tracks during visible charge, locks when
                    // the first stone leaves. No homing in flight. Directional
                    // manual aim always preserves the player's committed ray.
                    if(!r->order&&!sPsychic[side].manual&&!SmokeBlocks(b->x/Q,b->y/Q,target->x/Q,target->y/Q))
                    {sPsychic[side].aimX=target->x/Q;sPsychic[side].aimY=target->y/Q;}
                    if(!r->order&&!sPsychic[side].manual&&sDecoys[side^1].life&&sDecoys[side^1].age>=24
                        &&!SmokeBlocks(b->x/Q,b->y/Q,sDecoys[side^1].x,sDecoys[side^1].y))
                    {sPsychic[side].aimX=sDecoys[side^1].x;sPsychic[side].aimY=sDecoys[side^1].y;}
                    r->state=2;r->age=0;r->y-=r->height*Q;r->height=0;
                    dx=sPsychic[side].aimX*Q-r->x;dy=(sPsychic[side].aimY-6)*Q-r->y;
                    len=max(Abs(dx),Abs(dy))+min(Abs(dx),Abs(dy))/2;
                    if(!len){dy=-Q;len=Q;}
                    r->vx=dx*960/len;r->vy=dy*960/len;
                    gArenaPsychicTelemetry[2]++;PlaySE(SE_M_PSYBEAM2);
                }
                continue;
            }
            ox=r->x/Q;oy=r->y/Q;nx=(r->x+r->vx)/Q;ny=(r->y+r->vy)/Q;
            obstacle=ArenaNav_FirstObstacle(ox,oy,nx,ny,5);
            r->x+=r->vx;r->y+=r->vy;
            if(obstacle>=0)
            {ArenaPhysics_Hit(obstacle,r->vx,r->vy,3);gArenaPsychicTelemetry[5]++;PsychicImpact(i);continue;}
            if(DecoySegment(side^1,ox,oy,nx,ny,14))
            {sPsychic[side].connected=TRUE;PsychicImpact(i);continue;}
            if(!sPsychic[side].connected&&CoverSegment(side,MOVE_PSYCHIC,ox,oy,nx,ny,14))
            {sPsychic[side].connected=TRUE;PsychicImpact(i);continue;}
            if(!target->dash&&ArenaMoves_SegmentHit(ox,oy,nx,ny,target->x/Q,target->y/Q,14))
            {
                if(!sPsychic[side].connected)
                {sPsychic[side].connected=TRUE;gArenaPsychicTelemetry[4]++;ApplyMoveHit(side,MOVE_PSYCHIC);}
                PsychicImpact(i);continue;
            }
            if(nx<12||nx>228||ny<30||ny>146||r->age>60)
            {gArenaPsychicTelemetry[6]++;PsychicImpact(i);}
        }
        if(!count)sPsychic[side].active=FALSE;
    }
}
