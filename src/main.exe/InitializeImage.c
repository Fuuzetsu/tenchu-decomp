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
