#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ThinkBasicHuman2(void);
 *     THINK.C:247, 2 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * ThinkBasicHuman2 (0x8002f8c4, 0x24 bytes) — think-handler that just
 * forwards port-1 held-buttons (same "think" TU as Think1sleep.c; s16
 * return convention). The direct `return GetPad(1);` tail call still emits
 * the short-result sll16/sra16 pair right after the jal (a short-returning
 * call's result always needs this re-extension, even used bare as the
 * caller's own s16 return — see the cookbook's short-result-extension
 * rule).
 */
extern s16 GetPad(s16 port);

s16 ThinkBasicHuman2(void)
{
    return GetPad(1);
}
