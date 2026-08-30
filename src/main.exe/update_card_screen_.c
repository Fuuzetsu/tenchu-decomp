#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gfMemory;
 * END PSX.SYM */

/*
 * update_card_screen_ (0x8005a7a4) — advance the memory-card save UI state machine.
 *
 * The target selects the new state into a register and does ONE store. A
 * ternary is NOT equivalent (cc1 duplicates the McardRetry store into both
 * arms, +8). Three constructs in the shared `update_count` tail are
 * load-bearing; each is measured, and removing any of them costs bytes.
 *
 * The tail's block is LOAD-FREE, so sched cannot reorder it (every insn_cost
 * is 1, so priority() collapses to 1 and nothing moves) — its order is expand
 * order, and the answer had to be source structure:
 *
 *   - `cond = value < CARD_RETRY_LIMIT;` ahead of the store. cc1 emits a compare and its
 *     branch TOGETHER from the MIPS branch expander (`cmpsi` only records the
 *     operands and emits nothing), so no statement can be parked between them;
 *     hoisting the compare into a local is the only way to emit `slti` before
 *     the store. It MUST be spelled `< 3`, never `> 2`: as a *value*, `> 2`
 *     goes through store_flag, which cannot put the constant in the immediate
 *     and folds to `lui`/`slt` against 2<<16 (measured: 13). `< 3` is
 *     store_flag's natural `slti` — the target's exact instruction.
 *
 *   - the do{}while(0) around the McardRetry store. A fence emits CODE_LABELs,
 *     i.e. a basic-block boundary, and that boundary is what pins the result:
 *       * combine will not merge the compare into the branch across it, so the
 *         `slti` stays put instead of being re-split back down at the branch.
 *         Without the fence the `cond` local is entirely byte-neutral (12).
 *       * reorg's backward delay-slot scan stops dead at the label — which is
 *         why the target's `bnez` keeps an empty delay slot even though the
 *         `sh` sits right before it and never touches v0.
 *     Both of the target's oddities come from that single boundary.
 *
 *   - the do{}while(0) around the McardState store buys next_state one
 *     loop-depth-weighted ref, winning it a0 (regalloc.py: `p83 > p117: needs
 *     +1 weighted ref`). It must enclose ONLY this read: wrapping the `if`
 *     would also double saved_state's refs, and its shorter live range would
 *     then outrank next_state and take a0 the wrong way.
 *
 * Measured, so nobody re-derives them: cond alone 12, fence alone 12, both 0;
 * unwrapping the McardState fence 7. Moving the store after the `if` lets
 * reorg take it into the delay slot: 1020 bytes, 4 short.
 */

extern char *McardFile;
extern s16 McardStateFlag;
extern s16 McardState;
extern s16 McardPage;
extern s16 McardRetry;

extern s32 setup_card_screen_(s16 mode);
/* The definition is s32 update_card_message_(s16 *state, u16 *message) --
 * this TU's swapped pointer types and s16 return are retail's own
 * prototype drift, and they are byte-required: correcting the extern (the
 * cast moves to the other argument) changes the caller's frame. 1998
 * shipped without a shared header here. */
extern s16 update_card_message_(u16 *state, s16 *page);
extern s16 check_card_file_(char *name);
extern s16 SaveCard(s32 target, u8 *name, void *mem, s32 size, s16 write_data);
extern s32 draw_card_help_(s32 page, s32 pad);

s32 update_card_screen_(s32 pad)
{
    u16 saved_state;
    s16 value;
    s32 cond;
    u16 next_state;
    u16 newstate;
    u16 incremented;

    setup_card_screen_(0);
    switch (McardState)
    {
    case 10:
        McardPage = 24;
        break;
    case 20:
        McardPage = 25;
        break;
    case 38:
        McardState = 40;
        break;
    case 40:
        McardState = 43;
        break;
    case 43:
        value = check_card_file_(McardFile);
        if (value == 0)
            goto probe_missing;
        if (value == 5)
            goto probe_present;
        goto clear_state;
    probe_missing:
        McardState = 60;
        break;
    probe_present:
        McardState = 50;
        break;
    case 50:
        McardPage = 7;
        McardRetry = 0;
        goto increment_state;
    case 53:
        SaveCard(0, (u8 *)McardFile,
                 (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                 TENCHU_PERSISTENT_STATE_SIZE, 0);
        value = SaveCard(0, (u8 *)McardFile,
                         (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         TENCHU_PERSISTENT_STATE_SIZE, 1);
        switch (value)
        {
        default:
            newstate = 56;
            break;
        case 0:
            newstate = 54;
            break;
        case 1:
            newstate = 10;
            break;
        case 7:
            newstate = 70;
            break;
        case 4:
            McardState = 30;
            McardStateFlag = 0;
            goto retry_53;
        }
        McardState = newstate;
    retry_53:
        if (McardState == 54)
            break;
        next_state = 53;
        saved_state = (u16)McardState;
        value = McardRetry;
        incremented = value + 1;
        goto update_count;
    case 54:
        if (gfMemory == 0)
        {
            McardPage = 9;
            break;
        }
    case 55:
        McardState = 99;
        break;
    case 56:
        McardPage = 14;
        break;
    case 60:
        McardPage = 44;
        break;
    case 61:
        McardPage = 7;
        McardState = 63;
        McardRetry = 0;
        break;
    case 57:
    case 62:
        McardState = 90;
        break;
    case 41:
    case 42:
    case 51:
    case 52:
    case 63:
    case 64:
    increment_state:
        McardState++;
        break;
    case 65:
        SaveCard(0, (u8 *)McardFile,
                 (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                 TENCHU_PERSISTENT_STATE_SIZE, 0);
        value = SaveCard(0, (u8 *)McardFile,
                         (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         TENCHU_PERSISTENT_STATE_SIZE, 1);
        switch (value)
        {
        default:
            newstate = 56;
            break;
        case 0:
            newstate = 54;
            break;
        case 1:
            newstate = 10;
            break;
        case 7:
            newstate = 70;
            break;
        case 4:
            McardState = 30;
            McardStateFlag = 0;
            goto retry_65;
        }
        McardState = newstate;
    retry_65:
        if (McardState == 54)
            break;
        next_state = 65;
        saved_state = (u16)McardState;
        value = McardRetry;
        incremented = value + 1;
    update_count:
        cond = value < CARD_RETRY_LIMIT;
        do
        {
            McardRetry = incremented;
        } while (0);
        if (!cond)
            next_state = saved_state;
        /* The unsigned identity is folded after flow; its extra consumer
         * reference keeps the selected state in $a0 for the shared retail
         * store (allocation staging, not recovered arithmetic). */
        McardState = (next_state + next_state) - next_state;
        break;
    case 70:
        McardPage = 26;
        break;
    case 11:
    case 21:
    case 71:
        McardState = -1;
        break;
    case 12:
    case 22:
    case 72:
    clear_state:
        McardState = 0;
        break;
    default:
        value = update_card_message_((u16 *)&McardState, &McardPage);
        if (value == 0)
        {
            McardPage = 0;
            McardRetry = 0;
            setup_card_screen_(1);
            if (McardState < 0)
            {
                McardState = 3;
                return 1;
            }
            McardState = 3;
            return -1;
        }
        break;
    }

    McardState += draw_card_help_(McardPage, pad);
    return 0;
}
