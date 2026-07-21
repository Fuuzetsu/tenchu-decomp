#include "common.h"
#include "main.exe.h"

/*
 * ResetInventory (0x8005663c, 0x84 bytes) — resets the per-run selected-item
 * purchase counts (TLinkInfo.counts[0x14], splat's SELECTED_ITEM_COUNTS):
 * item 0 (the starting weapon slot) to 0xff (infinite/preselected), items
 * 1..8 to 0 (available, not yet bought), items 9..0x13 to 0xfe (locked).
 * Called by DoBriefingAndInventorySelection.
 *
 * Matching notes: the raw .s reaches the array through a bare
 * `lui $a1,%hi(SELECTED_ITEM_COUNTS)` with NO addiu, hoisted once per loop
 * and reused as the base for `addu`+displacement accesses — the cookbook's
 * "gp vs absolute globals" tell for a literal pointer-cast local
 * (`(TLinkInfo *)0x80010000`, the same PSTATE convention as
 * FUN_8001b2b8.c/FUN_800565f0.c), not a plain `extern u8
 * SELECTED_ITEM_COUNTS[]` (which would materialize a one-register `la`
 * instead). The loop counter is `short i`: each iteration re-sign-extends it
 * (`sll 16/sra 16`) before adding it to the hoisted base — the textbook
 * byte-array index shape (cookbook's "short loop counter suppresses
 * strength reduction", degenerate to a pure sign-extend since the stride is
 * 1). First loop is a bottom-tested `do/while` (no entry test in the asm);
 * second is a top-tested `while` whose asm shows the classic duplicated
 * entry test before falling into a second bottom-tested loop. The item-0
 * scalar store and the two loops are three UNRELATED accesses (each its own
 * literal `(TLinkInfo *)0x80010000` cast, not one shared pointer
 * local) — a shared local would CSE the address into ONE register reused by
 * all three, but the target computes the constant-index store through the
 * assembler's own $at one-line macro and hoists a fresh %hi into $a1 once
 * per loop (loop.c's invariant motion has no cross-loop memory).
 */

void ResetInventory(void)
{
    s16 i;

    ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->counts[0] = 0xff;
    i = 1;
    do {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->counts[i] = 0;
        i++;
    } while (i < 9);
    while (i < 0x14) {
        ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->counts[i] = 0xfe;
        i++;
    }
}
