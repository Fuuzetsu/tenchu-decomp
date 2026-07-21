#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ResetInfoview(int stage);
 *     INFOVIEW.C:1391, 18 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       int stage
 *     stack sp+16     struct GsIMAGE image
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 *     extern struct tag_TItem items[30];
 *     extern unsigned char *ImagePath;
 * END PSX.SYM */

/*
 * ResetInfoview (0x8004bfa4, 0x80 bytes) — resets the 5-entry LifeBar[]
 * pool (same struct/stride as PutLifeBarS.c) and, for a valid stage, reloads
 * the minimap sprite MapImage from "chizu.tim" via PathFileRead/GetTIMInfo/
 * LoadTIMAndFree/InitSprite (the same call chain InitSprite.c's own header
 * documents). PSX.SYM (an earlier build) recorded `LifeBar[4]`; the loop
 * here plainly zeroes 5 entries (i=4..0) like ReqLifeBar/PutLifeBarS's
 * already-matched `LifeBar[5]` — the retail array grew by one, so LifeBar[5]
 * is what reproduces the bytes (cookbook: "the layouts are from an earlier
 * build ... if the retail .s disagrees, the asm wins").
 *
 * `int i` (not `short`) lets loop.c strength-reduce `LifeBar[i].count = 0;`
 * into the walking cursor the asm shows (`addiu $v0,$v0,-0x14` each
 * iteration, starting at &LifeBar[4]); only one field is touched so there's
 * no walking-pointer field-order bias to worry about (cookbook Loops).
 */
extern LifeBarEntry LifeBar[5];
extern u8 *ImagePath;
extern char D_80012564[]; /* "chizu.tim" */
extern GsSPRITE MapImage;

void ResetInfoview(int stage)
{
    int i;
    u_long *adr;
    GsIMAGE image;

    for (i = 4; i >= 0; i--) {
        LifeBar[i].count = 0;
    }
    if (stage >= 0) {
        adr = PathFileRead(ImagePath, D_80012564);
        GetTIMInfo(adr, &image);
        LoadTIMAndFree(adr);
        InitSprite(&image, &MapImage);
    }
}
