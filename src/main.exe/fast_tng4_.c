#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

PACKET *fast_tng4_(TmdTexturedGouraudQuadRecord *record, VERT *vertices,
                   PACKET *packet,
                   int count, TMD_FAST_WORK *wp)
{
    TMD_FAST_WORK *work;
    GpuPolyGT4Packet *prim;
    u_long *flagAddr;
    GpuColorWord *color;
    s32 codeVal;
    u_long *sz0Ptr;
    u_long *rgbPtr;
    u_long *otSlot;
    u32 idx0, idx1, idx2;
    s32 b, c, lo, hi, otz;
    s32 z1, z2;

    work = wp;
    prim = &work->gt4;
    if (count != 0)
    {
        flagAddr = (u_long *)&work->flag;
        color = &work->gt4.gpu.vertex[0].color;
        codeVal = GPU_POLY_GT4_CODE;
        sz0Ptr = (u_long *)&work->sz[0];
        for (; count != 0; count--, record++)
        {
            idx0 = record->stream.vertex[0];
            idx1 = record->stream.vertex[1];
            idx2 = record->stream.vertex[2];
            gte_ldv3(TMD_VERTEX_AT(vertices, idx0),
                     TMD_VERTEX_AT(vertices, idx1),
                     TMD_VERTEX_AT(vertices, idx2));
            gte_rtpt();

            prim->gpu.vertex[0].texture.word = record->stream.texture[0].word;
            prim->gpu.vertex[1].texture.word = record->stream.texture[1].word;
            prim->gpu.vertex[2].texture.word = record->stream.texture[2].word;
            gte_stflg(flagAddr);
            if (work->flag < 0)
                continue;

            gte_nclip();
            prim->gpu.vertex[0].color.word = record->stream.color[0].word;
            color->channel.cd = codeVal;
            gte_stopz((u_long *)&work->opz);
            if (work->opz <= 0)
                continue;

            gte_stsxy3_gt3(&prim->packet);
            gte_ldv0(TMD_VERTEX_AT(vertices, record->stream.vertex[3]));
            gte_rtps();

            prim->gpu.vertex[3].texture.word = record->stream.texture[3].word;
            prim->gpu.vertex[1].color.word = record->stream.color[1].word;
            prim->gpu.vertex[2].color.word = record->stream.color[2].word;
            prim->gpu.vertex[3].color.word = record->stream.color[3].word;
            gte_stflg(flagAddr);
            if (work->flag < 0)
                continue;

            gte_stsxy(&prim->gpu.vertex[3].screen.word);

            lo = prim->packet.x0;
            b = prim->packet.x1;
            if (b < lo)
            {
                hi = lo;
                lo = b;
            }
            else
            {
                hi = b;
            }
            c = prim->packet.x2;
            if (c < lo)
            {
                lo = c;
            }
            else if (hi < c)
            {
                hi = c;
            }
            c = prim->packet.x3;
            if (c < lo)
            {
                lo = c;
            }
            else if (hi < c)
            {
                hi = c;
            }
            if (hi < work->clipx0)
                continue;
            if (work->clipx1 < lo)
                continue;

            lo = prim->packet.y0;
            b = prim->packet.y1;
            if (b < lo)
            {
                hi = lo;
                lo = b;
            }
            else
            {
                hi = b;
            }
            c = prim->packet.y2;
            if (c < lo)
            {
                lo = c;
            }
            else if (hi < c)
            {
                hi = c;
            }
            c = prim->packet.y3;
            if (c < lo)
            {
                lo = c;
            }
            else if (hi < c)
            {
                hi = c;
            }
            if (hi < work->clipy0)
                continue;
            if (work->clipy1 < lo)
                continue;

            gte_stsz4(sz0Ptr, (u_long *)&work->sz[1], (u_long *)&work->sz[2],
                      (u_long *)&work->sz[3]);
            lo = work->sz[0];
            b = work->sz[1];
            if (b < lo)
            {
                otz = lo;
                lo = b;
            }
            else
            {
                otz = b;
            }
            c = work->sz[2];
            if (c < lo)
            {
                lo = c;
            }
            else if (otz < c)
            {
                otz = c;
            }
            c = work->sz[3];
            if (c < lo)
            {
                lo = c;
            }
            else if (otz < c)
            {
                otz = c;
            }
            if (work->farz < lo)
                continue;

            work->otz = otz / 4;
            z1 = work->fogz;
            if (z1 < otz)
            {
                rgbPtr = &prim->gpu.vertex[0].color.word;
                z2 = work->sz[0];
                gte_ldrgb(rgbPtr);
                gte_lddp(z2 - z1);
                gte_dpcs();
                z2 = z1 < z2;
                if (z2 != 0)
                {
                    gte_strgb(rgbPtr);
                }

                rgbPtr = &prim->gpu.vertex[1].color.word;
                z1 = work->sz[1];
                gte_ldrgb(rgbPtr);
                z2 = work->fogz;
                gte_lddp(z1 - z2);
                gte_dpcs();
                z2 = z2 < z1;
                if (z2 != 0)
                {
                    gte_strgb(rgbPtr);
                }

                rgbPtr = &prim->gpu.vertex[2].color.word;
                z1 = work->sz[2];
                gte_ldrgb(rgbPtr);
                z2 = work->fogz;
                gte_lddp(z1 - z2);
                gte_dpcs();
                z2 = z2 < z1;
                if (z2 != 0)
                {
                    gte_strgb(rgbPtr);
                }

                rgbPtr = &prim->gpu.vertex[3].color.word;
                z1 = work->sz[3];
                gte_ldrgb(rgbPtr);
                z2 = work->fogz;
                gte_lddp(z1 - z2);
                gte_dpcs();
                z2 = z2 < z1;
                if (z2 != 0)
                {
                    gte_strgb(rgbPtr);
                }
            }

            otSlot = (u_long *)work->ot->org + (work->otz >> work->shift);
            prim->packet.tag = *otSlot;
            setlen(&prim->packet, GPU_POLY_GT4_LENGTH);
            *(GpuPolyGT4Packet *)packet = *prim;
            *otSlot = (u_long)packet & GPU_DMA_ADDRESS_MASK;
            packet += sizeof(GpuPolyGT4Packet);
        }
    }
    return packet;
}
