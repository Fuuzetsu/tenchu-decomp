#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void _PlayMusic(int MusicNo, int mode);
 *     IMAGES.C:438, 37 src lines, frame 264 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int MusicNo
 *     param $s4       int mode
 *     stack sp+24     unsigned char [200] fname
 *     stack sp+224    struct CdlLOC start
 *     stack sp+232    struct CdlLOC end
 *     reg   $s2       struct TMusicTable * music
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gSoundLevel;
 * END PSX.SYM */

extern char msg_bad_music_no[];           /* "bad music no" */
extern char fmt_xa_path[];                /* "\TENCHU\XA\%s;1" */
extern char fmt_playmusic_fail_chan_id[]; /* "playmusic fail %s  chan %d  id %d" */

extern void AdtMessageBox(char *fmt, ...);
extern void CdaStop(void);
extern int sprintf(char *buf, char *fmt, ...);
extern void SsSetMVol(int voll, int volr);
extern void set_cda_volume_(u8 voll, u8 volr);
extern void *memset(void *s, int c, u32 n);
extern int CdaPlayXA(u8 *fname, CdlLOC *start, CdlLOC *end, u8 channel, int mode);

static inline void InitMusicLocation(CdlLOC *location, u8 minute, u8 second)
{
    memset(location, 0, sizeof(CdlLOC));
    location->minute = minute;
    location->second = second;
    CdIntToPos(CdPosToInt(location) * 2 + OFFSET, location);
}

void _PlayMusic(int MusicNo, int mode)
{
    u8 fname[200];
    CdlLOC start;
    CdlLOC end;
    TMusicTable *music;
    u8 min;
    u8 sec;

    if (MusicNo < 0 ||
        (u32)(MusicNo - MUSIC_CUE_INVALID_FIRST) <
            MUSIC_CUE_INVALID_COUNT)
    {
        AdtMessageBox(msg_bad_music_no, MusicNo);
        CdaStop();
    }
    else if (MusicNo >= MUSIC_TRACK_COUNT)
    {
        PlayVoice(MusicNo +
                  (MusicNo >= MUSIC_CUE_TORA_FIRST
                       ? MUSIC_TORA_VOICE_ID_OFFSET
                       : MUSIC_NARRATION_VOICE_ID_OFFSET));
    }
    else
    {
        music = &MusicTable[MusicNo];
        sprintf((char *)fname, fmt_xa_path, music->file);
        SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
        set_cda_volume_(gSoundLevel, gSoundLevel);

        min = music->start_minute;
        sec = music->start_second;
        InitMusicLocation(&start, min, sec);

        min = music->end_minute;
        sec = music->end_second;
        InitMusicLocation(&end, min, sec);

        if (CdaPlayXA(fname, &start, &end, music->channel, (s16)mode) == 0)
        {
            AdtMessageBox(fmt_playmusic_fail_chan_id, fname, music->channel, MusicNo);
        }
    }
}
