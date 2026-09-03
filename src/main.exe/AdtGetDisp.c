#include "common.h"
#include "main.exe.h"
#include "adt.h"

void AdtGetDisp(TAdtDisp *disp)
{
    enum
    {
        ADT_PANEL_MARGIN = 32
    };
    DRAWENV de;
    DISPENV di;

    SetDispMask(1);
    DrawSync(0);
    setRECT(&disp->rect, AdtFnt.tx, AdtFnt.ty, 0x40, 0x100);
    StoreImage(&disp->rect, disp->backup);
    DrawSync(0);
    GetDrawEnv(&disp->draw);
    GetDispEnv(&disp->disp);
    SetDefDrawEnv(&de, 0, 0, SCREEN_W, SCREEN_H);
    SetDefDispEnv(&di, 0, 0, SCREEN_W, SCREEN_H);
    PutDrawEnv(&de);
    PutDispEnv(&di);
    FntLoad(AdtFnt.tx, AdtFnt.ty);
    FntOpen(ADT_PANEL_MARGIN, ADT_PANEL_MARGIN,
            SCREEN_W - 2 * ADT_PANEL_MARGIN,
            SCREEN_H - 2 * ADT_PANEL_MARGIN, 0, 512);
    setPolyF4(&disp->bg);
    /* The panel behind the debug font: a 32,32 - 288,208 screen quad. */
    setXY4(&disp->bg,
           ADT_PANEL_MARGIN, ADT_PANEL_MARGIN,
           SCREEN_W - ADT_PANEL_MARGIN, ADT_PANEL_MARGIN,
           ADT_PANEL_MARGIN, SCREEN_H - ADT_PANEL_MARGIN,
           SCREEN_W - ADT_PANEL_MARGIN, SCREEN_H - ADT_PANEL_MARGIN);
    setRGB0(&disp->bg, 1, 1, 100);
}
