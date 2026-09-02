#include "common.h"
#include "main.exe.h"

/*
 * get_tim_from_archive (0x8004f1d8, 0xa4 bytes) - generic archive-entry
 * accessor, near-twin of GetArcData.c: one-time-converts an ArcFile's
 * table of relative offsets into absolute pointers (same conversion
 * idiom - see GetArcData.c's matching notes for the staged entry offset),
 * then validates and returns `entry[idx]`. Unlike
 * GetArcData this takes the archive pointer directly as a PARAMETER (no
 * lazy FileRead/gp singleton caching), so `archive`/`idx` stay in
 * $a0/$a1 throughout with no register promotion.
 *
 * NOTE on the split: splat reports this as 2 pieces (a `__override__prt_`
 * glabel at 0x8004f254), but it is NOT a jump table - the raw .s has no
 * branch targeting 0x8004f254 itself; piece 1 falls straight through into
 * piece 2's first instruction (the `lui`/`jal`/`addiu` address-materialize
 * spans the piece boundary), and the one forward branch out of piece 1
 * lands mid-piece-2 at an ordinary internal label (.L8004F264). Per the
 * cookbook's "not every 2-piece report is a real jump table" rule, this
 * is written as ONE plain C function, no _jtbl array.
 */
extern void AdtMessageBox(char *fmt, ...);
extern char fmt_bad_archive_index[]; /* bad archive index %d */

u_long *get_tim_from_archive(ArcFile *archive, int idx)
{
    s32 i;
    s32 entry_offset;

    if (archive->loaded == ARC_ENTRIES_RELATIVE)
    {
        i = 0;
        if (archive->count > 0)
        {
            do
            {
                entry_offset =
                    archive->entry[i].offset + ARC_ENTRY_TABLE_OFFSET;
                archive->entry[i].data =
                    (u_long *)((u8 *)archive + entry_offset);
                i++;
            } while (i < archive->count);
        }
        archive->loaded = ARC_ENTRIES_ABSOLUTE;
    }
    if (idx < 0 || archive->count <= idx)
    {
        AdtMessageBox(fmt_bad_archive_index, idx);
        return 0;
    }
    return archive->entry[idx].data;
}
