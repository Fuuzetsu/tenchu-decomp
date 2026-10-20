#include "common.h"
#include "../main.exe/tuning.h"
#include "menu.exe.h"
#include "../main.exe/sound.h"
#include "../main.exe/vmemory.h"

/* Sound banks, spatial panning, and the menu's two playback pitches. */

/* AUDIO.C-private in the demo; externally linked while its storage is raw. */
extern s16 voice;
extern s16 highVoice;
extern char msg_sound_setup_failure[]; /* SOUND SETUP FAILURE */

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

/* The higher pitch uses an independent rotating voice counter. */
short PlaySEHigh(SoundEffect *se, short pt, long dv)
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
        highVoice = (highVoice + 1) % SOUND_VOICE_COUNT;
        if ((s16)SsUtKeyOnV(highVoice, se->VABid, SOUND_ID_PROGRAM(pt),
                            SOUND_ID_TONE(pt), (SOUND_KEY_NOTE + 12), 0, voll,
                            voll) >= 0)
        {
            SsUtAutoPan(highVoice, SOUND_PAN_CENTER,
                        (s16)(SOUND_PAN_CENTER - d), 1);
            return highVoice;
        }
    }
    return -1;
}

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
    SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
    SsSetSerialVol(SS_SERIAL_A, gSELevel, gSELevel);
}

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

void StopSE(short channel)
{
    if (channel < 0)
        SsUtAllKeyOff(0);
    else
        SsUtKeyOffV(channel);
}

short FindSETone(SoundEffect *se, short sound_id)
{
    short i = 0;
    short count;
    short program;
    short tone;
    VagAtr *attributes;

    if (se == 0)
        return 0;

    program = SOUND_ID_PROGRAM(sound_id);
    tone = SOUND_ID_TONE(sound_id);
    attributes = (VagAtr *)((u8 *)se->VABhead + sizeof(VabHdr) +
                            VAB_PROGRAM_ATTRIBUTE_COUNT * sizeof(ProgAtr));
    count = se->program * VAB_TONES_PER_PROGRAM;
    for (; i < count; i++)
    {
        if (attributes[i].prog == program && attributes[i].mode == 4 &&
            SOUND_ID_TONE(i) == tone)
            return i;
    }
    return -1;
}

/* MENU string storage. Keep the retail spelling and development paths. */
char msg_sound_setup_failure[] = "SOUND SETUP FAILURE";
