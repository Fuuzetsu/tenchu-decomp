#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

/*
 * FUN_800565f0 (0x800565f0, 0x4c bytes) — copies this run's selected
 * per-item purchase counts onto the current character's carried-item
 * counts. TCameraStatus/CamState.Owner (a Humanoid*) is the same proven
 * view DoInfoViewProc.c declares (truncated here to the one field used);
 * item.h's Humanoid.item[0x1A] sits at 0xB4. The source (config/symbols
 * 0x80010007) is TLinkInfo.counts — but indexing SELECTED_ITEM_COUNTS
 * BY NAME as a plain extern array compiles to an `la` (lui+addiu forming
 * its full address first, index added with a zero final displacement): the
 * target instead adds the index to just the %hi part and folds the low
 * offset into the lbu's own displacement (the cookbook's "`((T
 * *)0x80010000)->arr[i]` emits index-first with a split lui+lo displacement"
 * vs "an extern-array symbol emits la" rule) — so this reads through the
 * PSTATE raw-address cast instead, one instruction shorter.
 *
 * CamState's own base is an unrelated address (its own lui, not folded
 * into PSTATE's).
 */
extern TCameraStatus CamState;
#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

void FUN_800565f0(void)
{
    s16 i;

    for (i = 0; i < 0x14; i++) {
        CamState.Owner->item[i] = PSTATE->counts[i];
    }
}
