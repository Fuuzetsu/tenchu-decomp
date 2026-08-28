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
 *   - `cond = value < 3;` ahead of the store. cc1 emits a compare and its
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
extern s16 CardStateFlag;
extern s16 McardState;
extern s16 McardPage;
extern s16 McardRetry;

extern s32 setup_card_screen_(s16 mode);
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
    u16 assigned;
    u16 incremented;

    setup_card_screen_(0);
    switch (McardState)
    {
    case 0xa:
        McardPage = 0x18;
        break;
    case 0x14:
        McardPage = 0x19;
        break;
    case 0x26:
        McardState = 0x28;
        break;
    case 0x28:
        McardState = 0x2b;
        break;
    case 0x2b:
        value = check_card_file_(McardFile);
        if (value == 0)
            goto probe_missing;
        if (value == 5)
            goto probe_present;
        goto clear_state;
    probe_missing:
        McardState = 0x3c;
        break;
    probe_present:
        McardState = 0x32;
        break;
    case 0x32:
        McardPage = 7;
        McardRetry = 0;
        goto increment_state;
    case 0x35:
        SaveCard(0, (u8 *)McardFile,
                 (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                 TENCHU_PERSISTENT_STATE_SIZE, 0);
        value = SaveCard(0, (u8 *)McardFile,
                         (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         TENCHU_PERSISTENT_STATE_SIZE, 1);
        switch (value)
        {
        default:
            assigned = 0x38;
            break;
        case 0:
            assigned = 0x36;
            break;
        case 1:
            assigned = 10;
            break;
        case 7:
            assigned = 0x46;
            break;
        case 4:
            McardState = 0x1e;
            CardStateFlag = 0;
            goto save_2b_after_assign;
        }
        McardState = assigned;
    save_2b_after_assign:
        if (McardState == 0x36)
            break;
        next_state = 0x35;
        saved_state = (u16)McardState;
        value = McardRetry;
        incremented = value + 1;
        goto update_count;
    case 0x36:
        if (gfMemory == 0)
        {
            McardPage = 9;
            break;
        }
    case 0x37:
        McardState = 99;
        break;
    case 0x38:
        McardPage = 0xe;
        break;
    case 0x3c:
        McardPage = 0x2c;
        break;
    case 0x3d:
        McardPage = 7;
        McardState = 0x3f;
        McardRetry = 0;
        break;
    case 0x39:
    case 0x3e:
        McardState = 0x5a;
        break;
    case 0x29:
    case 0x2a:
    case 0x33:
    case 0x34:
    case 0x3f:
    case 0x40:
    increment_state:
        McardState++;
        break;
    case 0x41:
        SaveCard(0, (u8 *)McardFile,
                 (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                 TENCHU_PERSISTENT_STATE_SIZE, 0);
        value = SaveCard(0, (u8 *)McardFile,
                         (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         TENCHU_PERSISTENT_STATE_SIZE, 1);
        switch (value)
        {
        default:
            assigned = 0x38;
            break;
        case 0:
            assigned = 0x36;
            break;
        case 1:
            assigned = 10;
            break;
        case 7:
            assigned = 0x46;
            break;
        case 4:
            McardState = 0x1e;
            CardStateFlag = 0;
            goto save_37_after_assign;
        }
        McardState = assigned;
    save_37_after_assign:
        if (McardState == 0x36)
            break;
        next_state = 0x41;
        saved_state = (u16)McardState;
        value = McardRetry;
        incremented = value + 1;
    update_count:
        cond = value < 3;
        do
        {
            McardRetry = incremented;
        } while (0);
        if (!cond)
            next_state = saved_state;
        /*
         * Load-bearing fence (autorules fence-unwrap: unwrapping it costs 7
         * bytes, 0 -> 7). It buys next_state ONE loop-depth-weighted ref:
         * reg_n_refs is loop-depth weighted and global.c's priority is
         * floor_log2(refs) * refs/live_length. next_state (p83) scores
         * 2*4/16*10000 = 5000 and loses a0 to p117/p151 at 3/5 -> 6000;
         * doubling this one ref gives 5 weighted refs -> 6250 -> a0, which
         * fixes all six register-swapped instructions at once.
         * The fence must enclose ONLY this next_state read: wrapping the `if`
         * would also double saved_state's refs, and its shorter live range
         * (3/11) would then outrank next_state and take a0 the wrong way.
         */
        do
        {
            McardState = next_state;
        } while (0);
        break;
    case 0x46:
        McardPage = 0x1a;
        break;
    case 0xb:
    case 0x15:
    case 0x47:
        McardState = -1;
        break;
    case 0xc:
    case 0x16:
    case 0x48:
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
