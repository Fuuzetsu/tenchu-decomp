#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadTIMpack(unsigned long *adr);
 *     3DCTRL.C:759, 39 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned long * adr
 *     stack sp+16     struct RECT rect
 *     stack sp+24     struct GsIMAGE tim
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — exact 276-byte / 69-instruction extent, with 12
 * linked and whole-image bytes different. The candidate also has the target's
 * exact optimized CFG: 4 conditional branches, no unconditional jumps, 5
 * calls, and 1 return. The full prologue, outer loop, both guards, both call
 * sites, increment/compare tail, and epilogue are exact.
 *
 * This checkpoint closes the old one-instruction-short reorg tie with two
 * source-level phase boundaries. Naming `tim.clut` and ending a weight-free
 * empty one-shot loop immediately after its assignment keeps the call's $a1
 * producer ahead of the four CLUT geometry loads. Wrapping only the second
 * LoadImage call in a one-shot loop then makes reorg duplicate `i + 1` into
 * the CLUT guard's delay slot, exactly as retail does, without retaining a
 * branch or changing the runtime CFG.
 *
 * The remaining six aligned lines are one coupled allocation/schedule island.
 * Retail loads cx/cy/cw/ch into $v0/$v1/$a2/$a3, materializes `&r` in $a0,
 * then stores all four values; this draft uses $a0 for cw and $a2 for ch,
 * stores them, and sinks the `&r` addiu to the call delay slot. A bounded
 * 180-candidate guided loop/boundary/identical-arm search did not beat 12.
 * Focused producer temporaries, explicit pointer lifetimes, call-argument
 * comma forms, identical-arm pointer donors, all call-loop ranges, and
 * duplicated-increment/goto shapes were flat or regressed. Keep the guard;
 * the 12-byte pure-C checkpoint is useful, but not a match.
 *
 * LoadTIMpack (0x800189b4, 0x114 bytes) — LoadTIM.c's "pack" twin (same TU):
 * a packed archive's header is `adr[1]` (the element COUNT, low halfword)
 * followed by a table of ulong byte-offsets (`adr+2`, one per element,
 * walked with a plain walking pointer since only one field is touched per
 * iteration); each offset locates a TIM whose own leading ID word is
 * skipped exactly like LoadTIM.c/GetTIMpackInfo.c ("skip the leading
 * u_long ID word" convention) before handing it to GsGetTimInfo, then the
 * SAME pixel-then-optional-CLUT LoadImage pair as LoadTIM.c's body,
 * repeated per element. SystemOut is annotated noreturn by Ghidra but
 * falls straight through with no early return (LoadTIM.c's identical
 * idiom); DrawSync(0)'s return value IS this function's return value
 * (tail call, no separate sign-extend — the (short) truncation is a no-op
 * once nothing downstream reads the high bits).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `n` (adr[1], the pack's element count) is read via `lhu` and then
 *    explicit `sll 16`/`sra 16` sign-extends it for the loop's signed
 *    compare — same "narrow field feeds a widen-and-scale/extend pair"
 *    shape as CreateCloneModelArchive.c's `n`.
 *  - `i` is a plain `short` loop counter (GetTIMpackInfo.c's identical
 *    idiom in this same TU): combine proves the raw 32-bit accumulation
 *    need not be truncated at every `i + 1`, only the compare needs the
 *    16-bit view, so a throwaway sign-extended copy feeds `slt`.
 *  - THE WALKER IS `adr` ITSELF, not the `puVar2` copy. The address the loop
 *    hands GsGetTimInfo is `(int)base + table[k] + 4` where `base` (the fixed
 *    `adr+2` table origin) is constant and the table cursor advances — but the
 *    target keeps the INCOMING pointer register ($s0, from param `adr`) as the
 *    advancing cursor and saves the fixed base into a fresh callee reg ($s3).
 *    So write `adr` as the one that `adr = adr + 1`s each iteration and let
 *    `puVar2 = adr;` be the saved fixed base read as `(int)puVar2 + adr[0]`;
 *    writing it the other way round (puVar2 walks, adr fixed) rotates every
 *    callee-saved register by one and mismatches the whole prologue/epilogue.
 *  - `tim.pmode` is read TWICE with different widths: the CLUT-bit test
 *    reloads the FULL `lw` (needs all 32 bits for `>> 3 & 1`), independent
 *    of any earlier access — different machine modes/uses don't CSE.
 */
typedef struct
{
    s16 x; /* 0x0 */
    s16 y; /* 0x2 */
    s16 w; /* 0x4 */
    s16 h; /* 0x6 */
} RECT;    /* 0x8 (PSYQ libgpu.h) */

extern void GsGetTimInfo(u_long *tim, GsIMAGE *im);
extern int LoadImage(RECT *rect, u_long *p);
extern void SystemOut(char *msg);
extern short DrawSync(int mode);
extern char D_800110C8[]; /* "NO IMAGE PACK DATA" */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/LoadTIMpack", LoadTIMpack);
#else

short LoadTIMpack(u_long *adr)
{
    RECT r;
    GsIMAGE tim;
    u_long *puVar2;
    u_long *clut;
    u16 uVar1;
    short n;
    short i;

    if (adr == 0) {
        SystemOut(D_800110C8);
    }
    adr = adr + 1;
    uVar1 = *(u16 *)adr;
    adr = adr + 1;
    i = 0;
    n = (short)uVar1;
    puVar2 = adr;
    if (0 < n) {
        do {
            GsGetTimInfo((u_long *)((int)puVar2 + adr[0] + 4), &tim);
            r.x = tim.px;
            r.y = tim.py;
            r.w = tim.pw;
            r.h = tim.ph;
            LoadImage(&r, tim.pixel);
            if ((tim.pmode >> 3 & 1) != 0) {
                clut = tim.clut;
                do {
                } while (0);
                r.x = tim.cx;
                r.y = tim.cy;
                r.w = tim.cw;
                r.h = tim.ch;
                do {
                    LoadImage(&r, clut);
                } while (0);
            }
            i = i + 1;
            adr = adr + 1;
        } while (i < n);
    }
    DrawSync(0);
}
#endif
