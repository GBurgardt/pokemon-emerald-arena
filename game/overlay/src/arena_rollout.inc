// Rollout's curled-up ball.
//
// While a body rolls, its sprite shows a sphere drawn from the Pokémon's own
// palette: its most common colour for the body, a contrasting colour in bands
// that scroll along the heading as the ball turns, a fixed highlight toward
// the upper left and a shadowed limb. The work is spread over the wind-up a
// little per frame: first a map of the ball's geometry (one byte per pixel,
// shade class and band coordinate), then each of the eight band phases is
// coloured from that map through a lookup table into a heap block, and the
// phase matching the distance rolled is copied into the mon's sprite tiles.
// The even phases are coloured first, so the ball can roll on those alone
// while the odd ones are still being filled in. Without the heap block, or
// before the even phases are ready, the roll keeps the tumbling sprite.
#define BALL_PHASES 8
#define BALL_COLS 5
#define BALL_ROWS 5
#define BALL_TILES (BALL_COLS * BALL_ROWS)
#define BALL_STEPS (BALL_TILES + BALL_PHASES * BALL_TILES)   // map tiles, then coloured tiles
#define BALL_X0 8                                              // canvas origin of the ball region
#define BALL_Y0 0
#define BALL_CX 28                                             // ball centre on the canvas
#define BALL_CY 20
#define BALL_MIN_R 10
#define BALL_MAX_R 19
// Light direction in Q8: from the upper left, toward the viewer.
#define BALL_LX (-115)
#define BALL_LY (-141)
#define BALL_LZ 179
// Shade classes in a map byte's top three bits; the banded class keeps its
// band coordinate, 1/256 turn units around the rolling axis, in the low five.
#define BALL_CLEAR 0
#define BALL_OUTLINE 1
#define BALL_LIGHT 2
#define BALL_DARK 3
#define BALL_BANDED 4

struct RollBallSide
{
    u8 z[BALL_MAX_R * BALL_MAX_R + 1];   // sqrt(r^2 - d^2) / r in Q8, by squared distance
    s16 n[2 * BALL_MAX_R + 1];           // d / r in Q8, by offset from the centre
    u8 map[BALL_TILES][64];              // shade class and band coordinate per pixel
    u8 lut[BALL_PHASES][256];            // map byte to colour index, per phase
    u8 tiles[BALL_PHASES][BALL_TILES * 32];
};
struct RollBallHeap
{
    s8 asin[513];                        // angle in 1/256 turn whose sine is (i - 256) / 256
    struct RollBallSide side[2];
};
struct RollBall
{
    u8 radius, outline, dark, mid, band, light, shown, phase, pending, done, lutDone;
    u16 step, stepsDone;                 // roll angle per pixel travelled; work done so far
    s8 pivotX, pivotY;                   // the pose's centre, from the canvas centre
};
static EWRAM_DATA struct RollBallHeap *sBallHeap = NULL;
static EWRAM_DATA struct RollBall sBall[2] = {};
static const u8 sBallOrder[BALL_PHASES] = {0, 2, 4, 6, 1, 3, 5, 7};

static void RollBallInit(void)
{
    s32 k, i, last;
    memset(sBall, 0, sizeof(sBall));
    if (!sBallHeap) sBallHeap = Alloc(sizeof(*sBallHeap));
    if (!sBallHeap) return;
    for (i = 0; i <= 512; i++) sBallHeap->asin[i] = 127;
    for (k = 0; k <= 64; k++)
    {
        s32 s = Sin(k, 256);
        sBallHeap->asin[256 + s] = k;
        sBallHeap->asin[256 - s] = -k;
    }
    for (i = 256, last = 0; i <= 512; i++)
    {
        if (sBallHeap->asin[i] == 127) sBallHeap->asin[i] = last; else last = sBallHeap->asin[i];
    }
    for (i = 256, last = 0; i >= 0; i--)
    {
        if (sBallHeap->asin[i] == 127) sBallHeap->asin[i] = last; else last = sBallHeap->asin[i];
    }
}

static void RollBallExit(void)
{
    if (sBallHeap) { Free(sBallHeap); sBallHeap = NULL; }
    memset(sBall, 0, sizeof(sBall));
}

static bool8 RollBallReady(u8 side)
{
    return sBallHeap && (sBall[side].done & 0x55) == 0x55;   // the even phases
}

static bool8 RollBallVisible(u8 side)
{
    return Rolling(side) && !TossActive() && sArena.bodies[side].actionLife && RollBallReady(side);
}

// A new wind-up: everything is drawn afresh, starting next frame.
static void RollBallBegin(u8 side)
{
    struct RollBall *b = &sBall[side];
    const u32 *words = sMonFrameTiles[side];
    s32 w, n = 0, sx = 0, sy = 0;
    b->stepsDone = 0; b->shown = FALSE; b->radius = 0; b->pending = 2; b->done = 0; b->lutDone = 0;
    // The spin-up turns about the centre of the pose on screen, not the
    // canvas centre, so the body revs in place instead of swinging round.
    // Each opaque eight-pixel row segment counts once.
    for (w = 0; w < 512; w++)
    {
        if (!words[w]) continue;
        n++; sx += ((w >> 3) & 7) * 8 + 4; sy += (w >> 6) * 8 + (w & 7);
    }
    b->pivotX = n ? sx / n - 32 : 0;
    b->pivotY = n ? sy / n - 32 : 0;
}

// Choose the ball's colours and size from the pose currently decoded for the
// body. Runs on the first wind-up frame, which has no button work of its own.
static void RollBallColours(u8 side)
{
    struct RollBall *b = &sBall[side];
    const struct ArenaBody *body = &sArena.bodies[side];
    const u32 *words = sMonFrameTiles[side];
    u16 counts[16], lum[16], hue[16][3];
    s32 i, w, opaque = 0, minX = 64, maxX = -1, minY = 64, maxY = -1, floor;
    if (!sBallHeap || !body->art) return;
    memset(counts, 0, sizeof(counts));
    // Colour counts over a quarter of the pixels; the pose's extent per tile
    // is enough for the size. Transparent words, most of the canvas, are skipped.
    for (w = 0; w < 512; w += 2)
    {
        u32 word = words[w];
        s32 tx, ty;
        if (!word) continue;
        tx = (w >> 3) & 7; ty = w >> 6;
        if (tx < minX) minX = tx;
        if (tx > maxX) maxX = tx;
        if (ty < minY) minY = ty;
        if (ty > maxY) maxY = ty;
        counts[word & 15]++; counts[(word >> 8) & 15]++; counts[(word >> 16) & 15]++; counts[(word >> 24) & 15]++;
    }
    for (i = 1; i < 16; i++) opaque += counts[i];
    if (opaque < 32) return;
    floor = opaque / 20;   // the only divisions here are these few, once per roll
    // Brightness, and hue as each colour's share of red, green and blue, so
    // that shades of the body colour can be told from a differently coloured marking.
    for (i = 0; i < 16; i++)
    {
        u16 c = body->art->palette[i];
        s32 sum = (c & 31) + ((c >> 5) & 31) + ((c >> 10) & 31) + 1;
        lum[i] = 2 * (c & 31) + 3 * ((c >> 5) & 31) + ((c >> 10) & 31);
        hue[i][0] = (c & 31) * 256 / sum; hue[i][1] = ((c >> 5) & 31) * 256 / sum; hue[i][2] = ((c >> 10) & 31) * 256 / sum;
    }
    // The body takes the most common colour, the outline the darkest common one.
    b->mid = 1;
    for (i = 1; i < 16; i++) if (counts[i] > counts[b->mid]) b->mid = i;
    b->outline = b->mid;
    for (i = 1; i < 16; i++) if (counts[i] >= floor && lum[i] < lum[b->outline]) b->outline = i;
    if (b->outline == b->mid)
    {
        // A very dark body: the next most common colour fills the ball.
        s32 next = 0;
        for (i = 1; i < 16; i++) if (i != b->outline && (!next || counts[i] > counts[next])) next = i;
        if (next && counts[next] >= floor) b->mid = next;
    }
    // Shade and highlight: the darkest and lightest shades of the body colour.
    // Bands: the most common colour of another hue, else the most contrasting shade.
    b->light = b->mid; b->dark = b->outline; b->band = 0;
    for (i = 1; i < 16; i++)
    {
        s32 dist;
        if (counts[i] < floor || i == b->mid || i == b->outline) continue;
        dist = Abs(hue[i][0] - hue[b->mid][0]) + Abs(hue[i][1] - hue[b->mid][1]) + Abs(hue[i][2] - hue[b->mid][2]);
        if (dist < 48)
        {
            if (lum[i] > lum[b->light]) b->light = i;
            if (lum[i] < lum[b->mid] && (b->dark == b->outline || lum[i] < lum[b->dark])) b->dark = i;
        }
        else if (lum[i] > lum[b->outline] + 10 && (!b->band || counts[i] > counts[b->band])) b->band = i;
    }
    if (!b->band)
    {
        for (i = 1; i < 16; i++)
            if (counts[i] >= floor && i != b->mid && i != b->outline && Abs(lum[i] - lum[b->mid]) >= 12
                && (!b->band || Abs(lum[i] - lum[b->mid]) > Abs(lum[b->band] - lum[b->mid]))) b->band = i;
    }
    if (!b->band) b->band = b->dark != b->mid ? b->dark : b->light;
    // A curled body is about as wide as its standing pose.
    b->radius = Clamp((maxX - minX + maxY - minY + 2) * 2 - 2, BALL_MIN_R, BALL_MAX_R);
    b->step = 10430 / b->radius;   // 65536 / (2 pi r): one turn per circumference travelled
}

// The sphere's depth and normal tables for this radius; the second wind-up frame.
static void RollBallTables(u8 side)
{
    struct RollBall *b = &sBall[side];
    struct RollBallSide *h = &sBallHeap->side[side];
    s32 r = b->radius, r2 = r * r, inv = 255 * 256 / r, z, d2, i;
    for (d2 = 0, z = r; d2 <= r2; d2++)
    {
        while (z * z > r2 - d2) z--;
        h->z[d2] = (z * inv) >> 8;
    }
    inv = 65536 / r;
    for (i = -BALL_MAX_R; i <= BALL_MAX_R; i++) h->n[i + BALL_MAX_R] = (i * inv) >> 8;
}

// One tile of the geometry map.
static void RollBallMapTile(u8 side, u32 t)
{
    const struct RollBall *b = &sBall[side];
    const struct RollBallSide *h = &sBallHeap->side[side];
    const s8 *asin = sBallHeap->asin + 256;
    const s16 *ntab = h->n + BALL_MAX_R;
    const u8 *ztab = h->z;
    u8 *m = sBallHeap->side[side].map[t];
    s32 r2 = b->radius * b->radius, r2in = (b->radius - 1) * (b->radius - 1);
    s32 hx = sArena.bodies[side].attackX, hy = sArena.bodies[side].attackY;
    s32 ox = (t % BALL_COLS) * 8 + BALL_X0 - BALL_CX, oy = (t / BALL_COLS) * 8 + BALL_Y0 - BALL_CY, x, y;
    for (y = 0; y < 8; y++, m += 8)
    {
        s32 dy = oy + y, dy2 = dy * dy, ny = ntab[dy];
        s32 rowLit = ny * BALL_LY, rowU = ny * hy;
        if (dy2 > r2) { CpuFill32(0, m, 8); continue; }
        for (x = 0; x < 8; x++)
        {
            s32 dx = ox + x, d2 = dx * dx + dy2, nx, z, lit, u;
            if (d2 > r2) { m[x] = BALL_CLEAR << 5; continue; }
            if (d2 > r2in) { m[x] = BALL_OUTLINE << 5; continue; }
            nx = ntab[dx]; z = ztab[d2];
            lit = (nx * BALL_LX + rowLit + z * BALL_LZ) >> 8;
            if (lit > 210) { m[x] = BALL_LIGHT << 5; continue; }
            if (lit < 50) { m[x] = BALL_DARK << 5; continue; }
            // Bands perpendicular to the heading, 22.5 degrees apart around
            // the rolling axis: the coordinate is the angle from the top.
            u = (nx * hx + rowU) >> 8;
            if (u > 256) u = 256; else if (u < -256) u = -256;
            m[x] = (BALL_BANDED << 5) | ((asin[u] + 256) & 31);
        }
    }
}

// The colour of every map byte for one phase; each phase shifts the bands
// forward by an eighth of their period.
static void RollBallLut(u8 side, u32 phase)
{
    const struct RollBall *b = &sBall[side];
    u8 *lut = sBallHeap->side[side].lut[phase];
    u32 i;
    for (i = 0; i < 256; i++)
    {
        switch (i >> 5)
        {
        case BALL_OUTLINE: lut[i] = b->outline; break;
        case BALL_LIGHT: lut[i] = b->light; break;
        case BALL_DARK: lut[i] = b->dark; break;
        case BALL_BANDED: lut[i] = ((i - phase * 4) & 16) ? b->band : b->mid; break;
        default: lut[i] = 0;
        }
    }
}

// One tile of one phase, straight from the map through the lookup table.
static void RollBallColourTile(u8 side, u32 phase, u32 t)
{
    const struct RollBallSide *h = &sBallHeap->side[side];
    const u8 *m = h->map[t], *lut = h->lut[phase];
    u32 *dst = (u32 *)&sBallHeap->side[side].tiles[phase][t * 32];
    s32 y;
    for (y = 0; y < 8; y++, m += 8)
        dst[y] = lut[m[0]] | (lut[m[1]] << 4) | (lut[m[2]] << 8) | (lut[m[3]] << 12)
               | (lut[m[4]] << 16) | (lut[m[5]] << 20) | (lut[m[6]] << 24) | (lut[m[7]] << 28);
}

// Do some more of the work; called once per frame from the wind-up through
// the roll. A map tile costs four units, a phase's lookup table three and a
// coloured tile one; the frame's budget is smaller when the AI has just
// spent its thinking time this frame, and smaller again once the roll is
// under way.
static void RollBallTick(u8 side)
{
    struct RollBall *b = &sBall[side];
    bool8 heavy = gArenaFrameTelemetry.scanlines[1] > 40;
    s32 budget = sArena.bodies[side].actionLife ? (heavy ? 0 : 8) : (heavy ? 4 : 12);
    if (!sBallHeap) return;
    if (b->pending)
    {
        if (heavy) return;   // the setup passes wait for a quieter frame
        if (b->pending == 2) RollBallColours(side); else RollBallTables(side);
        if (!b->radius) b->pending = 0; else b->pending--;
        return;
    }
    if (!b->radius) return;
    while (budget > 0 && b->stepsDone < BALL_STEPS)
    {
        if (b->stepsDone < BALL_TILES)
        {
            RollBallMapTile(side, b->stepsDone);
            budget -= 4;
        }
        else
        {
            u32 n = b->stepsDone - BALL_TILES, phase = sBallOrder[n / BALL_TILES], t = n % BALL_TILES;
            if (!(b->lutDone & (1 << phase))) { RollBallLut(side, phase); b->lutDone |= 1 << phase; budget -= 3; }
            RollBallColourTile(side, phase, t);
            if (t == BALL_TILES - 1) b->done |= 1 << phase;
            budget -= 1;
        }
        b->stepsDone++;
    }
}

// Show the phase matching the distance rolled, written straight into the
// mon's sprite tiles: the ball sits low on the screen and the render runs
// during VBlank or the top rows, so the swap is never seen half done.
static void RollBallShow(u8 side, struct Sprite *sprite)
{
    struct RollBall *b = &sBall[side];
    u32 phase = (sArena.bodies[side].rollAngle >> 10) & (BALL_PHASES - 1), t;
    u8 *vram = (u8 *)OBJ_VRAM0 + GetSpriteTileStartByTag(MON_TAG + side * 2) * 32;
    if (!(b->done & (1 << phase))) phase &= ~1;   // an odd phase not coloured yet
    if (!b->shown)
    {
        CpuFill32(0, vram, 2048);
        b->shown = TRUE; b->phase = 0xFF;
        ArenaFeedback_Dust(sArena.bodies[side].x / Q, sArena.bodies[side].y / Q, FALSE);
    }
    if (phase != b->phase)
    {
        for (t = 0; t < BALL_ROWS; t++)
            CpuFastCopy(&sBallHeap->side[side].tiles[phase][t * BALL_COLS * 32],
                        vram + ((t + BALL_Y0 / 8) * 8 + BALL_X0 / 8) * 32, BALL_COLS * 32);
        b->phase = phase;
    }
    // The ball's centre sits at the body's position, like the pose's.
    sprite->x += 32 - BALL_CX;
    SetOamMatrix(sprite->oam.matrixNum, 0x100, 0, 0, 0x100);
}

// Back to the mon's own frames once the roll is over.
static void RollBallHide(u8 side)
{
    if (!sBall[side].shown) return;
    sBall[side].shown = FALSE;
    sArena.bodies[side].drawnFrame = 255;
}
