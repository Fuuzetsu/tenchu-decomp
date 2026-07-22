#include "common.h"
#include "main.exe.h"
#include "gte.h"

/*
 * drawF3 (0x8005d0d4, 0x128 bytes) — the flat-triangle (POLY_F3) primitive
 * renderer of the DrawTMD handler family, and the anchor for the restricted
 * gte.h inline-asm policy (docs/gte-policy.md).  The carved .s marks it a
 * "Handwritten function", and the calling convention below independently
 * supports that classification.
 *
 * STATUS: the guarded reference reconstruction is byte-exact (0/296).  The
 * original remains canonical assembly per config/handwritten-asm.txt.
 *
 * Fresh evidence explains why the final two instructions should not be forced
 * through invented C dependencies.  The demo body at 0x80016dc8 is all 296
 * bytes identical to retail, and PSX.SYM supplies no C translation-unit or
 * source-line records for it.  At the only former residual, the original reads
 * FLAG and immediately writes `addiu $s0,$zero,0`; cc1's RTL instead lowers
 * every C `code = 0` through movsi as `addu $s0,$zero,$zero`.  Splitting the
 * assignment from the flag test also lets reorg sink that move into the branch
 * delay slot, shortening the handler.
 *
 * COMPILER-PROVENANCE RE-AUDIT (2026-07-19). Replacing the helper with the
 * ordinary `gte_stflg_reg(flag); code = 0;` source was compiled through Sony
 * GCC 2.6.3, 2.7.2, 2.8.0, 2.8.1, 2.91.66 and 2.95.2. All six emit the same
 * 292-byte handler and the same tail at this site: `cfc2; and; bnez; move`,
 * with the zeroing move in the branch delay slot. None emits the target's
 * 296-byte `cfc2; addiu s0,zero,0; and; bnez; nop`. The boundary is therefore
 * stable across the available PsyQ-era backends, not a 2.8.1 codegen quirk.
 *
 * The local READ_FLAG_AND_CLEAR_CODE helper therefore preserves those adjacent
 * handwritten instructions as one assembly unit.  Everything around it remains
 * the coherent C reconstruction: all COP2 moves, the three GTE commands, the
 * deferred ordering-table insertion, and every guard delay slot match exactly.
 * The default build still selects the canonical INCLUDE_ASM body.
 *
 * Non-ABI entry — the DrawTMD dispatcher hands the handler its working state in
 * live registers (no stack frame, no callee-save):
 *   $v0 ($2)  batch primitive count (0x304 sentinel => 1)
 *   $t0 ($8)  deferred ordering-table slot from the previous primitive
 *   $t2 ($10) ordering-table base
 *   $t3 ($11) output packet cursor (POLY_F3, 0x14 bytes each)
 *   $t4 ($12) transformed-vertex (SVECTOR) base
 *   $t5 ($13) input primitive cursor (0x10 bytes each)
 *   $t6 ($14) running item budget, decremented by this batch's count
 *   $t9 ($25) ordering-table length (depth clamp)
 *   $s0 ($16) pending-primitive code byte carried across iterations (0 = none)
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/drawF3", drawF3);
#else

/* The input record is PsyQ's TMD_P_NF3, also recorded under that exact name
 * in PSX.SYM; its four color bytes form the packed GTE RGB/code word. */

/* $s0 is the only callee-saved register the handler clobbers.  A file-scope
 * global register variable reserves it WITHOUT a prologue save/restore — the
 * DrawTMD dispatcher owns the save, exactly the non-ABI handler contract.
 * $t6 (budget) and $t3 (packet) are handed back to the dispatcher updated, so
 * their final writes must not be dead-eliminated / sunk into the return delay
 * slot: global register variables keep the stores in place. */
register int code   __asm__("$16");
register int budget __asm__("$14");
register u_long *packet __asm__("$11");

/* This handler is an assembly original.  Keep its adjacent FLAG read and
 * pending-code clear as the one handwritten unit they were. */
#define READ_FLAG_AND_CLEAR_CODE(flag, pending)                                \
    __asm__ volatile("cfc2\t%0, $31;addiu\t%1, $zero, 0"                      \
                     : "=r"(flag), "=r"(pending))

void drawF3(void)
{
    register int     r_v0    __asm__("$2");
    register u_long *ot_in   __asm__("$8");
    register int     ot_base __asm__("$10");
    register int     vbase   __asm__("$12");
    register TMD_P_NF3 *prim __asm__("$13");
    register int     ot_max  __asm__("$25");

    register SVECTOR *r0 __asm__("$4");
    register SVECTOR *r1 __asm__("$5");
    register SVECTOR *r2 __asm__("$6");
    register int      n    __asm__("$15");
    register int      half __asm__("$9");
    register int      mask __asm__("$3");
    u_long *ot_slot;
    int flag;
    int mac0;

    ot_slot = ot_in;
    /* The temporary subtraction survives long enough to preserve operand
     * order, then combine reduces this back to the target equality branch. */
    mask = 0x304u - (u32)r_v0;
    n = 1;
    if (mask != 0)
    {
        n = r_v0;
    }
    budget = budget - n;
loop:
    {
        r0 = (SVECTOR *)(prim->v0 * 8);
        r1 = (SVECTOR *)(prim->v1 * 8);
        r2 = (SVECTOR *)(prim->v2 * 8);
        r0 = (SVECTOR *)((int)r0 + vbase);
        r1 = (SVECTOR *)((int)r1 + vbase);
        r2 = (SVECTOR *)((int)r2 + vbase);
        gte_ldv3(r0, r1, r2);
        mask = 0x1C66000;
        gte_rtpt_raw();
        if (code != 0)
        {
            *ot_slot = (u_long)packet;
            *((u8 *)ot_slot + 3) = 0;
            gte_strgb_mem(packet[1]);
            *((u8 *)packet + 7) = (u8)code;
            packet += 5;
        }
        READ_FLAG_AND_CLEAR_CODE(flag, code);
        if ((flag & mask) == 0)
        {
            gte_nclip_raw();
            gte_stsz3r(r0, r1, r2);
            ot_slot = (u_long *)((int)r0 + (int)r1);
            ot_slot = (u_long *)((int)ot_slot + (int)r2);
            half = (u_long)ot_slot >> 2;
            ot_slot = (u_long *)((u_long)ot_slot >> 4);
            gte_stmac0(mac0);
            ot_slot = (u_long *)((u_long)ot_slot + half);
            ot_slot = (u_long *)((u_long)ot_slot >> 4);
            if (mac0 > 0)
            {
                half = (int)ot_slot - ot_max;
                if ((int)ot_slot > 0)
                {
                    ot_slot = (u_long *)((int)ot_slot * 4);
                    if (half < 0)
                    {
                        /* The GTE consumes r0/g0/b0/code as one packed word. */
                        gte_ldrgb_mem(*(u_long *)&prim->r0);
                        ot_slot = (u_long *)((int)ot_slot + ot_base);
                        mac0 = *ot_slot;
                        gte_dpcs_raw();
                        *packet = mac0;
                        mask = 4;
                        *((u8 *)packet + 3) = mask;
                        gte_stsxy3_f3(packet);
                        code = 0x20;
                    }
                }
            }
        }
        n = n - 1;
        prim += 1;
    }
    if (n > 0)
    {
        goto loop;
    }
    if (code != 0)
    {
        *ot_slot = (u_long)packet;
        *((u8 *)ot_slot + 3) = 0;
        gte_strgb_mem(packet[1]);
        *((u8 *)packet + 7) = (u8)code;
        packet += 5;
    }
}

#endif
