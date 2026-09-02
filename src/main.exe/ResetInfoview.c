#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ResetInfoview(int stage);
 *     INFOVIEW.C:1391, 18 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int stage
 *     stack sp+16     struct GsIMAGE image
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 *     extern unsigned char *ImagePath;
 * END PSX.SYM */

extern char path_chizu_tim[]; /* chizu.tim */

void ResetInfoview(int stage)
{
    int i;
    u_long *adr;
    GsIMAGE image;

    for (i = nLifeBar - 1; i >= 0; i--)
    {
        LifeBar[i].count = 0;
    }
    if (stage >= 0)
    {
        adr = PathFileRead(ImagePath, path_chizu_tim);
        GetTIMInfo(adr, &image);
        LoadTIMAndFree(adr);
        InitSprite(&image, &MapImage);
    }
}
