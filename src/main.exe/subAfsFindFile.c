#include "common.h"
#include "main.exe.h"

/*
 * subAfsFindFile (0x8005f10c, 0xB4 bytes) — linear scan over a TAFS
 * volume's element table for an entry whose 20-byte `name` matches (a
 * `strncmp` of length 0x14) AND whose `flag` bits intersect `mask`;
 * returns the matching index, or -1 if `maxElements == 0` or nothing
 * matched. Same proven TAFS/TAFSElement layout as
 * AfsRead.c/AfsInit.c/AfsGetHeader.c — sits contiguously between AfsRead
 * (ends exactly at this address) and AfsGetHeader (begins exactly where
 * this ends) in the object file, confirming the same source TU. No
 * PSX.SYM block (compiled without -g) and no callers in this binary
 * (AfsOpen calls the real, much larger AfsFindFile instead, which
 * inlines the same search twice rather than calling this helper) — dead
 * code kept because the rest of its TU is used.
 *
 * Matching notes: the asm is a textbook bottom-tested `do { } while`
 * (loop.c's initial-test duplication/hoist: the `beqz maxElements==0`
 * entry guard IS the duplicated exit test, folded away as unreachable
 * for the loop's own re-test). Index the element table
 * (`handle->pElement[i].name`/`.flag`) rather than a walking pointer —
 * TWO fields are touched per iteration, and indexing keeps both
 * addresses built off the same per-iteration base with no bias (the
 * "index when >=2 fields touched" rule). The `&&` stays a genuine
 * short-circuit (the asm's first `bnez strncmp-result` skips the flag
 * test AND the return entirely) — no un-nesting needed.
 *
 * `mask` must be `u32`, not the Ghidra-inferred `ushort`: with a `u16`
 * parameter the draft was a pure 13-byte register-COLOR swap (`name`
 * landed in $s4/`mask` in $s3, target is the reverse) — same instructions,
 * same length, just the two parameter pseudos' tie-break flipped. Neither
 * autorules nor regalloc.py's copy-chain read suggested a fix; the
 * permuter (41 iterations, --stop-on-zero) found the width flip. Its
 * winning candidate also carried a dead `if (handle->pElement) {}`
 * statement — bisected out as a red herring, not load-bearing.
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

extern int strncmp(const char *a, const char *b, u32 n);

u32 subAfsFindFile(TAFS *handle, char *name, u32 mask)
{
    u32 i;

    i = 0;
    if (handle->maxElements != 0) {
        do {
            if (strncmp(name, (char *)handle->pElement[i].name, 0x14) == 0 &&
                (mask & handle->pElement[i].flag) != 0) {
                return i;
            }
            i = i + 1;
        } while (i < handle->maxElements);
    }
    return 0xffffffff;
}
