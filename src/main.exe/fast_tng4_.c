#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

/*
 * fast_tng4_ (0x8005961c, 0x4ec bytes) — DecodeTMD-family primitive
 * renderer, the POLY_GT4 sibling of the matched leaf pair
 * fast_tnf3_/fast_tng3_ (TMD quad in decode_tmd_fast_'s switch; all family
 * members share the (u_short *, u_long, u_long *, int, TMD_FAST_WORK *)
 * signature).
 * Builds one POLY_GT4 (Gouraud-shaded, textured quad, GPU code 0x3C) output
 * packet per input record: transforms the record's first 3 vertex indices
 * through the GTE (RTPT) with a FLAG check, backface-culls via NCLIP
 * (MAC0 <= 0), transforms the 4th vertex (RTPS) with a second FLAG check,
 * discards quads whose screen-space bbox misses the caller's clip rectangle,
 * computes an OTZ bucket index from the max depth / 4, applies depth-cueing
 * (DPCS) to each vertex's baked colour when beyond the far-fog range, then
 * copies the finished packet into the caller's output list and re-links the
 * caller's OT bucket to point at it.
 *
 * work is the shared per-call rendering context of the family
 * (TMD_FAST_WORK in tmdfast.h); THIS pair stages its POLY_GT4 at work->gt4
 * (the GT3 pair stages at work->gt3).
 *
 * Matching notes: applies the fast_tnf3_ recipe verbatim — except the OTZ store, which follows the near-Z guard here (its delay slot is already taken) (read that
 * header). The original TMD_P_TNG4 record type keeps the normal strength-
 * reduced loop on the target's single cursor; the former function-only flag
 * was compensating for decompiler-style byte offsets. New vs the leaf: the
 * context lives as TWO variables (work + the prim staging pointer — the
 * packet accesses go through prim, the context fields through work);
 * flagAddr is a precomputed loop invariant (used by BOTH gte_stflg sites);
 * the 52-byte POLY_GT4 struct assignment emits a 3-chunk movstrsi loop +
 * 4-byte remainder.
 */

u_long *fast_tng4_(u_short *primitive, u_long vertop, u_long *packet, int count, TMD_FAST_WORK *wp)
{
    TMD_FAST_WORK *work;
    POLY_GT4 *prim;
    u_long *flagAddr;
    u_char *codeAddr;
    s32 codeVal;
    u_long *sz0Ptr;
    TMD_P_TNG4 *record;
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
        codeAddr = &work->gt4.r0;
        codeVal = GPU_POLY_GT4_CODE;
        sz0Ptr = (u_long *)&work->sz[0];
        record = (TMD_P_TNG4 *)primitive;
        do
        {
            idx0 = record->v0;
            idx1 = record->v1;
            idx2 = record->v2;
            gte_ldv3((SVECTOR *)(idx0 * 8 + vertop), (SVECTOR *)(idx1 * 8 + vertop),
                     (SVECTOR *)(idx2 * 8 + vertop));
            gte_rtpt();

            *(s32 *)&prim->u0 = *(s32 *)&record->tu0;
            *(s32 *)&prim->u1 = *(s32 *)&record->tu1;
            *(s32 *)&prim->u2 = *(s32 *)&record->tu2;
            gte_stflg(flagAddr);
            if (work->flag < 0)
                goto next;

            gte_nclip();
            *(s32 *)&prim->r0 = *(s32 *)&record->r0;
            codeAddr[GPU_COLOR_CODE_BYTE] = codeVal;
            gte_stopz((u_long *)&work->opz);
            if (work->opz <= 0)
                goto next;

            gte_stsxy3_gt3(prim);
            gte_ldv0((SVECTOR *)(record->v3 * 8 + vertop));
            gte_rtps();

            *(s32 *)&prim->u3 = *(s32 *)&record->tu3;
            *(s32 *)&prim->r1 = *(s32 *)&record->r1;
            *(s32 *)&prim->r2 = *(s32 *)&record->r2;
            *(s32 *)&prim->r3 = *(s32 *)&record->r3;
            gte_stflg(flagAddr);
            if (work->flag < 0)
                goto next;

            gte_stsxy((u_long *)&prim->x3);

            lo = prim->x0;
            b = prim->x1;
            if (b < lo)
            {
                hi = lo;
                lo = b;
            }
            else
            {
                hi = b;
            }
            c = prim->x2;
            if (c < lo)
            {
                lo = c;
            }
            else if (hi < c)
            {
                hi = c;
            }
            c = prim->x3;
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

            lo = prim->y0;
            b = prim->y1;
            if (b < lo)
            {
                hi = lo;
                lo = b;
            }
            else
            {
                hi = b;
            }
            c = prim->y2;
            if (c < lo)
            {
                lo = c;
            }
            else if (hi < c)
            {
                hi = c;
            }
            c = prim->y3;
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
                rgbPtr = (u_long *)&prim->r0;
                z2 = work->sz[0];
                gte_ldrgb(rgbPtr);
                gte_lddp(z2 - z1);
                gte_dpcs();
                z2 = z1 < z2;
                if (z2 != 0)
                {
                    gte_strgb(rgbPtr);
                }

                rgbPtr = (u_long *)&prim->r1;
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

                rgbPtr = (u_long *)&prim->r2;
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

                rgbPtr = (u_long *)&prim->r3;
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
            prim->tag = *otSlot;
            ((u_char *)prim)[GPU_PACKET_LENGTH_BYTE] = GPU_POLY_GT4_LENGTH;
            *(POLY_GT4 *)packet = *prim;
            *otSlot = (u_long)packet & GPU_DMA_ADDRESS_MASK;
            packet += GPU_POLY_GT4_WORDS;

        next:
            count--;
            record++;
        } while (count != 0);
    }
    return packet;
}
