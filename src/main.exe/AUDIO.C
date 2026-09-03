#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "sound.h"
#include "vmemory.h"

/*
 * Demo AUDIO.C orders these routines as InitSoundEffect, SetupSE, DisposeSE,
 * PlaySE, and StopSE. StopSE was removed before both the trial and retail
 * builds, which place the surviving definitions in the order below. Both
 * orders are recorded in the translation-unit manifest.
 */

/* AUDIO.C-private in the demo; externally linked while its storage is raw. */
extern s16 voice;
extern char msg_sound_setup_failure[]; /* SOUND SETUP FAILURE */

extern u16 SsUtKeyOnV(s16 voice_id, s16 vab_id, s32 program, s32 tone,
                      s32 note, s32 fine, u32 volume_left,
                      u32 volume_right);
extern void SsUtAutoPan(s16 voice_id, s32 start, s16 end, s32 duration);
extern void SsInit(void);
extern void SsSetTickMode(int mode);
extern void SsStart(void);
extern void SsSetMVol(int volume_left, int volume_right);
extern void SsSetMono(void);
extern void SsSetStereo(void);
extern vab_id SsVabOpenHead(u8 *vab, vab_id requested_id);
extern void SsVabTransBody(u8 *body, vab_id id);
extern void SsVabTransCompleted(int flag);
extern void SsUtAllKeyOff(s32 flag);
extern void SsVabClose(vab_id id);


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short PlaySE(struct SoundEffect *se, short pt, long dv);
 *     AUDIO.C:72, 18 src lines, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct SoundEffect * se
 *     param $a1       short pt
 *     param $a2       long dv
 *     reg   $s0       short d
 *     reg   $v1       short v
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gSELevel;
 * END PSX.SYM */

short PlaySE(SoundEffect *se, short pt, long dv)
{
    s16 d;
    s16 v;
    s16 voll;

    if (se != NULL)
    {
        d = SOUND_SPATIAL_DIRECTION(dv);
        v = (s16)SOUND_SPATIAL_DIRECTION(dv);
        voll = (u32)(SOUND_SPATIAL_VOLUME(dv) * gSELevel) >>
               SOUND_LEVEL_SHIFT;
        if (v > 0)
        {
            d = -(s32)((u32)(SOUND_SPATIAL_DIRECTION(dv) &
                             SOUND_PAN_ANGLE_MASK) >>
                       SOUND_PAN_ANGLE_SHIFT);
        }
        else if (v < 0)
        {
            v = (v < 0) ? -v : v;
            d = (v & SOUND_PAN_ANGLE_MASK) >> SOUND_PAN_ANGLE_SHIFT;
        }
        voice = (voice + 1) % SOUND_VOICE_COUNT;
        if ((s16)SsUtKeyOnV(voice, se->VABid, SOUND_ID_PROGRAM(pt),
                            SOUND_ID_TONE(pt), SOUND_KEY_NOTE, 0, voll,
                            voll) >= 0)
        {
            SsUtAutoPan(voice, SOUND_PAN_CENTER,
                        (s16)(SOUND_PAN_CENTER - d), 1);
            return voice;
        }
    }
    return -1;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitSoundEffect(void);
 *     AUDIO.C:28, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gSound;
 * END PSX.SYM */

void InitSoundEffect(void)
{
    SsInit();
    SsSetTickMode(1);
    SsStart();
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    if (gSound != SOUND_MODE_MONO)
    {
        SsSetStereo();
    }
    else
    {
        SsSetMono();
    }
}

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeSE(struct SoundEffect *se);
 *     AUDIO.C:61, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct SoundEffect * se
 * END PSX.SYM */

void DisposeSE(SoundEffect *se)
{
    if (se != 0)
    {
        SsUtAllKeyOff(0);
        SsVabClose(se->VABid);
        vfree(se->VABhead);
        vfree(se);
    }
}
