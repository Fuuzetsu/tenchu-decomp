#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

void cd_init(void)
{
    int i;

    for (i = N_CD_FILE_HANDLES - 1; i >= 0; i--)
        FileHandlePool[i].flagUse = 0;
}
