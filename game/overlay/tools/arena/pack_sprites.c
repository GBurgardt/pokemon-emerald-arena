// Asset format compiler: original PNG -> GBA 4bpp tiles. Default is pixel exact;
// only explicitly listed oversized species use 2x nearest-neighbor reduction.
// Never clip opaque source extents, dither or invent animation frames.
#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Sheet { png_image image; unsigned char *rgba; int fw, fh, frames; const char *output; };
static uint16_t palette[16];
static int colors = 1;
static void fail(const char *s) { fprintf(stderr, "%s\n",s); exit(1); }
static unsigned color_index(const unsigned char *p)
{
    uint16_t color;
    int i;
    if (p[3] == 0) return 0;
    if (p[3] != 255) fail("Partial alpha: importer refuses to silently alter original pixels");
    color = (p[0] >> 3) | ((p[1] >> 3) << 5) | ((p[2] >> 3) << 10);
    for (i = 1; i < colors; ++i) if (palette[i] == color) return i;
    if (colors == 16) fail("More than 15 opaque colors: choose a deliberate palette conversion");
    palette[colors] = color;
    return colors++;
}
static void write_bytes(const char *path, const void *data, size_t size)
{
    FILE *f = fopen(path,"wb");
    if (!f || fwrite(data,1,size,f) != size) fail("Cannot write compiled sprite asset");
    fclose(f);
}
// Lossless per-animation tile dictionary. Header: LE32 dictionary offset and
// count; then 64 LE16 tile indices per frame, then the unique 32-byte tiles.
// First-occurrence order is deterministic and shared with the web preparer.
// Empty border tiles cost indices, not another 2 KB bitmap on every frame.
static void write_dictionary(const char *path, const unsigned char *tiles, size_t size)
{
    const size_t slots = 131072, capacity = size / 32;
    uint32_t *table = calloc(slots, sizeof(*table));
    unsigned char *dict = malloc(size), *out = calloc(1, 8 + capacity*2 + size);
    size_t count = 0, offset = 8 + capacity*2;
    if (!table || !dict || !out) fail("Dictionary allocation failed");
    for (size_t n = 0; n < capacity; n++)
    {
        const unsigned char *tile = tiles + n*32;
        uint32_t hash = 2166136261u;
        size_t slot, index;
        for (size_t j = 0; j < 32; j++) hash = (hash ^ tile[j]) * 16777619u;
        slot = hash & (slots-1);
        while (table[slot] && memcmp(dict+(table[slot]-1)*32,tile,32)) slot = (slot+1)&(slots-1);
        if (!table[slot])
        {
            if (count >= 65536) fail("Sprite dictionary exceeds 16-bit indices");
            memcpy(dict+count*32,tile,32);
            table[slot] = ++count;
        }
        index = table[slot]-1;
        out[8+n*2] = index & 255; out[9+n*2] = index >> 8;
    }
    for (size_t j=0; j<4; j++) {out[j]=offset>>(j*8);out[4+j]=count>>(j*8);}
    memcpy(out+offset,dict,count*32);
    write_bytes(path,out,offset+count*32);
    printf("Dictionary: %zu -> %zu bytes\n",size,offset+count*32);
    free(table); free(dict); free(out);
}
int main(int argc, char **argv)
{
    struct Sheet sheets[8] = {0};
    unsigned char pal_bytes[32];
    int count, i, dir, frame, x, y, scale, ox=0, oy=0, fitn=0, fitd=0;
    if (argc < 8 || (argc-3)%5 || (argc-3)/5 > 8) fail("palette scale [output png width height frames]...");
    scale=atoi(argv[2]);if(scale!=1&&scale!=2)fail("Scale must be 1 or 2");
    // Explicit, constant registration offset across every pose/direction.
    // This fits asymmetrically padded originals without shrinking or cropping.
    if(strchr(argv[2],','))
        if(sscanf(argv[2],"%d,%d,%d,%d,%d",&scale,&ox,&oy,&fitn,&fitd)<3 || abs(ox)>16 || abs(oy)>16)
            fail("Invalid source registration offset");
    if(fitn && (fitn>fitd || fitd>8 || fitn<1))fail("Invalid fit ratio");
    count = (argc-3)/5;
    for (i = 0; i < count; ++i)
    {
        struct Sheet *s = &sheets[i];
        s->output = argv[3+i*5];
        s->fw = atoi(argv[5+i*5]); s->fh = atoi(argv[6+i*5]); s->frames = atoi(argv[7+i*5]);
        if (s->fw < 1 || s->fh < 1 || s->frames < 1 || s->frames > 128) fail("Invalid frame geometry");
        s->image.version = PNG_IMAGE_VERSION;
        if(s->fw%scale||s->fh%scale)fail("Scale must exactly divide source geometry");
        if (!png_image_begin_read_from_file(&s->image,argv[4+i*5])) fail("Cannot read source PNG");
        s->image.format = PNG_FORMAT_RGBA;
        if (s->image.width != (unsigned)(s->fw*s->frames) || s->image.height != (unsigned)(s->fh*8)) fail("Expected exactly 8 directions and XML frame count");
        s->rgba = malloc(PNG_IMAGE_SIZE(s->image));
        if (!s->rgba || !png_image_finish_read(&s->image,NULL,s->rgba,0,NULL)) fail("Cannot decode source PNG");
        for (size_t n = 0; n < PNG_IMAGE_SIZE(s->image); n += 4) color_index(s->rgba+n);
    }
    for (i = 0; i < count; ++i)
    {
        struct Sheet *s = &sheets[i];
        size_t size = (size_t)s->frames * 8 * 2048;
        unsigned char *tiles = calloc(1,size);
        if (!tiles) fail("Allocation failed");
        for (dir = 0; dir < 8; ++dir)
            for (frame = 0; frame < s->frames; ++frame)
            {
                if(fitn)
                {
                    int left=s->fw,top=s->fh,right=0,bottom=0,w,h,dx,dy;
                    for(y=0;y<s->fh;y++)for(x=0;x<s->fw;x++)
                        if(s->rgba[((dir*s->fh+y)*s->image.width+frame*s->fw+x)*4+3])
                        {if(x<left)left=x;if(y<top)top=y;if(x+1>right)right=x+1;if(y+1>bottom)bottom=y+1;}
                    if(right<=left||bottom<=top)continue;
                    w=((right-left)*fitn+fitd-1)/fitd;h=((bottom-top)*fitn+fitd-1)/fitd;
                    if(w>64||h>64)fail("Fitted frame exceeds OBJ; refusing to crop");
                    // Remove transparent padding, not artwork. Fixed ratio for
                    // the whole species; no frame-dependent size changes.
                    for(dy=0;dy<h;dy++)for(dx=0;dx<w;dx++)
                    {
                        int sx=left+dx*fitd/fitn,sy=top+dy*fitd/fitn;
                        int tx=32-w/2+dx,ty=64-h+dy;
                        unsigned index=color_index(s->rgba+((dir*s->fh+sy)*s->image.width+frame*s->fw+sx)*4);
                        size_t offset=(dir*s->frames+frame)*2048+(ty/8*8+tx/8)*32+ty%8*4+tx%8/2;
                        tiles[offset]|=index<<((tx&1)*4);
                    }
                    continue;
                }
                for (y = 0; y < s->fh; ++y)
                    for (x = 0; x < s->fw; ++x)
                    {
                        const unsigned char *pixel = s->rgba + ((dir*s->fh+y)*s->image.width + frame*s->fw+x)*4;
                        unsigned index = color_index(pixel);
                        int tx = x/scale + 32 - s->fw/scale/2 + ox, ty = y/scale + 32 - s->fh/scale/2 + oy;
                        size_t offset;
                        if (!index) continue;
                        if (tx < 0 || tx >= 64 || ty < 0 || ty >= 64) fail("Opaque pixels exceed 64x64: refusing to crop art");
                        if(x%scale||y%scale)continue;
                        offset = (dir*s->frames+frame)*2048 + (ty/8*8+tx/8)*32 + ty%8*4 + tx%8/2;
                        tiles[offset] |= index << ((tx&1)*4);
                    }
            }
        write_dictionary(s->output,tiles,size);
        free(tiles); free(s->rgba); png_image_free(&s->image);
    }
    for (i = 0; i < 16; ++i) { pal_bytes[i*2] = palette[i]&255; pal_bytes[i*2+1] = palette[i]>>8; }
    write_bytes(argv[1],pal_bytes,32);
    printf("Packed %d sheets, 8 directions, %d opaque colors, no clipped pixels\n",count,colors-1);
    return 0;
}
