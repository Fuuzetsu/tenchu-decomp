#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct SoundEffect * SetupSE(unsigned char *vab);
 *     AUDIO.C:40, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *  - VabHdr.ps is read once (u16, lhu) and used TWICE: scaled by
 *    VAB_TONE_ATTRIBUTE_BYTES_PER_PROGRAM (sign-extend+scale idiom,
 *    sll16/sra7 — cookbook toolchain
 *    gotchas' "ordinary matchable" 2-instruction class, not the blocked
 *    3-instruction one) for `size`, and stored raw into se->program.
 *  - se->VABid is RELOADED (not kept live in a register) for the
 *    SsVabTransBody call: the intervening (conditional) SystemOut call
 *    clobbers the caller-saved copy, so cc1 re-reads it from memory —
 *    just write `se->VABid` again rather than caching it in a local.
 */
extern vab_id SsVabOpenHead(u8 *vab, vab_id requested_id);
extern void SsVabTransBody(u8 *body, vab_id id);
extern void SsVabTransCompleted(int flag);
extern void *valloc(u32 size);
extern void *vmemoryGC(void *p);
extern void *vrealloc(void *p, u32 size);

extern char msg_sound_setup_failure[]; /* SOUND SETUP FAILURE */

SoundEffect *SetupSE(u8 *vab)
{
    VabHdr *header;
    SoundEffect *se;
    s32 size;
    u16 programs;

    if (vab == 0)
    {
        return 0;
    }
    header = (VabHdr *)vab;
    se = (SoundEffect *)valloc(sizeof(SoundEffect));
    se->VABid = SsVabOpenHead(vab, VAB_ID_AUTO);
    if (se->VABid == VAB_ID_ERROR)
    {
        SystemOut(msg_sound_setup_failure);
    }
    programs = header->ps;
    size = ((programs << 16) >> (16 - VAB_TONE_ATTRIBUTE_SHIFT)) +
           VAB_FIXED_METADATA_SIZE;
    se->program = programs;
    SsVabTransBody(vab + size, se->VABid);
    SsVabTransCompleted(1);
    se->VABhead = vrealloc(vab, size);
    return vmemoryGC(se);
}
