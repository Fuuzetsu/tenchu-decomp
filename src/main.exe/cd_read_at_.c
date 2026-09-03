#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

void cd_read_at_(void *buffer, int sector, int count)
{
    cd_read_sectors_(buffer, sector, 0, count << 0xb);
}
