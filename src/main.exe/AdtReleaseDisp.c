#include "common.h"
#include "main.exe.h"
#include "adt.h"

/*
 * AdtReleaseDisp (0x8005fca4, 0x90 bytes) — counterpart of AdtGetDisp:
 * reloads the font adapter's PSYQ FntLoad/FntOpen state from D_8008F1B8
 * (same global as AdtFntLoad.c/AdtFntOpen.c/AdtQuiet.c — this TU reaches
 * ALL EIGHT fields x/y/w/h/isbg/n@0x0-0x14 and tx/ty@0x18/0x1c, so it
 * declares the full 8-field view), restores the saved screen region from
 * the backup buffer via LoadImage, then reinstalls the draw/display
 * environments via PutDrawEnv/PutDispEnv.
 *
 * Ghidra's decompilation types the parameter `DRAWENV *param_1` and reaches
 * the backup region through `param_1[1].tpage`/`param_1[1].dr_env.tag` —
 * an array-indexing artifact (DRAWENV is 0x5c bytes, so param_1[1] lands at
 * +0x5c, and DRAWENV's own tpage@0x14/dr_env@0x1c fields inside THAT slot
 * fall at +0x70/+0x78). Those are really TAdtDisp's OWN `rect`@0x70 and
 * `backup`@0x78 fields (reference/psxsym-types.h) — the real parameter is
 * `TAdtDisp *`, not a DRAWENV array. `LoadImage(&ad->rect, ad->backup)`
 * reproduces the exact same addresses without the array-indexing fiction.
 */
typedef struct
{
    s32 x, y, w, h, isbg, n;
    s32 tx, ty;
} AdtFntState;

extern AdtFntState D_8008F1B8;
extern void FntLoad(int tx, int ty);
extern int FntOpen(int x, int y, int w, int h, int isbg, int n);

void AdtReleaseDisp(TAdtDisp *ad)
{
    FntLoad(D_8008F1B8.tx, D_8008F1B8.ty);
    FntOpen(D_8008F1B8.x, D_8008F1B8.y, D_8008F1B8.w, D_8008F1B8.h,
            D_8008F1B8.isbg, D_8008F1B8.n);
    LoadImage(&ad->rect, (u_long *)ad->backup);
    DrawSync(0);
    PutDrawEnv(&ad->draw);
    PutDispEnv(&ad->disp);
}
