#include "common.h"
#include "main.exe.h"
#include "filesystem.h"
#include "vmemory.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitFileSystem(int mode);
 *     FILEIO.C:62, 40 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int mode
 *
 * Globals it touches, as the original declared them:
 *     extern int ReadMode;
 *     extern int TotalIO;
 *     extern unsigned long *virtual_memory_pool;
 *     extern struct MemoryDiskType *MDfat;
 *     extern struct TAFS systemAFS;
 * END PSX.SYM */

extern void PCinit(void);
extern void cd_init(void);
extern int strncmp(const char *a, const char *b, u32 n);
extern void vinit(void *adr, u32 size);
extern void *vcalloc(u32 size, u8 c);
extern int AfsOpenVolume(TAFS *handle, char *path);
extern u8 str_acqurememorydisk[16]; /* "ACQUREMEMORYDISK" */
extern char path_tenchu_data[];     /* TENCHU\\DATA */

void InitFileSystem(file_read_mode mode)
{
    u_long *saved_pool;

    ReadMode = mode;
    mode = mode & READ_SOURCE_MASK;
    TotalIO = 0;
    switch (mode)
    {
    case READ_SOURCE_DEVPC:
        PCinit();
        break;
    case READ_SOURCE_MEMORY:
        PCinit();
        if (strncmp((char *)TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS,
                    (char *)str_acqurememorydisk,
                    TENCHU_PC_MEMORY_HANDSHAKE_SIZE) != 0)
        {
            vinit(0, 0);
            __builtin_memcpy((void *)TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS,
                             str_acqurememorydisk, sizeof(str_acqurememorydisk));
            ReadMode |= READ_MODE_MEMORY_DISK_MASK;
        }
        if (ReadMode & READ_MODE_MEMORY_DISK_MASK)
        {
            saved_pool = virtual_memory_pool;
            vinit((void *)TENCHU_PC_MEMORY_POOL_ADDRESS,
                  TENCHU_PC_MEMORY_POOL_SIZE);
            vcalloc(MEMORY_DISK_SCRATCH_SIZE, 0);
            virtual_memory_pool = saved_pool;
        }
        MDfat = (MemoryDiskType *)TENCHU_PC_MEMORY_PAYLOAD_ADDRESS;
        break;
    case READ_SOURCE_CDROM:
        CdInit();
        cd_init();
        AfsOpenVolume(&systemAFS, path_tenchu_data);
        break;
    }
}
