#include "common.h"
#include "main.exe.h"

/*
 * cd_init (0x8005f710) — clears the `flagUse` slot-free flag for all 10
 * entries of the CD file-handle pool (`FileHandlePool`, Ghidra symbols.tsv
 * @0x800c2d70; element type is the already-proven `FILE` from
 * cd_close/cd_getsize/cd_tell/AfsInit — `sw $zero,0x18(...)` is a single
 * word store of the whole `s32 flagUse`, NOT the four separate byte fields
 * (`used`/`field19_0x19`/`field20_0x1a`/`field21_0x1b`) Ghidra's
 * disc_file_descriptor_t rendering suggests — its auto-typed struct just
 * has byte-granularity fields there, a display artifact; the m2c reference
 * (`var_v0->unk18 = 0;`, one assignment) and the raw asm agree it's one sw.
 * Walks the pool backwards from index 9 to 0.
 */

typedef struct CdlLOC CdlLOC;
typedef struct CdlFILE CdlFILE;
typedef struct FILE FILE;

struct CdlLOC
{
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
};

struct CdlFILE
{
    CdlLOC pos;
    u32 size;
    s8 name[16];
};

struct FILE
{
    CdlFILE finfo;   /* 0x0 */
    s32 flagUse;     /* 0x18 */
    s32 pos;         /* 0x1C */
};

extern FILE D_800C2D70[10]; /* FileHandlePool (Ghidra symbols.tsv) — the CD
                              * file-handle pool shared with cd_open. */

void cd_init(void)
{
    int i;

    for (i = 9; i >= 0; i--)
        D_800C2D70[i].flagUse = 0;
}
