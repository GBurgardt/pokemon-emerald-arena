// Technical atlas import only: approved Image API artwork, alpha crop, native
// sizing and deterministic RGB555 quantization. No procedural prop substitutes.
// Indices 1/2 stay reserved for move effects; each material owns indices 3..15.
#include <png.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const int kinds[7]={0,0,0,1,2,3,4};
static const int widths[7]={26,22,22,19,18,13,17};
static const int heights[7]={18,18,19,17,14,17,15};
static unsigned char tile[2048];
static uint16_t pixels[7][3][1024],palettes[5][16];
static unsigned histogram[32768];
static unsigned char lookup[5][32768];
#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
static const uint16_t fx[5][2]={{RGB(31,31,24),RGB(29,21,9)},
    {RGB(31,31,31),RGB(17,27,31)},{RGB(25,31,12),RGB(8,24,8)},
    {RGB(31,22,31),RGB(24,7,25)},{RGB(23,31,31),RGB(5,21,31)}};
static void die(const char*s){fprintf(stderr,"%s\n",s);exit(2);}
static int distance(unsigned a,unsigned b)
{
    int r=(int)(a&31)-(int)(b&31),g=(int)((a>>5)&31)-(int)((b>>5)&31),v=(int)(a>>10)-(int)(b>>10);
    return r*r*2+g*g*3+v*v;
}
static unsigned nearest(unsigned k,unsigned c,unsigned count)
{
    unsigned j,chosen=1;int best=0x7fffffff;
    for(j=1;j<count;j++){int d=distance(c,palettes[k][j]);if(d<best){best=d;chosen=j;}}
    return chosen;
}
static void import(const char*path)
{
    png_image im={0};unsigned char*rgba;unsigned i,s,x,y,k,c,j,round;
    im.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&im,path))die("Cannot open approved props atlas");
    if(!(im.format&PNG_FORMAT_FLAG_ALPHA))die("Props must contain real alpha, not a checkerboard");
    im.format=PNG_FORMAT_RGBA;rgba=malloc(PNG_IMAGE_SIZE(im));
    if(!rgba||!png_image_finish_read(&im,NULL,rgba,0,NULL))die("Cannot decode props atlas");
    memset(pixels,255,sizeof(pixels));
    for(i=0;i<7;i++)for(s=0;s<3;s++)
    {
        unsigned left=im.width,right=0,top=im.height,bottom=0,n=0;
        unsigned x0=kinds[i]*im.width/5,x1=(kinds[i]+1)*im.width/5;
        unsigned y0=s*im.height/3,y1=(s+1)*im.height/3;
        unsigned w,h,dw,dh,tx,ty;
        for(y=y0;y<y1;y++)for(x=x0;x<x1;x++)if(rgba[(y*im.width+x)*4+3]>=128)
        {
            if(x<left)left=x;if(x>right)right=x;
            if(y<top)top=y;if(y>bottom)bottom=y;n++;
        }
        if(!n||n>(x1-x0)*(y1-y0)*9/10)die("Missing sprite or opaque background in atlas cell");
        w=right-left+1;h=bottom-top+1;dw=widths[i]+4;dh=heights[i]+9;
        if(dw>31)dw=31;if(dh>31)dh=31;
        // Tall biome props must fit above the common ground-contact baseline.
        // Otherwise unsigned ty underflows and writes before the frame buffer.
        if(dh>16+(unsigned)heights[i]/2+1)dh=16+(unsigned)heights[i]/2+1;
        if(s==2)dh=9; // remnants are low, visibly traversable rubble
        if(dw*h>dh*w)dw=dh*w/h;else dh=dw*h/w;
        if(!dw)dw=1;if(!dh)dh=1;
        tx=16-dw/2;ty=16+heights[i]/2+1-dh;
        for(y=0;y<dh;y++)for(x=0;x<dw;x++)
        {
            unsigned sx=left+(x*2+1)*w/(dw*2),sy=top+(y*2+1)*h/(dh*2);
            const unsigned char*p=rgba+(sy*im.width+sx)*4;
            if(ty+y>=32 || tx+x>=32)die("Prop exceeds native frame bounds");
            if(p[3]>=128)pixels[i][s][(ty+y)*32+tx+x]=(p[0]>>3)|((p[1]>>3)<<5)|((p[2]>>3)<<10);
        }
    }
    free(rgba);png_image_free(&im);
    for(k=0;k<5;k++)
    {
        memset(histogram,0,sizeof(histogram));
        for(i=0;i<7;i++)if(kinds[i]==(int)k)for(s=0;s<3;s++)for(x=0;x<1024;x++)
            if(pixels[i][s][x]!=0xffff)histogram[pixels[i][s][x]]++;
        palettes[k][1]=fx[k][0];palettes[k][2]=fx[k][1];
        // Weighted farthest seeding followed by six deterministic Lloyd passes.
        for(j=3;j<16;j++)
        {
            unsigned best=0,chosen=0;
            for(c=0;c<32768;c++)if(histogram[c])
            {
                unsigned score=histogram[c]*(unsigned)distance(c,palettes[k][nearest(k,c,j)]);
                if(score>best){best=score;chosen=c;}
            }
            palettes[k][j]=chosen;
        }
        for(round=0;round<6;round++)
        {
            unsigned sums[16][4]={{0}};
            for(c=0;c<32768;c++)if(histogram[c])
            {
                j=nearest(k,c,16);sums[j][0]+=(c&31)*histogram[c];
                sums[j][1]+=((c>>5)&31)*histogram[c];sums[j][2]+=(c>>10)*histogram[c];sums[j][3]+=histogram[c];
            }
            for(j=3;j<16;j++)if(sums[j][3])palettes[k][j]=RGB(sums[j][0]/sums[j][3],sums[j][1]/sums[j][3],sums[j][2]/sums[j][3]);
        }
        for(c=0;c<32768;c++)lookup[k][c]=nearest(k,c,16);
    }
}
static void put(int x,int y,int size,unsigned c)
{size_t p=((y/8)*(size/8)+x/8)*32+y%8*4+x%8/2;tile[p]|=c<<((x&1)*4);}
static void save(FILE*f,int size)
{if(fwrite(tile,1,(size_t)size*size/2,f)!=(size_t)size*size/2)exit(2);}
int main(int argc,char**argv)
{
    FILE*f;int i,s,x,y,p,k;
    if(argc!=6)die("pack-props atlas.png props.4bpp pieces.4bpp blast.4bpp palettes.gbapal");
    import(argv[1]);
    f=fopen(argv[2],"wb");if(!f)return 2;
    for(i=0;i<7;i++)for(s=0;s<3;s++)
    {
        memset(tile,0,sizeof(tile));
        for(y=0;y<32;y++)for(x=0;x<32;x++)
        {unsigned c=pixels[i][s][y*32+x];put(x,y,32,c==0xffff?0:lookup[kinds[i]][c]);}
        save(f,32);
    }fclose(f);
    f=fopen(argv[3],"wb");if(!f)return 2;
    for(k=0;k<5;k++)for(p=0;p<4;p++)
    {
        memset(tile,0,sizeof(tile));
        for(y=0;y<8;y++)for(x=0;x<8;x++)
        {
            int a=x-3,b=y-3,rot=p&1?a:b,cross=p&1?b:a;unsigned c=0;
            if(k==0&&a*a+b*b<9)c=a<0?5:3;
            if(k==1&&abs(cross)<1&&abs(rot)<4)c=rot<0?9:6;
            if(k==2&&abs(rot+cross/2)<2&&abs(cross)<3)c=rot<0?10:8;
            if(k==3&&abs(rot)*2+abs(cross)<5)c=rot<0?12:11;
            if(k==4&&a*a+b*b<7)c=a<0?15:14;
            put(x,y,8,c);
        }save(f,8);
    }fclose(f);
    f=fopen(argv[4],"wb");if(!f)return 2;
    for(p=0;p<8;p++)
    {
        memset(tile,0,sizeof(tile));
        for(y=0;y<64;y++)for(x=0;x<64;x++)
        {
            double r=hypot(x-31.5,y-31.5),radius=5+p*3.5;
            unsigned c=fabs(r-radius)<1.4?(p<3?15:9):0;
            if(p<2&&r<radius-1)c=15;
            put(x,y,64,c);
        }save(f,64);
    }fclose(f);
    f=fopen(argv[5],"wb");if(!f)return 2;
    for(k=0;k<5;k++)for(i=0;i<16;i++){fputc(palettes[k][i]&255,f);fputc(palettes[k][i]>>8,f);}
    fclose(f);puts("Approved atlas: 21 native prop frames, five shared material/FX palettes, real alpha");return 0;
}
