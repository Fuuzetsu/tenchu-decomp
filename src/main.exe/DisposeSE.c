#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeSE(struct SoundEffect *se);
 *     AUDIO.C:61, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct SoundEffect * se
 * END PSX.SYM */

/*
 * DisposeSE (0x80018da0) — silence all voices, close the VAB, then free a
 * sound-effect object's VAB header buffer and the SoundEffect itself. Same
 * null-check-then-free shape as DisposeBG/DisposeAfterimage, plus an extra
 * unconditional SsUtAllKeyOff(0)/SsVabClose(se->VABid) pair before the frees
 * (se survives all four calls in a callee-saved register). Local struct
 * matches Ghidra's real SoundEffect exactly (VABid@0 s16, program@2 s16
 * unused here, VABhead@4 pointer) — no truncation needed, it's the whole type.
 */
extern void SsUtAllKeyOff(s32 flag);
extern void SsVabClose(s16 vabId);
extern void vfree(void *p);

void DisposeSE(SoundEffect *se)
{
    if (se != 0)
    {
        SsUtAllKeyOff(0);
        SsVabClose(se->VABid);
        vfree(se->VABhead);
        vfree(se);
    }
}

// triage: TRIVIAL — 19 insns, 3 callees, ~0.50 to DisposeBG

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void DisposeSE(SoundEffect *se)
//
// {
//   if (se != (SoundEffect *)0x0) {
//     SsUtAllKeyOff(0);
//     SsVabClose(se->VABid);
//     vfree((undefined *)se->VABhead);
//     vfree((undefined *)se);
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? SsUtAllKeyOff(?);                                 /* extern */
// ? SsVabClose(s16);                                  /* extern */
// ? vfree(void *);                                    /* extern */
//
// void DisposeSE(void *arg0) {
//     if (arg0 != NULL) {
//         SsUtAllKeyOff(0);
//         SsVabClose(arg0->unk0);
//         vfree(arg0->unk4);
//         vfree(arg0);
//     }
// }
