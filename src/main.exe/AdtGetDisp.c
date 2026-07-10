#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AdtGetDisp", AdtGetDisp);

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
