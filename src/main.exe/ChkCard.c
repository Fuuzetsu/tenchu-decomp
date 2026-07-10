#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ChkCard(void);
 *     MEMCARD.C:84, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     stack sp+16     long cmd
 *     stack sp+20     long result
 * END PSX.SYM */

/*
 * ChkCard (0x8001b4bc region's memory-card TU) — kicks off a MemCardAccept on
 * slot 0 and blocks on MemCardSync until it reports a result, returning that
 * result truncated to a short. `cmd` is MemCardSync's command out-param, which
 * this caller ignores; `result` is seeded with MemCardAccept's return and then
 * overwritten in place by MemCardSync, which is why both share one stack slot
 * in source. FormatCard.c is the same function over MemCardFormat.
 */

extern s32 MemCardAccept(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s16 ChkCard(void)
{
    s32 cmd;
    s32 result;

    result = MemCardAccept(0);
    MemCardSync(0, &cmd, &result);
    return result;
}
