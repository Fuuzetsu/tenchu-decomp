#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

/*
 * Render a batch of textured Gouraud quads with adaptive subdivision.
 * Each record supplies four object-space vertices, texture coordinates and
 * colours.  The GTE projects the root quad into work->v, and front-facing
 * quads are handed to subdivide_quad_ through work->frame[0].
 *
 * The remaining top-level volatile qualifier applies only to the incoming
 * ordering-table pointer. It keeps GCC 2.8's stack read at the use site; the
 * table itself is ordinary memory. The shift value follows normal by-value
 * flow and shares `t1` with the later, disjoint GTE scratch address.
 */

u_long *adiv_tng4_(TmdTexturedGouraudQuadRecord *primitive, VERT *vertices,
                   u_long *packet,
                   int count, u_long shift, GsOT *volatile ot,
                   ADIV_WORK *wp)
{
    int hwd;
    int vwd;
    u_long t0;
    int cd;
    int code;
    int cnt;
    int init;
    ADIV_WORK *work;
    u_long t1;
    u_long t2;
    ADIV_VERT *v0;
    ADIV_FRAME *frame;
    u_char b;
    ADIV_VERT *v3;
    ADIV_VERT *v2;
    ADIV_VERT *v1;
    ADIV_VERT **vp;

    /* These setup stores retain the workspace's scalar scratch view. */
    work = wp;
    hwd = HWD0;
    init = 4;
    ADIV_SCALAR_WORD(work, limit) = init;
    frame = &work->frame[0];
    vwd = VWD0;
    vp = frame->vp;
    ADIV_SCALAR_SHORT(work, adivw) = (short)(hwd / 2);
    ADIV_SCALAR_SHORT(work, adivh) = (short)(vwd / 2);
    t0 = (u_long)ot->org;
    t1 = shift;
    init = 150;
    work->adivz = init;
    work->shift = t1;
    setlen(&work->packet, GPU_POLY_GT4_LENGTH);
    code = GPU_POLY_GT4_CODE;
    work->out = packet;
    setcode(&work->packet, code);
    work->org = (u_long *)t0;
    cnt = count;
    if (cnt != 0)
    {
        v0 = &work->v[0];
        v1 = &work->v[1];
        v2 = &work->v[2];
        v3 = &work->v[3];
        cd = code;
        do
        {
            work->v[0].pos.vx =
                vertices[primitive->stream.vertex[0]].vx;
            work->v[0].pos.vy =
                vertices[primitive->stream.vertex[0]].vy;
            work->v[0].pos.vz =
                vertices[primitive->stream.vertex[0]].vz;
            work->v[1].pos.vx =
                vertices[primitive->stream.vertex[1]].vx;
            work->v[1].pos.vy =
                vertices[primitive->stream.vertex[1]].vy;
            work->v[1].pos.vz =
                vertices[primitive->stream.vertex[1]].vz;
            work->v[2].pos.vx =
                vertices[primitive->stream.vertex[2]].vx;
            work->v[2].pos.vy =
                vertices[primitive->stream.vertex[2]].vy;
            work->v[2].pos.vz =
                vertices[primitive->stream.vertex[2]].vz;
            work->v[3].pos.vx =
                vertices[primitive->stream.vertex[3]].vx;
            work->v[3].pos.vy =
                vertices[primitive->stream.vertex[3]].vy;
            work->v[3].pos.vz =
                vertices[primitive->stream.vertex[3]].vz;
            vp[0] = v0;
            vp[1] = v1;
            vp[2] = v2;
            vp[3] = v3;
            gte_ldv3(&v0->pos, &v1->pos, &v2->pos);
            gte_rtpt();
            work->v[0].texture.coordinates =
                primitive->stream.texture[0].coordinates;
            work->v[1].texture.coordinates =
                primitive->stream.texture[1].coordinates;
            t2 = (u_long)&work->v[0].screen.word;
            gte_stsxy3((u_long *)t2,
                       &work->v[1].screen.word,
                       &work->v[2].screen.word);
            gte_nclip();
            work->v[2].texture.coordinates =
                primitive->stream.texture[2].coordinates;
            work->v[3].texture.coordinates =
                primitive->stream.texture[3].coordinates;
            gte_stopz((u_long *)&work->zmax);
            if (0 < work->zmax)
            {
                gte_ldv0(&v3->pos);
                gte_rtps();
                work->v[0].color.word = primitive->stream.color[0].word;
                b = (u_char)cd;
                work->v[0].color.channel.cd = b;
                work->v[1].color.word = primitive->stream.color[1].word;
                work->v[1].color.channel.cd = b;
                work->v[2].color.word = primitive->stream.color[2].word;
                work->v[2].color.channel.cd = b;
                work->v[3].color.word = primitive->stream.color[3].word;
                work->v[3].color.channel.cd = b;
                gte_stsxy((u_long *)&work->v[3].screen.word);
                t0 = (u_long)&work->v[0].sz;
                t2 = (u_long)&work->v[1].sz;
                t1 = (u_long)&work->v[2].sz;
                gte_stsz4((u_long *)t0, (u_long *)t2, (u_long *)t1,
                           (u_long *)&work->v[3].sz);
                work->packet.clut =
                    primitive->stream.texture[0].component.metadata;
                work->packet.tpage =
                    primitive->stream.texture[1].component.metadata;
                subdivide_quad_(frame, work, 0);
            }
            cnt--;
            primitive++;
        } while (cnt != 0);
    }
    return work->out;
}
