#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadCard(int target, unsigned char *name);
 *     MEMCARD.C:119, 35 src lines, frame 8448 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       int target
 *     param $a1       unsigned char * name
 *     stack sp+24     unsigned char [200] fn
 *     stack sp+8416   long cmd
 *     stack sp+8420   long result
 *     stack sp+224    unsigned char [8192] block
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

extern char CardPathFormat[];

extern void *valloc(u32 size);
extern void vfree(void *ptr);
extern s32 MemCardAccept(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);
extern s32 MemCardReadFile(s32 chan, char *name, void *data, s32 offset,
                           s32 size);
extern int sprintf(char *buf, char *fmt, ...);

/*
 * The two apparent Ghidra buffers are one 8 KiB card block: the card header
 * occupies its first 0x200 bytes and the persistent payload begins at
 * `block + sizeof(TCardHeader)`. The fixed-size built-in payload copy
 * reproduces the compiler's aligned/unaligned loop pair.
 */
s16 LoadCard(s32 target, u8 *name)
{
    void *temp;
    u8 fn[200];
    u8 block[BLOCKSIZE];
    s32 cmd;
    s32 result;

    temp = valloc(BLOCKSIZE);
    result = MemCardAccept(0);
    MemCardSync(0, &cmd, &result);
    sprintf(fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardReadFile(0, fn, block, 0, BLOCKSIZE);
    MemCardSync(0, &cmd, &result);
    if (result != 0)
    {
        vfree(temp);
        temp = 0;
    }
    else
    {
        __builtin_memcpy((void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         block + sizeof(TCardHeader),
                         TENCHU_PERSISTENT_STATE_SIZE);
    }
    vfree(temp);
    return result;
}
