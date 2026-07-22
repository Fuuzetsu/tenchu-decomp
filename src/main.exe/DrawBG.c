#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawBG(struct BackGround *bg);
 *     3DCTRL.C:686, 6 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *  - m2c undercounts FUN_80063b94's call args: `bg` itself (a0) is carried
 *    in live from the caller and never overwritten before the jal, so
 *    m2c's basic-block-local view misses it — Ghidra's 4-arg rendering
 *    (bg, bg->work, OTablePt, bg->sz) is the real call (same undercount
 *    pattern as lePackEnemyLayout's memcpy/AdtMessageBox).
 *  - `bg->attribute & 1` is a narrowing (mask-only) use, so cc1 emits `lhu`
 *    even though the field is Ghidra-typed signed `short` (same rule as the
 *    item TU's lifemax/it narrowing loads).
 *  - OTablePt is %gp_rel in this TU (tools/gpsyms.py --write; Build.hs
 *    maspsxGpExterns + permute.py GP_EXTERNS both list DrawBG now).
 *  - Register tie: a single `ret=0; if(cond){...; ret=1;} return ret;`
 *    shared-variable form puts the `ret=0` default BEFORE the condition
 *    test, so its pseudo is simultaneously live with the test's own v0 temp
 *    (regalloc.py showed an explicit conflict edge to hard reg v0) and gets
 *    evicted to v1 + a trailing `move v0,v1` at the return. Two early
 *    returns (`if (cond) {...; return 1;} return 0;`) let cc1 target v0
 *    directly for each constant with no competing live temp — 0 bytes.
 *    (This is the opposite failure mode from InsertConflict's shared-`ret`
 *    fix: there splitting early returns avoided a copy-PREFERENCE toward a
 *    callee-saved reg; here the shared variable caused a hard CONFLICT with
 *    a caller-saved temp. Try both shapes when a flag-return is off by a
 *    register.)
 */
extern void FUN_80063b94(BackGround *bg, u32 *work, GsOT *ot, u16 sz);

short DrawBG(BackGround *bg)
{
    if ((bg->attribute & 1) == 0)
    {
        FUN_80063b94(bg, bg->work, OTablePt, bg->sz);
        return 1;
    }
    return 0;
}
