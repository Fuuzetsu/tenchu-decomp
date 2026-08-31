#include "common.h"
#include "main.exe.h"

/*
 * award_stage_items_ (0x80052ea8) — awards inventory stock at the end of a
 * stage.  The result kind chooses a deterministic one- or two-item sweep,
 * optional random bonuses, and the stage-specific reward; locked stock
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
 *    row[0x41f].  Writing it as state->gItem[chr*0x20 + 0x13] makes cc1 add
 *    0x13 to the index in a separate instruction instead of folding 0x41f
 *    into the lbu/sb memory operands.
 *  - `kind` and `remaining` are signed 16-bit values.  An unsigned-width
 *    mechanical rewrite happens to restore the instruction count while
 *    replacing the required sll/sra sign extension with andi/sltiu.
 */

extern s16 StageItem[];
extern s32 rand(void);

void award_stage_items_(TLinkInfo *state, ScoreResult *result)
{
    s16 kind;
    s16 i;
    s16 remaining;
    u8 *row;

    /* The worse the rank, the bigger the consolation award (a Grand
     * Master run gets nothing). A locked special item takes +2 before
     * the common +1 so 0xFE wraps to exactly 1 — the award unlocks it
     * with one unit. */
    kind = 4 - (u16)result->grade;
    if (kind >= 3)
    {
        remaining = 5;
        if (kind == 4)
        {
            remaining = 3;
        }
        while (remaining != 0)
        {
            i = rand() % 18 + 1;
            if (i < 9)
            {
                if (state->gItem[i + state->CharType * 0x20] == ITEM_LOCKED)
                {
                    state->gItem[i + state->CharType * 0x20] += 2;
                }
                state->gItem[i + state->CharType * 0x20]++;
                remaining--;
            }
            else if (state->gItem[i + state->CharType * 0x20] != ITEM_LOCKED)
            {
                state->gItem[i + state->CharType * 0x20]++;
                remaining--;
            }
        }
    }
    else if (kind == 2)
    {
        i = 1;
        do
        {
            if (state->gItem[i + state->CharType * 0x20] == ITEM_LOCKED)
            {
                state->gItem[i + state->CharType * 0x20] += 2;
            }
            state->gItem[i + state->CharType * 0x20]++;
            i++;
        } while (i < 9);
        while (i < 0x14)
        {
            if (state->gItem[i + state->CharType * 0x20] != ITEM_LOCKED)
            {
                state->gItem[i + state->CharType * 0x20]++;
            }
            i++;
        }
    }
    else if (kind == 1)
    {
        i = 1;
        do
        {
            if (state->gItem[i + state->CharType * 0x20] == ITEM_LOCKED)
            {
                state->gItem[i + state->CharType * 0x20] += 2;
            }
            state->gItem[i + state->CharType * 0x20]++;
            i++;
        } while (i < 9);
        while (i < 0x14)
        {
            if (state->gItem[i + state->CharType * 0x20] != ITEM_LOCKED)
            {
                state->gItem[i + state->CharType * 0x20]++;
            }
            i++;
        }

        remaining = 5;
        do
        {
            i = rand() % 18 + 1;
            if (i < 9)
            {
                if (state->gItem[i + state->CharType * 0x20] == ITEM_LOCKED)
                {
                    state->gItem[i + state->CharType * 0x20] += 2;
                }
                state->gItem[i + state->CharType * 0x20]++;
                remaining--;
            }
            else if (state->gItem[i + state->CharType * 0x20] != ITEM_LOCKED)
            {
                state->gItem[i + state->CharType * 0x20]++;
                remaining--;
            }
        } while (remaining != 0);
    }
    else
    {
        i = 1;
        do
        {
            if (state->gItem[i + state->CharType * 0x20] == ITEM_LOCKED)
            {
                state->gItem[i + state->CharType * 0x20] += 2;
            }
            state->gItem[i + state->CharType * 0x20] += 2;
            i++;
        } while (i < 9);
        while (i < 0x14)
        {
            if (state->gItem[i + state->CharType * 0x20] != ITEM_LOCKED)
            {
                state->gItem[i + state->CharType * 0x20] += 2;
            }
            i++;
        }

        i = StageItem[state->StageNo];
        if (state->gItem[i + state->CharType * 0x20] == ITEM_LOCKED)
        {
            state->gItem[i + state->CharType * 0x20] += 3;
        }
    }

    row = (u8 *)state + state->CharType * 0x20;
    if (row[0x41f] != ITEM_LOCKED)
    {
        row[0x41f] = 1;
    }
}
