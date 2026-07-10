#include "common.h"
#include "main.exe.h"

/*
 * AdtReleaseDisp (0x8005fca4, 0x90 bytes) — counterpart of AdtGetDisp:
 * reloads the font adapter's PSYQ FntLoad/FntOpen state from D_8008F1B8
 * (same global as AdtFntLoad.c/AdtFntOpen.c/AdtQuiet.c — this TU reaches
 * ALL EIGHT fields x/y/w/h/isbg/n@0x0-0x14 and tx/ty@0x18/0x1c, so it
 * declares the full 8-field view), restores the saved screen region from
 * the backup buffer via LoadImage, then reinstalls the draw/display
 * environments via PutDrawEnv/PutDispEnv.
 *
 * Ghidra's decompilation types the parameter `DRAWENV *param_1` and reaches
 * the backup region through `param_1[1].tpage`/`param_1[1].dr_env.tag` —
 * an array-indexing artifact (DRAWENV is 0x5c bytes, so param_1[1] lands at
 * +0x5c, and DRAWENV's own tpage@0x14/dr_env@0x1c fields inside THAT slot
 * fall at +0x70/+0x78). Those are really TAdtDisp's OWN `rect`@0x70 and
 * `backup`@0x78 fields (reference/psxsym-types.h) — the real parameter is
 * `TAdtDisp *`, not a DRAWENV array. `LoadImage(&ad->rect, ad->backup)`
 * reproduces the exact same addresses without the array-indexing fiction.
 */
typedef struct
{
    s16 x, y, w, h;
} RECT; /* 0x8 (PSYQ libgpu.h) */

typedef struct
{
    RECT clip;       /* +0x00 */
    s16 ofs[2];       /* +0x08 */
    RECT tw;         /* +0x0c */
    u16 tpage;        /* +0x14 */
    u8 dtd, dfe, isbg, r0, g0, b0; /* +0x16..+0x1b */
    u32 dr_env[16];   /* +0x1c: tag + code[15] */
} DRAWENV; /* 0x5c (PSYQ libgpu.h) */

typedef struct
{
    RECT disp;   /* +0x00 */
    RECT screen; /* +0x08 */
    u8 isinter, isrgb24, pad0, pad1; /* +0x10..+0x13 */
} DISPENV; /* 0x14 (PSYQ libgpu.h) */

typedef struct
{
    DRAWENV draw;      /* +0x0000 */
    DISPENV disp;      /* +0x005c */
    RECT rect;         /* +0x0070 */
    u8 backup[0x8000]; /* +0x0078 */
} TAdtDisp;

typedef struct
{
    s32 x, y, w, h, isbg, n;
    s32 tx, ty;
} AdtFntState;

extern AdtFntState D_8008F1B8;
extern void FntLoad(int tx, int ty);
extern int FntOpen(int x, int y, int w, int h, int isbg, int n);
extern int LoadImage(RECT *rect, u_long *p);
extern int DrawSync(int mode);
extern void PutDrawEnv(DRAWENV *env);
extern void PutDispEnv(DISPENV *env);

void AdtReleaseDisp(TAdtDisp *ad)
{
    FntLoad(D_8008F1B8.tx, D_8008F1B8.ty);
    FntOpen(D_8008F1B8.x, D_8008F1B8.y, D_8008F1B8.w, D_8008F1B8.h,
            D_8008F1B8.isbg, D_8008F1B8.n);
    LoadImage(&ad->rect, (u_long *)ad->backup);
    DrawSync(0);
    PutDrawEnv(&ad->draw);
    PutDispEnv(&ad->disp);
}
