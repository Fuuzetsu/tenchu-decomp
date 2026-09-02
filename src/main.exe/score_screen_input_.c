#include "common.h"
#include "main.exe.h"

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

/* Retail declares this s16 here although the definition returns s32. */
extern s16 update_card_screen_(s32 input);

void score_screen_input_(void)
{
    s32 lastpad;
    s32 result;
    if (PSTATE->StageNo != StageOrder[STAGE_UID_TRAINING])
    {
        if (PSTATE->StageNoMAX[PSTATE->CharType] <
            StageConfig[PSTATE->StageNo].uid)
        {
            PSTATE->StageNoMAX[PSTATE->CharType] =
                StageConfig[PSTATE->StageNo].uid;
        }
    }
    do
    {
        s32 prev;

        StartDrawing();
        prev = lastpad;
        lastpad = GetRealPad(PAD_PORT_1);
        result = 0;
        if (prev == 0 && lastpad != 0)
        {
            result = lastpad;
        }
        result = update_card_screen_(result);
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(2);
    } while (result == 0);
}
