#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern int cd_read(FILE *f, void *buffer, int length);
extern int strcmp(const char *a, const char *b);
extern char str_afs_vol_200[]; /* "AFS_VOL_200" */

int AfsGetHeader(TAFS *handle)
{
    AFSVolumeHeader header;
    u32 pos;

    cd_seek(handle->fpVol, 0, CDSEEK_SET);
    cd_read(handle->fpVol, &header, sizeof(header));
    if (strcmp((char *)header.signature, str_afs_vol_200) != 0)
    {
        return 1;
    }
    handle->maxElements = AFS_READ_BE32(header.element_count);
    pos = AFS_READ_BE32(header.index_position);
    handle->fModified = 0;
    handle->posElement = pos;
    return 0;
}
