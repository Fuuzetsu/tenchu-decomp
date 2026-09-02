#include "common.h"
#include "main.exe.h"
#include "adt.h"

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
