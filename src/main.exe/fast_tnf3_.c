#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

u_long *fast_tnf3_(TmdTexturedFlatTriangleRecord *record, VERT *vertices,
                   u_long *packet,
                   int count, TMD_FAST_WORK *wp)
{
    TMD_FAST_WORK *work;
    GpuPolyGT3Packet *prim;
    GpuColorWord *color;
    s32 codeVal;
    u_long *sz0Ptr;
    u_long *sz1Ptr;
    u_long *rgbPtr;
    u_long *otSlot;
    u32 idx0, idx1, idx2;
    s32 b, c, lo, hi, otz;
    s32 z1, z2;

    work = wp;
    prim = &work->gt3;
    if (count != 0)
    {
        color = &work->gt3.gpu.vertex[0].color;
        codeVal = GPU_POLY_GT3_CODE;
        sz0Ptr = (u_long *)&work->sz[0];
        sz1Ptr = (u_long *)&work->sz[1];
        do
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
            prim->gpu.vertex[0].color.word = record->stream.color.word;
            color->channel.cd = codeVal;
            gte_stflg((u_long *)&work->flag);
            if (work->flag < 0)
                goto next;

            gte_nclip();
            prim->gpu.vertex[1].color.word = record->stream.color.word;
            prim->gpu.vertex[2].color.word = record->stream.color.word;
            gte_stopz((u_long *)&work->opz);
            if (work->opz <= 0)
                goto next;

            gte_stsxy3_gt3(&prim->packet);

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
            if (hi < work->clipx0)
                goto next;
            if (work->clipx1 < lo)
                goto next;

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
            if (hi < work->clipy0)
                goto next;
            if (work->clipy1 < lo)
                goto next;

            gte_stsz3(sz0Ptr, sz1Ptr, (u_long *)&work->sz[2]);
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
            work->otz = otz / 4;
            if (work->farz < lo)
                goto next;

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
            }

            otSlot = (u_long *)work->ot->org + (work->otz >> work->shift);
            prim->packet.tag = *otSlot;
            setlen(&prim->packet, GPU_POLY_GT3_LENGTH);
            *(GpuPolyGT3Packet *)packet = *prim;
            *otSlot = (u_long)packet & GPU_DMA_ADDRESS_MASK;
            packet += GPU_POLY_GT3_WORDS;

        next:
            count--;
            record++;
        } while (count != 0);
    }
    return packet;
}
