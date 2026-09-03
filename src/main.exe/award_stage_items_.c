#include "common.h"
#include "main.exe.h"


void award_stage_items_(TLinkInfo *state, ScoreResult *result)
{
    enum
    {
        N_RANDOM_AWARD_ITEMS = ITEM_ARMOUR - ITEM_SHURIKEN
    };
    stage_award_tier award_tier;
    s16 i;
    s16 remaining;

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

    if (state->gItem[state->CharType][ITEM_ARMOUR] != ITEM_LOCKED)
    {
        state->gItem[state->CharType][ITEM_ARMOUR] = 1;
    }
}
