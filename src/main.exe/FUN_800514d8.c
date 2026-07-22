#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 * END PSX.SYM */

/*
 * Per-frame input loop for an end-of-level screen. It updates the character's
 * best stage uid, then passes only a newly pressed pad value to FUN_8005a7a4.
 * `lastpad` is intentionally uninitialized on the first iteration, matching
 * the original $s1 input state.
 *
 * The one `result` identity is significant and natural: it holds the default
 * zero input, the conditional new-pad value, and then FUN_8005a7a4's return.
 * With the reset written after GetRealPad, cse leaves that call's literal-zero
 * argument alone; the scheduler later hoists the independent reset into
 * StartDrawing's delay slot once `result` has been allocated to $s0. This is
 * the exact target sequence without a carrier, fence, or no-op scaffold.
 */
#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern s16 D_8008EA78[];
extern s16 FUN_8005a7a4(s32 input);

void FUN_800514d8(void)
{
    s32 lastpad;
    s32 result;
    u8 *best;

    if (PSTATE->StageNo != D_8008EA78[0])
    {
        best = (u8 *)PSTATE + PSTATE->CharType;
        if (best[0x60] < StageConfig[PSTATE->StageNo].uid)
        {
            best[0x60] = StageConfig[PSTATE->StageNo].uid;
        }
    }
    do
    {
        s32 prev;

        StartDrawing();
        prev = lastpad;
        lastpad = GetRealPad(0);
        result = 0;
        if (prev == 0 && lastpad != 0)
        {
            result = lastpad;
        }
        result = FUN_8005a7a4(result);
        SkipFrame = 2;
        EndDrawing(2);
    } while (result == 0);
}
