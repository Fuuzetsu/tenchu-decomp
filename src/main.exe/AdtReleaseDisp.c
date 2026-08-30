#include "common.h"
#include "main.exe.h"
#include "adt.h"

/*
 * AdtReleaseDisp (0x8005fca4, 0x90 bytes) — counterpart of AdtGetDisp:
 * reloads the font adapter's PSYQ FntLoad/FntOpen state from AdtFnt
 * (the shared AdtFntState also used by AdtFntLoad.c/AdtFntOpen.c/AdtQuiet.c;
 * this routine reaches x/y/w/h/isbg/n@0x0-0x14 and tx/ty@0x18/0x1c),
 * restores the saved screen region from the backup buffer via LoadImage,
 * then reinstalls the draw/display environments via PutDrawEnv/PutDispEnv.
 *
 * Ghidra's decompilation types the parameter `DRAWENV *param_1` and reaches
 * the backup region through `param_1[1].tpage`/`param_1[1].dr_env.tag` —
 * an array-indexing artifact (DRAWENV is 0x5c bytes, so param_1[1] lands at
 * +0x5c, and DRAWENV's own tpage@0x14/dr_env@0x1c fields inside THAT slot
 * fall at +0x70/+0x78). Those are really TAdtDisp's OWN `rect`@0x70 and
 * `backup`@0x78 fields (reference/psxsym-types.h) — the real parameter is
 * `TAdtDisp *`, not a DRAWENV array. `LoadImage(&disp->rect, disp->backup)`
 * reproduces the exact same addresses without the array-indexing fiction.
 */
extern AdtFntState AdtFnt;

void AdtReleaseDisp(TAdtDisp *disp)
{
    FntLoad(AdtFnt.tx, AdtFnt.ty);
    FntOpen(AdtFnt.x, AdtFnt.y, AdtFnt.w, AdtFnt.h,
            AdtFnt.isbg, AdtFnt.n);
    LoadImage(&disp->rect, disp->backup);
    DrawSync(0);
    PutDrawEnv(&disp->draw);
    PutDispEnv(&disp->disp);
}
