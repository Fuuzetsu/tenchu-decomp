#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern void AdtMessageBox(char *fmt, ...);
extern void *memset(void *s, int c, u32 n);
extern void *valloc(u32 size);
extern char msg_afsinit_not_enough_memory[]; /* "AfsInit: not enough memory!" */

void AfsInit(TAFS *handle)
{
    handle->fpVol = 0;
    handle->maxElements = 0;
    handle->maxElementArea = 0;
    handle->pElement = 0;
    handle->pHandle = valloc(N_AFS_FILE_HANDLES * sizeof(TAFSFileHandle));
    if (handle->pHandle == 0)
        AdtMessageBox(msg_afsinit_not_enough_memory);
    else
        memset(handle->pHandle, 0,
               N_AFS_FILE_HANDLES * sizeof(TAFSFileHandle));
}
