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

    /* The ADIV_* accessors derive scalar byte/word addresses from ADIV_WORK's
     * fields. Direct member stores do NOT match: they un-pin the interleaved
     * volatile parameter reads (see tmdfast.h and cookbook 3.13).
     * The read side keeps Sony's `u_long vertop` convention from the
     * GsTMDfast* siblings it is called beside, so a source vertex is
     * index * 8 off that base. */
    work = wp;
    hwd = HWD0;
    ADIV_WORD(work, limit) = 4;
    po = ADIV_WORD_ADDRESS(work, frame[0]);
    vp = po;
    vwd = VWD0;
    ADIV_SHORT(work, adivw) = (short)(hwd / 2);
    ADIV_SHORT(work, adivh) = (short)(vwd / 2);
    t0 = *(u_long *)(ot + 4);
    shiftWord = shift;
    ADIV_WORD(work, adivz) = 150;
    ADIV_WORD(work, shift) = shiftWord;
    setlen(&((ADIV_WORK *)work)->packet, GPU_POLY_GT4_LENGTH);
    code = GPU_POLY_GT4_CODE;
    ((ADIV_WORK *)work)->out = packet;
    setcode(&((ADIV_WORK *)work)->packet, code);
    ADIV_WORD(work, org) = t0;
    if (count != 0)
    {
        v0 = (SVECTOR *)ADIV_WORD_ADDRESS(work, v[0]);
        v1 = (SVECTOR *)ADIV_WORD_ADDRESS(work, v[1]);
        v2 = (SVECTOR *)ADIV_WORD_ADDRESS(work, v[2]);
        v3 = (SVECTOR *)ADIV_WORD_ADDRESS(work, v[3]);
        cd = code;
        primitive = (TMD_P_TNG4 *)primtop;
        do
        {
            ADIV_SHORT(work, v[0].pos.vx) =
                *(u_short *)(primitive->v0 * 8 + vertop);
            ADIV_SHORT(work, v[0].pos.vy) =
                *(u_short *)(primitive->v0 * 8 + vertop + 2);
            ADIV_SHORT(work, v[0].pos.vz) =
                *(u_short *)(primitive->v0 * 8 + vertop + 4);
            ADIV_SHORT(work, v[1].pos.vx) =
                *(u_short *)(primitive->v1 * 8 + vertop);
            ADIV_SHORT(work, v[1].pos.vy) =
                *(u_short *)(primitive->v1 * 8 + vertop + 2);
            ADIV_SHORT(work, v[1].pos.vz) =
                *(u_short *)(primitive->v1 * 8 + vertop + 4);
            ADIV_SHORT(work, v[2].pos.vx) =
                *(u_short *)(primitive->v2 * 8 + vertop);
            ADIV_SHORT(work, v[2].pos.vy) =
                *(u_short *)(primitive->v2 * 8 + vertop + 2);
            ADIV_SHORT(work, v[2].pos.vz) =
                *(u_short *)(primitive->v2 * 8 + vertop + 4);
            ADIV_SHORT(work, v[3].pos.vx) =
                *(u_short *)(primitive->v3 * 8 + vertop);
            ADIV_SHORT(work, v[3].pos.vy) =
                *(u_short *)(primitive->v3 * 8 + vertop + 2);
            ADIV_SHORT(work, v[3].pos.vz) =
                *(u_short *)(primitive->v3 * 8 + vertop + 4);
            *vp = (u_long)v0;
            vp[1] = (u_long)v1;
            vp[2] = (u_long)v2;
            vp[3] = (u_long)v3;
            gte_ldv3(v0, v1, v2);
            gte_rtpt();
            ADIV_SHORT(work, v[0].tu) = *(u16 *)&primitive->tu0;
            ADIV_SHORT(work, v[1].tu) = *(u16 *)&primitive->tu1;
            t2 = (u_long)ADIV_WORD_ADDRESS(work, v[0].sxy);
            gte_stsxy3((u_long *)t2,
                       ADIV_WORD_ADDRESS(work, v[1].sxy),
                       ADIV_WORD_ADDRESS(work, v[2].sxy));
            gte_nclip();
            ADIV_SHORT(work, v[2].tu) = *(u16 *)&primitive->tu2;
            ADIV_SHORT(work, v[3].tu) = *(u16 *)&primitive->tu3;
            gte_stopz(ADIV_WORD_ADDRESS(work, zmax));
            if (0 < (int)ADIV_WORD(work, zmax))
            {
                gte_ldv0(v3);
                gte_rtps();
                ADIV_WORD(work, v[0].col) = *(u_long *)&primitive->r0;
                b = (u_char)cd;
                ADIV_BYTE(work, v[0].col.cd) = b;
                ADIV_WORD(work, v[1].col) = *(u_long *)&primitive->r1;
                ADIV_BYTE(work, v[1].col.cd) = b;
                ADIV_WORD(work, v[2].col) = *(u_long *)&primitive->r2;
                ADIV_BYTE(work, v[2].col.cd) = b;
                ADIV_WORD(work, v[3].col) = *(u_long *)&primitive->r3;
                ADIV_BYTE(work, v[3].col.cd) = b;
                gte_stsxy(ADIV_WORD_ADDRESS(work, v[3].sxy));
                t0 = (u_long)ADIV_WORD_ADDRESS(work, v[0].sz);
                t2 = (u_long)ADIV_WORD_ADDRESS(work, v[1].sz);
                t1 = (u_long)ADIV_WORD_ADDRESS(work, v[2].sz);
                gte_stsz4((u_long *)t0, (u_long *)t2, (u_long *)t1,
                           ADIV_WORD_ADDRESS(work, v[3].sz));
                ((ADIV_WORK *)work)->packet.clut = primitive->clut;
                ((ADIV_WORK *)work)->packet.tpage = primitive->tpage;
                subdivide_quad_(po, work, 0);
            }
            count--;
            primitive++;
        } while (count != 0);
    }
    return ((ADIV_WORK *)work)->out;
}
