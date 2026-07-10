#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadTIM(unsigned long *adr);
 *     3DCTRL.C:718, 27 src lines, frame 64 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned long * adr
 *     stack sp+16     struct RECT rect
 *     stack sp+24     struct GsIMAGE tim
 * END PSX.SYM */

/*
 * LoadTIM (0x80018904, 0xb0 bytes) - loads a TIM's pixel data (and, when the
 * TIM carries a CLUT per GsIMAGE's pmode bit 3, its CLUT too) via the PSYQ
 * libgpu LoadImage, having fetched the image geometry through GsGetTimInfo
 * (same "skip the leading u_long ID word" convention as GetTIMInfo.c).
 * SystemOut is annotated noreturn by Ghidra but the compiled code falls
 * straight through after the call (no early return) - same shape as
 * InsertConflict.c/LoadAreaMap.c's identical SystemOut-then-continue idiom.
 * Per-TU RECT convention as FUN_80038ce0.c/DemoPatchInit.c (no libgpu.h
 * vendored in this repo). All matched callers (BriefingAndInventorySelectionScreen.c,
 * FUN_8004f598.c, LoadTIMAndFree.c) already pin the prototype as
 * `void LoadTIM(u_long *tim)` - the DrawSync(0) return value is discarded
 * (no move/sign-extend after the call in the asm), matching void.
 */
typedef struct
{
    s16 x;                        /* 0x0 */
    s16 y;                        /* 0x2 */
    s16 w;                        /* 0x4 */
    s16 h;                        /* 0x6 */
} RECT;                           /* 0x8 (PSYQ libgpu.h) */

extern int LoadImage(RECT *rect, u_long *p);
extern void GsGetTimInfo(u_long *tim, GsIMAGE *im);
extern void SystemOut(char *msg);
extern int DrawSync(int mode);
extern char D_800110B8[]; /* "NO IMAGE DATA" */

void LoadTIM(u_long *adr)
{
    RECT r;
    GsIMAGE im;

    if (adr == 0) {
        SystemOut(D_800110B8);
    }
    GsGetTimInfo(adr + 1, &im);
    r.x = im.px;
    r.y = im.py;
    r.w = im.pw;
    r.h = im.ph;
    LoadImage(&r, im.pixel);
    if ((im.pmode >> 3) & 1) {
        r.x = im.cx;
        r.y = im.cy;
        r.w = im.cw;
        r.h = im.ch;
        LoadImage(&r, im.clut);
    }
    DrawSync(0);
}
