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

/*
 * InitFileSystem (0x80019238, 0x15c bytes) — dispatches on `mode & 3`
 * (0/1 = PC-link dev, 2 = CD-ROM AFS volume; 3 has no case) to set up the
 * active file-loading backend FileRead() will use. Case 1 (PC-link,
 * "acquire memory disk") lazily writes a 16-byte "ACQUREMEMORYDISK" magic
 * into a fixed low-memory handshake buffer at 0x807f0000 (only if it isn't
 * already there), sets ReadMode's PC-link bits, then — if EITHER the
 * PC-link bit (1) or the just-set "acquire" bit (8) is set — reinitializes
 * the virtual-memory pool at a fixed 0x80200000/0x5f0000 region and
 * pre-allocates an 0x8000-byte scratch block, before pointing the original
 * `MemoryDiskType *MDfat` at the low-memory payload. MDfat is the neighbour
 * of OTablePt at 0x80097eb8, not OTablePt itself. Case 2 (CD-ROM)
 * inits the CD and opens the "TENCHU\DATA" AFS volume into the global
 * `systemAFS` handle.
 *
 * Matching notes:
 *  - `ReadMode = mode;` (the UNMASKED parameter) is stored FIRST, then
 *    `mode` is reassigned in place to `mode & 3` (the asm's `andi` operates
 *    directly on the $a0 parameter register, no separate move — the
 *    "reused parameter" idiom) and used as the dispatch value from then on.
 *  - The dispatch is a real if/else-if ladder (SIGNED `slti` for the `< 2`
 *    test, over the reused `mode` register) matching Ghidra's polarity
 *    directly: `mode==1` / else `mode<2` (nested `mode==0`) / else `mode==2`.
 *  - The 16-byte magic write is ONE aligned-1 byte-array copy,
 *    not Ghidra's 16 separate byte assignments (its usual block-move
 *    decompilation artifact — same class as the DRAWENV copies in
 *    cbAccess.c/stop_access_meter_.c). A fixed-size built-in copy reproduces the
 *    raw .s's lwl/lwr+swl/swr chunking.
 *  - `virtual_memory_pool`'s save/restore around the vinit+vcalloc pair
 *    sits INSIDE the `if (ReadMode & 9)` guard in the asm (the load is
 *    scheduled right after the guard's own delay slot, i.e. only reached
 *    when the branch falls through) — not hoisted unconditionally before
 *    the guard the way Ghidra renders it (functionally identical either way
 *    since the save/restore is a no-op when the guard is false, but this
 *    placement is what the asm's instruction order actually shows).
 */
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
