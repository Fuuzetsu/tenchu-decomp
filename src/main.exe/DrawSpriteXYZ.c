#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawSpriteXYZ(struct GsSPRITE *sprt, long x, long y, long z, long scale);
 *     EFFECT.C:204, 11 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct GsSPRITE * sprt
 *     param $a1       long x
 *     param $a2       long y
 *     param $a3       long z
 *     param stack+16  long scale
 *     stack sp+16     struct SVECTOR scr
 * END PSX.SYM */

/*
 * MATCH.
 *
 * DrawSpriteXYZ (0x8003a2a8, 0xf8 bytes) — shared "project a 3D point and
 * sort-draw a sprite there" epilogue: calls GetScreenPosition (camera-relative
 * transform + RotTransPers) to get a screen (x,y) and OTZ depth, bails if
 * the point is behind/too close (`otz <= 0x24`), else derives a uniform
 * scale from `scale*300/otz`, writes the sprite's x/y/scalex/scaley, and
 * GsSortSprite's it into the OT at a depth-derived priority (clamped to
 * [0, 0x4e1]) — the identical tail DrawBlood.c's own Ghidra decompilation
 * shows twice (once inline, once via `goto LAB_8003318c`), matching this
 * original EFFECT.C helper's role across the Draw* family. Its identified
 * retail caller is the still-unmatched FUN_8004c350.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - This TU divides by a runtime value (`scale*300/otz`) — needed
 *    `--expand-div` (Build.hs maspsxGpExterns' `extra` list + permute.py's
 *    MASPSX_EXTRA) for ASPSX's guarded bnez/break-7/break-6 expansion;
 *    without it the whole division-safety preamble is simply missing
 *    (10 fewer instructions, not just a different encoding).
 *  - The final `[0, 0x4e1]` clamp (`if (t<0) pri=0; else { pri=0x4e1; if
 *    (t<0x4e2) pri=t; }`) needed an explicit `goto` for the `t<0` case
 *    rather than a plain `if/else`: cc1 places an if/else's THEN branch as
 *    the fall-through and negates the condition to branch away for the
 *    ELSE (`if (t<0) {A} else {B}` compiles the test as `if (!(t<0))
 *    goto B;`, i.e. `bgez`) — but the target's own branch is `bltz`
 *    (testing `t<0` directly, branching AWAY for the zero case, with the
 *    clamp as fall-through). Since the "then" body here (`pri=0`) is
 *    trivial and target wants it as the BRANCH TARGET (not the
 *    fall-through), an explicit `if (t<0) goto zero; <clamp>; goto done;
 *    zero: pri=0; done:` reproduces the exact polarity and the reorg's
 *    eager-then-overridden delay-slot trick (the `t>=0x4e2` branch's
 *    delay slot sets `pri=0x4e1` unconditionally; the `t<0x4e2`
 *    fall-through's OWN jump then overrides it with `pri=t` in ITS delay
 *    slot) — a genuinely new lever beyond the cookbook's existing
 *    De-Morgan/`||`-vs-`&&` placement rule, for a plain `if/else` (not a
 *    boolean expression) where the trivial body must be the branch target.
 *  - `GsSortSprite`'s third argument needs an explicit `(u16)` cast at the
 *    call site (not just an `int` local) to reproduce the `andi
 *    a2,a2,0xffff` mask in the `jal`'s own delay slot — the prototype's
 *    plain `int pri` parameter alone does not imply truncation.
 */
extern GsOT *OTablePt;
extern void GetScreenPosition(s32 x, s32 y, s32 z, SVECTOR *scr);

void DrawSpriteXYZ(GsSPRITE *sprt, s32 x, s32 y, s32 z, s32 scale)
{
    SVECTOR scr;
    s16 sc;
    s32 otz;
    s32 t;
    s32 pri;

    GetScreenPosition(x, y, z, &scr);
    otz = scr.vz;
    if (otz > 0x24)
    {
        sc = (s16)((scale * 300) / otz) + 1;
        sprt->scaley = sc;
        sprt->scalex = sc;
        sprt->x = scr.vx;
        sprt->y = scr.vy;
        t = (s32)((u32)(u16)scr.vz << 16) >> 0x12;
        if (t < 0)
        {
            goto zero;
        }
        pri = 0x4e1;
        if (t < 0x4e2)
        {
            pri = t;
        }
        goto done;
    zero:
        pri = 0;
    done:
        GsSortSprite(sprt, OTablePt, (u16)pri);
    }
}
