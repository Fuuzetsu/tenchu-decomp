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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
extern int sprintf(char *buf, char *fmt, ...);

/* The card header and save payload are one MemoryCardFileBlock. */
card_result LoadCard(s32 target, u8 *name)
{
    void *allocation;
    u8 fn[200];
    MemoryCardFileBlock block;
    s32 cmd;
    enum card_result result;

    allocation = valloc(BLOCKSIZE);
    result = MemCardAccept(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    sprintf(fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardReadFile(MEMCARD_CHANNEL_0, (char *)fn, &block, 0,
                             sizeof(block));
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    if (result != CARD_RESULT_SUCCESS)
    {
        vfree(allocation);
        allocation = 0;
    }
    else
    {
        __builtin_memcpy((void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         block.payload,
                         TENCHU_PERSISTENT_STATE_SIZE);
    }
    vfree(allocation);
    return result;
}
