#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

/*
 * adiv_tnf4_ (0x80059008, 0x398 bytes) — a DrawTMD-family fast primitive
 * renderer (TMD primitive code 0x2d), the sibling of the switch arms in
 * decode_tmd_adiv_: it transforms each quad's four vertices through the GTE
 * (RTPT + a backface NCLIP + a per-quad RTPS) and assembles a POLY_GT4
 * (code 0x3c, len 0xc) packet, then hands the working state to subdivide_quad_.
 * Compiled-style GTE function under the restricted gte.h policy
 * (docs/gte-policy.md).
 *
 * The workspace layout is ADIV_WORK (tmdfast.h). Vertex-record and
 * timing-sensitive header stores must remain index-based off the `u_long *`
 * scratch: struct-member stores weaken gcc 2.8's alias dependencies and
 * reschedule the volatile shift/ot reads. The output cursor and terminal
 * POLY_GT4 metadata are safe typed exceptions: `out`, `clut`, `tpage`, and
 * `setlen`/`setcode` all compile identically. Same mechanism as the
 * decode_tmd_fast_ int-parameter lever (cookbook 3.13).
 *
 * This is the flat-colour TMD_P_TNF4-layout sibling of adiv_tng4_. Keeping the
 * primitive's real record shape is essential: under the common -O2 flags,
 * loop.c derives the target's one record cursor from the named fields. The
 * old u_short-offset draft created two competing induction pointers and only
 * matched when strength reduction was disabled for this artificial file.
 *
 * The final prologue match comes from two ordinary reused locals.  The loop
 * counts down a body-local copy of count, and one scalar initializes both
 * packet words (4 and 0x96).  The latter makes the two constant definitions
 * one multi-set quantity.  cc1 therefore does not give the first definition a
 * birthing priority bump, leaving a sched1 slot for the count copy; sched2 then
 * emits the s4 save/copy after the packet pointer, HWD0, and length leaders.
 * This replaces the old claimed 26-byte scheduling floor with coherent source.
 */

extern void subdivide_quad_(u_long *outv, u_long *packet, int mode);

u_long *adiv_tnf4_(TmdTexturedFlatQuadRecord *primitive, VERT *vertices,
                   u_long *packet,
                   int count, volatile u_long shift, GsOT *volatile ot,
                   u_long *wp)
{
    int hwd;
    int vwd;
    u_long t0;
    int cd;
    int code;
    int cnt;
    int init;
    GsOT *o;
    u_long *work;
    u_long t1;
    u_long t2;
    SVECTOR *v0;
    u_long *frame;
    u_char b;
    SVECTOR *v3;
    SVECTOR *v2;
    SVECTOR *v1;
    u_long *vp;

    /* The ADIV_* accessors derive scalar byte/word addresses from ADIV_WORK's
     * fields. Direct member stores do NOT match: they un-pin the interleaved
     * volatile parameter reads (see tmdfast.h and cookbook 3.13).
     * The read side uses Sony's VERT table directly: each primitive index
     * selects one authored position instead of rebuilding its byte address. */
    work = wp;
    hwd = HWD0;
    init = 4;
    ADIV_WORD(work, limit) = init;
    frame = ADIV_WORD_ADDRESS(work, frame[0]);
    vp = frame;
    vwd = VWD0;
    ADIV_SHORT(work, adivw) = (short)(hwd / 2);
    ADIV_SHORT(work, adivh) = (short)(vwd / 2);
    o = ot;
    t1 = shift;
    t0 = (u_long)o->org;
    init = 150;
    ADIV_WORD(work, adivz) = init;
    setlen(&((ADIV_WORK *)work)->packet, GPU_POLY_GT4_LENGTH);
    code = GPU_POLY_GT4_CODE;
    ADIV_WORD(work, shift) = t1;
    ((ADIV_WORK *)work)->out = packet;
    setcode(&((ADIV_WORK *)work)->packet, code);
    ADIV_WORD(work, org) = t0;
    cnt = count;
    if (cnt != 0)
    {
        v0 = (SVECTOR *)ADIV_WORD_ADDRESS(work, v[0]);
        v1 = (SVECTOR *)ADIV_WORD_ADDRESS(work, v[1]);
        v2 = (SVECTOR *)ADIV_WORD_ADDRESS(work, v[2]);
        v3 = (SVECTOR *)ADIV_WORD_ADDRESS(work, v[3]);
        cd = code;
        do
        {
            ADIV_SHORT(work, v[0].pos.vx) =
                vertices[primitive->stream.vertex[0]].vx;
            ADIV_SHORT(work, v[0].pos.vy) =
                vertices[primitive->stream.vertex[0]].vy;
            ADIV_SHORT(work, v[0].pos.vz) =
                vertices[primitive->stream.vertex[0]].vz;
            ADIV_SHORT(work, v[1].pos.vx) =
                vertices[primitive->stream.vertex[1]].vx;
            ADIV_SHORT(work, v[1].pos.vy) =
                vertices[primitive->stream.vertex[1]].vy;
            ADIV_SHORT(work, v[1].pos.vz) =
                vertices[primitive->stream.vertex[1]].vz;
            ADIV_SHORT(work, v[2].pos.vx) =
                vertices[primitive->stream.vertex[2]].vx;
            ADIV_SHORT(work, v[2].pos.vy) =
                vertices[primitive->stream.vertex[2]].vy;
            ADIV_SHORT(work, v[2].pos.vz) =
                vertices[primitive->stream.vertex[2]].vz;
            ADIV_SHORT(work, v[3].pos.vx) =
                vertices[primitive->stream.vertex[3]].vx;
            ADIV_SHORT(work, v[3].pos.vy) =
                vertices[primitive->stream.vertex[3]].vy;
            ADIV_SHORT(work, v[3].pos.vz) =
                vertices[primitive->stream.vertex[3]].vz;
            *vp = (u_long)v0;
            vp[1] = (u_long)v1;
            vp[2] = (u_long)v2;
            vp[3] = (u_long)v3;
            gte_ldv3(v0, v1, v2);
            gte_rtpt();
            ADIV_SHORT(work, v[0].texture.coordinates) =
                primitive->stream.texture[0].coordinates;
            ADIV_SHORT(work, v[1].texture.coordinates) =
                primitive->stream.texture[1].coordinates;
            t2 = (u_long)ADIV_WORD_ADDRESS(work, v[0].screen.word);
            gte_stsxy3((u_long *)t2,
                       ADIV_WORD_ADDRESS(work, v[1].screen.word),
                       ADIV_WORD_ADDRESS(work, v[2].screen.word));
            gte_nclip();
            ADIV_SHORT(work, v[2].texture.coordinates) =
                primitive->stream.texture[2].coordinates;
            ADIV_SHORT(work, v[3].texture.coordinates) =
                primitive->stream.texture[3].coordinates;
            gte_stopz(ADIV_WORD_ADDRESS(work, zmax));
            if (0 < (int)ADIV_WORD(work, zmax))
            {
                gte_ldv0(v3);
                gte_rtps();
                ADIV_WORD(work, v[0].color.word) = primitive->stream.color.word;
                b = (u_char)cd;
                ADIV_BYTE(work, v[0].color.channel.cd) = b;
                ADIV_WORD(work, v[1].color.word) = primitive->stream.color.word;
                ADIV_BYTE(work, v[1].color.channel.cd) = b;
                ADIV_WORD(work, v[2].color.word) = primitive->stream.color.word;
                ADIV_BYTE(work, v[2].color.channel.cd) = b;
                ADIV_WORD(work, v[3].color.word) = primitive->stream.color.word;
                ADIV_BYTE(work, v[3].color.channel.cd) = b;
                gte_stsxy(ADIV_WORD_ADDRESS(work, v[3].screen.word));
                t0 = (u_long)ADIV_WORD_ADDRESS(work, v[0].sz);
                t2 = (u_long)ADIV_WORD_ADDRESS(work, v[1].sz);
                t1 = (u_long)ADIV_WORD_ADDRESS(work, v[2].sz);
                gte_stsz4((u_long *)t0, (u_long *)t2, (u_long *)t1,
                           ADIV_WORD_ADDRESS(work, v[3].sz));
                ((ADIV_WORK *)work)->packet.clut =
                    primitive->stream.texture[0].component.metadata;
                ((ADIV_WORK *)work)->packet.tpage =
                    primitive->stream.texture[1].component.metadata;
                subdivide_quad_(frame, work, 0);
            }
            cnt--;
            primitive++;
        } while (cnt != 0);
    }
    return ((ADIV_WORK *)work)->out;
}
