#include "common.h"
#include "main.exe.h"

/*
 * FUN_800566fc (0x800566fc, 0xa0 bytes) — same PersistentState-at-0x80010000
 * family as FUN_800566c0.c/FUN_800565f0.c (game_types.h's TLinkInfo;
 * see FUN_800566c0.c for the `#define PSTATE` convention and the proven
 * saveItem@0x27/gItem@0x40C/CharType@0x4 offsets). This is FUN_800566c0's mirror
 * image: restores this character's shop stock row FROM the loadout backup
 * (`PSTATE->gItem[i + PSTATE->CharType*0x20] = PSTATE->saveItem[i];`), then fades
 * out, tears down (FUN_80038ce0), resets the stage layout number to 0xFF
 * and clears PersistentState's retry/continue flag (GameRetry &= ~1), then
 * calls FUN_8004f6c0(0x10) (a cleanup/teardown helper
 * — see its own file).
 *
 * Matching notes:
 *  - Ghidra's `FadeOutDirect(0x20,2,'\b','\b')` undercounts the call: the
 *    raw asm passes a FIFTH argument on the stack (`sw v0,0x10(sp)` in the
 *    jal's delay slot, v0==8 same as the two register args) — confirmed by
 *    BriefingAndInventorySelectionScreen.c's identical, already-matched
 *    `FadeOutDirect(0x20, 2, 8, 8, 8);` call (proto `s16,s16,u8,u8,u8`).
 *  - The loop index is `s16 i` (Ghidra's `iVar2`, shown as `int` but the
 *    raw asm's own per-iteration `sll/sra` pairs on BOTH the pre- and
 *    post-increment values only reproduce with a genuinely narrow
 *    counter) narrowed again into a SEPARATE `short idx` (Ghidra's
 *    `sVar1`) each iteration — one variable for both roles doesn't
 *    reproduce both narrows.
 *  - **Statement order inside the loop is load-bearing and reversed from
 *    Ghidra's own rendering**: the STORE must come BEFORE `i = i + 1;`
 *    (`idx = (s16)i; PSTATE->gItem[...] = PSTATE->saveItem[idx]; i = i +
 *    1;`), even though Ghidra shows the increment first. Writing the
 *    increment first (Ghidra's literal order) compiles to an
 *    instruction-for-instruction-identical function with every register
 *    past the first few shifted by one slot — found via
 *    `tools/permute.py` (score 730->40 in one run), then verified by hand
 *    against a plain byte diff (autorules'/permute's own internal scores
 *    are a DIFFERENT metric than raw bytes — always confirm a permuter
 *    or autorules "win" with `tools/matchdiff.py`, not just its own
 *    reported score, which side-tracked this exact residual once).
 */

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern void FadeOutDirect(s16 time, s16 attrib, u8 r, u8 g, u8 b);
extern void FUN_80038ce0(void);
extern void FUN_8004f6c0(int param_1);

void FUN_800566fc(void)
{
    s16 i;

    i = 0;
    do
    {
        s16 idx;

        idx = (s16)i;
        PSTATE->gItem[idx + PSTATE->CharType * 0x20] = PSTATE->saveItem[idx];
        i = i + 1;
    } while ((s16)i < 0x14);
    FadeOutDirect(0x20, 2, 8, 8, 8);
    FUN_80038ce0();
    PSTATE->layout = 0xff;
    PSTATE->GameRetry = PSTATE->GameRetry & 0xfe;
    FUN_8004f6c0(0x10);
}
