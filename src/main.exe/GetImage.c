#include "common.h"
#include "main.exe.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct GsIMAGE * GetImage(int index);
 *     IMAGES.C:52, 16 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int index
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsIMAGE Images[52];
 * END PSX.SYM */

extern void AdtMessageBox(char *fmt, ...);
extern void vfree(void *p);
extern char path_image_images_arc[]; /* K:\\WORK\\CDIMAGE\\IMAGE\\images.arc */
extern char msg_bad_image_file[];    /* bad image file */
extern char msg_bad_image_index[];   /* bad image index */
/* Qualified because INFOVIEW.C has its own static fInitialize. */
extern u8 Images_fInitialize;
extern GsIMAGE Images[N_IMAGES];

GsIMAGE *GetImage(ImageArchiveId index)
{
    ArcFile *archive;
    u_long *adr;
    int i;

    if (Images_fInitialize == 0)
    {
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
        Images_fInitialize = 1;
    }
    if ((unsigned)index >= N_IMAGES)
    {
        AdtMessageBox(msg_bad_image_index);
        return Images;
    }
    else
    {
        return Images + index;
    }
}
