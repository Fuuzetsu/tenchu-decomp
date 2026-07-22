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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       int mode
 *
 * Globals it touches, as the original declared them:
 *     extern int ReadMode;
 *     extern int TotalIO;
 *     extern unsigned long *virtual_memory_pool;
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
 * pre-allocates an 0x8000-byte scratch block, before writing a bookkeeping
 * pointer into D_80097EB8 (an unnamed neighbour of OTablePt, NOT OTablePt
 * itself — real OTablePt is 0x80097eb0 (StartDrawing.c), 8 bytes earlier;
 * PSX.SYM's demo-build global list is imprecise here). Case 2 (CD-ROM)
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
 *    cbAccess.c/FUN_80018f00.c). A fixed-size built-in copy reproduces the
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
extern u8 D_800110F0[16]; /* "ACQUREMEMORYDISK" */
extern char D_80011104[]; /* "TENCHU\DATA" */
extern void *D_80097EB8;

void InitFileSystem(int mode)
{
    PoolBlock *saved_pool;

    ReadMode = mode;
    mode = mode & 3;
    TotalIO = 0;
    switch (mode) {
    case 0:
        PCinit();
        break;
    case 1:
        PCinit();
        if (strncmp((char *)TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS,
                    (char *)D_800110F0,
                    TENCHU_PC_MEMORY_HANDSHAKE_SIZE) != 0) {
            vinit(0, 0);
            __builtin_memcpy((void *)TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS,
                             D_800110F0, sizeof(D_800110F0));
            ReadMode = ReadMode | 9;
        }
        if (ReadMode & 9) {
            saved_pool = virtual_memory_pool;
            vinit((void *)TENCHU_PC_MEMORY_POOL_ADDRESS,
                  TENCHU_PC_MEMORY_POOL_SIZE);
            vcalloc(0x8000, 0);
            virtual_memory_pool = saved_pool;
        }
        D_80097EB8 = (void *)TENCHU_PC_MEMORY_PAYLOAD_ADDRESS;
        break;
    case 2:
        CdInit();
        cd_init();
        AfsOpenVolume(&systemAFS, D_80011104);
        break;
    }
}
