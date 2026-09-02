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
