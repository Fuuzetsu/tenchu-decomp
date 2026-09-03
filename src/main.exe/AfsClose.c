#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "filesystem.h"

extern char msg_afsclose_invalid_handle[]; /* "AfsClose: invalid handle" */

int AfsClose(TAFSFileHandle *fd)
{
    if (fd == 0)
    {
        AdtMessageBox(msg_afsclose_invalid_handle);
        return -1;
    }
    fd->flagUse = 0;
    return 0;
}
