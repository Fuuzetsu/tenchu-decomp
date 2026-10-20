#ifndef MENU_PERSISTENT_H
#define MENU_PERSISTENT_H

#include "menu.exe.h"
#include "../main.exe/item.h"

enum
{
    MENU_STATE_MAGIC = 0x19981110,
    MENU_STATE_VISITED = 2,
    MENU_STATE_RETAIN_ON_LOAD = 0x34
};

/* New-game defaults also replace a loaded save with an incompatible magic. */
/* MAIN owns the callable definition; the card menus inline the same reset. */
#ifndef MENU_DEFINE_PERSISTENT_RESET
extern __inline__
#endif
void InitMenuPersistentState(void)
{
    TLinkInfo *state = MENU_STATE;

    memset(state, 0, TENCHU_PERSISTENT_STATE_SIZE);
    state->magic = MENU_STATE_MAGIC;
    state->Nannido = DIFFICULTY_NORMAL;
    state->Stereo = SOUND_MODE_STEREO;
    state->SoundLevel = 96;
    state->SELevel = 96;
    state->fMemory = 0;
    state->Anakon = 1;
    state->language = LANG_JAPANESE;
    state->StageNoMAX[AYAME_0] = 0;
    state->StageNoMAX[RIKIMARU_0] = 0;
    memset(state->gItem, (s8)ITEM_LOCKED, sizeof(state->gItem));
    state->gItem[RIKIMARU_0][ITEM_KAGINAWA] = ITEM_INFINITE;
    state->gItem[RIKIMARU_0][ITEM_SHURIKEN] = 6;
    state->gItem[RIKIMARU_0][ITEM_MAKIBISHI] = 6;
    state->gItem[RIKIMARU_0][ITEM_KUSURI] = 2;
    state->gItem[RIKIMARU_0][ITEM_FIRE] = 1;
    state->gItem[RIKIMARU_0][ITEM_SMOKE] = 1;
    state->gItem[RIKIMARU_0][ITEM_DOKUDANGO] = 3;
    state->gItem[RIKIMARU_0][ITEM_GOSHIKIMAI] = 5;
    __builtin_memcpy(state->gItem[AYAME_0], state->gItem[RIKIMARU_0],
                     sizeof(state->gItem[RIKIMARU_0]));

    /* The save image reserves another 0xA00 bytes beyond the game state. */
    memset(state + 1, 0xff,
           TENCHU_PERSISTENT_STATE_SIZE - sizeof(*state));
}

#endif
