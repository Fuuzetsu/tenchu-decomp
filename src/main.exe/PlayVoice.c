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

extern u8 CHOSEN_LANGUAGE;

/* Retail retains VoiceXaName and adds one filename pointer per localization. */
extern u8 *VoiceXaName;
extern u8 *VoiceXaNameF;
extern u8 *VoiceXaNameI;
extern u8 *VoiceXaNameJ;
/* Per-language voice tables. */
extern TVoiceTable *EventVoiceTables[N_LANGUAGES];

/* The two non-localized banks and their literal INTRO.XA/TORA.XA paths. */
extern TVoiceTable IntroVoiceTable[];
extern TVoiceTable ToraVoiceTable[];
extern u8 *IntroVoiceXaName;
extern u8 *ToraVoiceXaName;

/* Fallback (language/range-independent) voice table. */
extern TVoiceTable CommonVoiceTable[];

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
    u8 *filenames[N_LANGUAGES] = {
        VoiceXaName,
        VoiceXaNameF,
        VoiceXaNameI,
        VoiceXaNameJ,
    };
    TVoiceTable *tables[N_LANGUAGES];
    CdlLOC start;
    CdlLOC end;

    __builtin_memcpy(tables, EventVoiceTables, sizeof(tables));
    memset(&start, 0, sizeof(start));
    memset(&end, 0, sizeof(end));

    if (id >= VOICE_ID_INTRO_BASE)
    {
        if (id >= VOICE_ID_TORA_BASE)
        {
            voice = ToraVoiceTable;
            id -= VOICE_ID_TORA_BASE;
            FileName = ToraVoiceXaName;
        }
        else
        {
            voice = IntroVoiceTable;
            FileName = IntroVoiceXaName;
            id -= VOICE_ID_INTRO_BASE;
        }
        match = 0;
        if (voice->id != SOUND_TABLE_END)
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
                if (id == cursor->id)
                    break;
                next = cursor + 1;
                voice = next;
            } while (next->id != SOUND_TABLE_END);
            if (id != cursor->id)
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
        /* Empty loop retained for code layout; its original source construct is unknown. */
        do
        {
        } while (0);
        voice = *voice_entry;
        FileName = *filename_entry;
        match = 0;
        if (voice->id != SOUND_TABLE_END)
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
                if (id == cursor->id)
                    break;
                cursor++;
                match = 0;
            } while (cursor->id != end_marker);
        }
    }
found:
    if (match == 0)
    {
        fallback = CommonVoiceTable;
        if (fallback->id != SOUND_TABLE_END)
        {
            fallback_end = SOUND_TABLE_END;
            cursor = fallback;
            do
            {
                if (id == cursor->id)
                {
                    match = cursor;
                    goto found2;
                }
                cursor++;
            } while (cursor->id != fallback_end);
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

        min = match->start_minute;
        sec = match->start_second;
        BuildVoiceLocation(&start, min, sec);
    }

    {
        u8 min;
        u8 sec;

        min = match->end_minute;
        sec = match->end_second;
        BuildVoiceLocation(&end, min, sec);
    }

    if (CdaPlayXA(FileName, &start, &end, match->channel, CDA_ONCE) == 0)
    {
        AdtMessageBox(fmt_playvoice_fail_chan_id, FileName, match->channel, id);
    }
}
