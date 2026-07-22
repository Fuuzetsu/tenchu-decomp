#include "common.h"
#include "main.exe.h"

/*
 * FUN_800566c0 (0x800566c0, 0x3c bytes) — backs up this run's per-character
 * shop stock row into the loadout backup: same statement
 * BriefingAndInventorySelectionScreen uses inline
 * (`PSTATE->saveItem[i] = PSTATE->gItem[i + CHOSEN_CHARACTER * 0x20];`),
 * here standalone in its own function. Proven struct: game_types.h
 * TLinkInfo (saveItem@0x27, gItem@0x40C, CharType/CHOSEN_CHARACTER@0x4).
 *
 * All three field accesses go through the SAME `PSTATE` pointer expression
 * (the raw 0x80010000 cast, not the standalone CHOSEN_CHARACTER extern) so
 * cc1's cse folds all of them onto the ONE `lui %hi` it materializes for
 * the pointer constant — reading CharType via the separate CHOSEN_CHARACTER
 * symbol instead forces a second lui/lbu pair (verified: costs 4 bytes).
 * The `for` loop's initial 0 < 0x14 test folds away at compile time,
 * leaving the standard bottom-test do-while shape (jump.c
 * duplicate_loop_exit_test).
 */
#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

void FUN_800566c0(void)
{
    int i;

    for (i = 0; i < 0x14; i++) {
        PSTATE->saveItem[i] = PSTATE->gItem[i + PSTATE->CharType * 0x20];
    }
}
