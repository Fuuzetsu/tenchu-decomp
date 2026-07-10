#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DemoPatchInit(void);
 *     INFOVIEW.C:1196, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct RECT rc
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char DemoBackupArea[64];
 * END PSX.SYM */

/*
 * DemoPatchInit (0x8004c044) — snapshot a small VRAM rect (x=0x3f0,y=0x1ff,
 * w=0x10,h=1 — a single 16-pixel row, i.e. a 16-colour CLUT) into
 * DemoBackupArea via the PSYQ libgpu StoreImage2, then DrawSync(0) waits for
 * the transfer to finish. Same per-TU RECT convention as FUN_80038ce0.c (no
 * libgpu.h vendored in this repo).
 */
typedef struct
{
    s16 x;                        /* 0x0 */
    s16 y;                        /* 0x2 */
    s16 w;                        /* 0x4 */
    s16 h;                        /* 0x6 */
} RECT;                           /* 0x8 (PSYQ libgpu.h) */

extern int StoreImage2(RECT *rect, u_long *p);
extern int DrawSync(int mode);
extern u_long DemoBackupArea[];

void DemoPatchInit(void)
{
    RECT r;

    r.x = 0x3f0;
    r.y = 0x1ff;
    r.w = 0x10;
    r.h = 1;
    StoreImage2(&r, DemoBackupArea);
    DrawSync(0);
}
