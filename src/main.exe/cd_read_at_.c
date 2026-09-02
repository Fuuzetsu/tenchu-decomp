#include "common.h"
#include "main.exe.h"

extern void cd_read_sectors_(u8 *buffer, s32 sector, s32 byteOffset, s32 length);

void cd_read_at_(void *buffer, int sector, int count)
{
    cd_read_sectors_(buffer, sector, 0, count << 0xb);
}
