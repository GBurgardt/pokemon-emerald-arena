// Native 3x5 metadata lettering. Main names retain Emerald's original font.
static const u16 sHudTinyGlyphs[] = {
    0x7B6F,0x2492,0x73E7,0x73CF,0x5BC9,0x79CF,0x79EF,0x7249,0x7BEF,0x7BCF,
    0x2BED,0x6BAE,0x7927,0x6B6E,0x79E7,0x79E4,0x796F,0x5BED,0x7497,
    0x126A,0x5BAD,0x4927,0x5FED,0x5FFD,0x2B6A,0x6BE4,0x2B7B,0x6BAD,
    0x79CF,0x7492,0x5B6F,0x5B6A,0x5BFD,0x5AAD,0x5A92,0x72A7
};
static const u32 sHudPanelTiles[] = INCBIN_U32("graphics/arena/psychic/hud.4bpp");
static const u8 sHudPPLabel[] = _("PP ");
static EWRAM_DATA bool8 sHudCached=FALSE;
static EWRAM_DATA u16 sHudLastHp[2]={},sHudLastMax[2]={},sHudLastSpecies[2]={};
static EWRAM_DATA u8 sHudLastLevel[2]={};
static EWRAM_DATA u8 sHudLastBallState[2][PARTY_SIZE]={};
// Every ball keeps the same Poke Ball shape (cap / dark band with a white
// button / white base); a status just pulls the cap toward its color, same
// order as the ARENA_BALL_* constants (fainted, frozen, asleep, paralyzed,
// burned, poisoned, normal). Normal's cap is a pure red (240,0,0); burn uses
// a more orange red, kept distinct from it.
static const u8 sBallCapColor[7]={15,13,8,7,6,14,3};

static u8 PartyBallFlags(u32 status1)
{
    u32 f=0;
    if(status1&STATUS1_FREEZE)f|=ARENA_STATUS_FROZEN;
    if(status1&STATUS1_SLEEP)f|=ARENA_STATUS_ASLEEP;
    if(status1&STATUS1_PARALYSIS)f|=ARENA_STATUS_PARALYZED;
    if(status1&STATUS1_BURN)f|=ARENA_STATUS_BURNED;
    if(status1&STATUS1_PSN_ANY)f|=ARENA_STATUS_POISONED;
    return f;
}

// The active battler's HP/status are read live off gBattleMons (updated every
// frame); benched party members are read from the native party struct, which
// the arena keeps in sync on every HP/status change (see the SetMonData calls
// throughout realtime_arena.c).
static u8 PartySlotBallState(u32 side,u32 slot)
{
    u16 hp;u32 status;
    if(slot==gBattlerPartyIndexes[side])
    {
        hp=gBattleMons[side].hp;
        status=gBattleMons[side].status1;
    }
    else
    {
        struct Pokemon *mon=side?&gEnemyParty[slot]:&gPlayerParty[slot];
        hp=GetMonData(mon,MON_DATA_HP);
        status=GetMonData(mon,MON_DATA_STATUS);
    }
    return ArenaPartyStatus_BallState(hp,PartyBallFlags(status));
}

// gEnemyPartyCount is only kept current by link-trade code (src/trade.c); a
// normal trainer battle never refreshes it, so it reads back 0. Recompute
// both counts directly instead of trusting the cached globals.
static u32 PartyCountForSide(u32 side)
{
    return side?CalculateEnemyPartyCount():CalculatePlayerPartyCount();
}

// Wild battles keep no party-ball cost at all: the result is unused there.
static bool8 ComputePartyBallStates(u8 states[2][PARTY_SIZE])
{
    u32 side,i;
    bool8 changed=FALSE;
    if(!(gBattleTypeFlags&BATTLE_TYPE_TRAINER))return FALSE;
    for(side=0;side<2;side++)
    {
        u32 count=min(PartyCountForSide(side),PARTY_SIZE);
        for(i=0;i<count;i++)
        {
            states[side][i]=PartySlotBallState(side,i);
            if(states[side][i]!=sHudLastBallState[side][i])changed=TRUE;
        }
    }
    return changed;
}
static void HudTinyWindow(u8 window,u32 x,u32 y,const u8 *text,u8 color,u32 maxChars)
{
    u32 n=0;
    while(*text!=EOS&&n++<maxChars)
    {
        u8 c=*text++;
        u32 bits=0,row,col;
        if(c>=CHAR_0&&c<=CHAR_9)bits=sHudTinyGlyphs[c-CHAR_0];
        else if(c>=CHAR_A&&c<=CHAR_Z)bits=sHudTinyGlyphs[10+c-CHAR_A];
        else if(c==CHAR_COLON)bits=0x0410;
        else if(c==CHAR_PLUS)bits=0x05D0;
        else if(c==CHAR_HYPHEN)bits=0x01C0;
        else if(c==CHAR_EXCL_MARK)bits=0x2490|2;
        else if(c==CHAR_PERIOD)bits=2;
        for(row=0;row<5;row++)for(col=0;col<3;col++)
            if(bits&(1<<(14-row*3-col)))WindowRect(window,x+col,y+row,1,1,color);
        x+=4;
    }
}
static void HudTiny(u32 x,u32 y,const u8 *text,u8 color,u32 maxChars)
{HudTinyWindow(0,x,y,text,color,maxChars);}

static void DrawStatusHud(void)
{
    u32 side;
    u8 text[64];
    u8 ballStates[2][PARTY_SIZE];
    bool8 ballRowChanged=ComputePartyBallStates(ballStates);
    sHudRecovery=RecoveryPips();
    if(sHudCached && !ballRowChanged && sHudLastHp[0]==gBattleMons[0].hp && sHudLastHp[1]==gBattleMons[1].hp
        && sHudLastMax[0]==gBattleMons[0].maxHP && sHudLastMax[1]==gBattleMons[1].maxHP
        && sHudLastSpecies[0]==gBattleMons[0].species && sHudLastSpecies[1]==gBattleMons[1].species
        && sHudLastLevel[0]==gBattleMons[0].level && sHudLastLevel[1]==gBattleMons[1].level)return;
    // One small authored background; preserve the terrain through alpha corners.
    FillWindowPixelBuffer(0,0);
    CpuFastCopy(sHudPanelTiles,gWindows[0].tileData,30*2*32);
    HudRect(5,15,109,7,1);HudRect(126,15,109,7,1);
    for(side=0;side<2;side++)
    {
        const struct BattlePokemon *mon=&gBattleMons[side];
        u32 x=side?128:7,bar=side?129:8;
        u32 fill=mon->maxHP?mon->hp*102/mon->maxHP:0;
        u8 color=mon->hp*5<=mon->maxHP?6:mon->hp*2<=mon->maxHP?7:5;
        StringCopy(text,gSpeciesNames[mon->species]);
        Print(x,0,text,4);
        StringCopy(text,sQualityLevel);
        ConvertIntToDecimalStringN(text+4,mon->level,STR_CONV_MODE_LEFT_ALIGN,3);
        HudTiny(side?205:84,3,text,4,7);
        if(mon->hp&&!fill)fill=1;
        HudRect(bar,13,102,8,12);
        HudRect(bar,13,fill,1,4);
        HudRect(bar,14,fill,7,color);
        ConvertIntToDecimalStringN(text,mon->hp,STR_CONV_MODE_LEFT_ALIGN,3);
        HudTiny(bar+46,16,text,9,3);
        HudTiny(bar+45,15,text,4,3);
        sHudLastHp[side]=mon->hp;sHudLastMax[side]=mon->maxHP;sHudLastSpecies[side]=mon->species;
        sHudLastLevel[side]=mon->level;
    }
    if(gBattleTypeFlags&BATTLE_TYPE_TRAINER)
    {
        // Below the HP bar, in the 3px left in the 24px HUD strip (y=21-23).
        for(side=0;side<2;side++)
        {
            u32 count=min(PartyCountForSide(side),PARTY_SIZE),i;
            u32 x0=side?129:8;
            for(i=0;i<count;i++)
            {
                u32 x=x0+i*4,state=ballStates[side][i];
                HudRect(x,21,3,1,sBallCapColor[state]);
                HudRect(x,22,1,1,9);HudRect(x+1,22,1,1,10);HudRect(x+2,22,1,1,9);
                HudRect(x,23,3,1,10);
                sHudLastBallState[side][i]=state;
            }
        }
    }
    else if(GetSetPokedexFlag(SpeciesToNationalPokedexNum(gBattleMons[1].species),FLAG_GET_CAUGHT))
    {
        HudRect(118,4,5,7,9);HudRect(117,5,7,5,9);
        HudRect(119,4,3,1,6);HudRect(118,5,5,2,6);
        HudRect(118,8,5,2,4);HudRect(119,10,3,1,4);
        HudRect(120,7,1,1,4);
    }
    sHudCached=TRUE;
}
