#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawBG(struct BackGround *bg);
 *     3DCTRL.C:686, 6 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct BackGround * bg
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * DrawBG (0x80018818, 0x44 bytes) — sort the background layer into the GsOT
 * if it's enabled (attribute bit 0 clear); returns whether it drew.
 *
 * BackGround uses the complete PSX.SYM layout shared in game_types.h.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - m2c undercounts GsSortFixBg16's call args: `bg` itself (a0) is carried
 *    in live from the caller and never overwritten before the jal, so
 *    m2c's basic-block-local view misses it — Ghidra's 4-arg rendering
 *    (bg, bg->work, OTablePt, bg->sz) is the real call (same undercount
 *    pattern as lePackEnemyLayout's memcpy/AdtMessageBox).
 *  - `bg->attribute & 1` is a narrowing (mask-only) use, so cc1 emits `lhu`
 *    even though the field is Ghidra-typed signed `short` (same rule as the
 *    other narrowing field loads).
 *  - OTablePt is %gp_rel in this TU (tools/gpsyms.py --write; Build.hs
 *    maspsxGpExterns + permute.py GP_EXTERNS both list DrawBG now).
 *  - Register tie: a single `ret=0; if(cond){...; ret=1;} return ret;`
 *    shared-variable form puts the `ret=0` default BEFORE the condition
 *    test, so its pseudo is simultaneously live with the test's own v0 temp
 *    (regalloc.py showed an explicit conflict edge to hard reg v0) and gets
 *    evicted to v1 + a trailing `move v0,v1` at the return. The inverse
 *    disabled-bit guard followed by the draw and `return 1` lets cc1 target
 *    v0 directly for each constant with no competing live temp — 0 bytes.
 *    (This is the opposite failure mode from InsertConflict's shared-`ret`
 *    fix: there splitting early returns avoided a copy-PREFERENCE toward a
 *    callee-saved reg; here the shared variable caused a hard CONFLICT with
 *    a caller-saved temp. Try both shapes when a flag-return is off by a
 *    register.)
 */
/* Official libgs name: sits immediately before GsInitFixBg16 in the
 * same module order as the demo's GsSortFixBg32/GsInitFixBg32 pair
 * (the demo's DrawBG called the Bg32 variant; retail switched to
 * 16x16 cells). The 4-arg shape is the real implementation ABI. */
extern void GsSortFixBg16(BackGround *bg, u32 *work, GsOT *ot, u16 sz);

short DrawBG(BackGround *bg)
{
    if ((bg->attribute & 1) != 0)
    {
        return 0;
    }
    GsSortFixBg16(bg, bg->work, OTablePt, bg->sz);
    return 1;
}
