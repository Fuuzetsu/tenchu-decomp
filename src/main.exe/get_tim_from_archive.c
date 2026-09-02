#include "common.h"
#include "main.exe.h"

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
