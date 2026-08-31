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

/*
 * ResetInfoview (0x8004bfa4, 0x80 bytes) — resets the five-entry
 * LifeBar[nLifeBar] pool (same struct/stride as PutLifeBarS.c) and, for a
 * valid stage, reloads
 * the minimap sprite MapImage from "chizu.tim" via PathFileRead/GetTIMInfo/
 * LoadTIMAndFree/InitSprite (the same call chain InitSprite.c's own header
 * documents). PSX.SYM (an earlier build) recorded `LifeBar[4]`; the loop
 * here plainly zeroes five entries like ReqLifeBar/PutLifeBarS's
 * already-matched pool — the retail array grew by one, so `nLifeBar = 5`
 * is what reproduces the bytes (cookbook: "the layouts are from an earlier
 * build ... if the retail .s disagrees, the asm wins").
 *
 * `int i` (not `short`) lets loop.c strength-reduce `LifeBar[i].count = 0;`
 * into the walking cursor the asm shows (`addiu $v0,$v0,-0x14` each
 * iteration, starting at &LifeBar[4]); only one field is touched so there's
 * no walking-pointer field-order bias to worry about (cookbook Loops).
 */
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
