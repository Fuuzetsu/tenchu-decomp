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
    u32 magic;
    BootExecRecord *rec;

    magic = BOOT_EXEC_MAGIC;
    /* Empty loop retained for code layout; its original source construct is unknown. */
    do
    {
    } while (0);
    rec = (BootExecRecord *)TENCHU_EXECUTABLE_HANDOFF_ADDRESS;
    file += BOOT_EXEC_CDROM_PREFIX_SIZE;
    rec->magic = magic;
    i = 0;
    if (*file != 0)
    {
        do
        {
            rec->name[i] = *file;
            file++;
            i++;
        } while (*file != 0);
    }
    rec->name[i] = 0;
    rec->s_addr = stack;
    rec->s_size = size;
}
