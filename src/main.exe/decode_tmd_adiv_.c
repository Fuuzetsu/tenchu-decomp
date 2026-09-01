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

extern u_long *adiv_tng4_(u_short *primitive, u_long vertop,
                          u_long *packet, u_short count, u_long shift,
                          u_long ot, u_long work);
extern u_long *adiv_tnf4_(u_short *primitive, u_long vertop,
                          u_long *packet, u_short count, u_long shift,
                          u_long ot, u_long work);
extern u_long *GsTMDfastTNF3(u_short *primitive, u_long vertop,
                             u_long *packet, u_short count, u_long shift,
                             u_long ot, u_long work);
extern u_long *GsTMDfastTNG3(u_short *primitive, u_long vertop,
                             u_long *packet, u_short count, u_long shift,
                             u_long ot, u_long work);

void decode_tmd_adiv_(GsDOBJ2 *obj, u_long ot, u_long shift,
                      u_long work)
{
    int step;
    int count;
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

    while (n != 0)
    {
        switch (TMD_BATCH_MODE(prim) & TMD_PRIMITIVE_MODE_MASK)
        {
        case TMD_PRIM_GT4:
            GsOUT_PACKET_P = adiv_tng4_(prim, vertop, GsOUT_PACKET_P,
                                        TMD_BATCH_COUNT(prim), shift, ot,
                                        work);
            n -= TMD_BATCH_COUNT(prim);
            step = TMD_BATCH_COUNT(prim) * TMD_RECORD_WORDS(TMD_P_TNG4);
            step <<= 2;
            break;
        case TMD_PRIM_FT4:
            GsOUT_PACKET_P = adiv_tnf4_(prim, vertop, GsOUT_PACKET_P,
                                        TMD_BATCH_COUNT(prim), shift, ot,
                                        work);
            n -= TMD_BATCH_COUNT(prim);
            step = TMD_BATCH_COUNT(prim) * TMD_RECORD_BYTES(TMD_P_TNF4);
            break;
        case TMD_PRIM_FT3:
            GsOUT_PACKET_P = GsTMDfastTNF3(prim, vertop, GsOUT_PACKET_P,
                                           TMD_BATCH_COUNT(prim), shift, ot,
                                           work);
            /* The named count (here and in case 0x35) replaced two weight
             * fences: it re-orders the local v0/v1 quantities the fences
             * pinned. The other arms need the plain *prim spelling
             * (measured). */
            count = TMD_BATCH_COUNT(prim);
            n -= count;
            step = count * TMD_RECORD_WORDS(TMD_P_TNF3);
            step <<= 2;
            break;
        case TMD_PRIM_GT3:
            GsOUT_PACKET_P = GsTMDfastTNG3(prim, vertop, GsOUT_PACKET_P,
                                           TMD_BATCH_COUNT(prim), shift, ot,
                                           work);
            count = TMD_BATCH_COUNT(prim);
            n -= count;
            step = count * TMD_RECORD_WORDS(TMD_P_TNG3);
            step <<= 2;
            break;
        default:
            return;
        }
        prim = (u_short *)((int)prim + step);
    }
}
