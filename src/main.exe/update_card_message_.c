#include "common.h"
#include "main.exe.h"

/*
 * update_card_message_ (0x8005aba4) — advance the memory-card state/message machine.
 *
 * `case 4` (ChkCard() dispatch) has FIVE origins that all need the same
 * "is next_state == 40" shift+test. A prior draft wrote that test out
 * TWICE — once after each of the two inner default: arms — reasoning that
 * cc1 would fold `next_state = 40; shifted = (u32)(u16)next_state << 16;`
 * into a `lui` there (which it does) and leave the genuine dynamic
 * sll/sra only at the shared `card_state_shift:` label used by the other
 * three origins. That measured 13 bytes off across 6 instructions.
 *
 * The target's raw asm shows this fold is wrong: ALL FIVE origins reach
 * the SAME single `sll $v0,$s0,16 / sra $v0,$v0,16` — there is only ONE
 * `next_state != 40` test in the source, reached by `goto` from every
 * arm (including both defaults). The apparent "extra" sll/sra copies at
 * two addresses are a pure reorg (delay-slot-fill) artifact: the two
 * default arms' `addiu $s0,40` gets sunk into the PRECEDING beq's delay
 * slot (the fallthrough continuation is safe to run on the taken path too,
 * since the taken target immediately overwrites $s0), which leaves each
 * default's own trailing unconditional `j card_state_shift` with an empty
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

extern s16 ChkCard(void);
extern s16 FormatCard(void);
extern s32 MemCardExist(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s32 update_card_message_(s16 *state, u16 *message)
{
    s32 cmd;
    s32 result;
    s16 card_status;
    s16 next_state;
    u16 next_message;

    next_state = *state;
    next_message = *message;

    switch (next_state)
    {
    case 0:
        next_message = 16;
        goto increment_state;

    case 3:
        McardRetryCount = 0;
        next_state = 4;
        break;

    case 4:
        /* The two-stage card_status dispatch (pre-tests ==2/>=3 feeding two
         * tiny switches) is source, not a rendered decision tree: a single
         * switch over {1,2,4,default} would emit ONE default body, but
         * retail carries TWO separate next_state = 0x28 stores. */
        card_status = ChkCard();
        if (card_status == 2)
        {
            goto card_status_two;
        }
        if (card_status >= 3)
        {
            goto card_status_ge_three;
        }
        switch (card_status)
        {
        default:
            next_state = 40;
            break;
        case 1:
            goto card_status_one;
        }
        goto card_state_shift;

    card_status_ge_three:
        switch (card_status)
        {
        default:
            next_state = 40;
            break;
        case 4:
            goto card_status_four;
        }
        goto card_state_shift;

    card_status_one:
        next_state = 10;
        goto card_state_shift;
    card_status_two:
        next_state = 20;
        goto card_state_shift;
    card_status_four:
        McardStateFlag = 0;
        next_state = 30;

    card_state_shift:
        if (next_state != 40 && McardRetryCount++ < CARD_RETRY_LIMIT)
        {
            next_state = 4;
        }
        break;

    case 10:
        next_message = 18;
        break;

    case 20:
        next_message = 2;
        break;

    case 30:
        result = MemCardExist(0);
        MemCardSync(0, &cmd, &result);
        next_message = 3;
        if (result == 0)
        {
            break;
        }
        next_message = 0;
        /* fallthrough */
    case 37:
    case 92:
        next_state = 0;
        break;

    case 31:
        next_message = 10;
        McardRetryCount = 0;
        next_state = 33;
        break;

    case 32:
        next_state = 90;
        break;

    case 1:
    case 2:
    case 33:
    case 34:
    increment_state:
        next_state++;
        break;

    case 35:
        next_state = 36;
        card_status = FormatCard();
        if (card_status == 0)
        {
            next_state = 38;
        }
        if (next_state != 38 && McardRetryCount++ < CARD_RETRY_LIMIT)
        {
            next_state = 35;
        }
        break;

    case 36:
        next_message = 12;
        break;

    case 38:
        next_message = 11;
        break;

    case 90:
        next_message = 20;
        break;

    case 11:
    case 21:
    case 91:
        next_state = -1;
        break;

    default:
        return 0;
    }

    *state = next_state;
    *message = next_message;
    return -1;
}
