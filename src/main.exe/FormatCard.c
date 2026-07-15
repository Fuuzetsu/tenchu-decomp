#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short FormatCard(void);
 *     MEMCARD.C:108, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     stack sp+16     long cmd
 *     stack sp+20     long result
 * END PSX.SYM */

/*
 * FormatCard — MemCardFormat on slot 0, then block on MemCardSync and return
 * its result truncated to a short. Structurally identical to ChkCard.c (see
 * that file for the shared-stack-slot note); only the kick-off call differs.
 */

extern s32 MemCardFormat(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s16 FormatCard(void)
{
    s32 cmd;
    s32 result;

    result = MemCardFormat(0);
    MemCardSync(0, &cmd, &result);
    return result;
}
