#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetPadXY(short no, short *x, short *y);
 *     PADCMD.C:285, 6 src lines, frame 8 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a0       short no
 *     param $a1       short * x
 *     param $a2       short * y
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * GetPadXY (0x8001b480) — writes the x/y analog-stick fields of
 * PadPort[no][0] (offsets 2/4 of controller_input, i.e. unk_2[0]/unk_2[1])
 * through the out-parameters. Like GetPad/FUN_8001b174 (not GetRealPad/
 * PadShock) this indexes PadPort by the plain row `no` only, scaling by the
 * whole 0x38 (4*14) row stride — there is no `>>4`/`&3` column split here.
 *
 * STATUS: NON_MATCHING — this is the SAME sign-extension shift-split as
 * GetPad/FUN_8001b174 (see GetPad.c). The target sign-extends `no` as
 * sll16/sra12/sra4 (three shifts, the extend fused with the element
 * scale); the natural-C draft below compiles the textbook sll16/sra16
 * (two shifts, CSE'd once and shared by both field reads — cc1's
 * fold/combine refolds every plain-arithmetic respelling back to this
 * form) — confirmed via `tools/matchdiff.py GetPadXY`: carve extent 60
 * bytes, this draft links to 56 bytes (4 bytes / one instruction SHORT —
 * the missing extra `sra`, exactly the GetPad-class signature). GetPad.c's
 * header has the full verified byte-exact-but-inline-asm-barrier form and
 * the open project policy question; that decision applies identically here.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetPadXY", GetPadXY);
#else
void GetPadXY(short no, short *x, short *y)
{
    *x = (short)PadPort[no][0].unk_2[0];
    *y = (short)PadPort[no][0].unk_2[1];
}
#endif
