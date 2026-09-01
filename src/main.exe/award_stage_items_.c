#include "common.h"
#include "main.exe.h"

/*
 * award_stage_items_ (0x80052ea8) — awards inventory stock at the end of a
 * stage. The rank-specific award chooses a deterministic one- or two-item
 * sweep, optional random bonuses, and the stage-specific reward; locked stock
 * entries use 0xfe and are opened by adding through the byte value.
 *
 * STATUS: MATCHED — exact 1244 bytes / 311 instructions.
 *
 * Matching notes:
 *  - Keep the three deterministic sweeps as separate source loops.  cc1
 *    emits the target's repeated blocks and keeps their s16 induction
 *    variables in caller-saved registers; factoring them would change the
 *    control flow.
 *  - The final per-character flag first forms a raw row base and then uses
 *    the field-derived gItem[0][ITEM_ARMOUR] displacement. Writing it as
 *    state->gItem[chr][ITEM_ARMOUR] makes cc1 add the item index separately
 *    instead of folding the complete displacement into the lbu/sb operands.
 *  - `award_tier` and `remaining` are signed 16-bit values. An unsigned-width
 *    mechanical rewrite happens to restore the instruction count while
 *    replacing the required sll/sra sign extension with andi/sltiu.
 */

extern s16 StageItem[];
extern s32 rand(void);

void award_stage_items_(TLinkInfo *state, ScoreResult *result)
{
    enum
    {
        N_RANDOM_AWARD_ITEMS = ITEM_ARMOUR - ITEM_SHURIKEN,
        STAGE_AWARD_GRAND_MASTER = RANK_GRAND_MASTER - RANK_GRAND_MASTER,
        STAGE_AWARD_MASTER_NINJA = RANK_GRAND_MASTER - RANK_MASTER_NINJA,
        STAGE_AWARD_NINJA = RANK_GRAND_MASTER - RANK_NINJA,
        STAGE_AWARD_NOVICE = RANK_GRAND_MASTER - RANK_NOVICE,
        STAGE_AWARD_THUG = RANK_GRAND_MASTER - RANK_THUG
    };
    stage_award_tier award_tier;
    s16 i;
    s16 remaining;
    u8 *row;

    /* The selector runs in reverse rank order. A locked ordinary item takes
     * +2 before the common +1 so 0xFE wraps to exactly 1; the Grand Master
     * stage prize performs the equivalent +3 directly. */
    award_tier = RANK_GRAND_MASTER - (u16)result->grade;
    if (award_tier >= STAGE_AWARD_NOVICE)
    {
        remaining = 5;
        if (award_tier == STAGE_AWARD_THUG)
        {
            remaining = 3;
        }
        while (remaining != 0)
        {
            i = rand() % N_RANDOM_AWARD_ITEMS + ITEM_SHURIKEN;
            if (i < ITEM_NEMURI)
            {
                if (state->gItem[state->CharType][i] == ITEM_LOCKED)
                {
                    state->gItem[state->CharType][i] += 2;
                }
                state->gItem[state->CharType][i]++;
                remaining--;
            }
            else if (state->gItem[state->CharType][i] != ITEM_LOCKED)
            {
                state->gItem[state->CharType][i]++;
                remaining--;
            }
        }
    }
    else if (award_tier == STAGE_AWARD_NINJA)
    {
        i = ITEM_SHURIKEN;
        do
        {
            if (state->gItem[state->CharType][i] == ITEM_LOCKED)
            {
                state->gItem[state->CharType][i] += 2;
            }
            state->gItem[state->CharType][i]++;
            i++;
        } while (i < ITEM_NEMURI);
        while (i < N_LOADOUT_ITEMS)
        {
            if (state->gItem[state->CharType][i] != ITEM_LOCKED)
            {
                state->gItem[state->CharType][i]++;
            }
            i++;
        }
    }
    else if (award_tier == STAGE_AWARD_MASTER_NINJA)
    {
        i = ITEM_SHURIKEN;
        do
        {
            if (state->gItem[state->CharType][i] == ITEM_LOCKED)
            {
                state->gItem[state->CharType][i] += 2;
            }
            state->gItem[state->CharType][i]++;
            i++;
        } while (i < ITEM_NEMURI);
        while (i < N_LOADOUT_ITEMS)
        {
            if (state->gItem[state->CharType][i] != ITEM_LOCKED)
            {
                state->gItem[state->CharType][i]++;
            }
            i++;
        }

        remaining = 5;
        do
        {
            i = rand() % N_RANDOM_AWARD_ITEMS + ITEM_SHURIKEN;
            if (i < ITEM_NEMURI)
            {
                if (state->gItem[state->CharType][i] == ITEM_LOCKED)
                {
                    state->gItem[state->CharType][i] += 2;
                }
                state->gItem[state->CharType][i]++;
                remaining--;
            }
            else if (state->gItem[state->CharType][i] != ITEM_LOCKED)
            {
                state->gItem[state->CharType][i]++;
                remaining--;
            }
        } while (remaining != 0);
    }
    else /* STAGE_AWARD_GRAND_MASTER */
    {
        i = ITEM_SHURIKEN;
        do
        {
            if (state->gItem[state->CharType][i] == ITEM_LOCKED)
            {
                state->gItem[state->CharType][i] += 2;
            }
            state->gItem[state->CharType][i] += 2;
            i++;
        } while (i < ITEM_NEMURI);
        while (i < N_LOADOUT_ITEMS)
        {
            if (state->gItem[state->CharType][i] != ITEM_LOCKED)
            {
                state->gItem[state->CharType][i] += 2;
            }
            i++;
        }

        i = StageItem[state->StageNo];
        if (state->gItem[state->CharType][i] == ITEM_LOCKED)
        {
            state->gItem[state->CharType][i] += 3;
        }
    }

    row = (u8 *)state + SAVE_ITEM_ROW_OFFSET(state->CharType);
    if (row[TLINKINFO_BYTE_OFFSET(gItem[0][ITEM_ARMOUR])] != ITEM_LOCKED)
    {
        row[TLINKINFO_BYTE_OFFSET(gItem[0][ITEM_ARMOUR])] = 1;
    }
}
