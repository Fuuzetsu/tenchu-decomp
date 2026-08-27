#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

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

extern u_long DivDepth; /* GsDOBJ2 attribute bits 9..11 */

extern u_long *FUN_80058c70(u_short *primitive, u_long vertop,
                            u_long *packet, u_short count, u_long shift,
                            u_long ot, u_long work);
extern u_long *FUN_80059008(u_short *primitive, u_long vertop,
                            u_long *packet, u_short count, u_long shift,
                            u_long ot, u_long work);
extern u_long *GsTMDfastTNF3(u_short *primitive, u_long vertop,
                             u_long *packet, u_short count, u_long shift,
                             u_long ot, u_long work);
extern u_long *GsTMDfastTNG3(u_short *primitive, u_long vertop,
                             u_long *packet, u_short count, u_long shift,
                             u_long ot, u_long work);

void FUN_80058a54(GsDOBJ2 *obj, u_long ot, u_long shift,
                  u_long work)
{
    int step;
    struct TMD_STRUCT *tmd;
    u_short *prim;
    int n;
    u_long vertop;

    tmd = (struct TMD_STRUCT *)obj->tmd;
    GsLMODE = obj->attribute >> 3 & 3;
    prim = (u_short *)tmd->primtop;
    n = tmd->primn;
    GsLIGNR = obj->attribute >> 5 & 1;
    vertop = (u_long)tmd->vertop;
    GsLIOFF = obj->attribute >> 6 & 1;
    DivDepth = obj->attribute >> 9 & 7;
    GsTON = obj->attribute >> 0x1e & 1;

    while (n != 0) {
        switch (*(u_char *)((int)prim + 3) & 0xfd) {
        case 0x3d:
            GsOUT_PACKET_P = FUN_80058c70(prim, vertop, GsOUT_PACKET_P,
                                           *prim, shift, ot,
                                           work);
            n -= *prim;
            step = *prim * 0xb;
            step <<= 2;
            break;
        case 0x2d:
            GsOUT_PACKET_P = FUN_80059008(prim, vertop, GsOUT_PACKET_P,
                                           *prim, shift, ot,
                                           work);
            n -= *prim;
            step = *prim << 5;
            break;
        case 0x25:
            GsOUT_PACKET_P = GsTMDfastTNF3(prim, vertop, GsOUT_PACKET_P,
                                            *prim, shift, ot,
                                            work);
            n -= *prim;
            do {
                step = *prim * 7;
            } while (0);
            step <<= 2;
            break;
        case 0x35:
            GsOUT_PACKET_P = GsTMDfastTNG3(prim, vertop, GsOUT_PACKET_P,
                                            *prim, shift, ot,
                                            work);
            n -= *prim;
            do {
                step = *prim * 9;
            } while (0);
            step <<= 2;
            break;
        default:
            return;
        }
        prim = (u_short *)((int)prim + step);
    }
}
