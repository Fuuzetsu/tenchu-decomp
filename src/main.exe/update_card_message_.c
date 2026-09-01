#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/*
 * update_card_message_ (0x8005aba4) — advance the memory-card state/message machine.
 *
 * `CARD_STATE_CHECK` (4, the ChkCard() dispatch) has FIVE origins that all
 * need the same "is next_state == CARD_STATE_CARD_READY" shift+test. A
 * prior draft wrote that test out TWICE — once after each of the two inner
 * default: arms — reasoning that
 * cc1 would fold `next_state = CARD_STATE_CARD_READY;` into a `lui` there
 * (which it does) and leave the genuine dynamic sll/sra only at the shared
 * `retry_card_check:` label used by the other three origins. That measured
 * 13 bytes off across 6 instructions.
 *
 * The target's raw asm shows this fold is wrong: ALL FIVE origins reach
 * the SAME single `sll $v0,$s0,16 / sra $v0,$v0,16` — there is only ONE
 * `next_state != CARD_STATE_CARD_READY` test in the source, reached by
 * `goto` from every arm (including both defaults). The apparent "extra"
 * sll/sra copies at
 * two addresses are a pure reorg (delay-slot-fill) artifact: the two
 * default arms' `addiu $s0,40` gets sunk into the PRECEDING beq's delay
 * slot (the fallthrough continuation is safe to run on the taken path too,
 * since the taken target immediately overwrites $s0), which leaves each
 * default's own trailing unconditional `j retry_card_check` with an empty
 * delay slot — reorg fills THAT with a duplicated copy of the jump
 * target's first instruction (the shared `sll`) and retargets the jump
 * to land just past it. Two lexical copies of the C statement produce
 * three lexical instances feeding the fold; one shared C statement,
 * reached by goto from all five arms, produces the target's exact
 * three-physical-copy, zero-fold shape for free — plainer C, exact match.
 * Lesson: when a "duplicated" instruction sequence appears at multiple
 * addresses but every appearance is the delay slot of an unconditional
 * jump to a common label, suspect reorg's delay-slot fill-from-target
 * before suspecting the source has two copies of the statement.
 */

extern s16 McardStateFlag;
extern s16 McardRetryCount;

extern card_result ChkCard(void);
extern card_result FormatCard(void);

s32 update_card_message_(s16 *state, u16 *message)
{
    s32 cmd;
    enum card_result result;
    card_result card_status;
    card_state next_state;
    card_page next_message;

    next_state = *state;
    next_message = *message;

    switch (next_state)
    {
    case CARD_STATE_SHOW_CHECKING:
        next_message = CARD_PAGE_CHECKING;
        goto increment_state;

    case CARD_STATE_PREPARE_CHECK:
        McardRetryCount = 0;
        next_state = CARD_STATE_CHECK;
        break;

    case CARD_STATE_CHECK:
        /* The two-stage card_status dispatch (pre-tests ==2/>=3 feeding two
         * tiny switches) is source, not a rendered decision tree: a single
         * switch over {1,2,4,default} would emit ONE default body, but
         * retail carries TWO separate CARD_STATE_CARD_READY stores. */
        card_status = ChkCard();
        if (card_status == CARD_RESULT_DAMAGED)
        {
            goto card_damaged;
        }
        if (card_status >= CARD_RESULT_NEW_CARD)
        {
            goto card_status_ge_three;
        }
        switch (card_status)
        {
        default:
            next_state = CARD_STATE_CARD_READY;
            break;
        case CARD_RESULT_NO_CARD:
            goto card_missing;
        }
        goto retry_card_check;

    card_status_ge_three:
        switch (card_status)
        {
        default:
            next_state = CARD_STATE_CARD_READY;
            break;
        case CARD_RESULT_UNFORMATTED:
            goto card_unformatted;
        }
        goto retry_card_check;

    card_missing:
        next_state = CARD_STATE_NO_CARD;
        goto retry_card_check;
    card_damaged:
        next_state = CARD_STATE_DAMAGED;
        goto retry_card_check;
    card_unformatted:
        McardStateFlag = 0;
        next_state = CARD_STATE_FORMAT_PROMPT;

    retry_card_check:
        if (next_state != CARD_STATE_CARD_READY &&
            McardRetryCount++ < CARD_RETRY_LIMIT)
        {
            next_state = CARD_STATE_CHECK;
        }
        break;

    case CARD_STATE_NO_CARD:
        next_message = CARD_PAGE_NO_CARD_SAVE_WARNING;
        break;

    case CARD_STATE_DAMAGED:
        next_message = CARD_PAGE_DAMAGED;
        break;

    case CARD_STATE_FORMAT_PROMPT:
        result = MemCardExist(MEMCARD_CHANNEL_0);
        MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
        next_message = CARD_PAGE_FORMAT_PROMPT;
        if (result == CARD_RESULT_SUCCESS)
        {
            break;
        }
        next_message = CARD_PAGE_NONE;
        /* fallthrough */
    case CARD_STATE_RESTART_FORMAT_FAILED:
    case CARD_STATE_RESTART_SAVE:
        next_state = CARD_STATE_SHOW_CHECKING;
        break;

    case CARD_STATE_BEGIN_FORMAT:
        next_message = CARD_PAGE_FORMATTING;
        McardRetryCount = 0;
        next_state = CARD_STATE_FORMAT_WAIT_1;
        break;

    case CARD_STATE_CANCEL_FORMAT:
        next_state = CARD_STATE_CANNOT_SAVE_PROMPT;
        break;

    case CARD_STATE_CHECK_WAIT_1:
    case CARD_STATE_CHECK_WAIT_2:
    case CARD_STATE_FORMAT_WAIT_1:
    case CARD_STATE_FORMAT_WAIT_2:
        increment_state:
            next_state++;
            break;

    case CARD_STATE_FORMAT:
        next_state = CARD_STATE_FORMAT_FAILED;
        card_status = FormatCard();
        if (card_status == CARD_RESULT_SUCCESS)
        {
            next_state = CARD_STATE_FORMAT_COMPLETE;
        }
        if (next_state != CARD_STATE_FORMAT_COMPLETE &&
            McardRetryCount++ < CARD_RETRY_LIMIT)
        {
            next_state = CARD_STATE_FORMAT;
        }
        break;

    case CARD_STATE_FORMAT_FAILED:
        next_message = CARD_PAGE_FORMAT_FAILED;
        break;

    case CARD_STATE_FORMAT_COMPLETE:
        next_message = CARD_PAGE_FORMAT_COMPLETE;
        break;

    case CARD_STATE_CANNOT_SAVE_PROMPT:
        next_message = CARD_PAGE_CANNOT_SAVE_PROMPT;
        break;

    case CARD_STATE_EXIT_NO_CARD:
    case CARD_STATE_EXIT_DAMAGED:
    case CARD_STATE_EXIT_WITHOUT_SAVE:
        next_state = CARD_STATE_EXIT;
        break;

    default:
        return 0;
    }

    *state = next_state;
    *message = next_message;
    return -1;
}
