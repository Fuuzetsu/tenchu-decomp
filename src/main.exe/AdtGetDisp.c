#include "common.h"
#include "main.exe.h"
#include "adt.h"

/*
 * AdtGetDisp (0x8005f7f4) — save the current screen to the backup buffer and
 * (re)initialise the Adt display/draw environment. Same TAdtDisp combined
 * struct AdtMessageBox.c/AdtReleaseDisp.c use (param declared DRAWENV* by the
 * callers, but it's really &TAdtDisp.draw at offset 0). Grabs the font-adapter
 * texture coords (AdtFnt.tx/ty) into the backup RECT, StoreImage's the
 * framebuffer into TAdtDisp.backup, resets the draw/disp envs to the standard
 * 320x240 layout, reloads the font, and builds a semi-transparent POLY_F4 quad
 * primitive in TAdtDisp.bg (the 0x8078 offset forces cc1's large-displacement
 * `addu base,param,0x8000` + small-offset stores).
 *
 * The RECT x/y take the low u16 of the s32 tx/ty (narrowing store -> lhu+sh);
 * FntLoad takes the full s32 tx/ty. DRAWENV/DISPENV are SEPARATE stack locals
 * (each rounded up to a multiple of 8: 0x5c->0x60, 0x14->0x18) which reproduces
 * the sp+0x18 / sp+0x78 layout exactly, so no combined struct is needed here.
 */

extern AdtFntState AdtFnt;

void AdtGetDisp(TAdtDisp *disp)
{
    DRAWENV de;
    DISPENV di;

    SetDispMask(1);
    DrawSync(0);
    disp->rect.x = AdtFnt.tx;
    disp->rect.y = AdtFnt.ty;
    disp->rect.w = 0x40;
    disp->rect.h = 0x100;
    StoreImage(&disp->rect, disp->backup);
    DrawSync(0);
    GetDrawEnv(&disp->draw);
    GetDispEnv(&disp->disp);
    SetDefDrawEnv(&de, 0, 0, 0x140, 0xf0);
    SetDefDispEnv(&di, 0, 0, 0x140, 0xf0);
    PutDrawEnv(&de);
    PutDispEnv(&di);
    FntLoad(AdtFnt.tx, AdtFnt.ty);
    FntOpen(0x20, 0x20, 0x100, 0xb0, 0, 0x200);
    setPolyF4(&disp->bg);
    setXY4(&disp->bg, 0x20, 0x20, 0x120, 0x20,
           0x20, 0xD0, 0x120, 0xD0);
    setRGB0(&disp->bg, 1, 1, 0x64);
}
