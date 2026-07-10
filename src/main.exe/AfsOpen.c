#include "common.h"
#include "main.exe.h"

/*
 * AfsOpen (0x8005ef34, 0x9c bytes) — finds `path` in the AFS volume's file
 * table (AfsFindFile) and, on a hit, claims the first free slot in
 * `handle`'s fixed 5-entry TAFSFileHandle pool (linear scan, `flagUse==0`),
 * filling it in (info/pos/flagUse) and returning it. Reports via
 * AdtMessageBox and returns NULL both when the file isn't found and when
 * all 5 handle slots are in use. Same proven TAFS/TAFSElement layout as
 * AfsRead.c/AfsInit.c/AfsClose.c/AfsFileSize.c. Real return type is
 * `TAFSFileHandle *` (established by LoadFromCDROM.c's callers), not
 * Ghidra's `TAFSElement *`.
 *
 * Matching notes:
 *  - The pool-slot cursor advances by 0x18 (24) bytes per slot, NOT the
 *    proven 0xC-byte TAFSFileHandle size AfsRead/AfsClose/AfsFileSize use —
 *    those only ever DEREFERENCE a single handle pointer, never INDEX the
 *    pool, so their truncated view is safe for them but wrong as an array
 *    stride here (cookbook: "a truncated per-TU struct breaks when the
 *    element is array-indexed"). Advance via an explicit byte-cast
 *    (`(u8 *)cur + 0x18`) rather than widening the shared struct.
 *  - The scan is a bottom-tested `do/while` with the counter incremented as
 *    the loop body's FIRST statement (matches Ghidra's literal order here;
 *    the asm's `addiu $v1,$v1,1` sits in the found-check branch's delay
 *    slot, i.e. unconditional every iteration, same shape either way it's
 *    read).
 *  - Flipping the guard to `if (entry == 0) {short} else {long}` (rather
 *    than the literal `if (entry != 0) {long}`) was needed to match the
 *    error-message address materialization order and the branch polarity —
 *    the long success arm belongs unnested/fallthrough here.
 *
 * STATUS: NON_MATCHING — 12 of 156 bytes differ, SAME LENGTH as the
 * target (39 instructions both sides). Every instruction matches 1:1 in
 * shape and position; the only difference is a register-allocation tie:
 * target colours `handle` into $s1 and `path` into $s0 (the reverse of the
 * natural pseudo-number tie-break), and separately materializes the two
 * error-message addresses straight into $a0 (`lui $a0,%hi/addiu
 * $a0,$a0,%lo`) where our compile routes the %hi half through a scratch
 * $v0 first (`lui $v0,%hi/addiu $a0,$v0,%lo`). `tools/regalloc.py` shows
 * both parameters have symmetric usage (one call argument + one later use
 * each) with no copy-chain to break — a pure global-alloc coloring tie.
 * Ruled out: reordering the local variable declarations (`fmt` first,
 * `count`/`cur`/`entry` first) — no effect; a `count=0;` moved before vs
 * after the `AfsFindFile` call — regressed (gave `count` its own extra
 * callee-saved register, 8 bytes worse). A bounded decomp-permuter run
 * (`--stop-on-zero -j4`, ~5 min) plateaued at score 20-45 (worse than this
 * baseline), never reaching 0 — matches the cookbook's named "sub-C-level
 * residual" class (a same-length pure register-color tie); no further
 * permuter session was run per the iteration protocol's early-stop.
 */
typedef struct FILE FILE;
typedef struct TAFS TAFS;
typedef struct TAFSElement TAFSElement;
typedef struct TAFSFileHandle TAFSFileHandle;

struct TAFSElement
{
    u16 flag;    /* 0x0 */
    u32 pos;     /* 0x4 */
    u32 size;    /* 0x8 */
    u32 psize;   /* 0xC */
    u8 name[20]; /* 0x10 */
};

struct TAFS
{
    FILE *fpVol;             /* 0x0 */
    s32 fModified;           /* 0x4 */
    u32 posElement;          /* 0x8 */
    TAFSElement *pElement;   /* 0xC */
    u32 maxElements;         /* 0x10 */
    s32 maxElementArea;      /* 0x14 */
    TAFSFileHandle *pHandle; /* 0x18 */
};

struct TAFSFileHandle
{
    s32 flagUse;       /* 0x0 */
    u32 pos;           /* 0x4 */
    TAFSElement *info; /* 0x8 */
};

extern TAFSElement *AfsFindFile(TAFS *handle, char *path, s32 mode);
extern void AdtMessageBox(char *fmt, ...);
extern char D_80014960[]; /* "AfsOpen: %s not found\n" */
extern char D_80014978[]; /* "AfsOpen: no more handle\n[%s]" */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AfsOpen", AfsOpen);
#else
TAFSFileHandle *AfsOpen(TAFS *handle, char *path)
{
    TAFSElement *entry;
    TAFSFileHandle *cur;
    u32 count;
    char *fmt;

    entry = AfsFindFile(handle, path, 1);
    count = 0;
    if (entry == 0) {
        fmt = D_80014960;
    } else {
        cur = handle->pHandle;
        do {
            count = count + 1;
            if (cur->flagUse == 0) {
                cur->info = entry;
                cur->pos = 0;
                cur->flagUse = 1;
                return cur;
            }
            cur = (TAFSFileHandle *)((u8 *)cur + 0x18);
        } while (count < 5);
        fmt = D_80014978;
    }
    AdtMessageBox(fmt, path);
    return 0;
}
#endif
