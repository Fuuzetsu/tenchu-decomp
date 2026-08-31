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
 * This is the flat-colour TMD_P_TNF4 sibling of adiv_tng4_.  Keeping the
 * primitive's original record type is essential: under the common -O2 flags,
 * loop.c derives the target's one record cursor from the named fields.  The
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

u_long *adiv_tnf4_(u_short *primtop, u_long vertop, u_long *packet, int count,
                   volatile u_long shift, volatile u_long ot, u_long *wp)
{
    int hwd;
    int vwd;
    u_long t0;
    int cd;
    int code;
    int cnt;
    int init;
    u_long o;
    u_long *work;
    u_long t1;
    u_long t2;
    SVECTOR *v0;
    TMD_P_TNF4 *primitive;
    u_long *frame;
    u_char b;
    SVECTOR *v3;
    SVECTOR *v2;
    SVECTOR *v1;
    u_long *vp;

    work = wp;
    hwd = HWD0;
    init = 4;
    *work = init;        /* limit */
    frame = work + 0x38; /* frame[0] */
    vp = frame;
    vwd = VWD0;
    *(short *)(work + 0xd) = (short)(hwd / 2);       /* adivw */
    *(short *)((int)work + 0x36) = (short)(vwd / 2); /* adivh */
    o = ot;
    t1 = shift;
    t0 = *(u_long *)(o + 4);
    init = 150;
    work[8] = init;                      /* adivz */
    setlen(&((ADIV_WORK *)work)->packet, 0xc);
    code = 0x3c;
    work[3] = t1;                         /* shift */
    ((ADIV_WORK *)work)->out = packet;
    setcode(&((ADIV_WORK *)work)->packet, code);
    work[4] = t0;                         /* org */
    cnt = count;
    if (cnt != 0)
    {
        v0 = (SVECTOR *)(work + 0x20); /* v[0..3] */
        v1 = (SVECTOR *)(work + 0x26);
        v2 = (SVECTOR *)(work + 0x2c);
        v3 = (SVECTOR *)(work + 0x32);
        cd = code;
        primitive = (TMD_P_TNF4 *)primtop;
        do
        {
            *(short *)(work + 0x20) = *(u_short *)(primitive->v0 * 8 + vertop);
            *(short *)((int)work + 0x82) = *(u_short *)(primitive->v0 * 8 + vertop + 2);
            *(short *)(work + 0x21) = *(u_short *)(primitive->v0 * 8 + vertop + 4);
            *(short *)(work + 0x26) = *(u_short *)(primitive->v1 * 8 + vertop);
            *(short *)((int)work + 0x9a) = *(u_short *)(primitive->v1 * 8 + vertop + 2);
            *(short *)(work + 0x27) = *(u_short *)(primitive->v1 * 8 + vertop + 4);
            *(short *)(work + 0x2c) = *(u_short *)(primitive->v2 * 8 + vertop);
            *(short *)((int)work + 0xb2) = *(u_short *)(primitive->v2 * 8 + vertop + 2);
            *(short *)(work + 0x2d) = *(u_short *)(primitive->v2 * 8 + vertop + 4);
            *(short *)(work + 0x32) = *(u_short *)(primitive->v3 * 8 + vertop);
            *(short *)((int)work + 0xca) = *(u_short *)(primitive->v3 * 8 + vertop + 2);
            *(short *)(work + 0x33) = *(u_short *)(primitive->v3 * 8 + vertop + 4);
            *vp = (u_long)v0;
            vp[1] = (u_long)v1;
            vp[2] = (u_long)v2;
            vp[3] = (u_long)v3;
            gte_ldv3(v0, v1, v2);
            gte_rtpt();
            *(short *)(work + 0x25) = *(u16 *)&primitive->tu0;
            *(short *)(work + 0x2b) = *(u16 *)&primitive->tu1;
            t2 = (u_long)(work + 0x23);
            gte_stsxy3((u_long *)t2, work + 0x29, work + 0x2f);
            gte_nclip();
            *(short *)(work + 0x31) = *(u16 *)&primitive->tu2;
            *(short *)(work + 0x37) = *(u16 *)&primitive->tu3;
            gte_stopz(work + 6); /* zmax */
            if (0 < (int)work[6])
            {
                gte_ldv0(v3);
                gte_rtps();
                work[0x22] = *(u_long *)&primitive->r0;
                b = (u_char)cd;
                *(u_char *)((int)work + 0x8b) = b;
                work[0x28] = *(u_long *)&primitive->r0;
                *(u_char *)((int)work + 0xa3) = b;
                work[0x2e] = *(u_long *)&primitive->r0;
                *(u_char *)((int)work + 0xbb) = b;
                work[0x34] = *(u_long *)&primitive->r0;
                *(u_char *)((int)work + 0xd3) = b;
                gte_stsxy(work + 0x35);
                t0 = (u_long)(work + 0x24);
                t2 = (u_long)(work + 0x2a);
                t1 = (u_long)(work + 0x30);
                gte_stsz4((u_long *)t0, (u_long *)t2, (u_long *)t1, work + 0x36);
                ((ADIV_WORK *)work)->packet.clut = primitive->clut;
                ((ADIV_WORK *)work)->packet.tpage = primitive->tpage;
                subdivide_quad_(frame, work, 0);
            }
            cnt--;
            primitive++;
        } while (cnt != 0);
    }
    return ((ADIV_WORK *)work)->out;
}
