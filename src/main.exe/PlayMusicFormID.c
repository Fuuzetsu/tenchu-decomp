#include "common.h"
#include "main.exe.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayMusicFormID(int id);
 *     IMAGES.C:524, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int id
 * END PSX.SYM */

/* Explicit binding avoids an eight-byte offset error in the generated symbol. */
void PlayMusicFormID(s32 event_audio_id)
{
    s32 cue;
    s16 track;

    if (event_audio_id < MUSIC_EVENT_ID_BASE)
    {
        PlayVoice(event_audio_id);
        return;
    }
    cue = event_audio_id - MUSIC_EVENT_ID_BASE;
    track = 0;
    while (MusicIdByTrack[track] != SOUND_TABLE_END)
    {
        if (MusicIdByTrack[track] == cue)
        {
            break;
        }
        track++;
    }
    if (MusicIdByTrack[track] != SOUND_TABLE_END)
    {
        cue = track;
    }
    _PlayMusic(cue, CDA_REPEAT);
}
