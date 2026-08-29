#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RestoreItemLayout(void *buf);
 *     ITEM.C:507, 22 src lines, frame 288 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       void * buf
 *     reg   $s3       struct TItemLayout * slot
 *     stack sp+16     unsigned char [200] fn
 *     reg   $s1       int i
 *     reg   $s1       int i
 *     stack sp+216    struct PARAM_ITEM_STAY param
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * RestoreItemLayout (0x8003d174) — PackItemLayout's consumer: wipes
 * the live item pool and respawns everything recorded in a saved
 * layout buffer, re-grounding each item against the current area map.
 * The first loop tears down all MAX_ITEMS (30) slots that still hold a
 * proc: mode is set to ITEM_MODE_DISPOSE, the item's own proc is
 * called once to let it clean up, DeleteConflict removes its collision
 * entry, and a handler that failed to clear mode is reported through
 * AdtMessageBox(msg_item_dispose_fail, type, mode). owner and proc are
 * then zeroed. The second loop walks the same 30 TItemLayout records
 * in buf, skipping any whose type is the -1 empty-slot sentinel. A
 * live record is copied into a zeroed PARAM_ITEM_STAY and probed with
 * GetAreaMapLevel against GlobalAreaMap. If that probe returns
 * LEVEL_NONE or lands 1000 or more away from the saved Y, it retries
 * at up to four DropOffsets probe pairs (each dx/dz scaled by 1000)
 * and the first one that lands adopts its x/z. An item whose four
 * retries all fail is dropped from the restore entirely; otherwise the
 * probed ground level replaces the Y and ReqItemStay respawns the item
 * there.
 */

/*
 * The decisive reconstruction was aggregate syntax: assigning the 16-byte
 * VECTOR into `tmp.locate` makes cc1 emit the target's batched t1-t4 load/store
 * copy, while the following whole PARAM_ITEM_STAY assignment emits its second
 * batched copy.  The late `search_success` trampoline reproduces the target's
 * success-only vx/vz stores and jump back to the shared k==4 check without
 * cloning ReqItemStay.  The three one-shot loops around the final level/x/z
 * stores are zero-code global-allocation weights: they order level, z, x,
 * offs, and k into the target s0-s4 homes while leaving scheduling intact.
 */

/* The [4] bound is LOAD-BEARING CODEGEN, not a claim about the table's length:
 * the search loop below walks 4 entries x 2 shorts = 8 shorts past this symbol.
 * The target materializes this address as `lui s3,%hi; addiu s3,s3,%lo` -- ONE
 * register.  That is an UNSPLIT `la` macro, not a high/lo_sum pair: cc1 splits
 * an address pre-reload into two *distinct* pseudos (`lui $tmp,%hi; addiu
 * $dst,$tmp,%lo`), and local-alloc's combine_regs refuses to tie them whenever
 * the destination is a multi-block pseudo -- which a loop cursor always is.  The
 * split is declined only when mips_check_split() sees SYMBOL_REF_FLAG, which
 * ENCODE_SECTION_INFO (mips.h) sets iff the symbol's declared type is COMPLETE
 * and 0 < sizeof <= the -G threshold (8).  `extern short DropOffsets[];` is an
 * incomplete type (size -1), so it always split.  Widening this bound past [4]
 * re-splits the address and costs 3 bytes.  See docs/matching-cookbook.md.
 */
extern short DropOffsets[4]; /* {dx,dz} probe pairs x1000 */

extern s32 abs(s32 x);
extern void *memset(void *s, int c, u32 n);

void RestoreItemLayout(void *buf)
{
    TItem *it;
    TItemLayout *slot;
    s32 i;
    s32 j;
    PARAM_ITEM_STAY param;
    s32 level;
    s32 one;
    s32 sentinel;
    s32 x;
    s32 z;

    i = 0;
    it = items;
loop1:
    if (i >= MAX_ITEMS)
        goto loop1_end;
    if (it->proc != 0)
    {
        it->mode = ITEM_MODE_DISPOSE;
        it->proc(it);
        DeleteConflict(it->locate);
        if (it->mode != 0)
        {
            AdtMessageBox(msg_item_dispose_fail, it->type, (u32)it->mode);
        }
        it->owner = 0;
        it->proc = 0;
    }
    it++;
    i++;
    goto loop1;
loop1_end:

    j = 0;
    one = 1;
    sentinel = LEVEL_NONE;
    slot = buf;
loop2:
    if (j >= MAX_ITEMS)
        return;
    if (slot->type != -1)
    {
        PARAM_ITEM_STAY tmp;

        memset(&tmp, 0, sizeof(PARAM_ITEM_STAY));
        tmp.type = slot->type;
        tmp.locate = slot->locate;
        param = tmp;

        level = GetAreaMapLevel(GlobalAreaMap, param.locate.vx, param.locate.vy, param.locate.vz, one);
        if (level == sentinel || abs(level - param.locate.vy) >= 1000)
        {
            s32 k = 0;
            short *offs = DropOffsets;

        searchloop:
            if (k < 4)
            {
                x = param.locate.vx + offs[0] * 1000;
                z = param.locate.vz + offs[1] * 1000;
                level = GetAreaMapLevel(GlobalAreaMap, x, param.locate.vy, z, one);
                if (level == sentinel || abs(level - param.locate.vy) >= 1000)
                {
                    offs += 2;
                    k++;
                    goto searchloop;
                }
                goto search_success;
            }
        search_check:
            if (k == 4)
            {
                goto skip_stay;
            }
        }
        do
        {
            param.locate.vy = level;
        } while (0);
        ReqItemStay(&param);
    skip_stay:;
    }
    slot++;
    j++;
    goto loop2;

search_success:
    do
    {
        param.locate.vx = x;
    } while (0);
    do
    {
        param.locate.vz = z;
    } while (0);
    goto search_check;
}
