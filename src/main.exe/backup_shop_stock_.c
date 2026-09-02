#include "common.h"
#include "main.exe.h"

#define PSTATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

void backup_shop_stock_(void)
{
    int i;

    for (i = 0; i < N_LOADOUT_ITEMS; i++)
    {
        PSTATE->saveItem[i] = PSTATE->gItem[PSTATE->CharType][i];
    }
}
