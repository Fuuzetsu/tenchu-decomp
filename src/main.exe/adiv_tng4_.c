#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

/*
 * adiv_tng4_ (0x80058c70, 0x398 bytes) — a DrawTMD-family fast primitive
 * renderer (TMD primitive code 0x3d), the sibling of the switch arms in
 * decode_tmd_adiv_: it transforms each quad's four vertices through the GTE
 * (RTPT + a backface NCLIP + a per-quad RTPS) and assembles a POLY_GT4
 * (code 0x3c, len 0xc) packet, then hands the working state to subdivide_quad_.
 * Compiled-style GTE function under the restricted gte.h policy
 * (docs/gte-policy.md).
 *
 * STATUS: MATCHED in pure C — 0 of 920 bytes differ.
 *
 * 2026-07-19 correction: this also matches under the common compiler flags.
 * The former -fno-strength-reduce exception was compensating for a lost data
 * model: spelling the input as its original TMD_P_TNG4 record gives loop.c one
 * coherent field cursor. The u_short-offset draft manufactured two induction
 * pointers, so the old conclusion that strength reduction was disabled in the
 * original object was wrong.
 *
 * The last residual was not solved by another scheduler fence. It came from
 * two decompiler-style carrier locals that happened to improve one region but
 * trapped the function in a local minimum. The coherent source shape is:
 *
 *  - read `ot` directly for the packet word;
 *  - keep the OT-shift word from `shift` in its own `shiftWord`
 *    local instead of reusing `t1`, whose later life is a GTE address;
 *  - initialize `work[8]`, then `work[3]`, before the byte/code fields.
 *
 * Compiler dumps confirm the mechanism. The distinct single-set colour local
 * changes sched1's birthing priorities and places the volatile stack reads in
 * the target order. In sched2 that also leaves the natural `count` entry copy
 * after the s0/HWD0/li-4 leaders, matching the target prologue. The natural
 * VWD0-early `/ 2` expressions remain intact; no manual strength expansion,
 * dummy control flow, register pinning, or artificial `cnt` local is needed.
 *
 * The superseded round-by-round investigation log for this function lives
 * in docs/matching-archive.md.
 */

extern void subdivide_quad_(u_long *outv, u_long *packet, int mode);

u_long *adiv_tng4_(u_short *primtop, u_long vertop, u_long *packet, int count,
                   volatile u_long shift, volatile u_long ot, u_long *wp)
{
    int hwd;
    int vwd;
    u_long t0;
    int cd;
    int code;
    u_long *work;
    u_long t1;
    u_long t2;
    SVECTOR *v0;
    TMD_P_TNG4 *primitive;
    u_long *po;
    u_char b;
    SVECTOR *v3;
    SVECTOR *v2;
    SVECTOR *v1;
    u_long *vp;
    u_long shiftWord;

    work = wp;
    hwd = HWD0;
    *work = 4; /* limit */
    po = work + 0x38;
    vp = po;
    vwd = VWD0;
    *(short *)(work + 0xd) = (short)(hwd / 2);       /* adivw */
    *(short *)((int)work + 0x36) = (short)(vwd / 2); /* adivh */
    t0 = *(u_long *)(ot + 4);
    shiftWord = shift;
    work[8] = 150;                       /* adivz */
    work[3] = shiftWord;                 /* shift */
    *(u_char *)((int)work + 0x4f) = 0xc; /* packet len */
    code = 0x3c;
    work[5] = (u_long)packet;             /* out */
    *(u_char *)((int)work + 0x53) = code; /* packet code */
    work[4] = t0;                         /* org */
    if (count != 0)
    {
        v0 = (SVECTOR *)(work + 0x20); /* v[0..3] */
        v1 = (SVECTOR *)(work + 0x26);
        v2 = (SVECTOR *)(work + 0x2c);
        v3 = (SVECTOR *)(work + 0x32);
        cd = code;
        primitive = (TMD_P_TNG4 *)primtop;
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
                work[0x28] = *(u_long *)&primitive->r1;
                *(u_char *)((int)work + 0xa3) = b;
                work[0x2e] = *(u_long *)&primitive->r2;
                *(u_char *)((int)work + 0xbb) = b;
                work[0x34] = *(u_long *)&primitive->r3;
                *(u_char *)((int)work + 0xd3) = b;
                gte_stsxy(work + 0x35);
                t0 = (u_long)(work + 0x24);
                t2 = (u_long)(work + 0x2a);
                t1 = (u_long)(work + 0x30);
                gte_stsz4((u_long *)t0, (u_long *)t2, (u_long *)t1, work + 0x36);
                *(short *)((int)work + 0x5a) = primitive->clut;  /* packet.clut */
                *(short *)((int)work + 0x66) = primitive->tpage; /* packet.tpage */
                subdivide_quad_(po, work, 0);
            }
            count--;
            primitive++;
        } while (count != 0);
    }
    return (u_long *)work[5]; /* out */
}
