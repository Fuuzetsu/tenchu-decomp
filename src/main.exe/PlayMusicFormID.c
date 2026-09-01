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

/*
 * STATUS: MATCHING — 176 bytes / 44 instructions.
 *
 * Event-audio IDs below 100 are voice clips. IDs from 100 are logical music
 * IDs and are remapped through the sentinel-terminated MusicIdByTrack before
 * being passed to _PlayMusic.
 *
 * _PlayMusic's real two-argument ABI is load-bearing: treating the table and
 * sentinel as extra call arguments lengthens their live ranges and creates a
 * false address-register conflict. The named table base plus the one-shot
 * first-load fence lets cc1 retain %hi(MusicIdByTrack) in $a2 and materialize
 * its low half only after the sentinel branch. Assigning `next_track` after
 * the match guard lets the scheduler move it into that guard's delay slot and
 * reuse $v0, while the signed integer pointer sums preserve the target's
 * index-first `addu` operand order (measured: plain p[i] flips the
 * addu operands). MusicIdAtTrack contains that byte-required address spelling
 * so the actual search can stay in the track/id vocabulary. The nested
 * one-shot fence supplies the loop weight needed for the retail register
 * colouring without emitting code.
 */

/* splat's auto MusicIdByTrack had drifted to 0x8008ea34 (+8 bytes, pre-existing
   accumulation drift in a still-raw data blob) — bound fresh at the correct
   address in config/symbols.main.exe.txt (see the cookbook's drifted-D_
   note). */
static inline u8 MusicIdAtTrack(u8 *ids_by_track, s16 track)
{
    return *(u8 *)(track + (s32)ids_by_track);
}

void PlayMusicFormID(s32 event_audio_id)
{
    s32 cue;
    u8 *ids_by_track;
    u8 *ids;
    u8 end_marker;
    u8 first_id;
    s16 track;
    s16 next_track;

    if (event_audio_id < MUSIC_EVENT_ID_BASE)
    {
        PlayVoice(event_audio_id);
        return;
    }
    ids = MusicIdByTrack;
    cue = event_audio_id - MUSIC_EVENT_ID_BASE;
    track = 0;
    first_id = *ids;
    /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
    do
    {
    } while (0);
    if (first_id != SOUND_TABLE_END)
    {
        ids_by_track = MusicIdByTrack;
        end_marker = SOUND_TABLE_END;
    search:
        if (MusicIdAtTrack(ids_by_track, track) == cue)
        {
            goto found;
        }
        next_track = track + 1;
        track = next_track;
        if (MusicIdAtTrack(ids_by_track, next_track) != end_marker)
        {
            goto search;
        }
    found:
        if (MusicIdAtTrack(ids_by_track, track) != SOUND_TABLE_END)
        {
            cue = track;
        }
    }
    _PlayMusic(cue, CDA_REPEAT);
}
