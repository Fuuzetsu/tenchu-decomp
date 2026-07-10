#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80059008", FUN_80059008);

// triage: VERY-HARD — 230 insns, 3 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 1 callees, ~0.04 to ProcItemTeleport
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// undefined4
// FUN_80059008(int param_1,int param_2,undefined4 param_3,int param_4,undefined4 param_5,int param_6,
//             undefined4 *param_7)
//
// {
//   int iVar1;
//   int iVar2;
//   undefined4 uVar3;
//   SVECTOR *r0;
//   undefined2 *puVar4;
//   undefined4 *puVar5;
//   undefined1 uVar6;
//   SVECTOR *r0_00;
//   SVECTOR *r2;
//   undefined1 *puVar7;
//   SVECTOR *r1;
//   undefined1 auStack_48 [16];
//   undefined4 *local_38;
//
//   iVar1 = DAT_800c6488;
//   puVar7 = auStack_48;
//   *param_7 = 4;
//   iVar2 = DAT_800c648c;
//   puVar5 = param_7 + 0x38;
//   *(short *)(param_7 + 0xd) = (short)(iVar1 / 2);
//   *(short *)((int)param_7 + 0x36) = (short)(iVar2 / 2);
//   uVar3 = *(undefined4 *)(param_6 + 4);
//   param_7[8] = 0x96;
//   *(undefined1 *)((int)param_7 + 0x4f) = 0xc;
//   param_7[3] = param_5;
//   param_7[5] = param_3;
//                     /* Possible PsyQ macro: setPolyGT4() */
//   *(undefined1 *)((int)param_7 + 0x53) = 0x3c;
//   param_7[4] = uVar3;
//   if (param_4 != 0) {
//     r0 = (SVECTOR *)(param_7 + 0x20);
//     r1 = (SVECTOR *)(param_7 + 0x26);
//     r2 = (SVECTOR *)(param_7 + 0x2c);
//     r0_00 = (SVECTOR *)(param_7 + 0x32);
//     uVar3 = 0x3c;
//     puVar4 = (undefined2 *)(param_1 + 10);
//     local_38 = puVar5;
//     do {
//       *(undefined2 *)(param_7 + 0x20) = *(undefined2 *)((uint)(ushort)puVar4[7] * 8 + param_2);
//       *(undefined2 *)((int)param_7 + 0x82) =
//            *(undefined2 *)((uint)(ushort)puVar4[7] * 8 + param_2 + 2);
//       *(undefined2 *)(param_7 + 0x21) = *(undefined2 *)((uint)(ushort)puVar4[7] * 8 + param_2 + 4);
//       *(undefined2 *)(param_7 + 0x26) = *(undefined2 *)((uint)(ushort)puVar4[8] * 8 + param_2);
//       *(undefined2 *)((int)param_7 + 0x9a) =
//            *(undefined2 *)((uint)(ushort)puVar4[8] * 8 + param_2 + 2);
//       *(undefined2 *)(param_7 + 0x27) = *(undefined2 *)((uint)(ushort)puVar4[8] * 8 + param_2 + 4);
//       *(undefined2 *)(param_7 + 0x2c) = *(undefined2 *)((uint)(ushort)puVar4[9] * 8 + param_2);
//       *(undefined2 *)((int)param_7 + 0xb2) =
//            *(undefined2 *)((uint)(ushort)puVar4[9] * 8 + param_2 + 2);
//       *(undefined2 *)(param_7 + 0x2d) = *(undefined2 *)((uint)(ushort)puVar4[9] * 8 + param_2 + 4);
//       *(undefined2 *)(param_7 + 0x32) = *(undefined2 *)((uint)(ushort)puVar4[10] * 8 + param_2);
//       *(undefined2 *)((int)param_7 + 0xca) =
//            *(undefined2 *)((uint)(ushort)puVar4[10] * 8 + param_2 + 2);
//       *(undefined2 *)(param_7 + 0x33) = *(undefined2 *)((uint)(ushort)puVar4[10] * 8 + param_2 + 4);
//       *puVar5 = r0;
//       puVar5[1] = r1;
//       puVar5[2] = r2;
//       puVar5[3] = r0_00;
//       gte_ldv3(r0,r1,r2);
//       gte_rtpt();
//       *(undefined2 *)(param_7 + 0x25) = puVar4[-3];
//       *(undefined2 *)(param_7 + 0x2b) = puVar4[-1];
//       gte_stsxy3(param_7 + 0x23,param_7 + 0x29,param_7 + 0x2f);
//       gte_nclip();
//       *(undefined2 *)(param_7 + 0x31) = puVar4[1];
//       *(undefined2 *)(param_7 + 0x37) = puVar4[3];
//       gte_stopz(param_7 + 6);
//       if (0 < (int)param_7[6]) {
//         gte_ldv0(r0_00);
//         gte_rtps();
//         param_7[0x22] = *(undefined4 *)(puVar4 + 5);
//         uVar6 = (undefined1)uVar3;
//         *(undefined1 *)((int)param_7 + 0x8b) = uVar6;
//         param_7[0x28] = *(undefined4 *)(puVar4 + 5);
//         *(undefined1 *)((int)param_7 + 0xa3) = uVar6;
//         param_7[0x2e] = *(undefined4 *)(puVar4 + 5);
//         *(undefined1 *)((int)param_7 + 0xbb) = uVar6;
//         param_7[0x34] = *(undefined4 *)(puVar4 + 5);
//         *(undefined1 *)((int)param_7 + 0xd3) = uVar6;
//         gte_stsxy(param_7 + 0x35);
//         gte_stsz4(param_7 + 0x24,param_7 + 0x2a,param_7 + 0x30,param_7 + 0x36);
//         *(undefined2 *)((int)param_7 + 0x5a) = puVar4[-2];
//         *(undefined2 *)((int)param_7 + 0x66) = *puVar4;
//         *(SVECTOR **)(puVar7 + 0x18) = r0;
//         FUN_80057b80(*(undefined4 *)(puVar7 + 0x10),param_7,0);
//         r0 = *(SVECTOR **)(puVar7 + 0x18);
//       }
//       param_4 = param_4 + -1;
//       puVar4 = puVar4 + 0x10;
//     } while (param_4 != 0);
//   }
//   return param_7[5];
// }
