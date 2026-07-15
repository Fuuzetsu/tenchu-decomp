#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetScreenPositionS(long x, long y, long z, struct SVECTOR *scr);
 *     EFFECT.C:588, 11 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a3       struct SVECTOR * scr
 * END PSX.SYM */

/*
 * GetScreenPositionS (0x80039610, 0x74 bytes) — camera-relative coordinate
 * transform helper: writes (x,y,z) - ViewInfo.(vpx,vpy,vpz) as a scratchpad
 * SVECTOR (fixed PS1 scratchpad RAM at 0x1F800000 — same region
 * PrepareGetScreenPositionS.c's MATRIX uses, Ghidra names it "Scratchpad"), then calls
 * RotTransPers (a GTE perspective-transform library wrapper, 0x80078704 >
 * 0x80060000, precompiled — only this call site's argument setup is
 * source-shaped) with that SVECTOR, the caller's own output pointer arg3
 * (passed through unmodified — RotTransPers presumably writes screen xy
 * into it internally), and two more scratchpad slots
 * (Scratchpad+0/Scratchpad+0x10). RotTransPers's returned OTZ (depth) gets
 * written to arg3's second word (`arg3[1]`, i.e. +4 bytes).
 *
 * ViewInfo.vpx/vpy/vpz are item.h-style proven TViewInfo fields (s32,
 * matching Ghidra's own independently-built GsRVIEW2 — see
 * ReqItemDefault.c). Reused here as a local typedef (not a shared header)
 * matching this repo's per-TU convention for this struct.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `arg0 - (short)ViewInfo.vpx` is a NARROWING use (the result stores
 *    into a scratchpad s16 field) of a s32 global's LOW HALF — cc1 emits
 *    `lhu` for it (the same "sign bits are provably dead" rule as
 *    DeleteConflict's ConflictObjects, just with an extra explicit
 *    narrowing cast on top; the outer store's truncation is what licenses
 *    it, not the cast alone).
 *  - Raw scratchpad addresses are plain integer-literal pointer casts
 *    (`(SVECTOR *)0x1F800080`), never a shared "Scratchpad + offset"
 *    symbol — matches PrepareGetScreenPositionS.c's `(MATRIX *)0x1F800000` precedent.
 *    A raw-constant cast materializes via `lui`+`ori` (not `addiu`): the
 *    `li` macro treats it as an unsigned bit pattern, unlike a relocatable
 *    symbol's `%lo()` (always `addiu`).
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
extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);

void GetScreenPositionS(s32 arg0, s32 arg1, s32 arg2, s32 *arg3)
{
    SVECTOR *sv = (SVECTOR *)0x1F800080;

    sv->vx = arg0 - (short)ViewInfo.vpx;
    sv->vy = arg1 - (short)ViewInfo.vpy;
    sv->vz = arg2 - (short)ViewInfo.vpz;
    *(short *)(arg3 + 1) = RotTransPers(sv, arg3, (void *)0x1F800000, (void *)0x1F800010);
}
