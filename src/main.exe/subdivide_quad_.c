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
        GpuPolyGT3Packet *pk;                                                 \
        u32 *slot;                                                            \
        int dz;                                                               \
        u16 tp;                                                               \
                                                                              \
        va = (a);                                                             \
        pk = (GpuPolyGT3Packet *)work->out;                                   \
        vb = (b);                                                             \
        pk->gpu.vertex[0].screen.word = va->screen.word;                      \
        pk->gpu.vertex[1].screen.word = vb->screen.word;                      \
        pk->gpu.vertex[2].screen.word = (m)->screen.word;                     \
        dz = va->sz;                                                          \
        if (dz < 0)                                                           \
        {                                                                     \
            dz += 3;                                                          \
        }                                                                     \
        work->zmax = dz >> 2;                                                 \
        pk->gpu.vertex[0].texture.word = va->texture.coordinates;             \
        pk->gpu.vertex[1].texture.word = vb->texture.coordinates;             \
        pk->gpu.vertex[2].texture.word = (m)->texture.coordinates;            \
        pk->gpu.vertex[0].color.word = va->color.word;                        \
        pk->gpu.vertex[1].color.word = vb->color.word;                        \
        pk->gpu.vertex[2].color.word = (m)->color.word;                       \
        pk->packet.clut = work->packet.clut;                                  \
        tp = work->packet.tpage;                                              \
        setlen(&pk->packet, GPU_POLY_GT3_LENGTH);                             \
        setcode(&pk->packet, GPU_POLY_GT3_CODE);                              \
        pk->packet.tpage = tp;                                                \
        slot = (u32 *)(work->org + (work->zmax >> work->shift));              \
        work->otp = (u_long *)slot;                                           \
        *(u32 *)pk = *slot & GPU_DMA_ADDRESS_MASK | GPU_DMA_TAG_GT3;          \
        *(u32 *)work->otp = (u32)pk & GPU_DMA_ADDRESS_MASK;                   \
        work->out += GPU_POLY_GT3_WORDS;                                      \
    }

/* Average one edge into this recursion frame. The x store intentionally
 * precedes the midpoint pointer assignment; all remaining fields go through
 * that pointer, matching the renderer's shared interpolation sequence. */
#define INTERPOLATE_ADIV_VERTEX(storage, midpoint, a, b)                      \
    storage.pos.vx = (short)((a->pos.vx + b->pos.vx) / 2);                    \
    midpoint = &storage;                                                      \
    midpoint->pos.vy = (short)((a->pos.vy + b->pos.vy) / 2);                  \
    midpoint->pos.vz = (short)((a->pos.vz + b->pos.vz) / 2);                  \
    midpoint->color.channel.r =                                               \
        (u8)((a->color.channel.r + b->color.channel.r) >> 1);                 \
    midpoint->color.channel.g =                                               \
        (u8)((a->color.channel.g + b->color.channel.g) >> 1);                 \
    midpoint->color.channel.b =                                               \
        (u8)((a->color.channel.b + b->color.channel.b) >> 1);                 \
    midpoint->color.channel.cd = a->color.channel.cd;                         \
    midpoint->texture.component.u =                                           \
        (u8)((a->texture.component.u + b->texture.component.u) >> 1);         \
    midpoint->texture.component.v =                                           \
        (u8)((a->texture.component.v + b->texture.component.v) >> 1)

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
 *  - GpuPolyGT3Packet/GpuPolyGT4Packet expose the same storage as both PsyQ
 *    packet fields and GPU vertex words. Leaf quads copy each complete texture
 *    word; triangle fan pieces zero-extend only the UV halfword before their
 *    CLUT/tpage fields are installed. The remaining casts are the DMA-tag/OT
 *    link writes whose aliasing keeps retail's load scheduling.
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
    GpuPolyGT4Packet *packet;
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
        if (fp->vp[0]->screen.component.vx >
            fp->vp[1]->screen.component.vx)
        {
            work->maxx = fp->vp[0]->screen.component.vx;
            work->minx = fp->vp[1]->screen.component.vx;
        }
        else
        {
            work->maxx = fp->vp[1]->screen.component.vx;
            work->minx = fp->vp[0]->screen.component.vx;
        }
        s = fp->vp[2]->screen.component.vx;
        u = fp->vp[2]->screen.component.vx;
        if (s < work->minx)
        {
            work->minx = u;
        }
        else if (work->maxx < s)
        {
            work->maxx = u;
        }
        s = fp->vp[3]->screen.component.vx;
        u = fp->vp[3]->screen.component.vx;
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
            if (fp->vp[0]->screen.component.vy >
                fp->vp[1]->screen.component.vy)
            {
                work->maxy = fp->vp[0]->screen.component.vy;
                work->miny = fp->vp[1]->screen.component.vy;
            }
            else
            {
                work->maxy = fp->vp[1]->screen.component.vy;
                work->miny = fp->vp[0]->screen.component.vy;
            }
            s = fp->vp[2]->screen.component.vy;
            u = fp->vp[2]->screen.component.vy;
            if (s < work->miny)
            {
                work->miny = u;
            }
            else if (work->maxy < s)
            {
                work->maxy = u;
            }
            s = fp->vp[3]->screen.component.vy;
            u = fp->vp[3]->screen.component.vy;
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
                            packet = (GpuPolyGT4Packet *)work->out;
                            packet->gpu.vertex[0].screen.word =
                                fp->vp[0]->screen.word;
                            packet->gpu.vertex[1].screen.word =
                                fp->vp[1]->screen.word;
                            packet->gpu.vertex[2].screen.word =
                                fp->vp[2]->screen.word;
                            packet->gpu.vertex[3].screen.word =
                                fp->vp[3]->screen.word;
                            packet->gpu.vertex[0].texture.word =
                                fp->vp[0]->texture.word;
                            packet->gpu.vertex[1].texture.word =
                                fp->vp[1]->texture.word;
                            packet->gpu.vertex[2].texture.word =
                                fp->vp[2]->texture.word;
                            packet->gpu.vertex[3].texture.word =
                                fp->vp[3]->texture.word;
                            packet->gpu.vertex[0].color.word =
                                fp->vp[0]->color.word;
                            packet->gpu.vertex[1].color.word =
                                fp->vp[1]->color.word;
                            packet->gpu.vertex[2].color.word =
                                fp->vp[2]->color.word;
                            packet->gpu.vertex[3].color.word =
                                fp->vp[3]->color.word;
                        } while (0);
                    } while (0);
                    packet->packet.clut = proto->clut;
                    packet->packet.tpage = proto->tpage;
                    *(u_long *)work->out = proto->tag;
                    otp = (u32 *)(work->org + (work->zmax >> work->shift));
                    work->otp = (u_long *)otp;
                    *(u32 *)work->out =
                        *otp & GPU_DMA_ADDRESS_MASK | GPU_DMA_TAG_GT4;
                    *(u32 *)work->otp =
                        (u32)work->out & GPU_DMA_ADDRESS_MASK;
                    tail = (int)work->out + sizeof(POLY_GT4);
                }
                else
                {
                    a = fp->vp[0];
                    b = fp->vp[1];
                    INTERPOLATE_ADIV_VERTEX(fp->mid[0], m01, a, b);
                    a = fp->vp[0];
                    b = fp->vp[2];
                    INTERPOLATE_ADIV_VERTEX(fp->mid[1], m02, a, b);
                    a = fp->vp[2];
                    b = fp->vp[3];
                    INTERPOLATE_ADIV_VERTEX(fp->mid[2], m23, a, b);
                    gte_ldv3((SVECTOR *)m01, (SVECTOR *)m02, (SVECTOR *)m23);
                    gte_rtpt();
                    a = fp->vp[3];
                    b = fp->vp[1];
                    INTERPOLATE_ADIV_VERTEX(fp->mid[3], m31, a, b);
                    a = fp->vp[0];
                    b = fp->vp[3];
                    fp->mid[4].pos.vx = (short)((a->pos.vx + b->pos.vx) / 2);
                    m03 = &fp->mid[4];
                    m03->pos.vy = (short)((a->pos.vy + b->pos.vy) / 2);
                    m03->pos.vz = (short)((a->pos.vz + b->pos.vz) / 2);
                    m03->color.channel.r =
                        (u8)((a->color.channel.r + b->color.channel.r) >> 1);
                    m03->color.channel.g =
                        (u8)((a->color.channel.g + b->color.channel.g) >> 1);
                    m03->color.channel.b =
                        (u8)((a->color.channel.b + b->color.channel.b) >> 1);
                    m03->color.channel.cd = a->color.channel.cd;
                    m03->texture.component.u =
                        (u8)((a->texture.component.u +
                              b->texture.component.u) >>
                             1);
                    m23sxy = (u_long *)&fp->mid[2].screen.word;
                    m03->texture.component.v =
                        (u8)((a->texture.component.v +
                              b->texture.component.v) >>
                             1);
                    gte_stsxy3((u_long *)&fp->mid[0].screen.word,
                               (u_long *)&fp->mid[1].screen.word, m23sxy);
                    gte_stsz3((u_long *)&fp->mid[0].sz, (u_long *)&fp->mid[1].sz, (u_long *)&fp->mid[2].sz);
                    gte_ldv3((SVECTOR *)m23, (SVECTOR *)m31, (SVECTOR *)m03);
                    gte_rtpt();
                    pv = fp->vp[0];
                    nf->vp[1] = m01;
                    nf->vp[2] = m02;
                    nf->vp[3] = m03;
                    nf->vp[0] = pv;
                    gte_stsxy3(m23sxy,
                               (u_long *)&fp->mid[3].screen.word,
                               (u_long *)&fp->mid[4].screen.word);
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
                        GpuPolyGT3Packet *pk;
                        u32 *slot;
                        int dz;
                        u16 tp;

                        vb = fp->vp[1];
                        pk = (GpuPolyGT3Packet *)work->out;
                        va = fp->vp[3];
                        pk->gpu.vertex[0].screen.word = vb->screen.word;
                        pk->gpu.vertex[1].screen.word = va->screen.word;
                        pk->gpu.vertex[2].screen.word = m31->screen.word;
                        dz = vb->sz;
                        if (dz < 0)
                        {
                            dz += 3;
                        }
                        work->zmax = dz >> 2;
                        pk->gpu.vertex[0].texture.word =
                            vb->texture.coordinates;
                        pk->gpu.vertex[1].texture.word =
                            va->texture.coordinates;
                        pk->gpu.vertex[2].texture.word =
                            m31->texture.coordinates;
                        pk->gpu.vertex[0].color.word = vb->color.word;
                        pk->gpu.vertex[1].color.word = va->color.word;
                        pk->gpu.vertex[2].color.word = m31->color.word;
                        pk->packet.clut = work->packet.clut;
                        tp = work->packet.tpage;
                        setlen(&pk->packet, GPU_POLY_GT3_LENGTH);
                        setcode(&pk->packet, GPU_POLY_GT3_CODE);
                        pk->packet.tpage = tp;
                        slot = (u32 *)(work->org + (work->zmax >> work->shift));
                        work->otp = (u_long *)slot;
                        *(u32 *)pk =
                            *slot & GPU_DMA_ADDRESS_MASK | GPU_DMA_TAG_GT3;
                        *(u32 *)work->otp = (u32)pk & GPU_DMA_ADDRESS_MASK;
                        tail = (int)(work->out + GPU_POLY_GT3_WORDS);
                    }
                }
                work->out = (u_long *)tail;
            }
        }
    }
    return;
}
