#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern char AfsPathFormat[];

static __inline__ void AfsFilenameFixInline(char *path)
{
    char *p;

    if (*path != 0)
    {
        p = path;
        do
        {
            *p = toupper(*p);
            p++;
        } while (*p != 0);
    }
}

static __inline__ u32 subAfsFindFileInline(TAFS *handle, char *name, u32 mask)
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
    return 0xffffffff;
}

TAFSElement *AfsFindFile(TAFS *handle, char *path, u32 flags)
{
    char buffer[200];
    char component[200];
    char *cursorPath;
    s32 cursor;
    s32 entryIndex;

    strncpy(buffer, path, 199);
    buffer[199] = 0;

    AfsFilenameFixInline(buffer);

    cursor = 0;
    while (buffer[cursor] != 0)
    {
        cursorPath = buffer;
        cursorPath += cursor;
        if (*cursorPath == '\\')
        {
            *cursorPath = 0;
            entryIndex = subAfsFindFileInline(handle, buffer, AfsFlag_Folder);
            if (entryIndex < 0)
            {
                goto not_found;
            }
            strcpy(component, buffer + cursor + 1);
            sprintf(buffer, AfsPathFormat, entryIndex, component);
            cursor = 0;
        }
        else
        {
            cursor++;
        }
    }

    entryIndex = subAfsFindFileInline(handle, buffer, flags);
    if (entryIndex < 0)
    {
    not_found:
        return 0;
    }
    return &handle->pElement[entryIndex];
}
