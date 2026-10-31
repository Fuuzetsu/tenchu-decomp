#include "common.h"
#include "main.exe.h"

#define BOOT_EXEC_MAGIC 0xDEF0C0DEu

enum
{
    BOOT_EXEC_CDROM_PREFIX_SIZE = 6,
    BOOT_EXEC_NAME_CAPACITY = 0x50
};

typedef struct
{
    u32 magic;       /* 0x00 = BOOT_EXEC_MAGIC */
    char name[BOOT_EXEC_NAME_CAPACITY]; /* 0x04 */
    u32 s_addr;      /* 0x54 */
    u32 s_size;      /* 0x58 */
} BootExecRecord;

void set_boot_exec_(u8 *file, u32 stack, u32 size)
{
    int i;
    BootExecRecord *rec;

    rec = (BootExecRecord *)TENCHU_EXECUTABLE_HANDOFF_ADDRESS;
    file += BOOT_EXEC_CDROM_PREFIX_SIZE;
    rec->magic = BOOT_EXEC_MAGIC;
    for (i = 0; file[i] != 0; i++)
    {
        rec->name[i] = file[i];
    }
    rec->name[i] = 0;
    rec->s_addr = stack;
    rec->s_size = size;
}
