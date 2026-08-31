#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetScreenPositionS(long x, long y, long z, struct SVECTOR *scr);
 *     EFFECT.C:588, 11 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * source-shaped) with that SVECTOR, the caller's own output pointer `scr`
 * (passed through unmodified — RotTransPers presumably writes screen xy
 * into it internally), and two more scratchpad slots
 * (Scratchpad+0/Scratchpad+0x10). RotTransPers's returned OTZ (depth) gets
 * written to `scr->vz` at +4 bytes.
 *
 * ViewInfo.vpx/vpy/vpz are canonical GsRVIEW2 `long` fields (s32 on PsyQ,
 * matching Ghidra's independently-built GsRVIEW2 — see ReqItemDefault.c).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `x - (short)ViewInfo.vpx` is a NARROWING use (the result stores
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

void GetScreenPositionS(s32 x, s32 y, s32 z, SVECTOR *scr)
{
    SVECTOR *sv = (SVECTOR *)TENCHU_SCRATCHPAD(0x80);

    sv->vx = x - (short)ViewInfo.vpx;
    sv->vy = y - (short)ViewInfo.vpy;
    sv->vz = z - (short)ViewInfo.vpz;
    scr->vz = RotTransPers(
        sv, (s32 *)scr, (s32 *)TENCHU_SCRATCHPAD_ADDRESS,
        (s32 *)TENCHU_SCRATCHPAD(0x10));
}
