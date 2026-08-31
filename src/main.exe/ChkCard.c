#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ChkCard(void);
 *     MEMCARD.C:84, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     long cmd
 *     stack sp+20     long result
 * END PSX.SYM */

/*
 * ChkCard (0x80056df8) — kicks off a MemCardAccept on
 * slot 0 and blocks on MemCardSync until it reports a result, returning that
 * result truncated to a short. `cmd` is MemCardSync's command out-param, which
 * this caller ignores; `result` is seeded with MemCardAccept's return and then
 * overwritten in place by MemCardSync, so the seed value is never read. FormatCard.c is the same function over MemCardFormat.
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
