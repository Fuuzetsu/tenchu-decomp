#include "common.h"
#include "main.exe.h"
#include "filesystem.h"


u32 subAfsFindFile(TAFS *handle, char *name, u32 mask)
{
    u32 i;

    i = 0;
    if (handle->maxElements != 0)
    {
        do
        {
            if (strncmp(name, (char *)handle->pElement[i].name,
                        sizeof(handle->pElement[i].name)) == 0 &&
                (mask & handle->pElement[i].flag) != 0)
            {
                return i;
            }
            i++;
        } while (i < handle->maxElements);
    }
    return -1;
}
