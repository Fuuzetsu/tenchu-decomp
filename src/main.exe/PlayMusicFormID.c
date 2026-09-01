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
 * IDs below 100 are voice clips. Larger IDs are remapped through the
 * sentinel-terminated MusicIDTable before being passed to _PlayMusic.
 *
 * _PlayMusic's real two-argument ABI is load-bearing: treating the table and
 * sentinel as extra call arguments lengthens their live ranges and creates a
 * false address-register conflict. The named table base plus the one-shot
 * first-load fence lets cc1 retain %hi(MusicIDTable) in $a2 and materialize
 * its low half only after the sentinel branch. Assigning `j` after the match
 * guard lets the scheduler move it into that guard's delay slot and reuse
 * $v0, while the signed integer pointer sums preserve the target's
 * index-first `addu` operand order (measured: plain p[i] flips the
 * addu operands; the minimal `*(u8 *)(i + (s32)p)` spelling is exact). The nested one-shot fence supplies the
 * loop weight needed for the retail register colouring without emitting code.
 */

/* splat's auto MusicIDTable had drifted to 0x8008ea34 (+8 bytes, pre-existing
   accumulation drift in a still-raw data blob) — bound fresh at the correct
   address in config/symbols.main.exe.txt (see the cookbook's drifted-D_
   note). */
extern u8 MusicIDTable[];
void PlayMusicFormID(s32 id)
{
    s32 MusicNo;
    u8 *search_table;
    u8 *table_base;
    u8 flag;
    u8 first;
    s16 i;
    s16 j;

    if (id < MUSIC_EVENT_ID_BASE)
    {
        PlayVoice(id);
        return;
    }
    table_base = MusicIDTable;
    MusicNo = id - MUSIC_EVENT_ID_BASE;
    i = 0;
    first = *table_base;
    /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
    do
    {
    } while (0);
    if (first != SOUND_TABLE_END)
    {
        search_table = MusicIDTable;
        flag = SOUND_TABLE_END;
    search:
        if (*(u8 *)(i + (s32)search_table) == MusicNo)
        {
            goto found;
        }
        j = i + 1;
        i = j;
        if (*(u8 *)(j + (s32)search_table) != flag)
        {
            goto search;
        }
    found:
        if (*(u8 *)(i + (s32)search_table) != SOUND_TABLE_END)
        {
            MusicNo = i;
        }
    }
    _PlayMusic(MusicNo, CDA_REPEAT);
}
