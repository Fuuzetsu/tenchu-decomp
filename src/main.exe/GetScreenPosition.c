#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetScreenPosition(long x, long y, long z, struct SVECTOR *scr);
 *     EFFECT.C:543, 32 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       struct SVECTOR * scr
 * END PSX.SYM */

/*
 * GetScreenPosition (0x800396c0, 0x9c bytes) — camera-relative coordinate
 * transform + perspective project: zeroes the GTE scratchpad MATRIX's
 * translation vector (fixed PS1 scratchpad RAM at 0x1F800000, same
 * `MATRIX *m` idiom as PrepareGetScreenPositionS.c's t[0..2] @ 0x14/0x18/0x1c), writes
 * (x,y,z) - ViewInfo.(vpx,vpy,vpz) into a scratchpad SVECTOR @ 0x1F800020
 * (same idiom as the twin GetScreenPositionS.c's sv @ 0x1F800080), installs the
 * (all-zero) translation and the global world-space rotation matrix
 * GsWSMATRIX, then calls RotTransPers (GTE perspective-transform library
 * wrapper, 0x80078704 > 0x80060000, precompiled) with that SVECTOR, the
 * caller's own output pointer `scr` (passed through unmodified), and two more
 * scratchpad slots (+0x28/+0x2c). RotTransPers's returned OTZ (depth) is
 * written to `scr->vz` at +4 bytes — identical tail
 * to the twin.
 *
 * ViewInfo.vpx/vpy/vpz: canonical GsRVIEW2 `long` fields (s32 on PsyQ,
 * Ghidra's own independently-built GsRVIEW2 — see ReqItemDefault.c/
 * GetScreenPositionS.c). SetTransMatrix/SetRotMatrix each take a single MATRIX*
 * (PrepareGetScreenPositionS.c proves both signatures); the OTHER a-registers still
 * holding values at those call sites are leftover from the adjacent
 * sv->vy/vz stores, not real arguments — m2c over-counts both calls'
 * argument lists by reading those live regs as args.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `x - (short)ViewInfo.vpx` is a NARROWING use (the result stores into
 *    a scratchpad s16 field) of a s32 global's LOW HALF — cc1 emits `lhu`
 *    for it, same rule as the twin.
 *  - Raw scratchpad addresses are plain integer-literal pointer casts,
 *    never a shared "Scratchpad + offset" symbol (PrepareGetScreenPositionS.c/
 *    GetScreenPositionS.c precedent).
 */

extern MATRIX GsWSMATRIX;

void GetScreenPosition(long x, long y, long z, SVECTOR *scr)
{
    MATRIX *m = (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS;
    SVECTOR *sv = (SVECTOR *)TENCHU_SCRATCHPAD(SCRATCH_POINT);

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    sv->vx = x - (short)ViewInfo.vpx;
    sv->vy = y - (short)ViewInfo.vpy;
    sv->vz = z - (short)ViewInfo.vpz;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
    scr->vz = RotTransPers(sv, (s32 *)scr,
                           (s32 *)TENCHU_SCRATCHPAD(SCRATCH_RTP_P),
                           (s32 *)TENCHU_SCRATCHPAD(SCRATCH_RTP_FLAG));
}
