#ifndef GTE_H
#define GTE_H

/*
 * Restricted inline-asm layer for the GTE/COP2 class — the ONLY sanctioned
 * use of __asm__ in matched code, and only in functions on the whitelist in
 * docs/gte-policy.md. Macro names mirror PsyQ's INLINE_N.H, the original
 * SDK's own inline-GTE layer: these operations never had a C spelling in the
 * original source either.
 */

#define gte_ldDQA(x) __asm__ volatile("ctc2\t%0, $27" : : "r"(x))
#define gte_ldDQB(x) __asm__ volatile("ctc2\t%0, $28" : : "r"(x))

/* ---- COP2 data moves (native mnemonics) ---- */

/* Load 1 vertex into VXY0/VZ0 from an SVECTOR pointer. */
#define gte_ldv0(r0)                                                           \
    __asm__ volatile("lwc2\t$0, 0(%0);lwc2\t$1, 4(%0)" : : "r"(r0))

/* Load 3 vertices into VXY0/VZ0, VXY1/VZ1, VXY2/VZ2 from 3 SVECTOR pointers. */
#define gte_ldv3(r0, r1, r2)                                                   \
    __asm__ volatile("lwc2\t$0, 0(%0);lwc2\t$1, 4(%0);"                        \
                     "lwc2\t$2, 0(%1);lwc2\t$3, 4(%1);"                        \
                     "lwc2\t$4, 0(%2);lwc2\t$5, 4(%2)"                         \
                     : : "r"(r0), "r"(r1), "r"(r2))

/* Load the RGB/code word (C2 data reg 6) through a POINTER — verbatim PsyQ
 * INLINE_N.H form ("r" operand, so the address is materialised in a register
 * and never folded into the lwc2 displacement; FUN_80059ff4's target proves
 * the `addiu t0,t3,4; lwc2 $6,0(t0)` shape). */
#define gte_ldrgb(r0) __asm__ volatile("lwc2\t$6, 0( %0 )" : : "r"(r0))

/* Reconstruction helper for HANDWRITTEN asm (drawF3): the "m" spelling lets
 * cc1 fold a constant offset into the lwc2 displacement, which the compiled
 * INLINE_N.H form above cannot produce. */
#define gte_ldrgb_mem(mem) __asm__ volatile("lwc2\t$6, %0" : : "m"(mem))

/* Load a value into IR0 (C2 data reg 8) — the interpolation factor consumed
 * by DPCS/DPCT/INTPL. Verbatim from the real PsyQ SDK's INLINE_C.H, which
 * names this `gte_lddp` (it is the general "load DP" mover for the
 * depth-cueing pipeline, not specific to one command). */
#define gte_lddp(r) __asm__ volatile("mtc2\t%0, $8" : : "r"(r))

/* Store RGB2 (C2 data reg 22, the last depth-cued colour) through a POINTER —
 * verbatim PsyQ INLINE_N.H form. */
#define gte_strgb(r0) __asm__ volatile("swc2\t$22, 0( %0 )" : : "r"(r0) : "memory")

/* Reconstruction helper for HANDWRITTEN asm (drawF3): offset-foldable "m"
 * spelling, see gte_ldrgb_mem. */
#define gte_strgb_mem(mem) __asm__ volatile("swc2\t$22, %0" : "=m"(mem))

/* Store SXY0/SXY1/SXY2 (C2 data regs 12/13/14) into a POLY_F3 packet's
 * three screen-XY fields (offsets 8/0xC/0x10). */
#define gte_stsxy3_f3(p)                                                       \
    __asm__ volatile("swc2\t$12, 8(%0);swc2\t$13, 0xC(%0);swc2\t$14, 0x10(%0)" \
                     : : "r"(p))

/* Store SXY0/SXY1/SXY2 (C2 data regs 12/13/14) into a POLY_GT3 packet's
 * three screen-XY fields (offsets 8/0x14/0x20). Verbatim from the real PsyQ
 * SDK's INLINE_C.H (this SDK revision's INLINE_N.H equivalent) — Ghidra's
 * decompile of FUN_80059ff4/FUN_8005a3cc already guessed this exact name
 * from the offset pattern, confirming it against the extracted psyq4.5
 * headers rather than inventing a name. */
#define gte_stsxy3_gt3(p)                                                     \
    __asm__ volatile("swc2\t$12, 8(%0);swc2\t$13, 0x14(%0);swc2\t$14, 0x20(%0)" \
                     : : "r"(p))

/* Store SXY0/SXY1/SXY2 (C2 data regs 12/13/14) to three separate pointers. */
#define gte_stsxy3(a, b, c)                                                    \
    __asm__ volatile("swc2\t$12, 0(%0);swc2\t$13, 0(%1);swc2\t$14, 0(%2)"      \
                     : : "r"(a), "r"(b), "r"(c))

/* Store SXY2 (C2 data reg 14, the last transformed screen point) to memory. */
#define gte_stsxy(mem) __asm__ volatile("swc2\t$14, 0(%0)" : : "r"(mem))

/* Store MAC0 (C2 data reg 24, e.g. the NCLIP winding result) to memory. */
#define gte_stopz(mem) __asm__ volatile("swc2\t$24, 0(%0)" : : "r"(mem))

/* Store SZ1/SZ2/SZ3 (C2 data regs 17/18/19) to three separate pointers. */
#define gte_stsz3(r0, r1, r2)                                                  \
    __asm__ volatile("swc2\t$17, 0(%0);swc2\t$18, 0(%1);swc2\t$19, 0(%2)"      \
                     : : "r"(r0), "r"(r1), "r"(r2))

/* Store SZ0/SZ1/SZ2/SZ3 (C2 data regs 16/17/18/19) to four separate pointers. */
#define gte_stsz4(a, b, c, d)                                                  \
    __asm__ volatile("swc2\t$16, 0(%0);swc2\t$17, 0(%1);"                      \
                     "swc2\t$18, 0(%2);swc2\t$19, 0(%3)"                       \
                     : : "r"(a), "r"(b), "r"(c), "r"(d))

/* Read the FLAG control register (C2 control reg 31) and store it through a
 * POINTER — verbatim PsyQ INLINE_N.H form: the value stages through the
 * HARDCODED $12 (t4) with the cop-move hazard nop, and the store is inside
 * the asm. FUN_80059ff4's target (`cfc2 t4,$31; nop; sw t4,0(v0)`) is this
 * macro exactly; a "=r" spelling can never reach $12 there (v1/a0/t0 are all
 * free at that point and the allocator walks upward from $2). */
#define gte_stflg(r0)                                                          \
    __asm__ volatile("cfc2\t$12, $31;"                                         \
                     "nop;"                                                    \
                     "sw\t$12, 0( %0 )"                                        \
                     :                                                         \
                     : "r"(r0)                                                 \
                     : "$12", "memory")

/* Reconstruction helper for HANDWRITTEN asm (drawF3): read FLAG straight into
 * a C variable — the hand-scheduled shape, not the INLINE_N.H macro. */
#define gte_stflg_reg(r) __asm__ volatile("cfc2\t%0, $31" : "=r"(r))

/* Read SZ1/SZ2/SZ3 (C2 data regs 17/18/19) into CPU REGISTERS.
 * `gte_stsz3` proper is the memory-store form above (that is what INLINE_N.H's
 * `st` prefix means, and FUN_80057b80's target proves it with `swc2 $17..$19`).
 * This mfc2 sibling is a reconstruction helper for drawF3's OTZ average and is
 * NOT a standard INLINE_N.H name — hence the distinct `r` suffix. */
#define gte_stsz3r(r0, r1, r2)                                                 \
    __asm__ volatile("mfc2\t%0, $17;mfc2\t%1, $18;mfc2\t%2, $19"               \
                     : "=r"(r0), "=r"(r1), "=r"(r2))

/* Read MAC0 (C2 data reg 24) into a CPU register. */
#define gte_stmac0(r) __asm__ volatile("mfc2\t%0, $24" : "=r"(r))

/* ---- GTE command issues (as .word; mnemonic in the comment) ----
 * A GTE command reads the COP2 register file, which is not stable until two
 * cycles after the preceding COP2 data op (lwc2/swc2), and cc1 cannot schedule
 * across the opaque `.word`. The two spellings below are NOT interchangeable —
 * they model two different original sources, so pick by provenance:
 *
 *   gte_<cmd>()      the PsyQ INLINE_N.H-style macro a COMPILED caller used:
 *                    the macro itself absorbs the latency with two nops,
 *                    because at the call site the loads and the command are
 *                    adjacent (FUN_80058c70 is exactly 2 instructions short per
 *                    command without them).
 *   gte_<cmd>_raw()  the bare command, for reconstructions of HANDWRITTEN
 *                    assembly (config/handwritten-asm.txt), whose authors
 *                    scheduled the latency by hand with real work — drawF3
 *                    fills it with a `lui/ori`. Using the nop form there adds
 *                    6 instructions and breaks the carve length. */
#define gte_rtps()  __asm__ volatile("nop;nop;.word\t0x4A180001") /* RTPS  */
#define gte_rtpt()  __asm__ volatile("nop;nop;.word\t0x4A280030") /* RTPT  */
#define gte_nclip() __asm__ volatile("nop;nop;.word\t0x4B400006") /* NCLIP */
#define gte_dpcs()  __asm__ volatile("nop;nop;.word\t0x4A780010") /* DPCS  */

#define gte_rtps_raw()  __asm__ volatile(".word\t0x4A180001") /* RTPS  */
#define gte_rtpt_raw()  __asm__ volatile(".word\t0x4A280030") /* RTPT  */
#define gte_nclip_raw() __asm__ volatile(".word\t0x4B400006") /* NCLIP */
#define gte_dpcs_raw()  __asm__ volatile(".word\t0x4A780010") /* DPCS  */

#endif /* GTE_H */
