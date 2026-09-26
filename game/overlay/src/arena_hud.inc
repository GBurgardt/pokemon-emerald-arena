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
    sHudRecovery=RecoveryPips();
    if(sHudCached && sHudLastHp[0]==gBattleMons[0].hp && sHudLastHp[1]==gBattleMons[1].hp
        && sHudLastMax[0]==gBattleMons[0].maxHP && sHudLastMax[1]==gBattleMons[1].maxHP
        && sHudLastSpecies[0]==gBattleMons[0].species && sHudLastSpecies[1]==gBattleMons[1].species
        && sHudLastLevel[0]==gBattleMons[0].level && sHudLastLevel[1]==gBattleMons[1].level)return;
    // One small authored background; preserve the terrain through alpha corners.
    CpuFastCopy(sHudPanelTiles,gWindows[0].tileData,30*2*32);
    for(side=0;side<2;side++)
    {
        const struct BattlePokemon *mon=&gBattleMons[side];
        u32 x=side?128:7,bar=side?129:8;
        u32 fill=mon->maxHP?mon->hp*102/mon->maxHP:0;
        u8 color=mon->hp*5<=mon->maxHP?6:mon->hp*2<=mon->maxHP?7:5;
        StringCopy(text,gSpeciesNames[mon->species]);
        Print(x,0,text,4);
        ConvertIntToDecimalStringN(text,mon->hp,STR_CONV_MODE_LEFT_ALIGN,3);
        HudTiny(side?218:97,6,text,4,3);
        text[0]=CHAR_L;
        ConvertIntToDecimalStringN(text+1,mon->level,STR_CONV_MODE_LEFT_ALIGN,3);
        HudTiny(side?214:93,0,text,4,4);
        if(mon->hp&&!fill)fill=1;
        HudRect(bar,12,102,3,12);
        HudRect(bar,12,fill,1,4);
        HudRect(bar,13,fill,2,color);
        sHudLastHp[side]=mon->hp;sHudLastMax[side]=mon->maxHP;sHudLastSpecies[side]=mon->species;
        sHudLastLevel[side]=mon->level;
    }
    sHudCached=TRUE;
}
