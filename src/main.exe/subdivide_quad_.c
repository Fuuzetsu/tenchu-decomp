#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

/* Emit one textured gouraud triangle (POLY_GT3, code 0x34, len 9) for a
 * subdivided corner quad straight into the packet stream, z-sorted by the
 * FIRST vertex's depth. Retail copy-pastes this block four times; the
 * macro is reconstruction shorthand for the first three copies (expands
 * to the identical text). The fourth, final copy stays open-coded below:
 * its two corner loads are issued in the opposite order and it parks the
 * bumped output pointer in `tail` for the shared epilogue. */
#define EMIT_SUBDIV_GT3(a, b, m)                                              \
    {                                                                         \
        ADIV_VERT *va;                                                        \
        ADIV_VERT *vb;                                                        \
        u32 *pk;                                                              \
        u32 *slot;                                                            \
        int dz;                                                               \
        u16 tp;                                                               \
                                                                              \
        va = (a);                                                             \
        pk = (u32 *)work->out;                                                \
        vb = (b);                                                             \
        pk[2] = *(u32 *)&va->sxy;                                             \
        pk[5] = *(u32 *)&vb->sxy;                                             \
        pk[8] = *(u32 *)&(m)->sxy;                                            \
        dz = va->sz;                                                          \
        if (dz < 0)                                                           \
        {                                                                     \
            dz += 3;                                                      \
        }                                                                     \
        work->zmax = dz >> 2;                                                 \
        pk[3] = (u32) * (u16 *)&va->tu;                                       \
        pk[6] = (u32) * (u16 *)&vb->tu;                                       \
        pk[9] = (u32) * (u16 *)&(m)->tu;                                      \
        pk[1] = *(u32 *)&va->col;                                             \
        pk[4] = *(u32 *)&vb->col;                                             \
        pk[7] = *(u32 *)&(m)->col;                                            \
        ((POLY_GT3 *)pk)->clut = work->packet.clut;                           \
        tp = work->packet.tpage;                                              \
        setlen(pk, 9);                                                        \
        setcode(pk, 0x34);                                                    \
        ((POLY_GT3 *)pk)->tpage = tp;                                         \
        slot = (u32 *)(work->org + (work->zmax >> work->shift));              \
        work->otp = (u_long *)slot;                                           \
        *pk = *slot & 0xffffff | 0x9000000;                                   \
        *(u32 *)work->otp = (u32)pk & 0xffffff;                               \
        work->out += 10;                                           \
    }

/*
 * subdivide_quad_ (0x80057b80, 3796 bytes) — the recursive quad subdivider of
 * the active-subdivision cluster (entered from adiv_tng4_/adiv_tnf4_;
 * layout types in tmdfast.h).  Per call it takes one ADIV_FRAME (four
 * ADIV_VERT corner pointers), computes the quad's Z range and screen bbox,
 * rejects when off-screen, and either emits the leaf POLY_GT4 from the
 * workspace template (small enough on screen, or the depth limit reached)
 * or computes the five edge/centre midpoints in the frame, transforms them
 * (RTPT), and recurses into the next frame for the four sub-quads,
 * emitting a POLY_GT3 fan piece after each recursion.
 *
 * MATCHED: recursive primitive subdivision renderer using the PsyQ inline-GTE
 * macros.  The two pointer arguments are first copied into ordinary locals,
 * with the second assignment written before the first.  gcc coalesces those
 * locals into s1 and s0 while retaining that source order, which produces the
 * target's s1/a1 save-copy pair before its s0/a0 pair.
 *
 * This disproves the former signature-only impossibility proof: it assumed
 * every body use had to remain on the formal-parameter pseudos.  The nested
 * do/while boundary around the leaf vertex copies remains allocation-relevant;
 * it is consistent with nested primitive-copy macro expansion, while removing
 * it changes the function-wide register assignment.
 *
 * Spelling notes:
 *  - The screen-Y extent test reuses adivw (the half-WIDTH) as its bound —
 *    that is what retail's bytes do (offset 0x34 in both extent tests).
 *  - The midpoint blocks write the first pos.vx through the frame and the
 *    rest through the midpoint pointer, per the matched bytes.
 *  - Each GT3 emission re-derives the packet/OT fields from the workspace
 *    (never from the leaf `proto` pointer) — distinct spellings, kept.
 *  - Packet payload transfers use a u32 word view because each assignment
 *    copies a packed colour, XY, or UV word. Named metadata uses POLY_GT3/
 *    POLY_GT4 fields and the SDK setlen/setcode macros.
 */

void subdivide_quad_(ADIV_FRAME *afp, ADIV_WORK *awp, int depth)
{
    ADIV_FRAME *fp;
    ADIV_WORK *work;
    short s;
    u16 u;
    int tail;
    int zA;
    int zB;
    int zC;
    u32 *packet_words;
    ADIV_VERT *pv;
    ADIV_VERT *pv2;
    u32 *otp;
    ADIV_VERT *a;
    ADIV_VERT *b;
    ADIV_VERT *m01;
    ADIV_VERT *m02;
    ADIV_VERT *m23;
    ADIV_VERT *m31;
    ADIV_VERT *m03;
    u_long *m23sxy;
    ADIV_FRAME *nf;
    ADIV_FRAME *next;
    POLY_GT4 *proto;

    work = awp;
    fp = afp;
    /* nf carries the field stores, next the recursive calls: the split
     * pair is byte-required (one name mismatches; measured). */
    nf = fp + 1;
    next = fp + 1;
    proto = &work->packet;
    if (fp->vp[0]->sz > fp->vp[1]->sz)
    {
        work->zmax = fp->vp[0]->sz;
        work->zmin = fp->vp[1]->sz;
    }
    else
    {
        work->zmax = fp->vp[1]->sz;
        work->zmin = fp->vp[0]->sz;
    }
    /* weight fence — split per the DefaultActionHumanoid method: +1 fp ref
     * pairing with the depth-2 leaf-copy fence below (fp must out-rank work
     * for s0; 6*99/666 > 6*110/743). */
    do
    {
        zA = fp->vp[2]->sz;
    } while (0);
    if (zA < work->zmin)
    {
        work->zmin = zA;
    }
    else if (work->zmax < zA)
    {
        work->zmax = zA;
    }
    zB = fp->vp[3]->sz;
    if (zB < work->zmin)
    {
        work->zmin = zB;
    }
    else if (work->zmax < zB)
    {
        work->zmax = zB;
    }
    zC = work->zmax;
    if (zC < 0)
    {
        zC += 3;
    }
    work->zmax = zC >> 2;
    if (work->adivz <= zC >> 2)
    {
        if (fp->vp[0]->sxy.vx > fp->vp[1]->sxy.vx)
        {
            work->maxx = fp->vp[0]->sxy.vx;
            work->minx = fp->vp[1]->sxy.vx;
        }
        else
        {
            work->maxx = fp->vp[1]->sxy.vx;
            work->minx = fp->vp[0]->sxy.vx;
        }
        s = fp->vp[2]->sxy.vx;
        u = fp->vp[2]->sxy.vx;
        if (s < work->minx)
        {
            work->minx = u;
        }
        else if (work->maxx < s)
        {
            work->maxx = u;
        }
        s = fp->vp[3]->sxy.vx;
        u = fp->vp[3]->sxy.vx;
        if (s < work->minx)
        {
            work->minx = u;
        }
        else if (work->maxx < s)
        {
            work->maxx = u;
        }
        if ((-(int)work->adivw <= (int)work->maxx) &&
            ((int)work->minx <= (int)work->adivw))
        {
            if (fp->vp[0]->sxy.vy > fp->vp[1]->sxy.vy)
            {
                work->maxy = fp->vp[0]->sxy.vy;
                work->miny = fp->vp[1]->sxy.vy;
            }
            else
            {
                work->maxy = fp->vp[1]->sxy.vy;
                work->miny = fp->vp[0]->sxy.vy;
            }
            s = fp->vp[2]->sxy.vy;
            u = fp->vp[2]->sxy.vy;
            if (s < work->miny)
            {
                work->miny = u;
            }
            else if (work->maxy < s)
            {
                work->maxy = u;
            }
            s = fp->vp[3]->sxy.vy;
            u = fp->vp[3]->sxy.vy;
            if (s < work->miny)
            {
                work->miny = u;
            }
            else if (work->maxy < s)
            {
                work->maxy = u;
            }
            if ((-(int)work->adivw <= (int)work->maxy) &&
                ((int)work->miny <= (int)work->adivw))
            {
                if ((work->limit == depth) ||
                    ((work->maxx - work->minx < 0xff) &&
                     (work->maxy - work->miny < 0x7f)))
                {
                    /* Weight fence — split per the DefaultActionHumanoid
                     * method: depth 2 here + the zA fence above keep fp
                     * ahead of work (the old depth-3 tower overshot;
                     * depth 2 alone swaps s0/s1). */
                    do
                    {
                        do
                        {
                            packet_words = (u32 *)work->out;
                            packet_words[2] = *(u32 *)&fp->vp[0]->sxy;
                            packet_words[5] = *(u32 *)&fp->vp[1]->sxy;
                            packet_words[8] = *(u32 *)&fp->vp[2]->sxy;
                            packet_words[11] = *(u32 *)&fp->vp[3]->sxy;
                            packet_words[3] = *(u32 *)&fp->vp[0]->tu;
                            packet_words[6] = *(u32 *)&fp->vp[1]->tu;
                            packet_words[9] = *(u32 *)&fp->vp[2]->tu;
                            packet_words[12] = *(u32 *)&fp->vp[3]->tu;
                            packet_words[1] = *(u32 *)&fp->vp[0]->col;
                            packet_words[4] = *(u32 *)&fp->vp[1]->col;
                            packet_words[7] = *(u32 *)&fp->vp[2]->col;
                            packet_words[10] = *(u32 *)&fp->vp[3]->col;
                        } while (0);
                    } while (0);
                    ((POLY_GT4 *)packet_words)->clut = proto->clut;
                    ((POLY_GT4 *)packet_words)->tpage = proto->tpage;
                    *(u_long *)work->out = proto->tag;
                    otp = (u32 *)(work->org + (work->zmax >> work->shift));
                    work->otp = (u_long *)otp;
                    *(u32 *)work->out = *otp & 0xffffff | 0xc000000;
                    *(u32 *)work->otp = (u32)work->out & 0xffffff;
                    tail = (int)work->out + 0x34;
                }
                else
                {
                    a = fp->vp[0];
                    b = fp->vp[1];
                    fp->mid[0].pos.vx = (short)((a->pos.vx + b->pos.vx) / 2);
                    m01 = &fp->mid[0];
                    m01->pos.vy = (short)((a->pos.vy + b->pos.vy) / 2);
                    m01->pos.vz = (short)((a->pos.vz + b->pos.vz) / 2);
                    m01->col.r = (u8)((a->col.r + b->col.r) >> 1);
                    m01->col.g = (u8)((a->col.g + b->col.g) >> 1);
                    m01->col.b = (u8)((a->col.b + b->col.b) >> 1);
                    m01->col.cd = a->col.cd;
                    m01->tu = (u8)((a->tu + b->tu) >> 1);
                    m01->tv = (u8)((a->tv + b->tv) >> 1);
                    a = fp->vp[0];
                    b = fp->vp[2];
                    fp->mid[1].pos.vx = (short)((a->pos.vx + b->pos.vx) / 2);
                    m02 = &fp->mid[1];
                    m02->pos.vy = (short)((a->pos.vy + b->pos.vy) / 2);
                    m02->pos.vz = (short)((a->pos.vz + b->pos.vz) / 2);
                    m02->col.r = (u8)((a->col.r + b->col.r) >> 1);
                    m02->col.g = (u8)((a->col.g + b->col.g) >> 1);
                    m02->col.b = (u8)((a->col.b + b->col.b) >> 1);
                    m02->col.cd = a->col.cd;
                    m02->tu = (u8)((a->tu + b->tu) >> 1);
                    m02->tv = (u8)((a->tv + b->tv) >> 1);
                    a = fp->vp[2];
                    b = fp->vp[3];
                    fp->mid[2].pos.vx = (short)((a->pos.vx + b->pos.vx) / 2);
                    m23 = &fp->mid[2];
                    m23->pos.vy = (short)((a->pos.vy + b->pos.vy) / 2);
                    m23->pos.vz = (short)((a->pos.vz + b->pos.vz) / 2);
                    m23->col.r = (u8)((a->col.r + b->col.r) >> 1);
                    m23->col.g = (u8)((a->col.g + b->col.g) >> 1);
                    m23->col.b = (u8)((a->col.b + b->col.b) >> 1);
                    m23->col.cd = a->col.cd;
                    m23->tu = (u8)((a->tu + b->tu) >> 1);
                    m23->tv = (u8)((a->tv + b->tv) >> 1);
                    gte_ldv3((SVECTOR *)m01, (SVECTOR *)m02, (SVECTOR *)m23);
                    gte_rtpt();
                    a = fp->vp[3];
                    b = fp->vp[1];
                    fp->mid[3].pos.vx = (short)((a->pos.vx + b->pos.vx) / 2);
                    m31 = &fp->mid[3];
                    m31->pos.vy = (short)((a->pos.vy + b->pos.vy) / 2);
                    m31->pos.vz = (short)((a->pos.vz + b->pos.vz) / 2);
                    m31->col.r = (u8)((a->col.r + b->col.r) >> 1);
                    m31->col.g = (u8)((a->col.g + b->col.g) >> 1);
                    m31->col.b = (u8)((a->col.b + b->col.b) >> 1);
                    m31->col.cd = a->col.cd;
                    m31->tu = (u8)((a->tu + b->tu) >> 1);
                    m31->tv = (u8)((a->tv + b->tv) >> 1);
                    a = fp->vp[0];
                    b = fp->vp[3];
                    fp->mid[4].pos.vx = (short)((a->pos.vx + b->pos.vx) / 2);
                    m03 = &fp->mid[4];
                    m03->pos.vy = (short)((a->pos.vy + b->pos.vy) / 2);
                    m03->pos.vz = (short)((a->pos.vz + b->pos.vz) / 2);
                    m03->col.r = (u8)((a->col.r + b->col.r) >> 1);
                    m03->col.g = (u8)((a->col.g + b->col.g) >> 1);
                    m03->col.b = (u8)((a->col.b + b->col.b) >> 1);
                    m03->col.cd = a->col.cd;
                    m03->tu = (u8)((a->tu + b->tu) >> 1);
                    m23sxy = (u_long *)&fp->mid[2].sxy;
                    m03->tv = (u8)((a->tv + b->tv) >> 1);
                    gte_stsxy3((u_long *)&fp->mid[0].sxy, (u_long *)&fp->mid[1].sxy, m23sxy);
                    gte_stsz3((u_long *)&fp->mid[0].sz, (u_long *)&fp->mid[1].sz, (u_long *)&fp->mid[2].sz);
                    gte_ldv3((SVECTOR *)m23, (SVECTOR *)m31, (SVECTOR *)m03);
                    gte_rtpt();
                    pv = fp->vp[0];
                    nf->vp[1] = m01;
                    nf->vp[2] = m02;
                    nf->vp[3] = m03;
                    nf->vp[0] = pv;
                    gte_stsxy3(m23sxy, (u_long *)&fp->mid[3].sxy, (u_long *)&fp->mid[4].sxy);
                    gte_stsz3((u_long *)&fp->mid[2].sz, (u_long *)&fp->mid[3].sz, (u_long *)&fp->mid[4].sz);
                    depth++;
                    subdivide_quad_(next, work, depth);
                    EMIT_SUBDIV_GT3(fp->vp[0], fp->vp[1], m01);
                    nf->vp[0] = m01;
                    pv2 = fp->vp[1];
                    nf->vp[2] = m03;
                    nf->vp[1] = pv2;
                    nf->vp[3] = m31;
                    subdivide_quad_(next, work, depth);
                    EMIT_SUBDIV_GT3(fp->vp[2], fp->vp[0], m02);
                    nf->vp[0] = m02;
                    nf->vp[1] = m03;
                    nf->vp[2] = fp->vp[2];
                    nf->vp[3] = m23;
                    subdivide_quad_(next, work, depth);
                    EMIT_SUBDIV_GT3(fp->vp[3], fp->vp[2], m23);
                    nf->vp[0] = m03;
                    nf->vp[1] = m31;
                    nf->vp[2] = m23;
                    nf->vp[3] = fp->vp[3];
                    subdivide_quad_(next, work, depth);
                    {
                        ADIV_VERT *va;
                        ADIV_VERT *vb;
                        u32 *pk;
                        u32 *slot;
                        int dz;
                        u16 tp;

                        vb = fp->vp[1];
                        pk = (u32 *)work->out;
                        va = fp->vp[3];
                        pk[2] = *(u32 *)&vb->sxy;
                        pk[5] = *(u32 *)&va->sxy;
                        pk[8] = *(u32 *)&m31->sxy;
                        dz = vb->sz;
                        if (dz < 0)
                        {
                            dz += 3;
                        }
                        work->zmax = dz >> 2;
                        pk[3] = (u32) * (u16 *)&vb->tu;
                        pk[6] = (u32) * (u16 *)&va->tu;
                        pk[9] = (u32) * (u16 *)&m31->tu;
                        pk[1] = *(u32 *)&vb->col;
                        pk[4] = *(u32 *)&va->col;
                        pk[7] = *(u32 *)&m31->col;
                        ((POLY_GT3 *)pk)->clut = work->packet.clut;
                        tp = work->packet.tpage;
                        setlen(pk, 9);
                        setcode(pk, 0x34);
                        ((POLY_GT3 *)pk)->tpage = tp;
                        slot = (u32 *)(work->org + (work->zmax >> work->shift));
                        work->otp = (u_long *)slot;
                        *pk = *slot & 0xffffff | 0x9000000;
                        *(u32 *)work->otp = (u32)pk & 0xffffff;
                        tail = (int)(work->out + 10);
                    }
                }
                work->out = (u_long *)tail;
            }
        }
    }
    return;
}
