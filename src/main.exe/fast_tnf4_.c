#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

/*
 * fast_tnf4_ (0x80059b08, 0x4ec bytes) — DecodeTMD-family primitive
 * renderer, the 1.00 mnemonic clone of fast_tng4_ (the POLY_GT4 pair of
 * the family). The ONLY difference from fast_tng4_ is the
 * record type: the flat 0x20-byte TMD record has ONE packed colour word,
 * which feeds all four per-vertex colour slots (fast_tng4_ reads four
 * distinct words). Everything else —
 * including every matching note — is fast_tng4_.c verbatim; read that
 * file's header for the full mechanism account.
 *
 * Matching notes: applies the fast_tnf3_ recipe verbatim (read that
 * header). The dual-view TMD record keeps the original TMD_P_TNF4 layout and
 * the normal strength-reduced loop on the target's single cursor; the former
 * function-only flag was compensating for decompiler-style byte offsets. New
 * vs the leaf: the
 * context lives as TWO variables (work + the prim staging pointer — the
 * packet accesses go through prim, the context fields through work);
 * flagAddr is a precomputed loop invariant (used by BOTH gte_stflg sites);
 * the 52-byte packet assignment emits a 3-chunk movstrsi loop +
 * 4-byte remainder.
 */

u_long *fast_tnf4_(TmdTexturedFlatQuadRecord *record, VERT *vertices,
                   u_long *packet,
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
            gte_stflg(flagAddr);
            if (work->flag < 0)
                goto next;

            gte_nclip();
            prim->gpu.vertex[0].color.word = record->stream.color.word;
            color->channel.cd = codeVal;
            gte_stopz((u_long *)&work->opz);
            if (work->opz <= 0)
                goto next;

            gte_stsxy3_gt3(&prim->packet);
            gte_ldv0(TMD_VERTEX_AT(vertices, record->stream.vertex[3]));
            gte_rtps();

            prim->gpu.vertex[3].texture.word = record->stream.texture[3].word;
            prim->gpu.vertex[1].color.word = record->stream.color.word;
            prim->gpu.vertex[2].color.word = record->stream.color.word;
            prim->gpu.vertex[3].color.word = record->stream.color.word;
            gte_stflg(flagAddr);
            if (work->flag < 0)
                goto next;

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
                goto next;
            if (work->clipy1 < lo)
                goto next;

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
                goto next;

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
            packet += GPU_POLY_GT4_WORDS;

        next:
            count--;
            record++;
        } while (count != 0);
    }
    return packet;
}
