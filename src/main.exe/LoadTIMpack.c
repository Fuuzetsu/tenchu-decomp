#include "common.h"
#include "main.exe.h"
#include "timpack.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadTIMpack(unsigned long *adr);
 *     3DCTRL.C:759, 39 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * for cw/ch, `&rect` materialised early) because sched1 can then hoist the
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
 *   instruction short, with a hole where the `&rect` materialisation was.
 *   (The four `lhu`s are ineligible for a delay slot — MIPS loads carry
 *   hazard=delay — and every `sh` references a register the skipped loads put
 *   in reorg's `set`, so that `addiu` is the only thing the fallthrough
 *   offers.)
 *
 * LoadTIMpack (0x800189b4, 0x114 bytes) — LoadTIM.c's "pack" twin (same TU):
 * a packed archive's TIMPackIndex contains the element count followed by
 * one byte offset per element (walked with a plain pointer since only one
 * field is touched per iteration); each offset locates a TIM whose leading ID word is
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
 *  - `n` (TIMPackIndex.count) is read via `lhu` and then
 *    explicit `sll 16`/`sra 16` sign-extends it for the loop's signed
 *    compare — same "narrow field feeds a widen-and-scale/extend pair"
 *    shape as CreateCloneModelArchive.c's `n`.
 *  - `i` is a plain `short` loop counter (GetTIMpackInfo.c's identical
 *    idiom in this same TU): combine proves the raw 32-bit accumulation
 *    need not be truncated at every `i + 1`, only the compare needs the
 *    16-bit view, so a throwaway sign-extended copy feeds `slt`.
 *  - THE WALKER IS `adr` ITSELF, not the `p` copy. The address the loop
 *    hands GsGetTimInfo is `TIM_PACK_IMAGE(base, cursor)` where `base` (the
 *    fixed offset-table origin) is constant and the table cursor advances — but the
 *    target keeps the INCOMING pointer register ($s0, from param `adr`) as the
 *    advancing cursor and saves the fixed base into a fresh callee reg ($s3).
 *    So write `adr` as the one that `adr = adr + 1`s each iteration and let
 *    `p = adr;` be the saved fixed base read as `(int)p + adr[0]`;
 *    writing it the other way round (p walks, adr fixed) rotates every
 *    callee-saved register by one and mismatches the whole prologue/epilogue.
 *  - `tim.pmode` is read TWICE with different widths: the CLUT-bit test
 *    reloads the FULL `lw` (TIM_HAS_CLUT expands to `>> 3 & 1`), independent
 *    of any earlier access — different machine modes/uses don't CSE.
 */
extern char msg_no_image_pack_data[]; /* NO IMAGE PACK DATA */

short LoadTIMpack(unsigned long *adr)
{
    RECT rect;
    GsIMAGE tim;
    TIMPackIndex *index;
    u_long *p;
    u16 hw;
    short n;
    short i;

    if (adr == 0)
    {
        SystemOut(msg_no_image_pack_data);
    }
    adr++;
    index = (TIMPackIndex *)adr;
    hw = (u16)index->count;
    adr = index->offsets;
    i = 0;
    n = (short)hw;
    p = adr;
    if (n > 0)
    {
        do
        {
            GsGetTimInfo(TIM_PACK_IMAGE(p, adr), &tim);
            setRECT(&rect, tim.px, tim.py, tim.pw, tim.ph);
            LoadImage(&rect, tim.pixel);
            if (TIM_HAS_CLUT(tim.pmode) != 0)
            {
                setRECT(&rect, tim.cx, tim.cy, tim.cw, tim.ch);
                LoadImage(&rect, tim.clut);
                /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
                do
                {
                } while (0);
            }
            i++;
            adr++;
        } while (i < n);
    }
    DrawSync(0);
}
