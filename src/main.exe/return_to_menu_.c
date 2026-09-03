#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

extern void exec_process_(int id);

void return_to_menu_(void)
{
    s16 i;

    i = 0;
    do
    {
        s16 idx;

        idx = (s16)i;
        PSTATE->gItem[PSTATE->CharType][idx] = PSTATE->saveItem[idx];
        i++;
    } while ((s16)i < N_LOADOUT_ITEMS);
    FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
    clear_screen_();
    PSTATE->layout = STAGE_LAYOUT_RANDOM;
    PSTATE->GameRetry = PSTATE->GameRetry & (u8)~GAME_RETRY_REPLAY;
    exec_process_(PROCESS_MENU);
}
