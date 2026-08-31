#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupTelop(unsigned char *telop);
 *     CHRANIM.C:372, 44 src lines, frame 560 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       unsigned char * telop
 *     reg   $a2       short * font
 *     stack sp+16     short [16][16] bitmap
 *     reg   $s0       short n
 *     reg   $t0       short u
 *     reg   $t1       short v
 *     stack sp+528    struct RECT rect
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_FT4 TelopP;
 * END PSX.SYM */

/* STATUS: MATCHED — exact 1076 bytes / 269 instructions.
 *
 * ROUND 5 disproved the parked allocation floor by replacing the byte-chased
 * glyph loop as one coherent source package.  The retail target preserves the
 * demo's human structure: the PSX.SYM `u` local is the inner pixel counter;
 * `bitmap[v][15 - u]` is one ternary assignment; and the font-selection
 * if/else is plain control flow.  Removing the five nested one-shot fences
 * around that selection demoted `font` naturally, so `bits`/font/u landed in
 * the target $a1/$a2/$t0 homes with no carrier or weighting trick.  Writing
 * `v = 0` before `fill_white = 0x7fff` then put the zero init in the guard's
 * delay slot and closed the final two-instruction reorder.
 *
 * ROUNDS 1–4 below are retained as a falsified local-minimum autopsy: their
 * priority and conflict measurements were accurate for the scaffolded draft,
 * but their conclusions did not transfer to the human decomposition.
 * (ROUND 3 improved that old draft from 11/1076; ROUND 4 parked it at 9.)
 *
 * Exact instruction sequence; the ORIGINAL residual (ROUND 1/2) was a 3-cycle
 * register rotation in the bitmap-fill loop:
 *     col (inner var)  ours $a1  <->  target $t0
 *     bits             ours $t1  <->  target $a1
 *     fill_white       ours $t0  <->  target $t1
 *
 * DIAGNOSIS (global.c allocno_compare, verified against all 25 allocnos of
 * this function via tools/regalloc.py):
 *     priority = floor_log2(n_refs) * n_refs / live_length * 10000
 * where n_refs is LOOP-DEPTH-WEIGHTED (each RTL mention counts its depth).
 * Measured here: col 16/30 -> 21333, font 19/43 -> 17674, v 35/111 -> 15765,
 * fill_white 19/80 -> 9500, bits 7/19 -> 7368. Allocation walks that order and
 * takes the lowest free reg (MIPS REG_ALLOC_ORDER v0,v1,a0,a1,a2,a3,t0,t1...),
 * which reproduces our a1,a2,a3,t0,t1 exactly.
 *
 * The target's registers require the order  bits > font > v > col > fill_white.
 * So bits must outrank font: >17674 needs n_refs >= 12 at live_length 19.
 * bits has exactly TWO RTL mentions -- def at loop depth 3 (+3) and one use at
 * depth 4 (+4) = 7, and that is forced by the (already byte-identical) insn
 * sequence. Every extra mention costs an instruction. The only other way to
 * claim $a1 from last place is a hard-reg preference (regs_someone_prefers),
 * and gcc-2.8.1 records those ONLY from a pseudo<->hard-reg copy, i.e. a call;
 * loop 1 contains none. Both levers are therefore out of reach from C.
 *
 * MEASURED, so the next lane need not re-derive (all rebuilt + matchdiff'd):
 *   - col == u merged (PSX.SYM lists one `u`):  11 -> 38. Refuted. Target's
 *     loop-1 inner var and loop-2 `u` share $t0 but are NOT one variable --
 *     merging raises n_refs, so the pseudo allocates EARLIER and takes a LOWER
 *     reg ($a2), displacing font.
 *   - `*pixel = 0x7fff` literal instead of fill_white: LENGTH 1084 != 1076.
 *     Refuted -- fill_white must be a real local despite PSX.SYM's 7-local list.
 *   - dropping `final_u = fill_white`: 11 -> 33, but it DOES move fill_white
 *     t0->t1 (correct!) by removing one depth-4 ref. This is the one verified
 *     lever on this residual; it regresses the tail because final_u is reused
 *     at the GetTPage block.
 *   - byte-swap inlined into the `if` (as Ghidra renders it): 11 -> 13. loop.c
 *     still hoists it to 0x80057384, and the rotation is UNCHANGED -- an
 *     explicit variable and a loop-hoisted temp score identically. It also
 *     shows the target's srl-before-sll needs the two-statement form
 *     (`bits = raw_bits >> 8; bits |= raw_bits << 8;`); the fused expression
 *     flips the operand order.
 *   - tools/autorules.py: no improving edit among 73 candidates.
 *   - tools/permute.py, bounded 300s -j4 --stop-on-zero: no improvement.
 *
 * The ROUND 1-4 autopsy (the falsified local minimum) lives in
 * docs/matching-archive.md.
 */
extern s16 TelopFont[];

extern s16 *Krom2RawAdd(u32 code);
extern void *memset(void *dst, int value, u32 size);

void SetupTelop(u8 *telop, short line)
{
    s16 bitmap[16][16];
    RECT rect;
    s16 n;
    s16 u;
    s16 v;
    s16 *font;
    s16 bits;
    u16 raw_bits;
    s16 north;
    s16 fill_white;
    s16 outline_white;
    s32 scaled_y;
    s32 line_y;
    s32 signed_v;
    s32 final_v;
    s32 final_v2;

    TelopP.u1 = 0;
    TelopP.u0 = 0;
    if ((*telop & 0x80) != 0 && (telop[2] & 0x80) != 0)
    {
        scaled_y = line * 16;
        rect.x = 0x300;
        rect.y = 0x1f0 - scaled_y;
        rect.w = 0x100;
        rect.h = 0xf;
        line_y = scaled_y;
        ClearImage(&rect, 0, 0, 0);
        DrawSync(0);
        rect.w = 0x10;
        rect.h = 0xf;

        if (*telop == 0)
        {
            TelopP.u1 = 0;
            TelopP.u0 = 0;
            return;
        }

        n = 0;
        while (1)
        {
            if (n >= 0x20)
            {
                break;
            }

            if (telop[n] == 0x81 && telop[n + 1] == 0x99)
            {
                font = TelopFont;
            }
            else
            {
                font = Krom2RawAdd((telop[n] << 8) | telop[n + 1]);
            }

            if (font != (s16 *)-1)
            {
                v = 0;
                fill_white = 0x7fff;
                do
                {
                    raw_bits = font[v];
                    bits = raw_bits >> 8;
                    bits |= raw_bits << 8;
                    u = 0;
                    do
                    {
                        bitmap[v][15 - u] = ((bits >> u) & 1) ? fill_white : 0;
                        u++;
                    } while (u < 16);
                    v++;
                } while (v < 15);

                outline_white = 0x7fff;
                u = 1;
                v = 1;
                do
                {
                    if (bitmap[v][u] == 0)
                    {
                        north = bitmap[v - 1][u];
                        if ((north == outline_white && bitmap[v][u - 1] == outline_white) ||
                            (bitmap[v][u - 1] == outline_white && bitmap[v + 1][u] == outline_white) ||
                            (bitmap[v + 1][u] == outline_white && bitmap[v][u + 1] == outline_white) ||
                            (bitmap[v][u + 1] == outline_white && north == outline_white))
                        {
                            bitmap[v][u] = 0x1ce7;
                        }
                    }
                    u++;
                    signed_v = v;
                    if (u >= 15)
                    {
                        u = 1;
                        v = signed_v + 1;
                    }
                    else
                    {
                        v = signed_v;
                    }
                } while (v < 14);

                LoadImage(&rect, (u_long *)bitmap);
                DrawSync(0);
                rect.x += rect.w;
            }

            n += 2;
            if (telop[n] == 0)
            {
                break;
            }
        }

        memset(&TelopP, 0xff, sizeof(TelopP));
        final_v = SCREEN_H - line_y;
        final_v2 = (u8)rect.h + final_v;
        u = (u16)rect.x - 0x301;
        setlen(&TelopP, 9);
        TelopP.code = 0x2c;
        TelopP.u2 = 0;
        TelopP.u0 = 0;
        TelopP.v1 = final_v;
        TelopP.v0 = final_v;
        TelopP.u3 = u;
        TelopP.u1 = u;
        TelopP.v3 = final_v2;
        TelopP.v2 = final_v2;
        TelopP.tpage = GetTPage(2, 0, 0x300,
                                0x1f0 - (s16)line_y);
    }
}
