#include "common.h"
#include "main.exe.h"
#include "sound.h"

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

extern s16 voice;
extern u16 SsUtKeyOnV(s16, s16, s32, s32, s32, s32, u32, u32);
extern void SsUtAutoPan(s16, s32, s16, s32);

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
