#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTarget(long x, long y, long z, long color);
 *     EFFECT.C:602, 6 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long color
 *     stack sp+16     struct SVECTOR scr
 * END PSX.SYM */

/*
 * DrawTarget (0x80039544, 0xcc bytes) — same camera-relative-transform +
 * perspective-project shape as the twin GetScreenPosition.c (same TU: see
 * GetScreenPositionS.c/PrepareGetScreenPositionS.c for the scratchpad MATRIX/SVECTOR idiom),
 * but instead of writing OTZ into a caller-supplied output pointer, it reads
 * RotTransPers's packed screen (x,y) back off its own stack scratch and
 * calls DrawTargetS(x, y, otz - 5, arg3) — a line-draw/sort helper
 * (DrawTargetS's only other caller, FUN_8003d768, is also unmatched).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `arg0 - (short)ViewInfo.vpx` etc. — same NARROWING lhu-of-a-s32-global
 *    rule as the twin.
 *  - RotTransPers's `sxy` out-param is the packed `vx/vy` prefix of one
 *    address-taken `SVECTOR scr`; its return value is stored into `scr.vz`.
 *    This is the original PSX.SYM local and legally explains the adjacent
 *    stack shorts plus the later independent `lh` readbacks.
 *  - Scratchpad zero/coordinate stores are FLAT `*(s32/s16 *)0x1F8000xx`
 *    casts, one macro expansion each (repeated fresh `lui $at,0x1F80` per
 *    store) — NOT a shared cached `MATRIX *`/`SVECTOR *` local like
 *    GetScreenPosition.c/PrepareGetScreenPositionS.c use for the same scratchpad region: this
 *    function's asm never reuses one register across the individual
 *    zero/coordinate stores, unlike the twins.
 *
 * STATUS: MATCHING — exact 204-byte / 51-instruction pure C with the target
 * 0x28 frame. A short-lived `SVECTOR *p = &scr` supplies the RotTransPers
 * SXY argument and the post-call `p->vz` writeback. Because that alias
 * crosses the call, it is cached in `$s0`; `arg3` consequently takes `$s1`.
 * The DrawTargetS arguments deliberately use direct `scr.vx/vy/vz` member
 * spellings, so the values reload as `lh` from `$sp` after the pointer dies.
 * Using `scr` directly for every access drops the saved pointer and shrinks
 * the frame to 0x20; using separate x/y/otz scalars does not establish a
 * legal shared object for RotTransPers's packed SXY write and leaves the OTZ
 * value live in a register instead of round-tripping through the stack.
 */

typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
} TViewInfo;

extern TViewInfo ViewInfo;
extern MATRIX GsWSMATRIX;
extern void SetTransMatrix(MATRIX *m);
extern void SetRotMatrix(MATRIX *m);
extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);
extern void DrawTargetS(s32 x, s32 y, s32 z, s32 arg3);

void DrawTarget(s32 arg0, s32 arg1, s32 arg2, s32 arg3)
{
    SVECTOR scr;
    SVECTOR *p;

    *(s32 *)0x1F800014 = 0;
    *(s32 *)0x1F800018 = 0;
    *(s32 *)0x1F80001C = 0;
    *(s16 *)0x1F800020 = arg0 - (short)ViewInfo.vpx;
    *(s16 *)0x1F800022 = arg1 - (short)ViewInfo.vpy;
    *(s16 *)0x1F800024 = arg2 - (short)ViewInfo.vpz;
    SetTransMatrix((MATRIX *)0x1F800000);
    SetRotMatrix(&GsWSMATRIX);
    p = &scr;
    p->vz = RotTransPers((SVECTOR *)0x1F800020, (s32 *)p, (void *)0x1F800028, (void *)0x1F80002C);
    DrawTargetS(scr.vx, scr.vy, scr.vz - 5, arg3);
}
