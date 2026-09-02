#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/*
 * update_card_screen_ (0x8005a7a4) — advance the memory-card save UI state machine.
 *
 * The target selects the new state into a register and does ONE store. A
 * ternary is NOT equivalent (cc1 duplicates the McardRetry store into both
 * arms, +8). Three constructs in the shared `update_write_retry` tail are
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
extern card_state McardState;
extern s16 McardPage;
extern s16 McardRetry;

extern s32 setup_card_screen_(s16 mode);
/* The definition is s32 update_card_message_(s16 *state, u16 *message) --
 * this TU's swapped pointer types and s16 return are retail's own
 * prototype drift, and they are byte-required: correcting the extern (the
 * cast moves to the other argument) changes the caller's frame. 1998
 * shipped without a shared header here. */
extern s16 update_card_message_(u16 *state, s16 *page);
extern card_result check_card_file_(char *name);
extern card_result SaveCard(s32 target, u8 *name, void *mem, s32 size,
                            s16 write_data);
extern s32 draw_card_help_(s32 page, s32 pad);

s32 update_card_screen_(s32 pad)
{
    u16 saved_state;
    s16 value;
    s32 cond;
    u16 next_state;
    u16 save_result_state;
    u16 incremented;

    setup_card_screen_(0);
    switch (McardState)
    {
    case CARD_STATE_NO_CARD:
        McardPage = CARD_PAGE_NO_CARD_CANNOT_SAVE_PROMPT;
        break;
    case CARD_STATE_DAMAGED:
        McardPage = CARD_PAGE_DAMAGED_CANNOT_SAVE_PROMPT;
        break;
    case CARD_STATE_FORMAT_COMPLETE:
        McardState = CARD_STATE_CARD_READY;
        break;
    case CARD_STATE_CARD_READY:
        McardState = CARD_STATE_CHECK_SAVE_FILE;
        break;
    case CARD_STATE_CHECK_SAVE_FILE:
        value = check_card_file_(McardFile);
        switch (value)
        {
        default:
            goto restart_card_check;
        case CARD_RESULT_SUCCESS:
            McardState = CARD_STATE_OVERWRITE_GAME_DATA_PROMPT;
            break;
        case CARD_RESULT_FILE_NOT_FOUND:
            McardState = CARD_STATE_BEGIN_NEW_SAVE;
            break;
            }
            break;
        case CARD_STATE_BEGIN_NEW_SAVE:
            McardPage = CARD_PAGE_WRITING;
            McardRetry = 0;
            goto increment_state;
        case CARD_STATE_WRITE_NEW_SAVE:
            SaveCard(0, (u8 *)McardFile,
                     (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                     TENCHU_PERSISTENT_STATE_SIZE, 0);
            value = SaveCard(0, (u8 *)McardFile,
                             (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                             TENCHU_PERSISTENT_STATE_SIZE, 1);
            switch (value)
            {
        default:
            save_result_state = CARD_STATE_WRITE_FAILED;
            break;
        case CARD_RESULT_SUCCESS:
            save_result_state = CARD_STATE_WRITE_COMPLETE;
            break;
        case CARD_RESULT_NO_CARD:
            save_result_state = CARD_STATE_NO_CARD;
            break;
        case CARD_RESULT_FULL:
            save_result_state = CARD_STATE_NOT_ENOUGH_SPACE_PROMPT;
            break;
        case CARD_RESULT_UNFORMATTED:
            McardState = CARD_STATE_FORMAT_PROMPT;
            McardStateFlag = 0;
            goto retry_new_save;
        }
        McardState = save_result_state;
    retry_new_save:
        if (McardState == CARD_STATE_WRITE_COMPLETE)
            break;
        next_state = CARD_STATE_WRITE_NEW_SAVE;
        saved_state = (u16)McardState;
        value = McardRetry;
        incremented = value + 1;
        goto update_write_retry;
    case CARD_STATE_WRITE_COMPLETE:
        if (gfMemory == 0)
        {
            McardPage = CARD_PAGE_WRITE_COMPLETE;
            break;
        }
    case CARD_STATE_FINISH_SAVE:
        McardState = CARD_STATE_SAVE_COMPLETE_EXIT;
        break;
    case CARD_STATE_WRITE_FAILED:
        McardPage = CARD_PAGE_WRITE_FAILED;
        break;
    case CARD_STATE_OVERWRITE_GAME_DATA_PROMPT:
        McardPage = CARD_PAGE_OVERWRITE_GAME_DATA_PROMPT;
        break;
    case CARD_STATE_BEGIN_OVERWRITE:
        McardPage = CARD_PAGE_WRITING;
        McardState = CARD_STATE_OVERWRITE_WAIT_1;
        McardRetry = 0;
        break;
    case CARD_STATE_WRITE_FAILURE_ACKNOWLEDGED:
    case CARD_STATE_CANCEL_OVERWRITE:
        McardState = CARD_STATE_CANNOT_SAVE_PROMPT;
        break;
    case CARD_STATE_FILE_CHECK_WAIT_1:
    case CARD_STATE_FILE_CHECK_WAIT_2:
    case CARD_STATE_NEW_SAVE_WAIT_1:
    case CARD_STATE_NEW_SAVE_WAIT_2:
    case CARD_STATE_OVERWRITE_WAIT_1:
    case CARD_STATE_OVERWRITE_WAIT_2:
        increment_state:
            McardState++;
            break;
    case CARD_STATE_WRITE_OVERWRITE:
        SaveCard(0, (u8 *)McardFile,
                 (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                 TENCHU_PERSISTENT_STATE_SIZE, 0);
        value = SaveCard(0, (u8 *)McardFile,
                         (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         TENCHU_PERSISTENT_STATE_SIZE, 1);
        switch (value)
        {
        default:
            save_result_state = CARD_STATE_WRITE_FAILED;
            break;
        case CARD_RESULT_SUCCESS:
            save_result_state = CARD_STATE_WRITE_COMPLETE;
            break;
        case CARD_RESULT_NO_CARD:
            save_result_state = CARD_STATE_NO_CARD;
            break;
        case CARD_RESULT_FULL:
            save_result_state = CARD_STATE_NOT_ENOUGH_SPACE_PROMPT;
            break;
        case CARD_RESULT_UNFORMATTED:
            McardState = CARD_STATE_FORMAT_PROMPT;
            McardStateFlag = 0;
            goto retry_overwrite;
        }
        McardState = save_result_state;
    retry_overwrite:
        if (McardState == CARD_STATE_WRITE_COMPLETE)
            break;
        next_state = CARD_STATE_WRITE_OVERWRITE;
        saved_state = (u16)McardState;
        value = McardRetry;
        incremented = value + 1;
    update_write_retry:
        cond = value < CARD_RETRY_LIMIT;
        do
        {
            McardRetry = incremented;
        } while (0);
        if (!cond)
            next_state = saved_state;
        do
        {
            McardState = next_state;
        } while (0);
        break;
    case CARD_STATE_NOT_ENOUGH_SPACE_PROMPT:
        McardPage = CARD_PAGE_NOT_ENOUGH_SPACE_CANNOT_SAVE_PROMPT;
        break;
    case CARD_STATE_EXIT_NO_CARD:
    case CARD_STATE_EXIT_DAMAGED:
    case CARD_STATE_EXIT_NOT_ENOUGH_SPACE:
        McardState = CARD_STATE_EXIT;
        break;
    case CARD_STATE_RESTART_NO_CARD:
    case CARD_STATE_RESTART_DAMAGED:
    case CARD_STATE_RESTART_NOT_ENOUGH_SPACE:
        restart_card_check:
            McardState = CARD_STATE_SHOW_CHECKING;
            break;
    default:
        value = update_card_message_((u16 *)&McardState, &McardPage);
        if (value == 0)
        {
            McardPage = CARD_PAGE_NONE;
            McardRetry = 0;
            setup_card_screen_(1);
            if (McardState < CARD_STATE_SHOW_CHECKING)
            {
                McardState = CARD_STATE_PREPARE_CHECK;
                return 1;
            }
            McardState = CARD_STATE_PREPARE_CHECK;
            return -1;
        }
        break;
    }

    McardState += draw_card_help_(McardPage, pad);
    return 0;
}
