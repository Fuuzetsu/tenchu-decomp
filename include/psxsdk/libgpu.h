#ifndef PSXSDK_LIBGPU_H
#define PSXSDK_LIBGPU_H

#include <types.h>

/*
 * PsyQ LIBGPU ABI declarations used by Tenchu.
 *
 * The names, field order, and signedness below follow Sony's Release 4.4-4.6
 * LIBGPU.H headers and are independently present in the demo's PSX.SYM.  Keep
 * SDK-owned records here instead of recreating layout-compatible local types.
 */

typedef struct RECT RECT;
struct RECT
{
    short x, y;
    short w, h;
};

typedef struct RECT32 RECT32;
struct RECT32
{
    int x, y;
    int w, h;
};

typedef struct DR_ENV DR_ENV;
struct DR_ENV
{
    u_long tag;
    u_long code[15];
};

typedef struct DRAWENV DRAWENV;
struct DRAWENV
{
    RECT clip;
    short ofs[2];
    RECT tw;
    u_short tpage;
    u_char dtd;
    u_char dfe;
    u_char isbg;
    u_char r0, g0, b0;
    DR_ENV dr_env;
};

typedef struct DISPENV DISPENV;
struct DISPENV
{
    RECT disp;
    RECT screen;
    u_char isinter;
    u_char isrgb24;
    u_char pad0, pad1;
};

typedef struct P_TAG P_TAG;
struct P_TAG
{
    unsigned addr : 24;
    unsigned len : 8;
    u_char r0, g0, b0, code;
};

typedef struct P_CODE P_CODE;
struct P_CODE
{
    u_char r0, g0, b0, code;
};

typedef struct POLY_F3 POLY_F3;
struct POLY_F3
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
};

typedef struct POLY_F4 POLY_F4;
struct POLY_F4
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
};

typedef struct POLY_FT3 POLY_FT3;
struct POLY_FT3
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
    short x1, y1;
    u_char u1, v1;
    u_short tpage;
    short x2, y2;
    u_char u2, v2;
    u_short pad1;
};

typedef struct POLY_FT4 POLY_FT4;
struct POLY_FT4
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
    short x1, y1;
    u_char u1, v1;
    u_short tpage;
    short x2, y2;
    u_char u2, v2;
    u_short pad1;
    short x3, y3;
    u_char u3, v3;
    u_short pad2;
};

typedef struct POLY_G3 POLY_G3;
struct POLY_G3
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, pad1;
    short x1, y1;
    u_char r2, g2, b2, pad2;
    short x2, y2;
};

typedef struct POLY_G4 POLY_G4;
struct POLY_G4
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, pad1;
    short x1, y1;
    u_char r2, g2, b2, pad2;
    short x2, y2;
    u_char r3, g3, b3, pad3;
    short x3, y3;
};

typedef struct POLY_GT3 POLY_GT3;
struct POLY_GT3
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char u1, v1;
    u_short tpage;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_char u2, v2;
    u_short pad2;
};

typedef struct POLY_GT4 POLY_GT4;
struct POLY_GT4
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char u1, v1;
    u_short tpage;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_char u2, v2;
    u_short pad2;
    u_char r3, g3, b3, p3;
    short x3, y3;
    u_char u3, v3;
    u_short pad3;
};

typedef struct LINE_F2 LINE_F2;
struct LINE_F2
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
};

typedef struct LINE_G2 LINE_G2;
struct LINE_G2
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, p1;
    short x1, y1;
};

typedef struct LINE_F3 LINE_F3;
struct LINE_F3
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    u_long pad;
};

typedef struct LINE_G3 LINE_G3;
struct LINE_G3
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_long pad;
};

typedef struct LINE_F4 LINE_F4;
struct LINE_F4
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
    u_long pad;
};

typedef struct LINE_G4 LINE_G4;
struct LINE_G4
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_char r3, g3, b3, p3;
    short x3, y3;
    u_long pad;
};

typedef struct SPRT SPRT;
struct SPRT
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
    short w, h;
};

typedef struct SPRT_16 SPRT_16;
struct SPRT_16
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
};

typedef struct SPRT_8 SPRT_8;
struct SPRT_8
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
};

typedef struct TILE TILE;
struct TILE
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short w, h;
};

typedef struct TILE_16 TILE_16;
struct TILE_16
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
};

typedef struct TILE_8 TILE_8;
struct TILE_8
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
};

typedef struct TILE_1 TILE_1;
struct TILE_1
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
};

typedef struct DR_MODE DR_MODE;
struct DR_MODE
{
    u_long tag;
    u_long code[2];
};

typedef struct DR_TWIN DR_TWIN;
struct DR_TWIN
{
    u_long tag;
    u_long code[2];
};

typedef struct DR_AREA DR_AREA;
struct DR_AREA
{
    u_long tag;
    u_long code[2];
};

typedef struct DR_OFFSET DR_OFFSET;
struct DR_OFFSET
{
    u_long tag;
    u_long code[2];
};

typedef struct DR_STP DR_STP;
struct DR_STP
{
    u_long tag;
    u_long code[2];
};

typedef struct DR_MOVE DR_MOVE;
struct DR_MOVE
{
    u_long tag;
    u_long code[5];
};

typedef struct DR_LOAD DR_LOAD;
struct DR_LOAD
{
    u_long tag;
    u_long code[3];
    u_long p[13];
};

typedef struct DR_TPAGE DR_TPAGE;
struct DR_TPAGE
{
    u_long tag;
    u_long code[1];
};

typedef struct TIM_IMAGE TIM_IMAGE;
struct TIM_IMAGE
{
    u_long mode;
    RECT *crect;
    u_long *caddr;
    RECT *prect;
    u_long *paddr;
};

/* Common LIBGPU macros retain Sony's original comma-expression shape. */
#define setRECT(r, _x, _y, _w, _h) \
    (r)->x = (_x), (r)->y = (_y), (r)->w = (_w), (r)->h = (_h)

#define setRGB0(p, _r0, _g0, _b0) \
    (p)->r0 = (_r0), (p)->g0 = (_g0), (p)->b0 = (_b0)

#define setXY4(p, _x0, _y0, _x1, _y1, _x2, _y2, _x3, _y3) \
    (p)->x0 = (_x0), (p)->y0 = (_y0), \
    (p)->x1 = (_x1), (p)->y1 = (_y1), \
    (p)->x2 = (_x2), (p)->y2 = (_y2), \
    (p)->x3 = (_x3), (p)->y3 = (_y3)

#define setlen(p, _len) (((P_TAG *)(p))->len = (u_char)(_len))
#define setcode(p, _code) (((P_TAG *)(p))->code = (u_char)(_code))
#define setPolyF4(p) setlen((p), 5), setcode((p), 0x28)

DISPENV *GetDispEnv(DISPENV *env);
DISPENV *PutDispEnv(DISPENV *env);
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h);
DRAWENV *GetDrawEnv(DRAWENV *env);
DRAWENV *PutDrawEnv(DRAWENV *env);
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h);

int FntOpen(int x, int y, int w, int h, int isbg, int n);
int FntPrint();
u_long *FntFlush(int id);
void FntLoad(int tx, int ty);
int ResetGraph(int mode);
void SetDispMask(int mask);

void AddPrim(void *ot, void *p);
int ClearImage(RECT *rect, u_char r, u_char g, u_char b);
int ClearImage2(RECT *rect, u_char r, u_char g, u_char b);
void DrawPrim(void *p);
int DrawSync(int mode);
int LoadImage(RECT *rect, u_long *p);
int LoadImage2(RECT *rect, u_long *p);
int StoreImage(RECT *rect, u_long *p);
int StoreImage2(RECT *rect, u_long *p);
int MoveImage(RECT *rect, int x, int y);
int MoveImage2(RECT *rect, int x, int y);
void SetDrawMove(DR_MOVE *p, RECT *rect, int x, int y);
void SetPolyF4(POLY_F4 *p);
void SetPolyFT4(POLY_FT4 *p);
void SetPolyGT4(POLY_GT4 *p);
void SetSemiTrans(void *p, int abe);
u_short GetClut(int x, int y);
u_short GetTPage(int tp, int abr, int x, int y);

#endif
