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
 * model: spelling the input with its real TMD_P_TNG4 layout gives loop.c one
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

u_long *adiv_tng4_(TmdTexturedGouraudQuadRecord *primitive, VERT *vertices,
                   u_long *packet,
                   int count, volatile u_long shift, GsOT *volatile ot,
                   u_long *wp)
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
     * The read side uses Sony's VERT table directly: each primitive index
     * selects one authored position instead of rebuilding its byte address. */
    work = wp;
    hwd = HWD0;
    ADIV_WORD(work, limit) = 4;
    po = ADIV_WORD_ADDRESS(work, frame[0]);
    vp = po;
    vwd = VWD0;
    ADIV_SHORT(work, adivw) = (short)(hwd / 2);
    ADIV_SHORT(work, adivh) = (short)(vwd / 2);
    t0 = (u_long)ot->org;
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
                ADIV_WORD(work, v[0].color.word) = primitive->stream.color[0].word;
                b = (u_char)cd;
                ADIV_BYTE(work, v[0].color.channel.cd) = b;
                ADIV_WORD(work, v[1].color.word) = primitive->stream.color[1].word;
                ADIV_BYTE(work, v[1].color.channel.cd) = b;
                ADIV_WORD(work, v[2].color.word) = primitive->stream.color[2].word;
                ADIV_BYTE(work, v[2].color.channel.cd) = b;
                ADIV_WORD(work, v[3].color.word) = primitive->stream.color[3].word;
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
                subdivide_quad_(po, work, 0);
            }
            count--;
            primitive++;
        } while (count != 0);
    }
    return ((ADIV_WORK *)work)->out;
}
