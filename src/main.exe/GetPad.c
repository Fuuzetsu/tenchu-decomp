#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetPad(short no);
 *     PADCMD.C:293, 15 src lines, frame 8 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a0       short no
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * GetPad (0x8001b144) — held-buttons for PadPort[arg0][0], the first slot of
 * port `arg0` (a `short`; unlike GetRealPad, which splits a combined
 * port*16+slot index via >>4/&3).
 *
 * STATUS: NON_MATCHING — a sign-extension shift-split with no natural-C form.
 * The target sign-extends `arg0` as sll16/sra12/sra4 (three shifts, the extend
 * fused with the PadPort element scale); a plain "index PadPort by a short"
 * ALWAYS compiles to the textbook sll16/sra16 (two shifts), and cc1's
 * fold/combine refolds every plain-arithmetic respelling back to it (nested
 * expression, two statements, K&R params, explicit pointer casts, a
 * static-inline helper, even a literal transcription of the asm shifts — all
 * verified against cc1 -da RTL dumps + the gcc-2.8.1 sources). The natural
 * draft below is the 2-shift form (a few bytes off).
 *
 * A BYTE-EXACT match exists, but only via a genuine optimizer barrier — an
 * empty register-constrained inline asm, which is NOVEL to this project (no
 * other matched function uses one, and this codebase has parked other sub-C
 * ties as NON_MATCHING rather than hack them):
 *
 *     s32 t = arg0 << 4;         // cc1 fuses extend + <<4 into one sra12
 *     __asm__("" : "+r"(t));     // blocks combine from refolding the >>4 below
 *     return PadPort[t >> 4][0].held;
 *
 * Parked pending a project decision on whether to accept codegen barriers.
 * GetPadXY (0x8001b480) shares this exact idiom twice (x and y).
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetPad", GetPad);
#else
s16 GetPad(short no)
{
    return PadPort[no][0].held;
}
#endif
