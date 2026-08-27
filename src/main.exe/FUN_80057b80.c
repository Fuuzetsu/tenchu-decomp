#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"
/*
 * FUN_80057b80 (0x80057b80, 3796 bytes) — the recursive quad subdivider of
 * the active-subdivision cluster (entered from FUN_80058c70/FUN_80059008;
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
 */

void FUN_80057b80(ADIV_FRAME *afp, ADIV_WORK *awp, int depth)
{
    ADIV_FRAME *fp;
    ADIV_WORK *work;
    short s;
    u16 u;
    int tail;
    int zA;
    int zB;
    int zC;
    int prim;
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
    zA = fp->vp[2]->sz;
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
        zC = zC + 3;
    }
    work->zmax = zC >> 2;
    if (work->adivz <= zC >> 2)
    {
        if (fp->vp[0]->sxy.vx > fp->vp[1]->sxy.vx)
        {
            *(u16 *)&work->maxx = *(u16 *)&fp->vp[0]->sxy.vx;
            *(u16 *)&work->minx = *(u16 *)&fp->vp[1]->sxy.vx;
        }
        else
        {
            *(u16 *)&work->maxx = *(u16 *)&fp->vp[1]->sxy.vx;
            *(u16 *)&work->minx = *(u16 *)&fp->vp[0]->sxy.vx;
        }
        s = fp->vp[2]->sxy.vx;
        u = *(u16 *)&fp->vp[2]->sxy.vx;
        if (s < work->minx)
        {
            *(u16 *)&work->minx = u;
        }
        else if (work->maxx < s)
        {
            *(u16 *)&work->maxx = u;
        }
        s = fp->vp[3]->sxy.vx;
        u = *(u16 *)&fp->vp[3]->sxy.vx;
        if (s < work->minx)
        {
            *(u16 *)&work->minx = u;
        }
        else if (work->maxx < s)
        {
            *(u16 *)&work->maxx = u;
        }
        if ((-(int)work->adivw <= (int)work->maxx) &&
            ((int)work->minx <= (int)work->adivw))
        {
            if (fp->vp[0]->sxy.vy > fp->vp[1]->sxy.vy)
            {
                *(u16 *)&work->maxy = *(u16 *)&fp->vp[0]->sxy.vy;
                *(u16 *)&work->miny = *(u16 *)&fp->vp[1]->sxy.vy;
            }
            else
            {
                *(u16 *)&work->maxy = *(u16 *)&fp->vp[1]->sxy.vy;
                *(u16 *)&work->miny = *(u16 *)&fp->vp[0]->sxy.vy;
            }
            s = fp->vp[2]->sxy.vy;
            u = *(u16 *)&fp->vp[2]->sxy.vy;
            if (s < work->miny)
            {
                *(u16 *)&work->miny = u;
            }
            else if (work->maxy < s)
            {
                *(u16 *)&work->maxy = u;
            }
            s = fp->vp[3]->sxy.vy;
            u = *(u16 *)&fp->vp[3]->sxy.vy;
            if (s < work->miny)
            {
                *(u16 *)&work->miny = u;
            }
            else if (work->maxy < s)
            {
                *(u16 *)&work->maxy = u;
            }
            if ((-(int)work->adivw <= (int)work->maxy) &&
                ((int)work->miny <= (int)work->adivw))
            {
                if ((work->limit == depth) ||
                    ((work->maxx - work->minx < 0xff) &&
                     (work->maxy - work->miny < 0x7f)))
                {
                    do { do { do {
                    prim = (int)work->out;
                    *(u32 *)(prim + 8) = *(u32 *)&fp->vp[0]->sxy;
                    *(u32 *)(prim + 0x14) = *(u32 *)&fp->vp[1]->sxy;
                    *(u32 *)(prim + 0x20) = *(u32 *)&fp->vp[2]->sxy;
                    *(u32 *)(prim + 0x2c) = *(u32 *)&fp->vp[3]->sxy;
                    *(u32 *)(prim + 0xc) = *(u32 *)&fp->vp[0]->tu;
                    *(u32 *)(prim + 0x18) = *(u32 *)&fp->vp[1]->tu;
                    *(u32 *)(prim + 0x24) = *(u32 *)&fp->vp[2]->tu;
                    *(u32 *)(prim + 0x30) = *(u32 *)&fp->vp[3]->tu;
                    *(u32 *)(prim + 4) = *(u32 *)&fp->vp[0]->col;
                    *(u32 *)(prim + 0x10) = *(u32 *)&fp->vp[1]->col;
                    *(u32 *)(prim + 0x1c) = *(u32 *)&fp->vp[2]->col;
                    *(u32 *)(prim + 0x28) = *(u32 *)&fp->vp[3]->col;
                    } while (0); } while (0); } while (0);
                    *(u16 *)(prim + 0xe) = proto->clut;
                    *(u16 *)(prim + 0x1a) = proto->tpage;
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
                    fp->mid[0].pos.vx = (short)(((int)a->pos.vx + (int)b->pos.vx) / 2);
                    m01 = &fp->mid[0];
                    m01->pos.vy = (short)(((int)a->pos.vy + (int)b->pos.vy) / 2);
                    m01->pos.vz = (short)(((int)a->pos.vz + (int)b->pos.vz) / 2);
                    m01->col.r = (char)((int)((u32)a->col.r + (u32)b->col.r) >> 1);
                    m01->col.g = (char)((int)((u32)a->col.g + (u32)b->col.g) >> 1);
                    m01->col.b = (char)((int)((u32)a->col.b + (u32)b->col.b) >> 1);
                    m01->col.cd = a->col.cd;
                    m01->tu = (char)((int)((u32)a->tu + (u32)b->tu) >> 1);
                    m01->tv = (char)((int)((u32)a->tv + (u32)b->tv) >> 1);
                    a = fp->vp[0];
                    b = fp->vp[2];
                    fp->mid[1].pos.vx = (short)(((int)a->pos.vx + (int)b->pos.vx) / 2);
                    m02 = &fp->mid[1];
                    m02->pos.vy = (short)(((int)a->pos.vy + (int)b->pos.vy) / 2);
                    m02->pos.vz = (short)(((int)a->pos.vz + (int)b->pos.vz) / 2);
                    m02->col.r = (char)((int)((u32)a->col.r + (u32)b->col.r) >> 1);
                    m02->col.g = (char)((int)((u32)a->col.g + (u32)b->col.g) >> 1);
                    m02->col.b = (char)((int)((u32)a->col.b + (u32)b->col.b) >> 1);
                    m02->col.cd = a->col.cd;
                    m02->tu = (char)((int)((u32)a->tu + (u32)b->tu) >> 1);
                    m02->tv = (char)((int)((u32)a->tv + (u32)b->tv) >> 1);
                    a = fp->vp[2];
                    b = fp->vp[3];
                    fp->mid[2].pos.vx = (short)(((int)a->pos.vx + (int)b->pos.vx) / 2);
                    m23 = &fp->mid[2];
                    m23->pos.vy = (short)(((int)a->pos.vy + (int)b->pos.vy) / 2);
                    m23->pos.vz = (short)(((int)a->pos.vz + (int)b->pos.vz) / 2);
                    m23->col.r = (char)((int)((u32)a->col.r + (u32)b->col.r) >> 1);
                    m23->col.g = (char)((int)((u32)a->col.g + (u32)b->col.g) >> 1);
                    m23->col.b = (char)((int)((u32)a->col.b + (u32)b->col.b) >> 1);
                    m23->col.cd = a->col.cd;
                    m23->tu = (char)((int)((u32)a->tu + (u32)b->tu) >> 1);
                    m23->tv = (char)((int)((u32)a->tv + (u32)b->tv) >> 1);
                    gte_ldv3((SVECTOR *)m01, (SVECTOR *)m02, (SVECTOR *)m23);
                    gte_rtpt();
                    a = fp->vp[3];
                    b = fp->vp[1];
                    fp->mid[3].pos.vx = (short)(((int)a->pos.vx + (int)b->pos.vx) / 2);
                    m31 = &fp->mid[3];
                    m31->pos.vy = (short)(((int)a->pos.vy + (int)b->pos.vy) / 2);
                    m31->pos.vz = (short)(((int)a->pos.vz + (int)b->pos.vz) / 2);
                    m31->col.r = (char)((int)((u32)a->col.r + (u32)b->col.r) >> 1);
                    m31->col.g = (char)((int)((u32)a->col.g + (u32)b->col.g) >> 1);
                    m31->col.b = (char)((int)((u32)a->col.b + (u32)b->col.b) >> 1);
                    m31->col.cd = a->col.cd;
                    m31->tu = (char)((int)((u32)a->tu + (u32)b->tu) >> 1);
                    m31->tv = (char)((int)((u32)a->tv + (u32)b->tv) >> 1);
                    a = fp->vp[0];
                    b = fp->vp[3];
                    fp->mid[4].pos.vx = (short)(((int)a->pos.vx + (int)b->pos.vx) / 2);
                    m03 = &fp->mid[4];
                    m03->pos.vy = (short)(((int)a->pos.vy + (int)b->pos.vy) / 2);
                    m03->pos.vz = (short)(((int)a->pos.vz + (int)b->pos.vz) / 2);
                    m03->col.r = (char)((int)((u32)a->col.r + (u32)b->col.r) >> 1);
                    m03->col.g = (char)((int)((u32)a->col.g + (u32)b->col.g) >> 1);
                    m03->col.b = (char)((int)((u32)a->col.b + (u32)b->col.b) >> 1);
                    m03->col.cd = a->col.cd;
                    m03->tu = (char)((int)((u32)a->tu + (u32)b->tu) >> 1);
                    m23sxy = (u_long *)&fp->mid[2].sxy;
                    m03->tv = (char)((int)((u32)a->tv + (u32)b->tv) >> 1);
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
                    depth = depth + 1;
                    FUN_80057b80(next, work, depth);
                    {
                        ADIV_VERT *va;
                        ADIV_VERT *vb;
                        u32 *pk;
                        u32 *slot;
                        int dz;
                        u16 tp;

                        va = fp->vp[0];
                        pk = (u32 *)work->out;
                        vb = fp->vp[1];
                        pk[2] = *(u32 *)&va->sxy;
                        pk[5] = *(u32 *)&vb->sxy;
                        pk[8] = *(u32 *)&m01->sxy;
                        dz = va->sz;
                        if (dz < 0)
                        {
                            dz = dz + 3;
                        }
                        work->zmax = dz >> 2;
                        pk[3] = (u32)*(u16 *)&va->tu;
                        pk[6] = (u32)*(u16 *)&vb->tu;
                        pk[9] = (u32)*(u16 *)&m01->tu;
                        pk[1] = *(u32 *)&va->col;
                        pk[4] = *(u32 *)&vb->col;
                        pk[7] = *(u32 *)&m01->col;
                        *(u16 *)((int)pk + 0xe) = work->packet.clut;
                        tp = work->packet.tpage;
                        *(u8 *)((int)pk + 3) = 9;
                        *(u8 *)((int)pk + 7) = 0x34;
                        *(u16 *)((int)pk + 0x1a) = tp;
                        slot = (u32 *)(work->org + (work->zmax >> work->shift));
                        work->otp = (u_long *)slot;
                        *pk = *slot & 0xffffff | 0x9000000;
                        *(u32 *)work->otp = (u32)pk & 0xffffff;
                        work->out = work->out + 10;
                    }
                    nf->vp[0] = m01;
                    pv2 = fp->vp[1];
                    nf->vp[2] = m03;
                    nf->vp[1] = pv2;
                    nf->vp[3] = m31;
                    FUN_80057b80(next, work, depth);
                    {
                        ADIV_VERT *va;
                        ADIV_VERT *vb;
                        u32 *pk;
                        u32 *slot;
                        int dz;
                        u16 tp;

                        va = fp->vp[2];
                        pk = (u32 *)work->out;
                        vb = fp->vp[0];
                        pk[2] = *(u32 *)&va->sxy;
                        pk[5] = *(u32 *)&vb->sxy;
                        pk[8] = *(u32 *)&m02->sxy;
                        dz = va->sz;
                        if (dz < 0)
                        {
                            dz = dz + 3;
                        }
                        work->zmax = dz >> 2;
                        pk[3] = (u32)*(u16 *)&va->tu;
                        pk[6] = (u32)*(u16 *)&vb->tu;
                        pk[9] = (u32)*(u16 *)&m02->tu;
                        pk[1] = *(u32 *)&va->col;
                        pk[4] = *(u32 *)&vb->col;
                        pk[7] = *(u32 *)&m02->col;
                        *(u16 *)((int)pk + 0xe) = work->packet.clut;
                        tp = work->packet.tpage;
                        *(u8 *)((int)pk + 3) = 9;
                        *(u8 *)((int)pk + 7) = 0x34;
                        *(u16 *)((int)pk + 0x1a) = tp;
                        slot = (u32 *)(work->org + (work->zmax >> work->shift));
                        work->otp = (u_long *)slot;
                        *pk = *slot & 0xffffff | 0x9000000;
                        *(u32 *)work->otp = (u32)pk & 0xffffff;
                        work->out = work->out + 10;
                    }
                    nf->vp[0] = m02;
                    nf->vp[1] = m03;
                    nf->vp[2] = fp->vp[2];
                    nf->vp[3] = m23;
                    FUN_80057b80(next, work, depth);
                    {
                        ADIV_VERT *va;
                        ADIV_VERT *vb;
                        u32 *pk;
                        u32 *slot;
                        int dz;
                        u16 tp;

                        va = fp->vp[3];
                        pk = (u32 *)work->out;
                        vb = fp->vp[2];
                        pk[2] = *(u32 *)&va->sxy;
                        pk[5] = *(u32 *)&vb->sxy;
                        pk[8] = *(u32 *)&m23->sxy;
                        dz = va->sz;
                        if (dz < 0)
                        {
                            dz = dz + 3;
                        }
                        work->zmax = dz >> 2;
                        pk[3] = (u32)*(u16 *)&va->tu;
                        pk[6] = (u32)*(u16 *)&vb->tu;
                        pk[9] = (u32)*(u16 *)&m23->tu;
                        pk[1] = *(u32 *)&va->col;
                        pk[4] = *(u32 *)&vb->col;
                        pk[7] = *(u32 *)&m23->col;
                        *(u16 *)((int)pk + 0xe) = work->packet.clut;
                        tp = work->packet.tpage;
                        *(u8 *)((int)pk + 3) = 9;
                        *(u8 *)((int)pk + 7) = 0x34;
                        *(u16 *)((int)pk + 0x1a) = tp;
                        slot = (u32 *)(work->org + (work->zmax >> work->shift));
                        work->otp = (u_long *)slot;
                        *pk = *slot & 0xffffff | 0x9000000;
                        *(u32 *)work->otp = (u32)pk & 0xffffff;
                        work->out = work->out + 10;
                    }
                    nf->vp[0] = m03;
                    nf->vp[1] = m31;
                    nf->vp[2] = m23;
                    nf->vp[3] = fp->vp[3];
                    FUN_80057b80(next, work, depth);
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
                            dz = dz + 3;
                        }
                        work->zmax = dz >> 2;
                        pk[3] = (u32)*(u16 *)&vb->tu;
                        pk[6] = (u32)*(u16 *)&va->tu;
                        pk[9] = (u32)*(u16 *)&m31->tu;
                        pk[1] = *(u32 *)&vb->col;
                        pk[4] = *(u32 *)&va->col;
                        pk[7] = *(u32 *)&m31->col;
                        *(u16 *)((int)pk + 0xe) = work->packet.clut;
                        tp = work->packet.tpage;
                        *(u8 *)((int)pk + 3) = 9;
                        *(u8 *)((int)pk + 7) = 0x34;
                        *(u16 *)((int)pk + 0x1a) = tp;
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
