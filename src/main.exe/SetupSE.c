#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct SoundEffect * SetupSE(unsigned char *vab);
 *     AUDIO.C:40, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned char * vab
 * END PSX.SYM */

/*
 * SetupSE (0x80018ce8, 0xb8 bytes) — allocate a SoundEffect record and load
 * a VAB (SsVabOpenHead for the header, SsVabTransBody/SsVabTransCompleted +
 * vrealloc for the body), returning NULL when `vab` is NULL. SoundEffect
 * (VABid@0 s16, program@2 s16, VABhead@4 void*) proven by DisposeSE.c — this
 * is the function that ALLOCATES it (valloc(sizeof(SoundEffect))).
 *
 * Matching notes:
 *  - The 0x12 halfword is read once (u16, lhu) and used TWICE: scaled by
 *    512 (sign-extend+scale idiom, sll16/sra7 — cookbook toolchain
 *    gotchas' "ordinary matchable" 2-instruction class, not the blocked
 *    3-instruction one) for `size`, and stored raw into se->program.
 *  - se->VABid is RELOADED (not kept live in a register) for the
 *    SsVabTransBody call: the intervening (conditional) SystemOut call
 *    clobbers the caller-saved copy, so cc1 re-reads it from memory —
 *    just write `se->VABid` again rather than caching it in a local.
 */
typedef struct
{
    s16 VABid;
    s16 program;
    void *VABhead;
} SoundEffect;

extern short SsVabOpenHead(u8 *vab, short mode);
extern void SsVabTransBody(u8 *body, short vabId);
extern void SsVabTransCompleted(int flag);
extern void SystemOut(u8 *msg);
extern void *valloc(u32 size);
extern void *vmemoryGC(void *p);
extern void *vrealloc(void *p, s32 size);

extern char D_800110DC[]; /* "SOUND SETUP FAILURE" */

void *SetupSE(u8 *vab)
{
    SoundEffect *se;
    s32 size;
    u16 t;

    if (vab == 0)
    {
        return 0;
    }
    se = (SoundEffect *)valloc(sizeof(SoundEffect));
    se->VABid = SsVabOpenHead(vab, -1);
    if (se->VABid == -1)
    {
        SystemOut(D_800110DC);
    }
    t = *(u16 *)(vab + 0x12);
    size = ((t << 16) >> 7) + 0xA20;
    se->program = t;
    SsVabTransBody(vab + size, se->VABid);
    SsVabTransCompleted(1);
    se->VABhead = vrealloc(vab, size);
    return vmemoryGC(se);
}
