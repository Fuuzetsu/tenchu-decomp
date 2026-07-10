#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PackItemLayout(void *buf, long size);
 *     ITEM.C:473, 30 src lines, frame 224 bytes, saved-reg mask 0x80000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       void * buf
 *     param $a1       long size
 *     stack sp+16     unsigned char [200] fn
 *     reg   $a2       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * PackItemLayout (0x8003d0bc, 0xb8 bytes) — serialize the 30-slot item pool
 * into a caller-provided save buffer (FileOption's save path): 0x14 bytes
 * per slot (type + world position of the item's ModelType, `coord.t[0..2]`),
 * or a sentinel -1 for an empty slot. Complains via AdtMessageBox if the
 * buffer is too small (proven < 600 by the retail `.s`'s `sltiu` compare
 * against 0x258).
 *
 * The retail frame is only 0x18 bytes (sp -0x18) — PSX.SYM's `unsigned char
 * fn[200]` stack local is from an earlier build and unused here (matches
 * Ghidra's own rendering, which has no such buffer); the loop is the real
 * frame content.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - A provably-30-iterations loop with i=0 initial and no jump to a
 *    preheader test is the "for/while constant-folds its entry test"
 *    bottom-test do-while shape (jump.c's duplicate_loop_exit_test): the
 *    else branch falls straight into the loop body with the `slti`/`bnez`
 *    test at the BOTTOM, not a hand-rolled goto loop (contrast
 *    ClearItemLayout, which never hoists and keeps a top test).
 *  - **Index `items[i]`, don't walk a `tag_TItem *it` pointer with `it++`**:
 *    a user-declared walking pointer whose field is read 3× per iteration
 *    (`it->locate` here) gets loop.c's giv treatment and a SEPARATE
 *    loop-carried address register is strength-reduced for it (a real,
 *    longer-by-4-insns structural miss) — but the compiler's OWN internal
 *    iv from strength-reducing `items[i]` does not get this treatment for
 *    the identical 3× reuse. Same effect must hold for `buf`: this function
 *    needed BOTH `items[i]` (not `it++`) AND `p = (u8*)buf + i*0x14`
 *    recomputed fresh each iteration (not `buf = buf + 0x14;` reassigning
 *    the parameter) to avoid a spurious persistent second cursor.
 *  - **The empty/non-empty polarity is the OPPOSITE of Ghidra's literal
 *    `if (proc == 0) {-1} else {4 stores}`.** The target's `beqz` skips
 *    straight to the tiny `-1` store sitting right before the loop
 *    increment (no trailing jump needed) with the four-store body as the
 *    fallthrough (ending in a `j` over the `-1` store) — write
 *    `if (proc != 0) {4 stores} else {-1}` (cookbook's "opposite polarity"
 *    rule for a plain if/else, not the null-guard-with-two-returns
 *    exception, since both arms merge into the same loop increment).
 *  - **A THIRD store into the same base needs its own second pointer,
 *    but only for the LAST TWO of three same-base offsets.** `p` (buf+i*20)
 *    is used directly for offsets 0 and 4; `q = p + 4;` declared right
 *    after the offset-4 store is then used for offsets 4/8 relative to
 *    itself (i.e. buf+8/buf+0xc) — writing all four stores directly off
 *    `p` (no `q`) compiles 2 bytes long (an extra `addiu` fills where the
 *    target reuses `q`'s one materialization twice).
 *  - The empty-slot sentinel is a raw `-1` (Ghidra's `~ITEM_KAGINAWA` is
 *    just how it renders the constant `~0` given `ITEM_KAGINAWA == 0`; the
 *    asm materializes it once, outside the loop, with `addiu $a3,$zero,-1`
 *    and reuses that register every empty-slot iteration).
 *  - `D_80012200` ("item storing size too small %d/%d") is this function's
 *    OWN string, still sitting as raw copied bytes in the leading uncarved
 *    `data` blob (not near this function's own code — see the cookbook's
 *    "matching a function can delete the very D_ symbol its C body needs"
 *    entry). PackItemLayout was the sole `.s` referencing it, so matching
 *    would have dropped the auto-symbol; added a `D_80012200 = 0x80012200;`
 *    line to config/symbols.main.exe.txt (same fix as D_800976DC) rather
 *    than let cc1 synthesize a brand-new string.
 */

extern char D_80012200[]; /* "item storing size too small %d/%d" */

void PackItemLayout(void *buf, s32 size)
{
    s32 i;
    u8 *p;
    u8 *q;

    if ((u32)size < 600)
    {
        AdtMessageBox(D_80012200, size, 600);
    }
    else
    {
        i = 0;
        do
        {
            p = (u8 *)buf + i * 0x14;
            if (items[i].proc != 0)
            {
                *(s32 *)p = items[i].type;
                *(s32 *)(p + 4) = items[i].locate->locate.coord.t[0];
                q = p + 4;
                *(s32 *)(q + 4) = items[i].locate->locate.coord.t[1];
                *(s32 *)(q + 8) = items[i].locate->locate.coord.t[2];
            }
            else
            {
                *(s32 *)p = -1;
            }
            i++;
        } while (i < 0x1e);
    }
}
