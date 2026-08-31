#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void MoveHumanoid(struct Humanoid *human, short ordr, short side);
 *     HUMAN.C:349, 17 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short ordr
 *     param $a2       short side
 * END PSX.SYM */

/*
 * MoveHumanoid (0x8002952c) — set a character's velocity vector from an
 * order/side speed pair, rotated by the character's facing (rotate->vy).
 * ordr/side are signed bytes stored in shorts, so 0x80..0xFF are re-signed
 * via `- 0x100`. Velocity = R(-sin,-cos) applied to (ordr, side), >> 12.
 *
 * Matching notes:
 *  - The guard is the `||` form `if (ordr != 0 || side != 0) { move } else
 *    { stop }`, NOT `if (==0 && ==0) {stop} else {move}`: cc1 lays the THEN
 *    branch (the big compute) physically first, and the branch polarity is
 *    `bnez ordr -> move; beqz side -> stop`. The De-Morgan `&&` form inverts
 *    the side test (bnez) and puts the stop body first — wrong layout.
 *  - `int io = ordr;` is a real variable, distinct from the multiply operand
 *    copy `o`. It forces `(int)ordr` into ONE sign-extended pseudo (sll+sra,
 *    $s2) shared by the zero-test AND the `& 0xff80` byte-resign test across
 *    the rsin/rcos calls (so callee-saved). Inline `(int)ordr` in a `!= 0`
 *    test compiles to sll+bnez with NO sra, leaving nothing for the andi to
 *    reuse — it then re-ands the raw copy and the shared pseudo never forms.
 *  - The byte-resign writes a SEPARATE copy (`o = ordr - 0x100`, $s5), leaving
 *    the short param `ordr` ($s1, the subtraction source) and `io` ($s2) live.
 *    Three ordr-derived regs coexist across the calls.
 *  - `s = -rsin(...)` negates at the assignment; reorg steals `negu $s3` into
 *    the rcos delay slot, so -sin is live across rcos -> callee-saved ($s3),
 *    while -cos ($v1, no calls after) stays caller-saved.
 */
void MoveHumanoid(Humanoid *human, short ordr, short side)
{
    int s, c;
    int io;
    short o, si;

    io = ordr;
    o = ordr;
    si = side;
    if (io != 0 || side != 0)
    {
        s = -rsin(human->rotate->vy);
        c = -rcos(human->rotate->vy);
        /* Sign-extend a signed byte, but only when nothing is set above
         * it -- a plain (s8) cast would also fold 0x180 and costs 96
         * lines. Callers pass wider values, so the guard is the point. */
        if ((io & 0xff80) == 0x80)
        {
            o = ordr - 0x100;
        }
        if ((side & 0xff80) == 0x80)
        {
            si = side - 0x100;
        }
        human->vector.vx = (short)(((short)s * o - (short)c * si) >> 0xc);
        human->vector.vz = (short)(((short)c * o + (short)s * si) >> 0xc);
    }
    else
    {
        human->vector.vz = 0;
        human->vector.vx = 0;
    }
}
