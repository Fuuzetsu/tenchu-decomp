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

extern u_long DivDepth; /* GsDOBJ2 attribute bits 9..11 */

extern u_long *fast_tng4_(u_short *primitive, u_long vertop, u_long *packet,
                          u_short count, u_long *work);
extern u_long *fast_tnf4_(u_short *primitive, u_long vertop, u_long *packet,
                          u_short count, u_long *work);
extern u_long *fast_tnf3_(u_short *primitive, u_long vertop, u_long *packet,
                          u_short count, u_long *work);
extern u_long *fast_tng3_(u_short *primitive, u_long vertop, u_long *packet,
                          u_short count, u_long *work);

void decode_tmd_fast_(GsDOBJ2 *obj, u_long ot, u_long shift, int work)
{
    u_long attr;
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
    attr = *(volatile u_long *)&obj->attribute;
    DivDepth = obj->attribute >> 9 & 7;
    TMD_FAST_WORD(work, shift) = shift;
    TMD_FAST_WORD(work, ot) = ot;
    GsTON = attr >> 0x1e & 1;
    TMD_FAST_WORD(work, clipx0) = -SCREEN_W / 2;
    TMD_FAST_WORD(work, clipx1) = SCREEN_W / 2;
    TMD_FAST_WORD(work, clipy0) = -SCREEN_H / 2;
    TMD_FAST_WORD(work, clipy1) = SCREEN_H / 2;
    TMD_FAST_WORD(work, farz) = TMD_FAST_FAR_Z;
    TMD_FAST_WORD(work, fogz) = TMD_FAST_FOG_Z;
    while (n != 0)
    {
        switch (*(u_char *)((int)prim + TMD_PRIMITIVE_MODE_BYTE) &
                TMD_PRIMITIVE_MODE_MASK)
        {
        case TMD_PRIM_GT4:
            GsOUT_PACKET_P = fast_tng4_(prim, vertop, GsOUT_PACKET_P, *prim, work);
            n -= *prim;
            prim = (u_short *)((int)prim + *prim * 0x2c);
            continue;
        case TMD_PRIM_FT4:
            GsOUT_PACKET_P = fast_tnf4_(prim, vertop, GsOUT_PACKET_P, *prim, work);
            n -= *prim;
            prim = (u_short *)((int)prim + (*prim << 5));
            continue;
        case TMD_PRIM_FT3:
            GsOUT_PACKET_P = fast_tnf3_(prim, vertop, GsOUT_PACKET_P, *prim, work);
            n -= *prim;
            prim = (u_short *)((int)prim + *prim * 0x1c);
            continue;
        case TMD_PRIM_GT3:
            GsOUT_PACKET_P = fast_tng3_(prim, vertop, GsOUT_PACKET_P, *prim, work);
            n -= *prim;
            prim = (u_short *)((int)prim + *prim * 0x24);
            continue;
        case TMD_PRIM_G4:
            n -= *prim;
            prim = (u_short *)((int)prim + *prim * 0x1c);
            continue;
        case TMD_PRIM_G3:
            n -= *prim;
            prim = (u_short *)((int)prim + *prim * 0x18);
            continue;
        case TMD_PRIM_F3:
        case TMD_PRIM_F4:
            n -= *prim;
            prim = (u_short *)((int)prim + (*prim << 4));
            continue;
        default:
            return;
        }
    }
}
