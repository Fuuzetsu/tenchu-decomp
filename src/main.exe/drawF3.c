#include "common.h"
#include "main.exe.h"
#include "tmdfast.h"
#include "gte.h"

/*
 * Canonical handwritten DrawTMD handler (docs/gte-policy.md). The dispatcher
 * supplies a non-ABI live-register set, and the handler directly schedules
 * GTE operations and packet writes.
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
register int code __asm__("$16");
register int budget __asm__("$14");
register POLY_F3 *packet __asm__("$11");

/* This handler is an assembly original.  Keep its adjacent FLAG read and
 * pending-code clear as the one handwritten unit they were. */
#define READ_FLAG_AND_CLEAR_CODE(flag, pending)          \
    __asm__ volatile("cfc2\t%0, $31;addiu\t%1, $zero, 0" \
                     : "=r"(flag), "=r"(pending))

void drawF3(void)
{
    register int r_v0 __asm__("$2");
    register u_long *ot_in __asm__("$8");
    register int ot_base __asm__("$10");
    register int vbase __asm__("$12");
    register TMD_P_NF3 *prim __asm__("$13");
    register int ot_max __asm__("$25");

    register SVECTOR *r0 __asm__("$4");
    register SVECTOR *r1 __asm__("$5");
    register SVECTOR *r2 __asm__("$6");
    register int n __asm__("$15");
    register int half __asm__("$9");
    register int mask __asm__("$3");
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
        setlen(ot_slot, 0);
        gte_strgb_mem(*(u_long *)&packet->r0);
        packet->code = (u8)code;
        packet++;
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
                    packet->tag = mac0;
                    mask = GPU_POLY_F3_LENGTH;
                    setlen(packet, mask);
                    gte_stsxy3_f3(packet);
                    code = GPU_POLY_F3_CODE;
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
        setlen(ot_slot, 0);
        gte_strgb_mem(*(u_long *)&packet->r0);
        packet->code = (u8)code;
        packet++;
    }
}

#endif
