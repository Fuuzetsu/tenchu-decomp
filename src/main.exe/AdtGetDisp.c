#include "common.h"
#include "main.exe.h"
#include "adt.h"

/*
 * AdtGetDisp (0x8005f7f4) — save the current screen to the backup buffer and
 * (re)initialise the Adt display/draw environment. Same TAdtDisp combined
 * struct AdtMessageBox.c/AdtReleaseDisp.c use (param declared DRAWENV* by the
 * callers, but it's really &TAdtDisp.draw at offset 0). Grabs the font-adapter
 * texture coords (D_8008F1B8.tx/ty) into the backup RECT, StoreImage's the
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

extern AdtFntState D_8008F1B8;

void AdtGetDisp(TAdtDisp *disp)
{
    DRAWENV de;
    DISPENV di;

    SetDispMask(1);
    DrawSync(0);
    disp->rect.x = D_8008F1B8.tx;
    disp->rect.y = D_8008F1B8.ty;
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
    FntLoad(D_8008F1B8.tx, D_8008F1B8.ty);
    FntOpen(0x20, 0x20, 0x100, 0xb0, 0, 0x200);
    setPolyF4(&disp->bg);
    setXY4(&disp->bg, 0x20, 0x20, 0x120, 0x20,
           0x20, 0xD0, 0x120, 0xD0);
    setRGB0(&disp->bg, 1, 1, 0x64);
}

// triage: EASY — 89 insns, 11 callees, ~0.10 to FUN_80038c0c

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void AdtGetDisp(DRAWENV *param_1)
//
// {
//   undefined2 uVar1;
//   undefined1 uVar2;
//   int iVar3;
//   DRAWENV DStack_90;
//   DISPENV DStack_30;
//
//   SetDispMask(1);
//   DrawSync(0);
//   param_1[1].tpage = (u_short)AdtFnt_8008f1b8.field6_0x18;
//   uVar1 = (undefined2)AdtFnt_8008f1b8.field7_0x1c;
//   param_1[1].isbg = '@';
//   param_1[1].r0 = '\0';
//   param_1[1].g0 = '\0';
//   iVar3 = AdtFnt_8008f1b8.field7_0x1c;
//   param_1[1].b0 = '\x01';
//   AdtFnt_8008f1b8.field7_0x1c._0_1_ = (undefined1)uVar1;
//   AdtFnt_8008f1b8.field7_0x1c._1_1_ = SUB21(uVar1,1);
//   uVar2 = AdtFnt_8008f1b8.field7_0x1c._1_1_;
//   param_1[1].dtd = (undefined1)AdtFnt_8008f1b8.field7_0x1c;
//   AdtFnt_8008f1b8.field7_0x1c = iVar3;
//   param_1[1].dfe = uVar2;
//   StoreImage((RECT *)&param_1[1].tpage,&param_1[1].dr_env.tag);
//   DrawSync(0);
//   GetDrawEnv(param_1);
//   GetDispEnv((DISPENV *)(param_1 + 1));
//   SetDefDrawEnv(&DStack_90,0,0,0x140,0xf0);
//   SetDefDispEnv(&DStack_30,0,0,0x140,0xf0);
//   PutDrawEnv(&DStack_90);
//   PutDispEnv(&DStack_30);
//   FntLoad(AdtFnt_8008f1b8.field6_0x18,AdtFnt_8008f1b8.field7_0x1c);
//   FntOpen(0x20,0x20,0x100,0xb0,0,0x200);
//   *(undefined1 *)((int)param_1[0x165].dr_env.code + 0xf) = 5;
//                     /* Possible PsyQ macro: setPolyF4() */
//   *(undefined1 *)((int)param_1[0x165].dr_env.code + 0x13) = 0x28;
//   *(undefined2 *)(param_1[0x165].dr_env.code + 5) = 0x20;
//   *(undefined2 *)((int)param_1[0x165].dr_env.code + 0x16) = 0x20;
//   *(undefined2 *)((int)param_1[0x165].dr_env.code + 0x1a) = 0x20;
//   *(undefined2 *)(param_1[0x165].dr_env.code + 7) = 0x20;
//   *(undefined2 *)((int)param_1[0x165].dr_env.code + 0x1e) = 0xd0;
//   *(undefined2 *)((int)param_1[0x165].dr_env.code + 0x22) = 0xd0;
//   *(undefined1 *)(param_1[0x165].dr_env.code + 4) = 1;
//   *(undefined1 *)((int)param_1[0x165].dr_env.code + 0x11) = 1;
//   *(undefined2 *)(param_1[0x165].dr_env.code + 6) = 0x120;
//   *(undefined2 *)(param_1[0x165].dr_env.code + 8) = 0x120;
//   *(undefined1 *)((int)param_1[0x165].dr_env.code + 0x12) = 100;
//   return;
// }
