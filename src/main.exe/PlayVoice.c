#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>
#include "images.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayVoice(int id);
 *     IMAGES.C:485, 35 src lines, frame 72 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int id
 *     reg   $s6       unsigned char * FileName
 *     stack sp+24     struct CdlLOC start
 *     stack sp+32     struct CdlLOC end
 *     reg   $s4       struct TVoiceTable * voice
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *     reg   $s2       struct CdlLOC * loc
 *     reg   $s0       unsigned char min
 *     reg   $s1       unsigned char sec
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *VoiceXaName;
 *     extern unsigned char gSELevel;
 * END PSX.SYM */

typedef struct TVoiceTable
{
    u8 no;      /* 0x0 */
    u8 channel; /* 0x1 */
    u8 smin;    /* 0x2 */
    u8 ssec;    /* 0x3 */
    u8 emin;    /* 0x4 */
    u8 esec;    /* 0x5 */
} TVoiceTable;  /* 0x6 */

extern u8 CHOSEN_LANGUAGE;

/* Retail retains VoiceXaName and adds one filename pointer per localization. */
extern u8 *VoiceXaName;
extern u8 *VoiceXaNameF;
extern u8 *VoiceXaNameI;
extern u8 *VoiceXaNameJ;
/* Per-language voice tables. */
extern TVoiceTable *VoiceTables[4];

/* INTRO/TORA voice tables + their filenames (id ranges [100,200)/[200,300)). */
extern TVoiceTable VoiceBank1[];
extern TVoiceTable VoiceBank2[];
extern u8 *VoiceFiles1;
extern u8 *VoiceFiles2;

/* Fallback (language/range-independent) voice table. */
extern TVoiceTable VoiceCommon[]; /* fallback bank searched when no stage table matches */

extern char fmt_bad_voice_no[];           /* bad voice no %d */
extern char fmt_playvoice_fail_chan_id[]; /* playvoice fail %s  chan %d  id %d */

extern void AdtMessageBox(char *fmt, ...);
extern void CdaStop(void);
extern void SsSetMVol(int voll, int volr);
extern void set_cda_volume_(u8 voll, u8 volr);
extern void *memset(void *s, int c, u32 n);
extern int CdaPlayXA(u8 *fname, CdlLOC *start, CdlLOC *end, u8 channel, int mode);

static inline void BuildVoiceLocation(CdlLOC *loc, u8 min, u8 sec)
{
    s32 pos;

    memset(loc, 0, sizeof(CdlLOC));
    loc->minute = min;
    loc->second = sec;
    pos = CdPosToInt(loc);
    CdIntToPos(pos * 2 + OFFSET, loc);
}

/*
 * MATCHED: PlayVoice (0x8004eee4, 756 bytes / 189 instructions) searches a
 * language table for ids below 100, VoiceBank1 for ids 100-199, or VoiceBank2
 * for higher ids,
 * then falls back to VoiceCommon using the first localized filename. A miss
 * reports the id and stops CD audio.
 * A hit clamps gSELevel, restores the XA mix, converts the row's minute/second
 * pairs to start/end locations, and plays its channel; playback failure is
 * reported with the filename, channel, and id.
 *
 * Matching constraints:
 *  - VoiceXaName, VoiceXaNameF, VoiceXaNameI, and VoiceXaNameJ are distinct
 *    globals. Treating them as one global array changes the loads and grows
 *    the function from 756 to 772 bytes.
 *  - The language loop advances and then re-caches the current id. The
 *    INTRO/TORA loop instead peeks the next row through a separate pointer
 *    before advancing. Do not normalize these two source shapes.
 *  - The two BuildVoiceLocation calls need separate min/sec block scopes, and
 *    volume is s32. Together they preserve the playback tail's saved-register
 *    assignment.
 *  - The high-bank search keeps current and next as distinct pointer
 *    identities; jump2 erases the equal-arm copy that expresses this.
 *  - end_marker belongs only to the language loop. The fallback search has
 *    separate fallback and fallback_end locals; sharing the sentinel creates
 *    a false live-range conflict and swaps the fallback registers.
 *  - Select the filename slot first, then derive language from its pointer
 *    difference. That extra real reference gives the filename and table bases
 *    their target saved-register homes; direct indexing swaps them.
 *  - Keep the zero-code region after forming the two indexed addresses. It
 *    changes only sched's dependency region, producing filename address,
 *    table address, voice load, then filename load in the voice load's delay
 *    slot.
 *  - Primary-table hits are loop breaks. The fallback hit remains a goto so
 *    jump2 retains the target's small out-of-line hit block.
 */
void PlayVoice(int id)
{
    u8 *FileName;
    TVoiceTable *voice;
    s32 volume;
    TVoiceTable *match;
    TVoiceTable *cursor;
    TVoiceTable *next;
    int end_marker;
    TVoiceTable *fallback;
    int fallback_end;
    u8 *filenames[4] = {
        VoiceXaName,
        VoiceXaNameF,
        VoiceXaNameI,
        VoiceXaNameJ,
    };
    TVoiceTable *tables[4];
    CdlLOC start;
    CdlLOC end;

    __builtin_memcpy(tables, VoiceTables, sizeof(tables));
    memset(&start, 0, sizeof(start));
    memset(&end, 0, sizeof(end));

    if (id >= 100)
    {
        if (id >= 200)
        {
            voice = VoiceBank2;
            id -= 200;
            FileName = VoiceFiles2;
        }
        else
        {
            voice = VoiceBank1;
            FileName = VoiceFiles1;
            id -= 100;
        }
        match = 0;
        if (voice->no != SOUND_TABLE_END)
        {
            do
            {
                next = cursor = voice;
                if (id != 0)
                {
                    voice = next;
                }
                else
                {
                    voice = next;
                }
                match = voice;
                if (id == cursor->no)
                    break;
                next = cursor + 1;
                voice = next;
            } while (next->no != SOUND_TABLE_END);
            if (id != cursor->no)
                match = 0;
        }
        goto found;
    }
    else
    {
        int language;
        u8 **filename_entry;
        TVoiceTable **voice_entry;

        filename_entry = filenames + CHOSEN_LANGUAGE;
        language = filename_entry - filenames;
        voice_entry = tables + language;
        /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
        do
        {
        } while (0);
        voice = *voice_entry;
        FileName = *filename_entry;
        match = 0;
        if (voice->no != SOUND_TABLE_END)
        {
            end_marker = SOUND_TABLE_END;
            cursor = voice;
            do
            {
                next = match = cursor;
                if (id != 0)
                {
                    cursor = next;
                }
                else
                {
                    if (voice != 0)
                    {
                        cursor = next;
                    }
                    else
                    {
                        cursor = next;
                    }
                }
                if (id == cursor->no)
                    break;
                cursor++;
                match = 0;
            } while (cursor->no != end_marker);
        }
    }
found:
    if (match == 0)
    {
        fallback = VoiceCommon;
        if (fallback->no != SOUND_TABLE_END)
        {
            fallback_end = SOUND_TABLE_END;
            cursor = fallback;
            do
            {
                if (id == cursor->no)
                {
                    match = cursor;
                    goto found2;
                }
                cursor++;
            } while (cursor->no != fallback_end);
        }
        match = 0;
    found2:
        FileName = VoiceXaName;
        if (match == 0)
        {
            AdtMessageBox(fmt_bad_voice_no, id);
            CdaStop();
            return;
        }
    }

    volume = gSELevel;
    if (volume >= SOUND_VOLUME_MAX)
        volume = SOUND_VOLUME_MAX;
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    set_cda_volume_(volume, volume);

    {
        u8 min;
        u8 sec;

        min = match->smin;
        sec = match->ssec;
        BuildVoiceLocation(&start, min, sec);
    }

    {
        u8 min;
        u8 sec;

        min = match->emin;
        sec = match->esec;
        BuildVoiceLocation(&end, min, sec);
    }

    if (CdaPlayXA(FileName, &start, &end, match->channel, CDA_ONCE) == 0)
    {
        AdtMessageBox(fmt_playvoice_fail_chan_id, FileName, match->channel, id);
    }
}
