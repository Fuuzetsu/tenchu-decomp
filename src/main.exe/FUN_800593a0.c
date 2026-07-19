#include "common.h"
#include "main.exe.h"

/*
 * Decode the linked TMD primitive stream and hand each supported packet type
 * to its specialized renderer.
 *
 * Matching notes (636 bytes / 159 instructions):
 *  - The real linked-TMD field layout is load-bearing for the prologue's load
 *    schedule.
 *  - The volatile attribute read preserves the retail reload across the two
 *    packet-parameter stores.
 *  - Direct per-case cursor updates retain the two distinct x7 switch tails.
 *  - The 29-entry switch table is routed through this object's .rodata carve
 *    at 0x80013C20.
 */

extern int D_800C6588;

extern u_long *FUN_8005961c(u_short *param_1, u_long param_2, u_long *param_3,
                            u_short param_4, u_long *param_5);
extern u_long *FUN_80059b08(u_short *param_1, u_long param_2, u_long *param_3,
                            u_short param_4, u_long *param_5);
extern u_long *FUN_80059ff4(u_short *param_1, u_long param_2, u_long *param_3,
                            u_short param_4, u_long *param_5);
extern u_long *FUN_8005a3cc(u_short *param_1, u_long param_2, u_long *param_3,
                            u_short param_4, u_long *param_5);

void FUN_800593a0(GsDOBJ2 *param_1, u_long param_2, u_long param_3, int param_4)
{
    u_long uVar1;
    struct TMD_STRUCT *puVar3;
    u_short *puVar4;
    int iVar5;
    u_long uVar6;

    puVar3 = (struct TMD_STRUCT *)param_1->tmd;
    GsLMODE = param_1->attribute >> 3 & 3;
    puVar4 = (u_short *)puVar3->primtop;
    iVar5 = puVar3->primn;
    GsLIGNR = param_1->attribute >> 5 & 1;
    uVar6 = (u_long)puVar3->vertop;
    GsLIOFF = param_1->attribute >> 6 & 1;
    uVar1 = *(volatile u_long *)&param_1->attribute;
    D_800C6588 = param_1->attribute >> 9 & 7;
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
            puVar4 = (u_short *)((int)puVar4 + *puVar4 * 0x2c);
            continue;
        case 0x2d:
            GsOUT_PACKET_P = FUN_80059b08(puVar4, uVar6, GsOUT_PACKET_P, *puVar4, param_4);
            iVar5 -= *puVar4;
            puVar4 = (u_short *)((int)puVar4 + (*puVar4 << 5));
            continue;
        case 0x25:
            GsOUT_PACKET_P = FUN_80059ff4(puVar4, uVar6, GsOUT_PACKET_P, *puVar4, param_4);
            iVar5 -= *puVar4;
            puVar4 = (u_short *)((int)puVar4 + *puVar4 * 0x1c);
            continue;
        case 0x35:
            GsOUT_PACKET_P = FUN_8005a3cc(puVar4, uVar6, GsOUT_PACKET_P, *puVar4, param_4);
            iVar5 -= *puVar4;
            puVar4 = (u_short *)((int)puVar4 + *puVar4 * 0x24);
            continue;
        case 0x39:
            iVar5 -= *puVar4;
            puVar4 = (u_short *)((int)puVar4 + *puVar4 * 0x1c);
            continue;
        case 0x31:
            iVar5 -= *puVar4;
            puVar4 = (u_short *)((int)puVar4 + *puVar4 * 0x18);
            continue;
        case 0x21:
        case 0x29:
            iVar5 -= *puVar4;
            puVar4 = (u_short *)((int)puVar4 + (*puVar4 << 4));
            continue;
        default:
            return;
        }
    }
}
