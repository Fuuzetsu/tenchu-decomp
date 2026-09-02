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
