#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

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
 * MATCHED (276/276 bytes, 69 instructions, 0 whole-image diffs).
 *
 * The two LoadImage sites are byte-identical in retail apart from the `tim`
 * field offsets, so they are spelled identically here — the CLUT site is just
 * the pixel site again. An earlier checkpoint parked at 12 bytes by naming
 * `tim.clut` into a local and wrapping the second call in a one-shot loop;
 * both were scaffolding, and both were the cause of the 12 bytes rather than
 * a partial cure. Removing them fixes the register naming outright ($a2/$a3
 * for cw/ch, `&r` materialised early) because sched1 can then hoist the
 * `addiu $a0,$sp,0x10` above the four stores, which makes $a0 conflict with
 * the geometry temps and pushes them off $a0/$a1.
 *
 * The trailing empty `do {} while (0)` IS load-bearing — without it the
 * function is 68 instructions, one SHORT (measured 272). It costs zero
 * instructions and only flips reorg's branch prediction:
 *
 *   reorg.c `mostly_true_jump` scans BACKWARD from the branch's target label
 *   over NOTES ONLY; reaching a NOTE_INSN_LOOP_BEG it returns 2 = mostly
 *   taken. An empty one-shot loop emits LOOP_BEG/LOOP_CONT/LOOP_END with no
 *   insns between, so the scan from the endif label reaches LOOP_BEG and the
 *   CLUT guard is predicted TAKEN. `fill_eager_delay_slots` therefore fills
 *   from the TARGET thread first, and since the fallthrough falls into the
 *   merge block reorg does NOT own that thread — so it must COPY the merge
 *   block's leading `addiu $v0,$s1,1` into the delay slot and redirect the
 *   label past it: +1 insn, `i + 1` duplicated, exactly as retail does.
 *   Predicted NOT taken, reorg instead raids the FALLTHROUGH, which it DOES
 *   own, and MOVES `addiu $a0,$sp,0x10` out of the block into the slot: one
 *   instruction short, with a hole where the `&r` materialisation was.
 *   (The four `lhu`s are ineligible for a delay slot — MIPS loads carry
 *   hazard=delay — and every `sh` references a register the skipped loads put
 *   in reorg's `set`, so that `addiu` is the only thing the fallthrough
 *   offers.)
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
extern void GsGetTimInfo(u_long *tim, GsIMAGE *im);
extern void SystemOut(char *msg);
extern char D_800110C8[]; /* "NO IMAGE PACK DATA" */

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
                do {
                } while (0);
            }
            i = i + 1;
            adr = adr + 1;
        } while (i < n);
    }
    DrawSync(0);
}
