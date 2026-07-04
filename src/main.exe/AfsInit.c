#include "common.h"
#include "main.exe.h"

/*
 * AfsInit (0x8005ee80) — initializes a TAFS archive-filesystem handle:
 * zeroes 4 of its 7 fields (fModified and posElement are left untouched —
 * proven by the asm, only fpVol/pElement/maxElements/maxElementArea get an
 * `sw zero`), then valloc()s a fixed 0x3C-byte TAFSFileHandle block; on OOM
 * it reports via AdtMessageBox and returns early, otherwise it zeroes the
 * freshly-allocated block with memset. Called by AfsOpenVolume.
 *
 * TAFS/TAFSElement/TAFSFileHandle/FILE mirror the Ghidra export's types
 * (a Sony afs.h/libapi shape); only TAFS's own field layout matters here, so
 * the others stay opaque.
 *
 * Matching notes: statement order matches the Ghidra rendering directly —
 * the 4 field zero-stores, the unconditional `handle->pHandle = valloc(...)`
 * before the null check, then the if/else. cc1's scheduler independently
 * floats the dependency-free `0x3C` argument constant above the prologue's
 * `sw $ra` and folds the last field-zero store into the call's delay slot;
 * no source reordering needed for that.
 */

typedef struct FILE FILE;
typedef struct TAFSElement TAFSElement;
typedef struct TAFSFileHandle TAFSFileHandle;

typedef struct TAFS
{
    FILE *fpVol;             /* 0x0 */
    s32 fModified;           /* 0x4 */
    u32 posElement;          /* 0x8 */
    TAFSElement *pElement;   /* 0xC */
    u32 maxElements;         /* 0x10 */
    s32 maxElementArea;      /* 0x14 */
    TAFSFileHandle *pHandle; /* 0x18 */
} TAFS;

extern void AdtMessageBox(char *fmt, ...);
extern void *memset(void *s, int c, u32 n);
extern void *valloc(u32 size);
extern char D_80014944[]; /* "AfsInit: not enough memory!" — lives in this
                            * TU's unsplit data blob (splat auto-symbol), same
                            * pattern as D_800121CC/D_8001113C elsewhere: a
                            * fresh string literal here would land in a NEW
                            * .rodata for this .c.o, which the linker script
                            * has no section-order slot for (rodata is only
                            * routed via explicit per-TU yaml carves). */

void AfsInit(TAFS *handle)
{
    void *p;

    handle->fpVol = 0;
    handle->maxElements = 0;
    handle->maxElementArea = 0;
    handle->pElement = 0;
    p = valloc(0x3C);
    handle->pHandle = p;
    if (p == 0)
        AdtMessageBox(D_80014944);
    else
        memset(p, 0, 0x3C);
}
