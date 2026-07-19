#include "common.h"
#include "main.exe.h"

/*
 * Decode the linked TMD primitive stream and hand each supported triangle
 * packet type to its specialized renderer.
 *
 * Matching notes (540 bytes / 135 instructions):
 *  - Giving the linked TMD object its real field layout is load-bearing for
 *    the prologue's load scheduling.
 *  - The one-shot loops around the x7 and x9 stride expressions emit no
 *    control flow. Their loop notes make local-alloc choose the retail
 *    $v0/$v1 coloring for those two switch arms.
 */

extern u_long D_800C6588;

extern u_long *FUN_80058c70(u_short *primitive, u_long vertices,
                            u_long *packet, u_short count, u_long arg2,
                            u_long arg1, u_long arg3);
extern u_long *FUN_80059008(u_short *primitive, u_long vertices,
                            u_long *packet, u_short count, u_long arg2,
                            u_long arg1, u_long arg3);
extern u_long *GsTMDfastTNF3(u_short *primitive, u_long vertices,
                             u_long *packet, u_short count, u_long arg2,
                             u_long arg1, u_long arg3);
extern u_long *GsTMDfastTNG3(u_short *primitive, u_long vertices,
                             u_long *packet, u_short count, u_long arg2,
                             u_long arg1, u_long arg3);

void FUN_80058a54(GsDOBJ2 *param_1, u_long param_2, u_long param_3,
                  u_long param_4)
{
    int iVar1;
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
    D_800C6588 = param_1->attribute >> 9 & 7;
    GsTON = param_1->attribute >> 0x1e & 1;

    while (iVar5 != 0) {
        switch (*(u_char *)((int)puVar4 + 3) & 0xfd) {
        case 0x3d:
            GsOUT_PACKET_P = FUN_80058c70(puVar4, uVar6, GsOUT_PACKET_P,
                                           *puVar4, param_3, param_2,
                                           param_4);
            iVar5 -= *puVar4;
            iVar1 = *puVar4 * 0xb;
            iVar1 <<= 2;
            break;
        case 0x2d:
            GsOUT_PACKET_P = FUN_80059008(puVar4, uVar6, GsOUT_PACKET_P,
                                           *puVar4, param_3, param_2,
                                           param_4);
            iVar5 -= *puVar4;
            iVar1 = *puVar4 << 5;
            break;
        case 0x25:
            GsOUT_PACKET_P = GsTMDfastTNF3(puVar4, uVar6, GsOUT_PACKET_P,
                                            *puVar4, param_3, param_2,
                                            param_4);
            iVar5 -= *puVar4;
            do {
                iVar1 = *puVar4 * 7;
            } while (0);
            iVar1 <<= 2;
            break;
        case 0x35:
            GsOUT_PACKET_P = GsTMDfastTNG3(puVar4, uVar6, GsOUT_PACKET_P,
                                            *puVar4, param_3, param_2,
                                            param_4);
            iVar5 -= *puVar4;
            do {
                iVar1 = *puVar4 * 9;
            } while (0);
            iVar1 <<= 2;
            break;
        default:
            return;
        }
        puVar4 = (u_short *)((int)puVar4 + iVar1);
    }
}
