#include "common.h"
#include "main.exe.h"
#include "item.h"

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

void apply_purchases_(void)
{
    s16 i;

    for (i = 0; i < N_LOADOUT_ITEMS; i++)
    {
        CamState.Owner->item[i] = PSTATE->selItem[i];
    }
}
