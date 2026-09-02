#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern void AdtMessageBox(char *fmt, ...);
extern char msg_afsfilesize_invalid_handle[]; /* "AfsFileSize: invalid handle" */

int AfsFileSize(TAFS *handle, TAFSFileHandle *fh)
{
    if (fh == 0)
    {
        AdtMessageBox(msg_afsfilesize_invalid_handle);
        return 0;
    }
    return fh->info->size;
}
