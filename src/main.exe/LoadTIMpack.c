#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadTIMpack(unsigned long *adr);
 *     3DCTRL.C:759, 39 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 * STATUS: NON_MATCHING — 1 instruction short (68 vs 69), residual is a single
 * reorg delay-slot-fill tie (the named permuter-immune class in
 * docs/matching-cookbook.md's "guard's DELAY-SLOT FILL tie"). Everything else
 * byte-matches: after making `adr` itself the walking cursor (so the incoming
 * pointer register $s0 stays the walker and the fixed table base is the saved
 * copy in $s3 — exactly the target's register roles) the whole loop, the
 * i-in-$s1/n-in-$s2 allocation, the `i+1`-via-$v0-then-`move $s1`-reused-in-the-
 * compare form, and the `adr++` in the loop-back branch's delay slot are all
 * identical. The ONLY diff: the CLUT-present guard `beqz` at 0x80018a54.
 *   - Target: reorg DUPLICATES the branch-taken target's head instruction
 *     (`addiu $v0,$s1,1`, the no-CLUT path's own `i+1`) into the beqz delay
 *     slot and retargets the branch +1 (to the shared `move $s1,$v0` join) —
 *     making the function 69 insns.
 *   - Ours: reorg instead fills that same delay slot from the FALL-THROUGH
 *     (the CLUT block's first instruction, `addiu $a0,$sp,16` — the second
 *     LoadImage's RECT-pointer arg), leaving the branch pointed straight at
 *     the single `addiu $v0,$s1,1`/`move $s1,$v0` — 68 insns.
 * Both candidate fills are equally ready and data-independent of the CLUT bit
 * ($v0), so which one reorg picks is a scheduler tie with no source lever.
 * Confirmed permuter-immune: a bounded `tools/permute.py LoadTIMpack` run
 * (400s / j4, killed at the timeout) plateaued at score 40 and never
 * approached 0; manual attempts at the alternative shapes all diverged (an
 * `if(==0){i++}else{...;i++}` else-duplication inverted the branch polarity to
 * `bnez`; an `if(!=0){...;i++}else{i++}` form duplicated `adr++` too, going one
 * insn LONG at 70). Parked per the cookbook's sub-C-level early-stop.
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
                r.x = tim.cx;
                r.y = tim.cy;
                r.w = tim.cw;
                r.h = tim.ch;
                LoadImage(&r, tim.clut);
            }
            i = i + 1;
            adr = adr + 1;
        } while (i < n);
    }
    DrawSync(0);
}
#endif
