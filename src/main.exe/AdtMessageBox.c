#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AdtMessageBox", AdtMessageBox);

// triage: MEDIUM — 156 insns, indirect-call, 12 callees, ~0.04 to DebugMenuItemSet
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void AdtMessageBox(char *fmt,...)
//
// {
//   uint uVar1;
//   undefined4 in_a1;
//   char *pcVar2;
//   undefined4 in_a2;
//   char *pcVar3;
//   undefined4 in_a3;
//   uint uVar4;
//   char *local_res0;
//   undefined4 local_res4;
//   undefined4 local_res8;
//   undefined4 local_resc;
//   char acStack_8298 [504];
//   DRAWENV DStack_80a0;
//   DISPENV DStack_8044;
//   RECT RStack_8030;
//   u_long auStack_8028 [8192];
//   undefined1 auStack_28 [24];
//
//   uVar4 = 0;
//   local_res0 = fmt;
//   if ((code *)AdtReadPadFunc == AdtDmyPadRead) {
//     local_res0 = "*** AdtInit not called ***";
//   }
//   if (AdtFnt_8008f1b8.field8_0x20 == 1) {
//     return;
//   }
//   if (*local_res0 == '%') {
//     if (local_res0[1] == '#') {
//       uVar4 = 2;
//     }
//     else {
//       if (local_res0[1] != '$') goto LAB_8005fa18;
//       uVar4 = 1;
//     }
//     local_res0 = local_res0 + 2;
//   }
// LAB_8005fa18:
//   local_res4 = in_a1;
//   local_res8 = in_a2;
//   local_resc = in_a3;
//   uVar1 = (*(code *)AdtReadPadFunc)(0);
//   if (((uVar1 & 0x100) == 0) || (uVar1 = (*(code *)AdtReadPadFunc)(0), (uVar1 & 0x800) == 0)) {
//     pcVar2 = acStack_8298;
//     pcVar3 = (char *)0x1f4;
//     AdtVsprintf(&local_res4);
//     AdtGetDisp(&DStack_80a0);
//     if (uVar4 < 2) {
//       DrawPrim(auStack_28);
//       pcVar2 = s_adtMessageBoxCount + 1;
//       s_adtMessageBoxCount = pcVar2;
//       FntPrint("AdtMessageBox #%d\n\n",pcVar2,pcVar3,local_res0);
//     }
//     FntPrint(acStack_8298,pcVar2,pcVar3,local_res0);
//     if (uVar4 == 0) {
//       FntPrint("\n\nPress start to continue...",pcVar2,pcVar3,local_res0);
//     }
//     FntFlush(-1);
//     DrawSync(0);
//     VSync(2);
//     if (uVar4 == 0) {
//       while (uVar4 = (*(code *)AdtReadPadFunc)(0), (uVar4 & 0x800) != 0) {
//         VSync(0);
//       }
//       while (uVar4 = (*(code *)AdtReadPadFunc)(0), (uVar4 & 0x800) == 0) {
//         VSync(0);
//       }
//       DrawPrim(auStack_28);
//       DrawSync(0);
//     }
//     FntLoad(AdtFnt_8008f1b8.field6_0x18,AdtFnt_8008f1b8.field7_0x1c);
//     FntOpen(AdtFnt_8008f1b8.field0_0x0,AdtFnt_8008f1b8.field1_0x4,AdtFnt_8008f1b8.field2_0x8,
//             AdtFnt_8008f1b8.field3_0xc,AdtFnt_8008f1b8.field4_0x10,AdtFnt_8008f1b8.field5_0x14);
//     LoadImage(&RStack_8030,auStack_8028);
//     DrawSync(0);
//     PutDrawEnv(&DStack_80a0);
//     PutDispEnv(&DStack_8044);
//   }
//   return;
// }
