#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long * GetArcData(int index);
 *     IMAGES.C:158, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int index
 * END PSX.SYM */

/*
 * GetArcData (0x8004f37c, 0xd0 bytes) — lazily loads "models.arc" via
 * FileRead into the gp-relative static ArcData (defined in this TU, hence
 * gp-addressed — see the cookbook's gp section), one-time-converts
 * its table of `count` ArcEntry offsets (each stored where it will end up
 * holding an absolute pointer — the union names both views of the same slot,
 * as in ProcItemDrop's shared-constant idiom but here for a whole table) into
 * absolute pointers relative to `ARC_ENTRY_TABLE_OFFSET` (the entry table
 * follows the {count,loaded} header word), marks `loaded` so the conversion
 * only runs once, then returns `entry[index]` after validating `index`
 * against `count`.
 *
 * The relocation is an ordinary counted loop. Grouping the entry's relative
 * offset with the table-header displacement before adding the archive base
 * mirrors the wire format and preserves the retail arithmetic order.
 */

/* PSX.SYM names IMAGES.C's original archive pointer ArcData. */
extern ArcFile *ArcData;
extern void AdtMessageBox(char *fmt, ...);
extern char path_image_models_arc[]; /* K:\\WORK\\CDIMAGE\\IMAGE\\models.arc */
extern char fmt_bad_archive_index[]; /* bad archive index %d */

u_long *GetArcData(int index)
{
    s32 i;
    ArcFile *arc;

    if (ArcData == 0)
    {
        ArcData = (ArcFile *)FileRead(path_image_models_arc);
    }
    arc = ArcData;
    if (arc->loaded == ARC_ENTRIES_RELATIVE)
    {
        for (i = 0; i < arc->count; i++)
        {
            arc->entry[i].data =
                (u_long *)((u8 *)arc +
                           (arc->entry[i].offset + ARC_ENTRY_TABLE_OFFSET));
        }
        arc->loaded = ARC_ENTRIES_ABSOLUTE;
    }
    if (index < 0 || arc->count <= index)
    {
        AdtMessageBox(fmt_bad_archive_index, index);
        return 0;
    }
    return arc->entry[index].data;
}
