#include "common.h"
#include "main.exe.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeImage(void);
 *     IMAGES.C:32, 16 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsIMAGE Images[52];
 * END PSX.SYM */

/*
 * InitializeImage (0x8004f44c, 0xac bytes) — loads the shared 62-entry
 * Images[] pool from the images.arc archive: read the whole archive via
 * FileRead, sanity-check its entry count, then for each of the 0x3e (62)
 * slots fetch the packed TIM via get_tim_from_archive, read its geometry
 * with GetTIMInfo and upload it with LoadTIM, before freeing the archive
 * buffer. The source indexes the owning Images[] array directly; GCC turns
 * that fixed-size indexing into the same advancing cursor used by retail.
 *
 * The bad-file `AdtMessageBox` call and the loop's `i = 0;` init share ONE
 * source statement via the branch's delay slot: reorg hoists `i = 0;` (the
 * statement immediately after the whole if) into the `beqz`'s delay slot
 * (safe on both paths — neither path has touched `i` yet), so it executes
 * unconditionally before the branch decides whether to also run
 * AdtMessageBox first. Do NOT write a redundant `i = 0;` inside the if body;
 * Ghidra's own rendering (one `iVar1 = 0;` positioned after the whole if)
 * is the real single source statement — the cookbook's "hoist a shared
 * default to the single entry" mechanism (Think1trace) already covers this
 * shape without restructuring.
 */
extern void AdtMessageBox(char *fmt, ...);
extern void vfree(void *p);
extern char path_image_images_arc[]; /* K:\\WORK\\CDIMAGE\\IMAGE\\images.arc */
extern char msg_bad_image_file[];    /* bad image file */
extern GsIMAGE Images[N_IMAGES];

void InitializeImage(void)
{
    ArcFile *archive;
    u_long *adr;
    int i;

    archive = (ArcFile *)FileRead(path_image_images_arc);
    if (archive->count < N_IMAGES)
    {
        AdtMessageBox(msg_bad_image_file);
    }
    for (i = 0; i < N_IMAGES; i++)
    {
        adr = get_tim_from_archive(archive, i);
        GetTIMInfo(adr, &Images[i]);
        LoadTIM(adr);
    }
    vfree(archive);
}
