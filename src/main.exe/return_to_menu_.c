#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"
#include "images.h"

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

void return_to_menu_(void)
{
    s16 i;

    for (i = 0; i < N_LOADOUT_ITEMS; i++)
    {
        PSTATE->gItem[PSTATE->CharType][i] = PSTATE->saveItem[i];
    }
    FadeOutDirect(SCREEN_FADE_FRAMES, SCREEN_FADE_BLEND, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL, SCREEN_FADE_LEVEL);
    clear_screen_();
    PSTATE->layout = STAGE_LAYOUT_RANDOM;
    PSTATE->GameRetry = PSTATE->GameRetry & (u8)~GAME_RETRY_REPLAY;
    exec_process_(PROCESS_MENU);
}
