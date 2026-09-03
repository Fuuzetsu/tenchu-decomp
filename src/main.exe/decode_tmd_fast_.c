#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

extern u_long *fast_tng4_(TmdTexturedGouraudQuadRecord *primitive,
                          VERT *vertices,
                          u_long *packet, u_short count,
                          TMD_FAST_WORK *work);
extern u_long *fast_tnf4_(TmdTexturedFlatQuadRecord *primitive,
                          VERT *vertices,
                          u_long *packet, u_short count,
                          TMD_FAST_WORK *work);
extern u_long *fast_tnf3_(TmdTexturedFlatTriangleRecord *primitive,
                          VERT *vertices,
                          u_long *packet, u_short count,
                          TMD_FAST_WORK *work);
extern u_long *fast_tng3_(TmdTexturedGouraudTriangleRecord *primitive,
                          VERT *vertices,
                          u_long *packet, u_short count,
                          TMD_FAST_WORK *work);

void decode_tmd_fast_(GsDOBJ2 *obj, GsOT *ot, u_long shift,
                      TMD_FAST_WORK *work)
{
    TmdObjectRecord *tmd;
    TmdPrimitiveRecord *prim;
    int n;
    VERT *vertices;

    tmd = (TmdObjectRecord *)obj->tmd;
    GsLMODE = GS_DOBJ_LMODE(obj->attribute);
    prim = tmd->linked.primitives;
    n = tmd->linked.primitive_count;
    GsLIGNR = GS_DOBJ_LIGNR(obj->attribute);
    vertices = tmd->linked.vertices;
    GsLIOFF = GS_DOBJ_LIOFF(obj->attribute);
    DivDepth = GS_DOBJ_DIVISION_DEPTH(obj->attribute);
    GsTON = GS_DOBJ_TON(obj->attribute);
    work->shift = shift;
    work->ot = ot;
    work->clipx0 = -SCREEN_W / 2;
    work->clipx1 = SCREEN_W / 2;
    work->clipy0 = -SCREEN_H / 2;
    work->clipy1 = SCREEN_H / 2;
    work->farz = TMD_FAST_FAR_Z;
    work->fogz = TMD_FAST_FOG_Z;
    while (n != 0)
    {
        switch (prim->batch.mode & TMD_PRIMITIVE_MODE_MASK)
        {
        case TMD_PRIM_GT4:
            GsOUT_PACKET_P = fast_tng4_(
                &prim->gt4, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), work);
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, gt4);
            continue;
        case TMD_PRIM_FT4:
            GsOUT_PACKET_P = fast_tnf4_(
                &prim->ft4, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), work);
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, ft4);
            continue;
        case TMD_PRIM_FT3:
            GsOUT_PACKET_P = fast_tnf3_(
                &prim->ft3, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), work);
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, ft3);
            continue;
        case TMD_PRIM_GT3:
            GsOUT_PACKET_P = fast_tng3_(
                &prim->gt3, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), work);
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, gt3);
            continue;
        case TMD_PRIM_G4:
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, g4);
            continue;
        case TMD_PRIM_G3:
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, g3);
            continue;
        case TMD_PRIM_F3:
        case TMD_PRIM_F4:
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, f3);
            continue;
        default:
            return;
        }
    }
}
