// Code-native effects plus Kenney's CC0 trace (graphics/arena/cc0).
// Built offline: no trig, allocation or
// rasterization runs on the GBA. 8 facings, 4 phases, transparent 4bpp tiles.
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
static unsigned char trace[512*512*4];
static unsigned sampleTrace(double x,double y)
{
    int ix=(int)(x*511),iy=(int)(y*511);
    unsigned alpha;
    if(ix<0||iy<0||ix>511||iy>511)return 0;
    alpha=trace[(iy*512+ix)*4+3];
    return alpha>180?1:alpha>55?2:0;
}
static const double directions[8][2] = {
    {0,1},{.7071,.7071},{1,0},{.7071,-.7071},
    {0,-1},{-.7071,-.7071},{-1,0},{-.7071,.7071}
};
static void frame(FILE *f, int visual, int direction, int phase, int size)
{
    uint8_t tiles[2048] = {0};
    double ux=directions[direction][0], uy=directions[direction][1];
    int x,y;
    for (y=0;y<size;y++) for(x=0;x<size;x++)
    {
        double dx=x-(size-1)*.5, dy=y-(size-1)*.5;
        double a=dx*ux+dy*uy, b=dx*-uy+dy*ux, r=hypot(dx,dy);
        unsigned color=0;
        if (visual==0) // crescent sweep, moving from one edge to the other
        {
            double angle=atan2(b,a), center=-.70+phase*.46;
            if (a>0 && fabs(angle-center)<.60 && r>23 && r<31)
                color=r>26 && r<29 ? 1 : 2;
            if (a>0 && fabs(angle-center)<.50 && r>18 && r<20) color=2;
        }
        else if (visual==1) // committed rush streaks behind the actor
        {
            if (a < -3 && a > -28+phase*2 &&
                (fabs(b)<1.2 || fabs(fabs(b)-6)<.9)) color=fabs(b)<1.2?1:2;
        }
        else if (visual==2) // cone origin lies 32 px behind this sprite center
        {
            double forward=a+32, radius=hypot(forward,b), ring=26+phase*12;
            if (forward>0 && forward>=fabs(b)*2 && fabs(radius-ring)<2)
                color=fabs(radius-ring)<.8?1:2;
            if (forward>3 && forward<14 && fabs(fabs(b)-3)<2 && phase<2) color=1;
        }
        else if (visual==3) // two feathered wing sweeps, expanding forward
        {
            double spread=9+phase*3, edge=15+fabs(b)*.35;
            if(a>4 && a<30 && fabs(b)<spread && fabs(a-edge)<2.3)color=1;
            if(a>6 && a<24 && fabs(b)>3 && fabs(b)<spread
                && fabs(fmod(fabs(b)+a*.45,5))<1.2)color=2;
        }
        else if (visual==4) // heavy palm/ground impact, short radial spikes
        {
            double rr=hypot(a-18,b), ring=6+phase*3;
            if(a>2 && fabs(rr-ring)<2)color=fabs(rr-ring)<.8?1:2;
            if(a>2 && rr>ring && rr<ring+6 &&
                (fabs(b)<1.2 || fabs(a-18)<1.2 || fabs(fabs(a-18)-fabs(b))<1.2))color=2;
        }
        else if (visual==5) // narrow pointed peck, no broad sweep
        {
            double tip=18+phase*3;
            if(a>7 && a<tip && fabs(b)<(tip-a)*.22)color=fabs(b)<1?1:2;
        }
        else if (visual==6) // three parallel claw cuts, staggered in time
        {
            double slash=a+b*.45-(13+phase*3);
            if(a>4 && a<30 && fabs(b)<15 &&
                (fabs(slash)<1.2 || fabs(slash-7)<1.2 || fabs(slash+7)<1.2))color=fabs(b)<10?1:2;
        }
        else if (visual==7) // opposing jaws close on the target, not claw scratches
        {
            double gap=11-phase*3, forward=a-19;
            if(fabs(forward)<12 && fabs(fabs(b)-gap)<1.6)color=2;
            if(fabs(forward)<11 && fabs(b)<gap && fabs(b)>gap-4
                && fmod(forward+12,6)<3)color=1;
        }
        else if (visual==8) // Kenney CC0 trace, baked into a sharp green blade
        {
            double slash=a+b*.45-(12+phase*4);
            color=sampleTrace(.5+slash/14,.5+b/48);
            if(a>4 && fabs(b)<16 && fabs(slash+7)<.7)color=2;
        }
        else if (visual==9) // revolving leaf/seed, not a recolored bullet
        {
            double turn=phase*1.5707963, xx=dx*cos(turn)-dy*sin(turn), yy=dx*sin(turn)+dy*cos(turn);
            if (fabs(xx+yy*.65)<2.8 && fabs(yy)<6) color=2;
            if (r<2.7) color=1;
            if (fabs(xx+yy*.65)<.6 && fabs(yy)<5) color=1;
        }
        else if (visual==10) // elongated water jet with bright core
        {
            if(a>-7 && a<6 && fabs(b)<2.8-((a+7)/15))color=2;
            if(a>-6 && a<5 && fabs(b)<.9)color=1;
            if(a<-3 && fabs(b-(phase&1?4:-4))<1)color=2;
        }
        else if (visual==11) // hollow ghostly ring and drifting wisps
        {
            if (fabs(r-(4+phase%2))<1.4) color=2;
            if (fabs(r-4)<.7 && a>0) color=1;
            if (a<-3 && fabs(b-(phase-1.5))<.8) color=2;
        }
        else if (visual==12) // irregular acid glob, bright rim and small bubbles
        {
            if ((a-1)*(a-1)*.7+b*b<20) color=2;
            if ((a-2)*(a-2)+(b+2)*(b+2)<4) color=1;
            if ((a+5)*(a+5)+(b-(phase-1)*2)*(b-(phase-1)*2)<2) color=1;
        }
        else if(visual==13) // mud clod with two loose grains trailing behind
        {
            if(a*a*.8+b*b<20 && a<4)color=2;
            if((a-1)*(a-1)+(b+2)*(b+2)<3)color=1;
            if(a<-4 && a>-7 && fabs(b-(phase-1)*2)<1.2)color=2;
        }
        else if(visual==14) // faceted, rotating thrown stone
        {
            double qx=dx*cos(phase*.35)-dy*sin(phase*.35),qy=dx*sin(phase*.35)+dy*cos(phase*.35);
            if(fabs(qx)<5 && fabs(qy)<5 && fabs(qx)+fabs(qy)<7.5)color=2;
            if(qy<-1 && qx>-3 && qx<2 && fabs(qx)+fabs(qy)<6)color=1;
        }
        else if(visual==15) // small flame head and alternating tongues
        {
            if(a>-6 && a<4 && fabs(b)<(5-a)*.45)color=2;
            if(a>-3 && a<3 && fabs(b)<1.3)color=1;
            if(a<-3 && a>-7 && fabs(b-(phase&1?3:-3))<1.4)color=2;
        }
        else if(visual==16) // hollow rising bubbles, no solid acid glob
        {
            double rr=hypot(a-1,b),r2=hypot(a+4,b-3+(phase&1));
            if(fabs(rr-4)<1 || fabs(r2-2)<.8)color=2;
            if(fabs(rr-4)<.8 && b<-1)color=1;
        }
        else if(visual==17) // three curved wind strokes
        {
            if(a>-5 && a<5 && fabs(b-(a*a/12-3))<1)color=1;
            if(a>-6 && a<3 && fabs(b-(a*a/14+1))<1)color=2;
            if(a>-4 && a<3 && fabs(b-5)<.8)color=2;
        }
        else if(visual==18) // four-point spinning Swift star
        {
            double qx=dx*cos(phase*.4)-dy*sin(phase*.4),qy=dx*sin(phase*.4)+dy*cos(phase*.4);
            if(fabs(qx)*fabs(qy)<3 && r<7)color=2;
            if(fabs(qx)+fabs(qy)<3)color=1;
        }
        else if(visual==19) // Psychic: rotating elliptical wave with a bright nucleus
        {
            double angle=phase*.785398, xx=dx*cos(angle)-dy*sin(angle), yy=dx*sin(angle)+dy*cos(angle);
            if(fabs(hypot(xx*.7,yy*1.5)-5)<1.2)color=2;
            if(fabs(hypot(xx*1.5,yy*.7)-5)<.7)color=1;
            if(r<1.6)color=1;
        }
        else if(visual==20) // Shadow Ball: dark orb, orbiting crescent and trailing wisps
        {
            if(r<4.8)color=2;
            if(r>3.4&&r<4.7&&a+b>1)color=1;
            if(a<-4&&a>-7&&fabs(b-(phase&1?3:-3))<1)color=2;
        }
        if (color)
        {
            if(visual==15)color+=2; // Shared fire feedback palette: yellow/red.
            size_t p=((y/8)*(size/8)+x/8)*32+(y%8)*4+(x%8)/2;
            tiles[p]|=color<<((x&1)*4);
        }
    }
    if (fwrite(tiles,1,(size_t)size*size/2,f)!=(size_t)size*size/2) exit(2);
}
int main(int argc,char **argv)
{
    FILE *f;int v,d,p;
    png_image image;
    if(argc!=3) return 1;
    memset(&image,0,sizeof(image));image.version=PNG_IMAGE_VERSION;
    if(!png_image_begin_read_from_file(&image,"graphics/arena/cc0/trace_01.png"))return 3;
    image.format=PNG_FORMAT_RGBA;
    if(image.width!=512||image.height!=512||!png_image_finish_read(&image,0,trace,0,0))return 3;
    png_image_free(&image);
    f=fopen(argv[1],"wb");if(!f)return 2;
    for(v=0;v<9;v++)for(d=0;d<8;d++)for(p=0;p<4;p++)frame(f,v,d,p,64);
    fclose(f);
    f=fopen(argv[2],"wb");if(!f)return 2;
    for(v=9;v<21;v++)for(d=0;d<8;d++)for(p=0;p<4;p++)frame(f,v,d,p,16);
    fclose(f);
    return 0;
}
