#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

/*
 * fast_tng3_ (0x8005a3cc, 0x3d8 bytes) — DecodeTMD-family primitive
 * renderer, the 1.00 mnemonic clone of fast_tnf3_ (TMD primitive code
 * 0x25's gradation sibling in decode_tmd_fast_'s switch). The ONLY
 * differences from fast_tnf3_ are the Gouraud record's three distinct
 * colour words and its 0x24-byte stride (the flat record is 0x1c bytes).
 * Everything else — including every matching note below — is
 * fast_tnf3_.c verbatim; read that file's header for the full
 * mechanism account.
 * Builds one POLY_GT3 (Gouraud-shaded, textured triangle, GPU code 0x34)
 * output packet per input record: transforms the record's 3 vertex indices
 * through the GTE (RTPT), discards back-facing/degenerate triangles (NCLIP
 * MAC0 <= 0), discards triangles whose screen-space bbox misses the caller's
 * clip rectangle, computes an OTZ bucket index from the max depth / 4,
 * applies depth-cueing (DPCS) to each vertex's baked colour when beyond the
 * far-fog range, then copies the finished packet into the caller's output
 * list and re-links the caller's OT bucket to point at it.
 *
 * param_5 ("work") is the shared per-call rendering context also used by the
 * sibling pair adiv_tng4_/adiv_tnf4_: +0x34 POLY_GT3 staging, +0x38 the
 * staging code byte's word, +0x5c/+0x60/+0x64 per-vertex SZ, +0x74 GTE FLAG,
 * +0x78 OTZ index, +0x80 NCLIP/MAC0 winding, +0x84 near-Z reject threshold,
 * +0x88 OT bucket shift, +0x8c far-fog Z, +0x90 indirect OT table pointer,
 * +0x94/+0x98/+0x9c/+0xa0 screen clip box.
 *
 * Matching notes (this file applies the adiv_tng4_ recipe; read that
 * header for the full mechanism account):
 *  - The dual-view record retains the original TMD_P_TNG3 layout behind the
 *    target's ONE record cursor. Normal -O2 strength reduction is correct;
 *    the old byte-pointer draft split named fields into a second GIV and
 *    falsely made -fno-strength-reduce look necessary for this artificial
 *    file. Real do-while loop notes stay ON (ref weighting drives the priority
 *    allocation that fills every caller-saved register).
 *  - The 40-byte packet copy is a STRUCT ASSIGNMENT, not a hand loop:
 *    mips.c expand_block_move (40 > 2*MAX_MOVE_BYTES) emits copy_addr_to_reg
 *    copies (move t2,a2 / move t0,t3), final_src (addiu a0,t0,0x20), a
 *    16-byte movstrsi_internal loop and an 8-byte straight remainder. The
 *    lw/sw temps are the FOUR match_scratch "=&d" clobbers of that one insn,
 *    assigned by RELOAD from potential_reload_regs — and order_regs_for_reload
 *    (no REG_ALLOC_ORDER) lists [unused caller-saved ascending] then [unused
 *    callee-saved ascending]. Every caller-saved reg is occupied by a pseudo
 *    here, so the scratches get s0-s3: the target's copy-loop registers and
 *    its entire prologue (only s0-s3 saved) fall out of reload, not the
 *    allocator. No C-level copy temps can reproduce that.
 *  - gte_stflg is the SDK INLINE_N.H pointer form (cfc2 $12; nop; sw $12) —
 *    the target's cfc2 t4 is the macro's HARDCODED staging register, not an
 *    allocated variable (v1/a0/t0 are free there; no "=r" local can reach
 *    t4). gte_ldrgb/gte_strgb likewise take the POINTER ("r"), keeping the
 *    rgbPtr addiu materialised (the "m" forms folded it into the lwc2).
 *  - The three invariant GTE address temps (work+0x74/+0x80/+0x64) are
 *    IN-MACRO-ARG expressions, exactly like the matched sibling's
 *    gte_stopz(pkt + 6): each is a fresh single-use temp that local_alloc
 *    colours v0, scheduled BELOW the preceding mem-to-mem copy (the target's
 *    stall nops before them are real: an own-statement `addr =` variable
 *    instead gets backward-scheduled into the load-delay bubble, overlaps
 *    the colour temp's v0 life, and is exiled to v1 — measured, 3 sites).
 *    58-threshold move_movables does NOT hoist them (score ~116-174 vs
 *    insn_count ~210).
 *  - Min/max ladders: `lo` doubles as the FIRST operand (no separate `a`),
 *    and `hi = lo` sits INSIDE the then-arm: `if (b < lo) { hi = lo;
 *    lo = b; } else hi = b;`. Outside the arm, optimize_reg_copy_1
 *    propagates the copy into the compare (slt reads hi), transferring lo's
 *    refs to hi and inverting their allocation ranks; inside, the immediate
 *    `lo = b` redefinition blocks the scan. reorg then steals hi=lo (the
 *    fallthrough leader) into the beqz delay slot — it executes on both
 *    paths, dead on the else path.
 *  - The SZ ladder's max is a SEPARATE variable from the screen ladders'
 *    (`otz`, the depth that becomes the OT bucket index — its /4's three
 *    RTL mentions plus z1<otz would otherwise push a merged `hi` to 36
 *    weighted refs / live 50 = priority 36000, outranking lo's 42/65 =
 *    32307 and stealing a0). Split: lo(42w) > otz(29090) > hi(28571), so
 *    lo->a0 first and both maxes share t0 (disjoint lives).
 *  - DPCS blocks: two reused scratch locals. z1 = farZ guard (block 1) then
 *    sz1/sz2 (blocks 2/3); z2 = sz0 (block 1) then the farZ RELOADS (blocks
 *    2/3, one load each, held across dpcs). Each field is read ONCE per
 *    block into its local; the register role swap between block 1 and
 *    blocks 2/3 (a0/v1) is exactly this variable identity. Each strgb
 *    guard's CONDITION is written into the dying z2 itself (`z2 = z1 < z2;
 *    if (z2 != 0)`) — one variable one register, so the slt/beqz pair reads
 *    v1 where a fresh compare temp would take v0.
 *  - The OTZ index is a plain signed otz/4 (bgez/addiu 3/sra 2), and
 *    `*(work+0x78) = otz/4` precedes the near-Z guard so the store lands in
 *    the reject branch's delay slot.
 *  - `prim = work + 0x34` sits ABOVE the `if (param_4 != 0)` guard: it is
 *    the only insn reorg's backward scan may steal into the entry beqz's
 *    delay slot (inside the guard, the scan takes `sw s0,0(sp)` instead and
 *    the whole prologue rotates). Dead but harmless when param_4 == 0.
 */

u_long *fast_tng3_(TmdTexturedGouraudTriangleRecord *record, VERT *vertices,
                   u_long *packet,
                   int count, TMD_FAST_WORK *wp)
{
    TMD_FAST_WORK *work;
    GpuPolyGT3Packet *prim;
    GpuColorWord *color;
    s32 codeVal;
    u_long *sz0Ptr;
    u_long *sz1Ptr;
    u_long *rgbPtr;
    u_long *otSlot;
    u32 idx0, idx1, idx2;
    s32 b, c, lo, hi, otz;
    s32 z1, z2;

    work = wp;
    prim = &work->gt3;
    if (count != 0)
    {
        color = &work->gt3.gpu.vertex[0].color;
        codeVal = GPU_POLY_GT3_CODE;
        sz0Ptr = (u_long *)&work->sz[0];
        sz1Ptr = (u_long *)&work->sz[1];
        do
        {
            idx0 = record->stream.vertex[0];
            idx1 = record->stream.vertex[1];
            idx2 = record->stream.vertex[2];
            gte_ldv3(TMD_VERTEX_AT(vertices, idx0),
                     TMD_VERTEX_AT(vertices, idx1),
                     TMD_VERTEX_AT(vertices, idx2));
            gte_rtpt();

            prim->gpu.vertex[0].texture.word = record->stream.texture[0].word;
            prim->gpu.vertex[1].texture.word = record->stream.texture[1].word;
            prim->gpu.vertex[2].texture.word = record->stream.texture[2].word;
            prim->gpu.vertex[0].color.word = record->stream.color[0].word;
            color->channel.cd = codeVal;
            gte_stflg((u_long *)&work->flag);
            if (work->flag < 0)
                goto next;

            gte_nclip();
            prim->gpu.vertex[1].color.word = record->stream.color[1].word;
            prim->gpu.vertex[2].color.word = record->stream.color[2].word;
            gte_stopz((u_long *)&work->opz);
            if (work->opz <= 0)
                goto next;

            gte_stsxy3_gt3(&prim->packet);

            lo = prim->packet.x0;
            b = prim->packet.x1;
            if (b < lo)
            {
                hi = lo;
                lo = b;
            }
            else
            {
                hi = b;
            }
            c = prim->packet.x2;
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

            lo = prim->packet.y0;
            b = prim->packet.y1;
            if (b < lo)
            {
                hi = lo;
                lo = b;
            }
            else
            {
                hi = b;
            }
            c = prim->packet.y2;
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

            gte_stsz3(sz0Ptr, sz1Ptr, (u_long *)&work->sz[2]);
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
            work->otz = otz / 4;
            if (work->farz < lo)
                goto next;

            z1 = work->fogz;
            if (z1 < otz)
            {
                rgbPtr = &prim->gpu.vertex[0].color.word;
                z2 = work->sz[0];
                gte_ldrgb(rgbPtr);
                gte_lddp(z2 - z1);
                gte_dpcs();
                z2 = z1 < z2;
                if (z2 != 0)
                {
                    gte_strgb(rgbPtr);
                }

                rgbPtr = &prim->gpu.vertex[1].color.word;
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

                rgbPtr = &prim->gpu.vertex[2].color.word;
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
            }

            otSlot = (u_long *)work->ot->org + (work->otz >> work->shift);
            prim->packet.tag = *otSlot;
            setlen(&prim->packet, GPU_POLY_GT3_LENGTH);
            *(GpuPolyGT3Packet *)packet = *prim;
            *otSlot = (u_long)packet & GPU_DMA_ADDRESS_MASK;
            packet += GPU_POLY_GT3_WORDS;

        next:
            count--;
            record++;
        } while (count != 0);
    }
    return packet;
}
