#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4chase(void);
 *     THINK_4.C:75, 189 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $a0       short pad
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 191 of 404 bytes differ. Right length in the
 * outer dispatch (the `0x5B <= actcnt` guard-polarity fix from
 * Think4contact.c applies identically here and is byte-confirmed); the
 * residual is entirely the SAME systemic turn/Degree dispatch tie
 * documented in Think4contact.c's header (this function's inner dispatch
 * is a 3-way version of that same 2-way one, nested one level deeper under
 * `actcnt < 0x1E`) — see that file for the full mechanism writeup and the
 * list of tried-and-rejected source variants (all byte-identical to each
 * other here too: nested if/else vs if/else-if/else vs goto-ladder vs
 * do{}while(0) wrapper). `tools/permute.py` was not re-run separately for
 * this function since it shares Think4contact's exact residual shape one
 * level deeper; the same jump.c/reorg block-layout conclusion applies.
 *
 * Think4chase (0x8002f5a8, 0x194 bytes) — think-handler, same "think" TU as
 * Think4contact.c/Think4abandon.c; structurally identical to Think4contact
 * except the no-chase-target branch has a NESTED sub-gate (actcnt < 0x1e,
 * on top of the outer actcnt < 0x5b) and different turn-result constants
 * (0x1000 default/0x3000 right/-0x7000 left, vs Think4contact's 0/0x2000/
 * -0x8000) — see Think4contact.c's header for the shared mechanics
 * (FUN_8002b990 == turn_towards_player_, the one-pseudo `result`, the
 * per-return-site truncation rules).
 *
 * `result = 0x1000;` is set unconditionally right after the actcnt
 * increment (default for BOTH "actcnt >= 0x1e" and the inner "else" arm),
 * then reasserted verbatim in the inner else — a byte-neutral redundant
 * store cc1's local cse collapses back to nothing extra (same register,
 * same value, no intervening call), matching Ghidra's own literal
 * (non-simplified) rendering of the source.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think4chase", Think4chase);
#else
extern s16 SR;
extern s16 Attrib;
extern s16 Degree;
extern s16 Think4abandon(void);
extern s32 SquareRoot0(s32 x);

s16 Think4chase(void)
{
    s32 result;

    if (SR == 1)
    {
        Attrib = (Attrib & 0xFFFC) | 2;
        return 0;
    }

    if (Me_THINK_C->some_other_x_position == 0 && Me_THINK_C->some_other_z_position == 0)
    {
        if (0x5B <= Me_THINK_C->actcnt)
        {
            return Think4abandon();
        }
        else
        {
            Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
            result = 0x1000;
            if (Me_THINK_C->actcnt < 0x1E)
            {
                if (Me_THINK_C->character_rotation_speed < Degree)
                {
                    result = 0x3000;
                }
                else
                {
                    result = 0x1000;
                    if (Degree < -Me_THINK_C->character_rotation_speed)
                    {
                        result = -0x7000;
                    }
                }
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me_THINK_C->actscnt = Me_THINK_C->actscnt + 1;
        dx = Me_THINK_C->some_other_x_position - Me_THINK_C->some_kind_of_current_position->vx;
        dz = Me_THINK_C->some_other_z_position - Me_THINK_C->some_kind_of_current_position->vz;
        result = turn_towards_player_(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < 1000 || Me_THINK_C->actscnt == 0)
        {
            Me_THINK_C->some_other_z_position = 0;
            Me_THINK_C->some_other_x_position = 0;
            Me_THINK_C->actcnt = 0;
        }
    }
    return result;
}
#endif
