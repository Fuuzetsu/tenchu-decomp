#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80057b80", FUN_80057b80);

// triage: VERY-HARD — 949 insns, 2 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 callees, ~0.05 to ReqItemGoshikimai
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80057b80(int *param_1,int *param_2,int param_3)
//
// {
//   short sVar1;
//   undefined2 uVar2;
//   int iVar3;
//   uint *puVar4;
//   uint *puVar5;
//   int iVar6;
//   short *psVar7;
//   long *r2;
//   int iVar8;
//   undefined4 uVar9;
//   short *psVar10;
//   long *r2_00;
//   undefined4 uVar11;
//   SVECTOR *r2_01;
//   SVECTOR *r2_02;
//   SVECTOR *r1;
//   SVECTOR *r0;
//   SVECTOR *r1_00;
//   undefined1 *puVar12;
//   int *piVar13;
//   undefined1 auStack_40 [16];
//   int *local_30;
//
//   puVar12 = auStack_40;
//   piVar13 = param_1 + 0x22;
//   if (*(int *)(param_1[1] + 0x10) < *(int *)(*param_1 + 0x10)) {
//     param_2[6] = *(int *)(*param_1 + 0x10);
//     iVar3 = param_1[1];
//   }
//   else {
//     param_2[6] = *(int *)(param_1[1] + 0x10);
//     iVar3 = *param_1;
//   }
//   param_2[7] = *(int *)(iVar3 + 0x10);
//   iVar3 = *(int *)(param_1[2] + 0x10);
//   if (iVar3 < param_2[7]) {
//     param_2[7] = iVar3;
//   }
//   else if (param_2[6] < iVar3) {
//     param_2[6] = iVar3;
//   }
//   iVar3 = *(int *)(param_1[3] + 0x10);
//   if (iVar3 < param_2[7]) {
//     param_2[7] = iVar3;
//   }
//   else if (param_2[6] < iVar3) {
//     param_2[6] = iVar3;
//   }
//   iVar3 = param_2[6];
//   if (iVar3 < 0) {
//     iVar3 = iVar3 + 3;
//   }
//   param_2[6] = iVar3 >> 2;
//   if (param_2[8] <= iVar3 >> 2) {
//     if (*(short *)(param_1[1] + 0xc) < *(short *)(*param_1 + 0xc)) {
//       *(undefined2 *)(param_2 + 0xc) = *(undefined2 *)(*param_1 + 0xc);
//       iVar3 = param_1[1];
//     }
//     else {
//       *(undefined2 *)(param_2 + 0xc) = *(undefined2 *)(param_1[1] + 0xc);
//       iVar3 = *param_1;
//     }
//     *(undefined2 *)(param_2 + 0xb) = *(undefined2 *)(iVar3 + 0xc);
//     sVar1 = *(short *)(param_1[2] + 0xc);
//     uVar2 = *(undefined2 *)(param_1[2] + 0xc);
//     if (sVar1 < (short)param_2[0xb]) {
//       *(undefined2 *)(param_2 + 0xb) = uVar2;
//     }
//     else if ((short)param_2[0xc] < sVar1) {
//       *(undefined2 *)(param_2 + 0xc) = uVar2;
//     }
//     sVar1 = *(short *)(param_1[3] + 0xc);
//     uVar2 = *(undefined2 *)(param_1[3] + 0xc);
//     if (sVar1 < (short)param_2[0xb]) {
//       *(undefined2 *)(param_2 + 0xb) = uVar2;
//     }
//     else if ((short)param_2[0xc] < sVar1) {
//       *(undefined2 *)(param_2 + 0xc) = uVar2;
//     }
//     if ((-(int)(short)param_2[0xd] <= (int)(short)param_2[0xc]) &&
//        ((int)(short)param_2[0xb] <= (int)(short)param_2[0xd])) {
//       if (*(short *)(param_1[1] + 0xe) < *(short *)(*param_1 + 0xe)) {
//         *(undefined2 *)((int)param_2 + 0x32) = *(undefined2 *)(*param_1 + 0xe);
//         iVar3 = param_1[1];
//       }
//       else {
//         *(undefined2 *)((int)param_2 + 0x32) = *(undefined2 *)(param_1[1] + 0xe);
//         iVar3 = *param_1;
//       }
//       *(undefined2 *)((int)param_2 + 0x2e) = *(undefined2 *)(iVar3 + 0xe);
//       sVar1 = *(short *)(param_1[2] + 0xe);
//       uVar2 = *(undefined2 *)(param_1[2] + 0xe);
//       if (sVar1 < *(short *)((int)param_2 + 0x2e)) {
//         *(undefined2 *)((int)param_2 + 0x2e) = uVar2;
//       }
//       else if (*(short *)((int)param_2 + 0x32) < sVar1) {
//         *(undefined2 *)((int)param_2 + 0x32) = uVar2;
//       }
//       sVar1 = *(short *)(param_1[3] + 0xe);
//       uVar2 = *(undefined2 *)(param_1[3] + 0xe);
//       if (sVar1 < *(short *)((int)param_2 + 0x2e)) {
//         *(undefined2 *)((int)param_2 + 0x2e) = uVar2;
//       }
//       else if (*(short *)((int)param_2 + 0x32) < sVar1) {
//         *(undefined2 *)((int)param_2 + 0x32) = uVar2;
//       }
//       if ((-(int)(short)param_2[0xd] <= (int)*(short *)((int)param_2 + 0x32)) &&
//          ((int)*(short *)((int)param_2 + 0x2e) <= (int)(short)param_2[0xd])) {
//         if ((*param_2 == param_3) ||
//            (((int)(short)param_2[0xc] - (int)(short)param_2[0xb] < 0xff &&
//             ((int)*(short *)((int)param_2 + 0x32) - (int)*(short *)((int)param_2 + 0x2e) < 0x7f))))
//         {
//           iVar3 = param_2[5];
//           *(undefined4 *)(iVar3 + 8) = *(undefined4 *)(*param_1 + 0xc);
//           *(undefined4 *)(iVar3 + 0x14) = *(undefined4 *)(param_1[1] + 0xc);
//           *(undefined4 *)(iVar3 + 0x20) = *(undefined4 *)(param_1[2] + 0xc);
//           *(undefined4 *)(iVar3 + 0x2c) = *(undefined4 *)(param_1[3] + 0xc);
//           *(undefined4 *)(iVar3 + 0xc) = *(undefined4 *)(*param_1 + 0x14);
//           *(undefined4 *)(iVar3 + 0x18) = *(undefined4 *)(param_1[1] + 0x14);
//           *(undefined4 *)(iVar3 + 0x24) = *(undefined4 *)(param_1[2] + 0x14);
//           *(undefined4 *)(iVar3 + 0x30) = *(undefined4 *)(param_1[3] + 0x14);
//           *(undefined4 *)(iVar3 + 4) = *(undefined4 *)(*param_1 + 8);
//           *(undefined4 *)(iVar3 + 0x10) = *(undefined4 *)(param_1[1] + 8);
//           *(undefined4 *)(iVar3 + 0x1c) = *(undefined4 *)(param_1[2] + 8);
//           *(undefined4 *)(iVar3 + 0x28) = *(undefined4 *)(param_1[3] + 8);
//           *(undefined2 *)(iVar3 + 0xe) = *(undefined2 *)((int)param_2 + 0x5a);
//           *(undefined2 *)(iVar3 + 0x1a) = *(undefined2 *)((int)param_2 + 0x66);
//           *(int *)param_2[5] = param_2[0x13];
//           puVar4 = (uint *)(param_2[4] + (param_2[6] >> (param_2[3] & 0x1fU)) * 4);
//           param_2[0xe] = (int)puVar4;
//           *(uint *)param_2[5] = *puVar4 & 0xffffff | 0xc000000;
//           *(uint *)param_2[0xe] = param_2[5] & 0xffffff;
//           iVar3 = param_2[5] + 0x34;
//         }
//         else {
//           psVar7 = (short *)*param_1;
//           psVar10 = (short *)param_1[1];
//           *(short *)(param_1 + 4) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
//           r0 = (SVECTOR *)(param_1 + 4);
//           *(short *)((int)param_1 + 0x12) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
//           *(short *)(param_1 + 5) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
//           *(char *)(param_1 + 6) =
//                (char)((int)((uint)*(byte *)(psVar7 + 4) + (uint)*(byte *)(psVar10 + 4)) >> 1);
//           *(char *)((int)param_1 + 0x19) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 9) + (uint)*(byte *)((int)psVar10 + 9)) >>
//                      1);
//           *(char *)((int)param_1 + 0x1a) =
//                (char)((int)((uint)*(byte *)(psVar7 + 5) + (uint)*(byte *)(psVar10 + 5)) >> 1);
//           *(undefined1 *)((int)param_1 + 0x1b) = *(undefined1 *)((int)psVar7 + 0xb);
//           *(char *)(param_1 + 9) =
//                (char)((int)((uint)*(byte *)(psVar7 + 10) + (uint)*(byte *)(psVar10 + 10)) >> 1);
//           *(char *)((int)param_1 + 0x25) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 0x15) +
//                            (uint)*(byte *)((int)psVar10 + 0x15)) >> 1);
//           psVar7 = (short *)*param_1;
//           psVar10 = (short *)param_1[2];
//           *(short *)(param_1 + 10) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
//           r1_00 = (SVECTOR *)(param_1 + 10);
//           *(short *)((int)param_1 + 0x2a) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
//           *(short *)(param_1 + 0xb) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
//           *(char *)(param_1 + 0xc) =
//                (char)((int)((uint)*(byte *)(psVar7 + 4) + (uint)*(byte *)(psVar10 + 4)) >> 1);
//           *(char *)((int)param_1 + 0x31) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 9) + (uint)*(byte *)((int)psVar10 + 9)) >>
//                      1);
//           *(char *)((int)param_1 + 0x32) =
//                (char)((int)((uint)*(byte *)(psVar7 + 5) + (uint)*(byte *)(psVar10 + 5)) >> 1);
//           *(undefined1 *)((int)param_1 + 0x33) = *(undefined1 *)((int)psVar7 + 0xb);
//           *(char *)(param_1 + 0xf) =
//                (char)((int)((uint)*(byte *)(psVar7 + 10) + (uint)*(byte *)(psVar10 + 10)) >> 1);
//           *(char *)((int)param_1 + 0x3d) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 0x15) +
//                            (uint)*(byte *)((int)psVar10 + 0x15)) >> 1);
//           psVar7 = (short *)param_1[2];
//           psVar10 = (short *)param_1[3];
//           *(short *)(param_1 + 0x10) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
//           r2_01 = (SVECTOR *)(param_1 + 0x10);
//           *(short *)((int)param_1 + 0x42) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
//           *(short *)(param_1 + 0x11) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
//           *(char *)(param_1 + 0x12) =
//                (char)((int)((uint)*(byte *)(psVar7 + 4) + (uint)*(byte *)(psVar10 + 4)) >> 1);
//           *(char *)((int)param_1 + 0x49) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 9) + (uint)*(byte *)((int)psVar10 + 9)) >>
//                      1);
//           *(char *)((int)param_1 + 0x4a) =
//                (char)((int)((uint)*(byte *)(psVar7 + 5) + (uint)*(byte *)(psVar10 + 5)) >> 1);
//           *(undefined1 *)((int)param_1 + 0x4b) = *(undefined1 *)((int)psVar7 + 0xb);
//           *(char *)(param_1 + 0x15) =
//                (char)((int)((uint)*(byte *)(psVar7 + 10) + (uint)*(byte *)(psVar10 + 10)) >> 1);
//           *(char *)((int)param_1 + 0x55) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 0x15) +
//                            (uint)*(byte *)((int)psVar10 + 0x15)) >> 1);
//           local_30 = piVar13;
//           gte_ldv3(r0,r1_00,r2_01);
//           gte_rtpt();
//           psVar7 = (short *)param_1[3];
//           psVar10 = (short *)param_1[1];
//           *(short *)(param_1 + 0x16) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
//           r1 = (SVECTOR *)(param_1 + 0x16);
//           *(short *)((int)param_1 + 0x5a) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
//           *(short *)(param_1 + 0x17) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
//           *(char *)(param_1 + 0x18) =
//                (char)((int)((uint)*(byte *)(psVar7 + 4) + (uint)*(byte *)(psVar10 + 4)) >> 1);
//           *(char *)((int)param_1 + 0x61) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 9) + (uint)*(byte *)((int)psVar10 + 9)) >>
//                      1);
//           *(char *)((int)param_1 + 0x62) =
//                (char)((int)((uint)*(byte *)(psVar7 + 5) + (uint)*(byte *)(psVar10 + 5)) >> 1);
//           *(undefined1 *)((int)param_1 + 99) = *(undefined1 *)((int)psVar7 + 0xb);
//           *(char *)(param_1 + 0x1b) =
//                (char)((int)((uint)*(byte *)(psVar7 + 10) + (uint)*(byte *)(psVar10 + 10)) >> 1);
//           *(char *)((int)param_1 + 0x6d) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 0x15) +
//                            (uint)*(byte *)((int)psVar10 + 0x15)) >> 1);
//           psVar7 = (short *)*param_1;
//           psVar10 = (short *)param_1[3];
//           *(short *)(param_1 + 0x1c) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
//           r2_02 = (SVECTOR *)(param_1 + 0x1c);
//           *(short *)((int)param_1 + 0x72) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
//           *(short *)(param_1 + 0x1d) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
//           *(char *)(param_1 + 0x1e) =
//                (char)((int)((uint)*(byte *)(psVar7 + 4) + (uint)*(byte *)(psVar10 + 4)) >> 1);
//           *(char *)((int)param_1 + 0x79) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 9) + (uint)*(byte *)((int)psVar10 + 9)) >>
//                      1);
//           *(char *)((int)param_1 + 0x7a) =
//                (char)((int)((uint)*(byte *)(psVar7 + 5) + (uint)*(byte *)(psVar10 + 5)) >> 1);
//           *(undefined1 *)((int)param_1 + 0x7b) = *(undefined1 *)((int)psVar7 + 0xb);
//           *(char *)(param_1 + 0x21) =
//                (char)((int)((uint)*(byte *)(psVar7 + 10) + (uint)*(byte *)(psVar10 + 10)) >> 1);
//           r2_00 = param_1 + 0x13;
//           *(char *)((int)param_1 + 0x85) =
//                (char)((int)((uint)*(byte *)((int)psVar7 + 0x15) +
//                            (uint)*(byte *)((int)psVar10 + 0x15)) >> 1);
//           gte_stsxy3(param_1 + 7,param_1 + 0xd,r2_00);
//           r2 = param_1 + 0x14;
//           gte_stsz3(param_1 + 8,param_1 + 0xe,r2);
//           gte_ldv3(r2_01,r1,r2_02);
//           gte_rtpt();
//           iVar3 = *param_1;
//           piVar13[1] = (int)r0;
//           piVar13[2] = (int)r1_00;
//           piVar13[3] = (int)r2_02;
//           *piVar13 = iVar3;
//           gte_stsxy3(r2_00,param_1 + 0x19,param_1 + 0x1f);
//           gte_stsz3(r2,param_1 + 0x1a,param_1 + 0x20);
//           iVar3 = *(int *)(puVar12 + 0x48);
//           *(int *)(puVar12 + 0x48) = iVar3 + 1;
//           FUN_80057b80(*(undefined4 *)(puVar12 + 0x10),param_2,iVar3 + 1);
//           iVar6 = *param_1;
//           puVar4 = (uint *)param_2[5];
//           iVar8 = param_1[1];
//           puVar4[2] = *(uint *)(iVar6 + 0xc);
//           puVar4[5] = *(uint *)(iVar8 + 0xc);
//           puVar4[8] = *(uint *)&r0[1].vz;
//           iVar3 = *(int *)(iVar6 + 0x10);
//           if (iVar3 < 0) {
//             iVar3 = iVar3 + 3;
//           }
//           param_2[6] = iVar3 >> 2;
//           puVar4[3] = (uint)*(ushort *)(iVar6 + 0x14);
//           puVar4[6] = (uint)*(ushort *)(iVar8 + 0x14);
//           puVar4[9] = (uint)(ushort)r0[2].vz;
//           puVar4[1] = *(uint *)(iVar6 + 8);
//           puVar4[4] = *(uint *)(iVar8 + 8);
//           puVar4[7] = *(uint *)(r0 + 1);
//           *(undefined2 *)((int)puVar4 + 0xe) = *(undefined2 *)((int)param_2 + 0x5a);
//           uVar2 = *(undefined2 *)((int)param_2 + 0x66);
//           *(undefined1 *)((int)puVar4 + 3) = 9;
//                     /* Probable PsyQ macro: setPolyGT3() */
//           *(undefined1 *)((int)puVar4 + 7) = 0x34;
//           *(undefined2 *)((int)puVar4 + 0x1a) = uVar2;
//           puVar5 = (uint *)(param_2[4] + (param_2[6] >> (param_2[3] & 0x1fU)) * 4);
//           param_2[0xe] = (int)puVar5;
//           uVar9 = *(undefined4 *)(puVar12 + 0x10);
//           *puVar4 = *puVar5 & 0xffffff | 0x9000000;
//           *(uint *)param_2[0xe] = (uint)puVar4 & 0xffffff;
//           param_2[5] = param_2[5] + 0x28;
//           *piVar13 = (int)r0;
//           iVar3 = param_1[1];
//           uVar11 = *(undefined4 *)(puVar12 + 0x48);
//           piVar13[2] = (int)r2_02;
//           piVar13[1] = iVar3;
//           piVar13[3] = (int)r1;
//           FUN_80057b80(uVar9,param_2,uVar11);
//           iVar6 = param_1[2];
//           puVar4 = (uint *)param_2[5];
//           iVar8 = *param_1;
//           puVar4[2] = *(uint *)(iVar6 + 0xc);
//           puVar4[5] = *(uint *)(iVar8 + 0xc);
//           puVar4[8] = *(uint *)&r1_00[1].vz;
//           iVar3 = *(int *)(iVar6 + 0x10);
//           if (iVar3 < 0) {
//             iVar3 = iVar3 + 3;
//           }
//           param_2[6] = iVar3 >> 2;
//           puVar4[3] = (uint)*(ushort *)(iVar6 + 0x14);
//           puVar4[6] = (uint)*(ushort *)(iVar8 + 0x14);
//           puVar4[9] = (uint)(ushort)r1_00[2].vz;
//           puVar4[1] = *(uint *)(iVar6 + 8);
//           puVar4[4] = *(uint *)(iVar8 + 8);
//           puVar4[7] = *(uint *)(r1_00 + 1);
//           *(undefined2 *)((int)puVar4 + 0xe) = *(undefined2 *)((int)param_2 + 0x5a);
//           uVar2 = *(undefined2 *)((int)param_2 + 0x66);
//           *(undefined1 *)((int)puVar4 + 3) = 9;
//                     /* Probable PsyQ macro: setPolyGT3() */
//           *(undefined1 *)((int)puVar4 + 7) = 0x34;
//           *(undefined2 *)((int)puVar4 + 0x1a) = uVar2;
//           puVar5 = (uint *)(param_2[4] + (param_2[6] >> (param_2[3] & 0x1fU)) * 4);
//           param_2[0xe] = (int)puVar5;
//           uVar9 = *(undefined4 *)(puVar12 + 0x10);
//           *puVar4 = *puVar5 & 0xffffff | 0x9000000;
//           *(uint *)param_2[0xe] = (uint)puVar4 & 0xffffff;
//           param_2[5] = param_2[5] + 0x28;
//           *piVar13 = (int)r1_00;
//           piVar13[1] = (int)r2_02;
//           uVar11 = *(undefined4 *)(puVar12 + 0x48);
//           piVar13[2] = param_1[2];
//           piVar13[3] = (int)r2_01;
//           FUN_80057b80(uVar9,param_2,uVar11);
//           iVar6 = param_1[3];
//           puVar4 = (uint *)param_2[5];
//           iVar8 = param_1[2];
//           puVar4[2] = *(uint *)(iVar6 + 0xc);
//           puVar4[5] = *(uint *)(iVar8 + 0xc);
//           puVar4[8] = *(uint *)&r2_01[1].vz;
//           iVar3 = *(int *)(iVar6 + 0x10);
//           if (iVar3 < 0) {
//             iVar3 = iVar3 + 3;
//           }
//           param_2[6] = iVar3 >> 2;
//           puVar4[3] = (uint)*(ushort *)(iVar6 + 0x14);
//           puVar4[6] = (uint)*(ushort *)(iVar8 + 0x14);
//           puVar4[9] = (uint)(ushort)r2_01[2].vz;
//           puVar4[1] = *(uint *)(iVar6 + 8);
//           puVar4[4] = *(uint *)(iVar8 + 8);
//           puVar4[7] = *(uint *)(r2_01 + 1);
//           *(undefined2 *)((int)puVar4 + 0xe) = *(undefined2 *)((int)param_2 + 0x5a);
//           uVar2 = *(undefined2 *)((int)param_2 + 0x66);
//           *(undefined1 *)((int)puVar4 + 3) = 9;
//                     /* Probable PsyQ macro: setPolyGT3() */
//           *(undefined1 *)((int)puVar4 + 7) = 0x34;
//           *(undefined2 *)((int)puVar4 + 0x1a) = uVar2;
//           puVar5 = (uint *)(param_2[4] + (param_2[6] >> (param_2[3] & 0x1fU)) * 4);
//           param_2[0xe] = (int)puVar5;
//           uVar9 = *(undefined4 *)(puVar12 + 0x10);
//           *puVar4 = *puVar5 & 0xffffff | 0x9000000;
//           *(uint *)param_2[0xe] = (uint)puVar4 & 0xffffff;
//           param_2[5] = param_2[5] + 0x28;
//           *piVar13 = (int)r2_02;
//           piVar13[1] = (int)r1;
//           piVar13[2] = (int)r2_01;
//           uVar11 = *(undefined4 *)(puVar12 + 0x48);
//           piVar13[3] = param_1[3];
//           FUN_80057b80(uVar9,param_2,uVar11);
//           iVar8 = param_1[1];
//           puVar4 = (uint *)param_2[5];
//           iVar6 = param_1[3];
//           puVar4[2] = *(uint *)(iVar8 + 0xc);
//           puVar4[5] = *(uint *)(iVar6 + 0xc);
//           puVar4[8] = *(uint *)&r1[1].vz;
//           iVar3 = *(int *)(iVar8 + 0x10);
//           if (iVar3 < 0) {
//             iVar3 = iVar3 + 3;
//           }
//           param_2[6] = iVar3 >> 2;
//           puVar4[3] = (uint)*(ushort *)(iVar8 + 0x14);
//           puVar4[6] = (uint)*(ushort *)(iVar6 + 0x14);
//           puVar4[9] = (uint)(ushort)r1[2].vz;
//           puVar4[1] = *(uint *)(iVar8 + 8);
//           puVar4[4] = *(uint *)(iVar6 + 8);
//           puVar4[7] = *(uint *)(r1 + 1);
//           *(undefined2 *)((int)puVar4 + 0xe) = *(undefined2 *)((int)param_2 + 0x5a);
//           uVar2 = *(undefined2 *)((int)param_2 + 0x66);
//           *(undefined1 *)((int)puVar4 + 3) = 9;
//                     /* Probable PsyQ macro: setPolyGT3() */
//           *(undefined1 *)((int)puVar4 + 7) = 0x34;
//           *(undefined2 *)((int)puVar4 + 0x1a) = uVar2;
//           puVar5 = (uint *)(param_2[4] + (param_2[6] >> (param_2[3] & 0x1fU)) * 4);
//           param_2[0xe] = (int)puVar5;
//           *puVar4 = *puVar5 & 0xffffff | 0x9000000;
//           *(uint *)param_2[0xe] = (uint)puVar4 & 0xffffff;
//           iVar3 = param_2[5] + 0x28;
//         }
//         param_2[5] = iVar3;
//       }
//     }
//   }
//   return;
// }
