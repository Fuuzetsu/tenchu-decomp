#include "common.h"
#include "main.exe.h"

/*
 * FUN_800396c0 (0x800396c0, 0x9c bytes) — camera-relative coordinate
 * transform + perspective project: zeroes the GTE scratchpad MATRIX's
 * translation vector (fixed PS1 scratchpad RAM at 0x1F800000, same
 * `MATRIX *m` idiom as FUN_80039684.c's t[0..2] @ 0x14/0x18/0x1c), writes
 * (x,y,z) - ViewInfo.(vpx,vpy,vpz) into a scratchpad SVECTOR @ 0x1F800020
 * (same idiom as the twin FUN_80039610.c's sv @ 0x1F800080), installs the
 * (all-zero) translation and the global world-space rotation matrix
 * GsWSMATRIX, then calls RotTransPers (GTE perspective-transform library
 * wrapper, 0x80078704 > 0x80060000, precompiled) with that SVECTOR, the
 * caller's own output pointer arg3 (passed through unmodified), and two more
 * scratchpad slots (+0x28/+0x2c). RotTransPers's returned OTZ (depth) is
 * written to arg3's second word (`arg3[1]`, i.e. +4 bytes) — identical tail
 * to the twin.
 *
 * ViewInfo.vpx/vpy/vpz: item.h-style proven TViewInfo fields (s32, matching
 * Ghidra's own independently-built GsRVIEW2 — see ReqItemDefault.c/
 * FUN_80039610.c). SetTransMatrix/SetRotMatrix each take a single MATRIX*
 * (FUN_80039684.c proves both signatures); the OTHER a-registers still
 * holding values at those call sites are leftover from the adjacent
 * sv->vy/vz stores, not real arguments — m2c over-counts both calls'
 * argument lists by reading those live regs as args.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `arg0 - (short)ViewInfo.vpx` is a NARROWING use (the result stores into
 *    a scratchpad s16 field) of a s32 global's LOW HALF — cc1 emits `lhu`
 *    for it, same rule as the twin.
 *  - Raw scratchpad addresses are plain integer-literal pointer casts,
 *    never a shared "Scratchpad + offset" symbol (FUN_80039684.c/
 *    FUN_80039610.c precedent).
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

void FUN_800396c0(s32 arg0, s32 arg1, s32 arg2, s32 *arg3)
{
    MATRIX *m = (MATRIX *)0x1F800000;
    SVECTOR *sv = (SVECTOR *)0x1F800020;

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    sv->vx = arg0 - (short)ViewInfo.vpx;
    sv->vy = arg1 - (short)ViewInfo.vpy;
    sv->vz = arg2 - (short)ViewInfo.vpz;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
    *(short *)(arg3 + 1) = RotTransPers(sv, arg3, (void *)0x1F800028, (void *)0x1F80002C);
}
