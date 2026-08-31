#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTarget(long x, long y, long z, long color);
 *     EFFECT.C:602, 6 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * calls DrawTargetS(x, y, otz - 5, color) — a line-draw/sort helper
 * (DrawTargetS's only other caller, draw_map_items_, is also unmatched).
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `x - (s16)ViewInfo.vpx` etc. — same NARROWING lhu-of-a-s32-global
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
 * crosses the call, it is cached in `$s0`; `color` consequently takes `$s1`.
 * The DrawTargetS arguments deliberately use direct `scr.vx/vy/vz` member
 * spellings, so the values reload as `lh` from `$sp` after the pointer dies.
 * Using `scr` directly for every access drops the saved pointer and shrinks
 * the frame to 0x20; using separate x/y/otz scalars does not establish a
 * legal shared object for RotTransPers's packed SXY write and leaves the OTZ
 * value live in a register instead of round-tripping through the stack.
 */

extern MATRIX GsWSMATRIX;
extern void DrawTargetS(s32 x, s32 y, s32 z, s32 color);

void DrawTarget(s32 x, s32 y, s32 z, s32 color)
{
    SVECTOR scr;
    SVECTOR *p;

    *(s32 *)TENCHU_SCRATCHPAD(SCRATCH_LS_TX) = 0;
    *(s32 *)TENCHU_SCRATCHPAD(SCRATCH_LS_TY) = 0;
    *(s32 *)TENCHU_SCRATCHPAD(SCRATCH_LS_TZ) = 0;
    *(s16 *)TENCHU_SCRATCHPAD(SCRATCH_POINT_X) = x - (s16)ViewInfo.vpx;
    *(s16 *)TENCHU_SCRATCHPAD(SCRATCH_POINT_Y) = y - (s16)ViewInfo.vpy;
    *(s16 *)TENCHU_SCRATCHPAD(SCRATCH_POINT_Z) = z - (s16)ViewInfo.vpz;
    SetTransMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
    SetRotMatrix(&GsWSMATRIX);
    p = &scr;
    p->vz = RotTransPers((SVECTOR *)TENCHU_SCRATCHPAD(SCRATCH_POINT), (s32 *)p,
                         (s32 *)TENCHU_SCRATCHPAD(SCRATCH_RTP_P),
                         (s32 *)TENCHU_SCRATCHPAD(SCRATCH_RTP_FLAG));
    DrawTargetS(scr.vx, scr.vy, scr.vz - 5, color);
}
