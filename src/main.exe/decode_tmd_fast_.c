#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"

/*
 * Decode the linked TMD primitive stream and hand each supported packet type
 * to its specialized renderer.
 *
 * Fills the shared TMD_FAST_WORK context first (tmdfast.h): the OT and
 * bucket shift, the far-Z reject (0x4a98) and depth-cue start (15000), and
 * the 320x240 screen clip box; the per-store comments name the fields.
 *
 * Matching notes (636 bytes / 159 instructions):
 *  - The real linked-TMD field layout is load-bearing for the prologue's load
 *    schedule.
 *  - The mode tag selects the concrete packed TMD record view; every renderer
 *    receives that typed stream together with the shared Sony VERT table.
 *  - The volatile attribute read preserves the retail reload across the two
 *    packet-parameter stores.
 *  - The workspace parameter must stay a plain INT (an opaque scratch
 *    address, each store cast at the site).  Typing it as any pointer
 *    (TMD_FAST_WORK * or u_long *) gives cc1's alias pass a known REG base
 *    for those MEMs: the scheduler then hoists the eight context stores
 *    above the volatile-adjacent attribute loads and the whole prologue
 *    reschedules/re-allocates (+4 bytes, attr lands in v0 instead of the
 *    dying a0).  Measured both ways; the leaves are unaffected because
 *    their workspace pointer arrives as a fifth argument they only read
 *    through.
 *  - Direct per-case cursor updates retain the two distinct x7 switch tails.
 *  - The 29-entry switch table is routed through this object's .rodata carve
 *    at 0x80013C20.
 */

extern u_long DivDepth;

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

void decode_tmd_fast_(GsDOBJ2 *obj, u_long ot, u_long shift, int work)
{
    u_long attr;
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
    attr = *(volatile u_long *)&obj->attribute;
    DivDepth = GS_DOBJ_DIVISION_DEPTH(obj->attribute);
    TMD_FAST_WORD(work, shift) = shift;
    TMD_FAST_WORD(work, ot) = ot;
    GsTON = GS_DOBJ_TON(attr);
    TMD_FAST_WORD(work, clipx0) = -SCREEN_W / 2;
    TMD_FAST_WORD(work, clipx1) = SCREEN_W / 2;
    TMD_FAST_WORD(work, clipy0) = -SCREEN_H / 2;
    TMD_FAST_WORD(work, clipy1) = SCREEN_H / 2;
    TMD_FAST_WORD(work, farz) = TMD_FAST_FAR_Z;
    TMD_FAST_WORD(work, fogz) = TMD_FAST_FOG_Z;
    while (n != 0)
    {
        switch (TMD_BATCH_MODE(prim) & TMD_PRIMITIVE_MODE_MASK)
        {
        case TMD_PRIM_GT4:
            GsOUT_PACKET_P = fast_tng4_(
                &prim->gt4, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), (TMD_FAST_WORK *)work);
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, gt4);
            continue;
        case TMD_PRIM_FT4:
            GsOUT_PACKET_P = fast_tnf4_(
                &prim->ft4, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), (TMD_FAST_WORK *)work);
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, ft4);
            continue;
        case TMD_PRIM_FT3:
            GsOUT_PACKET_P = fast_tnf3_(
                &prim->ft3, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), (TMD_FAST_WORK *)work);
            n -= TMD_BATCH_COUNT(prim);
            prim = TMD_NEXT_BATCH(prim, ft3);
            continue;
        case TMD_PRIM_GT3:
            GsOUT_PACKET_P = fast_tng3_(
                &prim->gt3, vertices, GsOUT_PACKET_P,
                TMD_BATCH_COUNT(prim), (TMD_FAST_WORK *)work);
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
