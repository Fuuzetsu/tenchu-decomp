#include "common.h"
#include "main.exe.h"

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_800593a0", FUN_800593a0);
#else
extern int GsLMODE;
extern int GsLIGNR;
extern int GsLIOFF;
extern int GsTON;
extern int D_800C6588;
extern u_long *GsOUT_PACKET_P;

extern u_long *FUN_8005961c(u_short *param_1, u_long param_2, u_long *param_3,
                            u_short param_4, u_long *param_5);
extern u_long *FUN_80059b08(u_short *param_1, u_long param_2, u_long *param_3,
                            u_short param_4, u_long *param_5);
extern u_long *FUN_80059ff4(u_short *param_1, u_long param_2, u_long *param_3,
                            u_short param_4, u_long *param_5);
extern u_long *FUN_8005a3cc(u_short *param_1, u_long param_2, u_long *param_3,
                            u_short param_4, u_long *param_5);

void FUN_800593a0(u_long *param_1, u_long param_2, u_long param_3, int param_4) {
    u_long uVar1;
    int iVar2;
    u_long *puVar3;
    u_short *puVar4;
    int iVar5;
    u_long uVar6;

    puVar3 = (u_long *)param_1[2];
    GsLMODE = *param_1 >> 3 & 3;
    puVar4 = (u_short *)puVar3[4];
    iVar5 = puVar3[5];
    GsLIGNR = *param_1 >> 5 & 1;
    uVar6 = *puVar3;
    GsLIOFF = *param_1 >> 6 & 1;
    uVar1 = *param_1;
    D_800C6588 = *param_1 >> 9 & 7;
    *(u_long *)(param_4 + 0x88) = param_3;
    *(u_long *)(param_4 + 0x90) = param_2;
    GsTON = uVar1 >> 0x1e & 1;
    *(u_long *)(param_4 + 0x94) = -0xa0;
    *(u_long *)(param_4 + 0x98) = 0xa0;
    *(u_long *)(param_4 + 0x9c) = -0x78;
    *(u_long *)(param_4 + 0xa0) = 0x78;
    *(u_long *)(param_4 + 0x84) = 0x4a98;
    *(u_long *)(param_4 + 0x8c) = 15000;
    while (iVar5 != 0) {
        switch (*(u_char *)((int)puVar4 + 3) & 0xfd) {
        case 0x3d:
            GsOUT_PACKET_P = FUN_8005961c(puVar4, uVar6, GsOUT_PACKET_P, *puVar4, param_4);
            iVar5 -= *puVar4;
            iVar2 = *puVar4 * 0xb;
            iVar2 <<= 2;
            break;
        case 0x2d:
            GsOUT_PACKET_P = FUN_80059b08(puVar4, uVar6, GsOUT_PACKET_P, *puVar4, param_4);
            iVar5 -= *puVar4;
            iVar2 = *puVar4 << 5;
            break;
        case 0x25:
            GsOUT_PACKET_P = FUN_80059ff4(puVar4, uVar6, GsOUT_PACKET_P, *puVar4, param_4);
        case 0x39:
            iVar5 -= *puVar4;
            iVar2 = *puVar4 * 7;
            iVar2 <<= 2;
            break;
        case 0x35:
            GsOUT_PACKET_P = FUN_8005a3cc(puVar4, uVar6, GsOUT_PACKET_P, *puVar4, param_4);
            iVar5 -= *puVar4;
            iVar2 = *puVar4 * 9;
            iVar2 <<= 2;
            break;
        case 0x31:
            iVar5 -= *puVar4;
            iVar2 = *puVar4 * 0x18;
            break;
        case 0x21:
        case 0x29:
            iVar5 -= *puVar4;
            iVar2 = *puVar4 << 4;
            break;
        default:
            return;
        }
        puVar4 = (u_short *)((int)puVar4 + iVar2);
    }
}
#endif

// triage: EASY — 75 insns, 1 callees, ~0.07 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_800593a0(uint *param_1,undefined4 param_2,undefined4 param_3,int param_4)
//
// {
//   uint uVar1;
//   int iVar2;
//   undefined4 *puVar3;
//   ushort *puVar4;
//   int iVar5;
//   undefined4 uVar6;
//
//   puVar3 = (undefined4 *)param_1[2];
//   DAT_800c6568 = *param_1 >> 3 & 3;
//   puVar4 = (ushort *)puVar3[4];
//   iVar5 = puVar3[5];
//   DAT_800c656c = *param_1 >> 5 & 1;
//   uVar6 = *puVar3;
//   DAT_800c6564 = *param_1 >> 6 & 1;
//   uVar1 = *param_1;
//   DAT_800c6588 = *param_1 >> 9 & 7;
//   *(undefined4 *)(param_4 + 0x88) = param_3;
//   *(undefined4 *)(param_4 + 0x90) = param_2;
//   DAT_800c6584 = uVar1 >> 0x1e & 1;
//   *(undefined4 *)(param_4 + 0x94) = 0xffffff60;
//   *(undefined4 *)(param_4 + 0x98) = 0xa0;
//   *(undefined4 *)(param_4 + 0x9c) = 0xffffff88;
//   *(undefined4 *)(param_4 + 0xa0) = 0x78;
//   *(undefined4 *)(param_4 + 0x84) = 0x4a98;
//   *(undefined4 *)(param_4 + 0x8c) = 15000;
//   while (iVar5 != 0) {
//     switch(*(byte *)((int)puVar4 + 3) & 0xfd) {
//     case 0x21:
//     case 0x29:
//       iVar5 = iVar5 - (uint)*puVar4;
//       iVar2 = (uint)*puVar4 << 4;
//       break;
//     default:
//       goto LAB_800595fc;
//     case 0x25:
//       GsOUT_PACKET_P = FUN_80059ff4(puVar4,uVar6,GsOUT_PACKET_P,*puVar4,param_4);
//       iVar5 = iVar5 - (uint)*puVar4;
//       iVar2 = (uint)*puVar4 * 0x1c;
//       break;
//     case 0x2d:
//       GsOUT_PACKET_P = FUN_80059b08(puVar4,uVar6,GsOUT_PACKET_P,*puVar4,param_4);
//       iVar5 = iVar5 - (uint)*puVar4;
//       iVar2 = (uint)*puVar4 << 5;
//       break;
//     case 0x31:
//       iVar5 = iVar5 - (uint)*puVar4;
//       iVar2 = (uint)*puVar4 * 0x18;
//       break;
//     case 0x35:
//       GsOUT_PACKET_P = FUN_8005a3cc(puVar4,uVar6,GsOUT_PACKET_P,*puVar4,param_4);
//       iVar5 = iVar5 - (uint)*puVar4;
//       iVar2 = (uint)*puVar4 * 0x24;
//       break;
//     case 0x39:
//       iVar5 = iVar5 - (uint)*puVar4;
//       iVar2 = (uint)*puVar4 * 0x1c;
//       break;
//     case 0x3d:
//       GsOUT_PACKET_P = FUN_8005961c(puVar4,uVar6,GsOUT_PACKET_P,*puVar4,param_4);
//       iVar5 = iVar5 - (uint)*puVar4;
//       iVar2 = (uint)*puVar4 * 0x2c;
//     }
//     puVar4 = (ushort *)((int)puVar4 + iVar2);
//   }
// LAB_800595fc:
//   return;
// }
