#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>
#include "images.h"

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
 * PlayVoice (0x8004eee4) — look up voice-clip `id` in one of several
 * TVoiceTable arrays and CdaPlayXA it. id < 100: current-language event
 * table (VoiceTables..EC / VoiceXaName, indexed by CHOSEN_LANGUAGE).
 * 100 <= id < 200: the INTRO table (id -= 100). id >= 200: the TORA table
 * (id -= 200). If none of those has the id, falls back to the shared
 * VoiceCommon table (always using the first event-table filename).
 * Each table is a linear array of {no,channel,smin,ssec,emin,esec}
 * records terminated by no==0xff. On a total miss: AdtMessageBox + CdaStop.
 * On a hit: clamp the persisted volume byte (gSELevel) to 0x7f, reset the
 * CD-XA mix volume, re-apply the persisted volume, build `start`/`end`
 * CdlLOCs from the record's min/sec (the lead-in-compensated "*2+OFFSET"
 * conversion, exactly _PlayMusic.c's own start/end construction), and
 * CdaPlayXA; a zero return gets its own AdtMessageBox.
 * The selected retail table row is named `match`: PSX.SYM's original `loc`
 * local was instead a CdlLOC pointer, so that name does not belong to this
 * TVoiceTable view.
 *
 * Matching notes:
 *  - Retail's four localized filename pointers are distinct globals, not one
 *    global array: the target loads all four independently through $gp before
 *    constructing the local `filenames` array. Declaring a global
 *    `VoiceXaName[4]` instead makes cc1 materialize one base and load at
 *    offsets 0/4/8/12, growing this function from 756 to 772 bytes. The first
 *    pointer retains PSX.SYM's original VoiceXaName name; F/I/J are the added
 *    retail localizations visible in the pointed-to filenames.
 *  - The two search loops are DIFFERENT shapes (confirmed against the raw
 *    target .s and cross-checked with Ghidra's own two independent
 *    reconstructions): the CHOSEN_LANGUAGE-indexed loop re-caches the id
 *    scalar (`entry = voice; if (match) break; voice++; entry = 0;` —
 *    Ghidra's own `uVar3 = *pbVar5;` re-read-after-advance shape), while
 *    the INTRO/TORA loop peeks the NEXT record's id through a SEPARATE
 *    pointer before actually advancing the cursor (`entry = voice; if
 *    (match) goto found; entry = voice + 1; voice++; } while (entry->no
 *    != 0xff);` — Ghidra literally renders this as two assignments,
 *    `pbVar6 = pbVar5 + 6; pbVar5 = pbVar5 + 6;`, that a naive reading
 *    would collapse into one).
 *
 * STATUS: MATCHED — 756 bytes / 189 instructions, pure C.
 *
 * PSX.SYM lists two distinct nested `min`/`sec` pairs. Giving each
 * BuildVoiceLocation call its own block scope, while keeping the volume clamp
 * in an `s32`, fixes the saved-register cycle and makes the complete playback
 * tail exact. The high-ID search keeps separate current and peek-next pointer
 * identities through a jump2-erased equal-arm copy.
 *
 * Two fixes closed 14 -> 4 (both confirmed with tools/regalloc.py, not
 * guessed): (1) naming the language-search loop's exit sentinel
 * (`end_marker = 0xff;` local instead of the literal `0xff` in
 * `while (voice_id != 0xff)`) fixed the emission order of the cached
 * sentinel vs. the `cursor = voice` copy — matches a live permuter find,
 * ported by delta not score, from a bounded run (RESULT.md's
 * `output-50-1`): 14 -> 10 bytes. (2) The SAME reused `end_marker` local,
 * shared with the fallback search (`VoiceCommon`), was forced to CONFLICT
 * with `voice` in regalloc.py's `.greg` dump (`82 conflicts: ... 88`) —
 * `voice`'s pseudo is read every iteration of the CHOSEN_LANGUAGE loop by
 * its nested `if (voice != 0)` fence, so it stays live well past where the
 * target's `a0` copy of it actually dies, and the reused sentinel pseudo
 * (live across BOTH loops) is born inside that extended range. That false
 * conflict exiled the sentinel out of `voice`'s register in the fallback
 * loop too, cascading into the a0/a1 swap there. Giving the fallback search
 * its own `fallback`/`fallback_end` locals (declared, never read by the
 * other two branches) removes the shared pseudo and the false conflict
 * outright — collapsed 3 clusters in one edit: 9 -> 4 bytes, exactly the
 * "fixing one allocation collapses several clusters" pattern.
 *
 * The last 4-byte residual was not a sub-C floor. Compiler dumps showed that
 * it comprised two decisions: global allocation of the adjacent filename and
 * voice-table aggregate bases, then local scheduling of their two indexed
 * addresses. Selecting a filename slot first and deriving `language` from its
 * pointer difference is ordinary pointer code and gives the filename base one
 * additional real RTL reference. In the exact `.lreg`/`.greg` dumps the two
 * bases are p95 = 3 refs / 8 live insns -> s0 and p96 = 2 / 8 -> s1, exactly
 * the target homes. Directly indexing both arrays had instead produced 2 / 10
 * and 2 / 7 and exchanged those homes.
 *
 * With the saved homes fixed, `sched` still chose the table-address `addu`
 * first because its load feeds the immediately following voice-id test. The
 * target forms both addresses first (filename, then table), loads the voice,
 * then fills its load delay with the filename load. The zero-code loop boundary
 * after address formation leaves global allocation unchanged but gives sched
 * exactly that dependency boundary. This was verified pass-by-pass in `.cse`,
 * `.sched`, `.lreg`, and `.greg`; removing only the boundary preserves length
 * and the saved homes but exchanges the four v0/v1 operands at 0x8004f024-30.
 */
void PlayVoice(int id)
{
    u8 *FileName;
    TVoiceTable *voice;
    s32 volume;
    TVoiceTable *match;
    TVoiceTable *cursor;
    TVoiceTable *next;
    u8 voice_id;
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
        if (voice->no != 0xff)
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
                voice_id = cursor->no;
                match = voice;
                if (id == voice_id)
                    goto found;
                next = cursor + 1;
                voice = next;
            } while (next->no != 0xff);
            match = 0;
        }
        goto found;
    fallback_hit:
        match = cursor;
        goto found2;
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
        if (voice->no != 0xff)
        {
            end_marker = 0xff;
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
                voice_id = cursor->no;
                if (id == voice_id)
                    goto found;
                cursor++;
                voice_id = cursor->no;
                match = 0;
            } while (voice_id != end_marker);
        }
    }
found:
    if (match == 0)
    {
        fallback = VoiceCommon;
        if (fallback->no != 0xff)
        {
            fallback_end = 0xff;
            cursor = fallback;
            do
            {
                voice_id = cursor->no;
                if (id == voice_id)
                {
                    goto fallback_hit;
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
    if (volume >= 0x7f)
        volume = 0x7f;
    SsSetMVol(0x7f, 0x7f);
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
