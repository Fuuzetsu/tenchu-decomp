#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "filesystem.h"

extern TAFSElement *AfsFindFile(TAFS *handle, char *path, u32 flags);
extern char msg_afsopen_not_found[]; /* "AfsOpen: %s not found\n" */
extern char msg_afsopen_no_handle[]; /* "AfsOpen: no more handle\n[%s]" */

TAFSFileHandle *AfsOpen(TAFS *handle, char *path)
{
    TAFSElement *entry;
    TAFSFileHandle *cur;
    u32 count;

    entry = AfsFindFile(handle, path, AfsFlag_File);
    count = 0;
    if (entry == 0)
    {
        AdtMessageBox(msg_afsopen_not_found, path);
    }
    else
    {
        cur = handle->pHandle;
        do
        {
            count++;
            if (cur->flagUse == 0)
            {
                cur->info = entry;
                cur->pos = 0;
                cur->flagUse = 1;
                return cur;
            }
            cur += 2;
        } while (count < N_AFS_FILE_HANDLES);
        AdtMessageBox(msg_afsopen_no_handle, path);
    }
    return 0;
}
