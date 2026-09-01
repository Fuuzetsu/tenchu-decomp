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
 *  - The tagged batch cursor selects the TMD record member named by its mode;
 *    the renderer interface otherwise retains Sony's VERT and GsOT types.
 *  - The one-shot loops around the x7 and x9 stride expressions emit no
 *    control flow. Their loop notes make local-alloc choose the retail
 *    $v0/$v1 coloring for those two switch arms.
 */

extern u_long DivDepth;

extern u_long *adiv_tng4_(TMD_P_TNG4 *primitive, VERT *vertices,
                          u_long *packet, u_short count, u_long shift,
                          GsOT *ot, u_long *work);
extern u_long *adiv_tnf4_(TMD_P_TNF4 *primitive, VERT *vertices,
                          u_long *packet, u_short count, u_long shift,
                          GsOT *ot, u_long *work);
extern u_long *GsTMDfastTNF3(TMD_P_TNF3 *primitive, VERT *vertices,
                             u_long *packet, u_short count, u_long shift,
                             GsOT *ot, u_long *work);
extern u_long *GsTMDfastTNG3(TMD_P_TNG3 *primitive, VERT *vertices,
                             u_long *packet, u_short count, u_long shift,
                             GsOT *ot, u_long *work);

void decode_tmd_adiv_(GsDOBJ2 *obj, GsOT *ot, u_long shift,
                      u_long *work)
{
    int step;
    int count;
    struct TMD_STRUCT *tmd;
    TmdPrimitiveRecord *prim;
    int n;
    VERT *vertices;

    tmd = (struct TMD_STRUCT *)obj->tmd;
    GsLMODE = GS_DOBJ_LMODE(obj->attribute);
    prim = (TmdPrimitiveRecord *)tmd->primtop;
    n = tmd->primn;
    GsLIGNR = GS_DOBJ_LIGNR(obj->attribute);
    vertices = (VERT *)tmd->vertop;
    GsLIOFF = GS_DOBJ_LIOFF(obj->attribute);
    DivDepth = GS_DOBJ_DIVISION_DEPTH(obj->attribute);
    GsTON = GS_DOBJ_TON(obj->attribute);

    while (n != 0)
    {
        switch (TMD_BATCH_MODE(prim) & TMD_PRIMITIVE_MODE_MASK)
        {
        case TMD_PRIM_GT4:
            GsOUT_PACKET_P = adiv_tng4_(&prim->gt4, vertices,
                                        GsOUT_PACKET_P,
                                        TMD_BATCH_COUNT(prim), shift, ot,
                                        work);
            n -= TMD_BATCH_COUNT(prim);
            step = TMD_BATCH_COUNT(prim) * TMD_MEMBER_WORDS(prim, gt4);
            step <<= 2;
            break;
        case TMD_PRIM_FT4:
            GsOUT_PACKET_P = adiv_tnf4_(&prim->ft4, vertices,
                                        GsOUT_PACKET_P,
                                        TMD_BATCH_COUNT(prim), shift, ot,
                                        work);
            n -= TMD_BATCH_COUNT(prim);
            step = TMD_BATCH_COUNT(prim) * TMD_MEMBER_BYTES(prim, ft4);
            break;
        case TMD_PRIM_FT3:
            GsOUT_PACKET_P = GsTMDfastTNF3(
                &prim->ft3, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), shift, ot, work);
            /* The named count (here and in case 0x35) replaced two weight
             * fences: it re-orders the local v0/v1 quantities the fences
             * pinned. The other arms need the plain *prim spelling
             * (measured). */
            count = TMD_BATCH_COUNT(prim);
            n -= count;
            step = count * TMD_MEMBER_WORDS(prim, ft3);
            step <<= 2;
            break;
        case TMD_PRIM_GT3:
            GsOUT_PACKET_P = GsTMDfastTNG3(
                &prim->gt3, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), shift, ot, work);
            count = TMD_BATCH_COUNT(prim);
            n -= count;
            step = count * TMD_MEMBER_WORDS(prim, gt3);
            step <<= 2;
            break;
        default:
            return;
        }
        prim = (TmdPrimitiveRecord *)((int)prim + step);
    }
}
