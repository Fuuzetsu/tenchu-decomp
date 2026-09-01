#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawSpriteXYZ(struct GsSPRITE *sprt, long x, long y, long z, long scale);
 *     EFFECT.C:204, 11 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * retail caller is the matched-but-still-unnamed proc_misc_bonfire_.
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

void DrawSpriteXYZ(GsSPRITE *sprt, s32 x, s32 y, s32 z, s32 scale)
{
    SVECTOR scr;
    s32 otz;
    s32 t;
    s32 pri;

    GetScreenPosition(x, y, z, &scr);
    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        sprt->scalex = sprt->scaley =
            (s16)((scale * PROJECTION_DISTANCE) / otz) + 1;
        sprt->x = scr.vx;
        sprt->y = scr.vy;
        t = (s16)(u16)scr.vz >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sprt, OTablePt, (u16)pri);
    }
}
