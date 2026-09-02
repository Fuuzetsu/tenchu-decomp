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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * The decisive reconstruction was aggregate syntax: assigning the 16-byte
 * VECTOR into `tmp.locate` makes cc1 emit the target's batched t1-t4 load/store
 * copy, while the following whole PARAM_ITEM_STAY assignment emits its second
 * batched copy.  The nested infinite loops retain the source loop scopes while
 * the late `search_success` trampoline reproduces the target's success-only
 * vx/vz stores and jump back to the shared k==4 check.  That shared exit keeps
 * one ReqItemStay call and gives level, z, x, offs, and k their target register
 * order without artificial arithmetic at the final stores.
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
    PARAM_ITEM_STAY param;
    s32 level;
    s32 level_mode;
    s32 sentinel;
    s32 x;
    s32 z;

    {
        s32 i;

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
            if (it->mode != ITEM_MODE_START)
            {
                AdtMessageBox(msg_item_dispose_fail, it->type, (u32)it->mode);
            }
            it->owner = 0;
            it->proc = 0;
        }
        it++;
        i++;
        goto loop1;
    loop1_end:;
    }

    i = 0;
    level_mode = AREA_LEVEL_STEP_DOWN;
    sentinel = LEVEL_NONE;
    slot = buf;
    for (;;)
    {
        if (i >= MAX_ITEMS)
            return;
        if (slot->type != ITEM_NONE)
        {
            PARAM_ITEM_STAY tmp;

            memset(&tmp, 0, sizeof(PARAM_ITEM_STAY));
            tmp.type = slot->type;
            tmp.locate = slot->locate;
            param = tmp;

            level = GetAreaMapLevel(GlobalAreaMap, param.locate.vx, param.locate.vy, param.locate.vz, level_mode);
            if (level == sentinel || abs(level - param.locate.vy) >= 1000)
            {
                s32 k;
                short *offs;

                k = 0;
                offs = DropOffsets;
                for (;;)
                {
                    if (k < 4)
                    {
                        x = param.locate.vx + offs[0] * 1000;
                        z = param.locate.vz + offs[1] * 1000;
                        level = GetAreaMapLevel(GlobalAreaMap, x,
                                                param.locate.vy, z, level_mode);
                        if (level != sentinel &&
                            abs(level - param.locate.vy) < 1000)
                        {
                            goto search_success;
                        }
                        offs += 2;
                        k++;
                        continue;
                    }
                search_check:
                    if (k == 4)
                    {
                        goto skip_stay;
                    }
                    break;
                }
            }
            param.locate.vy = level;
            ReqItemStay(&param);
        skip_stay:;
        }
        slot++;
        i++;
        continue;

    search_success:
        param.locate.vx = x;
        param.locate.vz = z;
        goto search_check;
    }
}
