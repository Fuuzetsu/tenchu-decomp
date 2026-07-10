#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeImage(void);
 *     IMAGES.C:32, 16 src lines, frame 40 bytes, saved-reg mask 0x800f0000
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
 * buffer. Walks Images with a plain pointer increment (`image = image + 1`)
 * — every loop iteration only touches ONE image record via the `image`
 * cursor (no second field of a later element), so no index-vs-pointer bias
 * applies (cookbook Loops: the walking-pointer bias only bites when 2+
 * fields of the SAME element are read through it).
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
extern u_long *FileRead(char *path);
extern void AdtMessageBox(char *fmt, ...);
extern u_long *get_tim_from_archive(u_long *pt, int i);
extern void LoadTIM(u_long *tim);
extern void vfree(void *p);
extern char D_8001287C[]; /* "K:\\WORK\\CDIMAGE\\IMAGE\\images.arc" */
extern char D_800128A0[]; /* "bad image file" */
extern GsIMAGE Images[62];

void InitializeImage(void)
{
    u_long *pt;
    u_long *adr;
    int i;
    GsIMAGE *image;

    pt = FileRead(D_8001287C);
    if ((short)*pt < 0x3e) {
        AdtMessageBox(D_800128A0);
    }
    i = 0;
    image = Images;
    do {
        adr = get_tim_from_archive(pt, i);
        GetTIMInfo(adr, image);
        LoadTIM(adr);
        i = i + 1;
        image = image + 1;
    } while (i < 0x3e);
    vfree(pt);
}
