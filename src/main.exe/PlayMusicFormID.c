#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PlayMusicFormID(int id);
 *     IMAGES.C:524, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * index-first `addu` operand order. The nested one-shot fence supplies the
 * loop weight needed for the retail register colouring without emitting code.
 */

/* splat's auto D_8008EA2C had drifted to 0x8008ea34 (+8 bytes, pre-existing
   accumulation drift in a still-raw data blob) — bound fresh at the correct
   address in config/symbols.main.exe.txt (see the cookbook's drifted-D_
   note). */
extern u8 MusicIDTable[];
extern void PlayVoice(s32 id);
extern void _PlayMusic(s32 id, s32 mode);

void PlayMusicFormID(s32 id)
{
    s32 MusicNo;
    u8 *p;
    u8 *table_base;
    u8 flag;
    u8 first;
    s16 i;
    s16 j;

    if (id < 100)
    {
        PlayVoice(id);
        return;
    }
    table_base = MusicIDTable;
    MusicNo = id - 100;
    i = 0;
    do
    {
        first = *table_base;
    } while (0);
    if (first != 0xFF)
    {
        p = MusicIDTable;
        flag = 0xFF;
    search:
        do
        {
            do
            {
                if (*(u8 *)((s32)i + (s32)p) == MusicNo)
                {
                    goto found;
                }
            } while (0);
        } while (0);
        j = i + 1;
        i = j;
        if (*(u8 *)((s32)j + (s32)p) != flag)
        {
            goto search;
        }
    found:
        if (*(u8 *)((s32)i + (s32)p) != 0xFF)
        {
            MusicNo = i;
        }
    }
    _PlayMusic(MusicNo, CDA_REPEAT);
}
