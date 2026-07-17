#include "common.h"
#include "main.exe.h"

/*
 * AdtSelect (0x8005fecc, 776 bytes) — modal debug-menu selection widget:
 * waits for pad release, saves the display state into a 0x8090-byte frame
 * buffer, then draws the choice list (18 per page) and moves the cursor on
 * edge-detected pad input until confirm (pad & 0x820 -> current entry) or
 * cancel (pad & 0x40 -> last entry); returns the entry's choice_number.
 *
 * STATUS: NON_MATCHING — 9 of 776 bytes differ.  Build the draft with
 * `NON_MATCHING=AdtSelect ./Build` (or tools/matchdiff.py, which sets it
 * automatically); the default build keeps the INCLUDE_ASM stub below.
 *
 * The 9 differing bytes are ONE reload decision, in the entry-count block:
 *
 *   target:  lw a3,0(a3); lw v0,0(a3);  ...  li t0,0x80CC; addu; lw v1,0(t0)
 *   ours:    lw t0,0(a3); lw v0,0(t0);  ...  li a3,0x80CC; addu; lw v1,0(a3)
 *
 * Both are the SAME 4-insn shape (`li`/`addu` materialise sp+0x80CC, load
 * menu, deref).  Only the register holding menu's value differs: the target
 * reuses the address register a3, we take t0.  Site 2's t0<->a3 is NOT an
 * independent difference — see the round-robin note below.
 *
 * ---- WHY (verified line-by-line against the nix-pinned gcc-2.8.1 sources,
 *          and against the .greg RTL dump — see ROUND 3 correction below) ----
 *
 * For `name = menu->choice_name` the insn is `(set (reg 93) (mem (reg 81)))`
 * with reg 81 = menu spilled (reg_equiv_address = sp+32972; 32972 > 32767 so
 * the address must be materialised).  The .greg dump confirms the operand is a
 * DOUBLE MEM — insn 48 carries
 *   REG_EQUIV (mem/s:SI (mem:SI (plus:SI (reg 29 sp) (const_int 32972))))
 * and reload emits exactly three insns:
 *   470/472  a3 = 32972 ; a3 = a3 + sp      <- reload A (materialise the slot)
 *   475      t0 = mem[a3]                   <- reload B (load menu's value)
 *   48       v0 = mem[t0]                   <- the movsi ('m' absorbs it)
 * Site 2 (`p = menu`, insns 479/481/55) has only ONE reload, hence one reg.
 *
 *   reload.c 2552   a MEM operand goes STRAIGHT to find_reloads_address — it
 *                   never reaches find_reloads_toplev.
 *   reload.c 4296   reg_equiv_address branch: recurse on the sp+32972 PLUS
 *                   (RELOAD_FOR_INPADDR_ADDRESS via ADDR_TYPE), THEN
 *                   push_reload menu's value (RELOAD_FOR_INPUT_ADDRESS).
 *                   A is therefore ALWAYS pushed before B.
 *   reload1.c 4345  reload_reg_class_lower's last tiebreak is `r1 - r2`
 *                   (push order); A and B tie on every earlier key (both
 *                   non-optional, both GR_REGS, non-solitary, nregs 1), so A is
 *                   ALWAYS allocated first and always wins a3.  Not perturbable.
 *
 * ---- ROUND 3 CORRECTION: the "operand must require a register" escape is
 *      NOT the discriminator.  Rounds 1-2 (and the cookbook) had this wrong. ----
 *
 * reload1.c's reload_reg_free_p bars B from A's register on BOTH paths, so
 * defeating reload.c 3812's retype would NOT produce the target:
 *   - RETYPED (what we get): both become RELOAD_FOR_OPERAND_ADDRESS, whose
 *     free-check returns 0 for any reg in reload_reg_used_in_op_addr.  A marks
 *     it, so B is barred.  ==> t0.
 *   - UN-RETYPED (the proposed escape): A stays RELOAD_FOR_INPADDR_ADDRESS and
 *     B stays RELOAD_FOR_INPUT_ADDRESS — but the RELOAD_FOR_INPUT_ADDRESS case
 *     explicitly bars BOTH reload_reg_used_in_input_addr[opnum] AND
 *     reload_reg_used_in_inpaddr_addr[opnum].  A marks the latter.  ==> t0 again.
 * So B can NEVER share A's register, retype or no retype.  The escape is dead
 * not because no C spelling reaches it, but because reaching it changes nothing.
 *
 * The self-tie that DOES exist is a different reload type.  `if (title == 0)`
 * emits `lw a3,0(a3)` at 0x8005FF3C — and we already match those bytes.  There
 * the pseudo's VALUE is a bare operand requiring a register, so it is reloaded
 * as RELOAD_FOR_INPUT, whose free-check scans input_addr/inpaddr_addr only for
 * `i > opnum` — an INPUT reload may therefore share its OWN address reload's
 * register.  Same for `menu[i]` / `menu[selection]`.  That is the real rule.
 *
 * Two consequences for site 1:
 *   1. Making the operand require a register would ADD a third reload
 *      (RELOAD_FOR_INPUT, on top of A and B) — it cannot merge A and B.
 *   2. No C spelling can make it anyway: every MIPS pattern with a 'd'-only
 *      constraint carries a register_operand PREDICATE (branch_zero, addsi3,
 *      mulsi3 …), so combine's recog refuses a MEM; and register_operand's only
 *      MEM escape (recog.c 883-892, `(subreg (mem))` pre-reload) is unreachable
 *      for a same-mode SImode pointer deref.  The deref is always a standalone
 *      movsi whose 'm' absorbs the MEM.
 *
 * WHAT THIS LEAVES: the target's site 1 emits `ori a3,0x80CC; addu a3,a3,sp;
 * lw a3,0(a3)` — ONE reload register.  Since two reloads at one opnum can never
 * share, the target must have only ONE reload there, i.e. menu's load is an
 * INPUT-type reload (like title's) rather than an INPUT_ADDRESS/INPADDR pair.
 * That is a sharper open question than round 3's brief posed, and it is NOT
 * answerable by respelling the deref: it needs a source shape in which menu's
 * value is a bare register operand whose reload register is then reused as the
 * deref address — which the copy lever (below) provably cannot supply either.
 *
 * ---- WHAT ROUND 2 DISPROVED (do not re-derive) ----
 *
 * Round 1's "open question" — *a copy that is single-use yet outlives combine*
 * — is a DEAD END even if solved.  A surviving copy `X = menu` puts menu's
 * value in X's ALLOCATED register, and reload's spill registers (a3, t0) are
 * excluded from allocation: the .greg dispositions here only ever use
 * $v0/$v1/$a0/$s0-$s7/$fp/HI.  So a copy can only ever emit
 * `lw v0/v1,0(a3); lw v0,0(v0/v1)` — never the target's `lw a3,0(a3)`.
 * Measured directly (tools/rtldump.py --src): the multi-use copy still emits
 * `lw $8,0($7)` at site 1 with p in $3.  Combine also copy-propagates
 * `p = menu` into the deref even when p is multi-use, so the catch-22 the old
 * note described is not even reachable.  Chasing a combine fence here is wasted
 * effort: the copy is the wrong lever, not a blocked one.
 *
 * Also disproved: adding any bare-operand use of `menu` to force the self-tie
 * (e.g. `if (menu == 0) return -1;`) raises menu's ref count enough that it is
 * no longer spilled at all — it lands in $fp and the whole shape changes.
 *
 * ---- WHY IT IS ONE DECISION, NOT TWO ----
 *
 * reload1.c 5082-5091: allocate_reload_reg starts its scan at
 * `last_spill_reg + 1` and wraps `% n_spills`, setting last_spill_reg on each
 * success (5185) — a ROUND ROBIN.  The function's reload regs therefore run
 * a3, t0, a3, t0, ...  Site 1 consuming ONE reload reg (target) instead of TWO
 * (ours) shifts every later reload by one position, which is exactly why site 2
 * flips t0<->a3.  Fixing site 1 would fix site 2 for free; there is no separate
 * site-2 bug.
 *
 * ---- CLOSED ----
 *
 * autorules (23 candidates, no improving edit; both do{}while(0) fences are
 * load-bearing, unwrapping either costs +16); a bounded decomp-permuter run
 * (flat at 9, base best); reghist (194/194 insns, delta sum 0 — no
 * decomposition lever).  The demo build's AdtSelect has a DIFFERENT frame
 * (0x80E0/0x80E4) yet emits the same a3/t0 split, so the shape is a stable
 * source property, not a frame-size artefact.  All other allocation (9
 * callee-saved pseudos + 2 spilled params) matches.
 *
 * ---- ROUND 4: the REG_EQUIV hypothesis is REFUTED, and the real
 *      discriminator is now named (reload.c 3806-3855, the RETYPE) ----
 *
 * Round 4 was asked to kill the double-MEM REG_EQUIV note by giving its pseudo
 * a second-basic-block reference.  Two measured findings:
 *
 * 1. THE NOTE IS NOT ON `menu`.  update_equiv_regs uses regno = REGNO(SET_DEST),
 *    so the note belongs to `name` (reg 93), whose .greg home is a HARD REGISTER
 *    ($v0).  Its *value* merely mentions menu's slot.  Everything in reload1.c
 *    keyed on reg_equiv_memory_loc[93] is gated on `reg_renumber[93] < 0`, so
 *    with reg 93 in $v0 the note is INERT.
 *
 * 2. KILLING IT CHANGES NOTHING AT SITE 1 (measured).  Reusing `name` as the
 *    count-loop's test variable makes reg_n_sets[93]==2, so update_equiv_regs
 *    skips it: insn 48's note list becomes (nil) and the double-MEM count in
 *    .greg drops to 0.  Site 1 still emits `lw t0,0(a3)`, byte-for-byte.  The
 *    edit costs +4 (13/776: a pure v0<->v1 rename downstream).  REVERTED.
 *    => The note was never causal.  Do not re-open it.
 *
 * THE ACTUAL DISCRIMINATOR (read from the pinned gcc-2.8.1 sources, and it
 * explains the title-vs-menu asymmetry the earlier rounds only described):
 *
 *   reload.c 2564  find_reloads passes address_type[i] (= RELOAD_FOR_INPUT_
 *                  ADDRESS for a read operand) into find_reloads_toplev.
 *   reload.c 4296  so the reg_equiv_address branch gives A = INPADDR_ADDRESS
 *                  (the recursion, via ADDR_TYPE) and B = INPUT_ADDRESS.
 *   reload.c 3806  THE RETYPE, gated on
 *                    `operand_reloadnum[reload_opnum[i]] < 0 || reload_optional[…]`
 *                  i.e. it fires exactly when THE OPERAND ITSELF IS NOT RELOADED.
 *                  Both A and B then become RELOAD_FOR_OPERAND_ADDRESS (3855).
 *   reload1.c 4618 RELOAD_FOR_OPERAND_ADDRESS returns 0 for any reg in
 *                  reload_reg_used_in_op_addr — A marks it, so B is barred.
 *
 * Why `if (title == 0)` self-ties and `name = menu->choice_name` cannot:
 * branch_zero's 'd' constraint RELOADS the operand, so operand_reloadnum >= 0
 * and THE RETYPE NEVER FIRES; A stays INPUT_ADDRESS and B is the operand's own
 * RELOAD_FOR_INPUT, which scans input_addr/inpaddr_addr only for `i > opnum`
 * (reload1.c 4558) and so may take A's register => `lw a3,0(a3)`.  The deref's
 * movsi 'm' absorbs (mem (reg 81)), so its operand is never reloaded, the retype
 * always fires, and RELOAD_FOR_INPUT additionally bars reload_reg_used_in_op_addr
 * (reload1.c 4546) — which poisons the INPUT path too, even if one were reached.
 *
 * So the target's site 1 is NOT this insn shape at all: it needs menu's VALUE as
 * a bare register operand.  Round 3 already proved no C spelling reaches that for
 * a deref (register_operand rejects a MEM), and round 2 proved a copy cannot
 * supply it (copies get ALLOCATED regs, never a3/t0).  Both re-verified here
 * against the actual sources — every claim in the ROUND 3 CORRECTION checks out.
 *
 * The demo build's ORIGINAL also self-ties at site 1 at a different frame
 * (0x80E0/0x80E4 vs 0x80C8/0x80CC), so it is a robust property of the original
 * source, not a frame artefact — but nothing reachable from this C reproduces it.
 *
 * ---- STATUS AFTER ROUND 4: EVIDENCE-COMPLETE, PARKED ----
 *
 * Round 3 was asked to make site 1's operand require a register.  That question
 * is answered NO twice over (see the ROUND 3 CORRECTION above): no C spelling
 * can do it, AND doing it would not produce the target anyway, because
 * reload_reg_free_p bars the second reload from the first's register on both
 * the retyped and un-retyped paths.  The lever the last two rounds were aimed
 * at does not exist.  Round 4 closed the REG_EQUIV lever the same way.
 *
 * Every source-level lever is now closed with measurements: autorules (23
 * candidates, none improving), a bounded permuter run (flat at 9), reghist
 * (194/194 insns, delta sum 0), both do{}while(0) fences load-bearing (+16 each
 * if unwrapped), the copy lever (dead — reload regs are not allocatable), and
 * every respelling of the deref (identical RTL: the operand is always a
 * standalone movsi absorbing (mem (reg 81)) via 'm').  The demo build emits the
 * same split at a different frame (0x80E0/0x80E4), so it is not a frame artefact.
 *
 * The remaining question is NOT a C-spelling question: it is why the original's
 * site 1 has ONE reload where gcc-2.8.1 gives this source structure TWO.  Any
 * future round should start there — from the .greg RTL and reload.c 4296 — and
 * should NOT re-open the operand-constraint or copy levers.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AdtSelect", AdtSelect);

#else /* NON_MATCHING */

extern s32 (*AdtPadRead)(s32);
extern void AdtGetDisp(u8 *buf);
extern void AdtReleaseDisp(u8 *buf);
extern void DrawPrim(u8 *prim);
extern void FntPrint(char *fmt, ...);
extern s32 FntFlush(s32 id);
extern s32 VSync(s32 mode);

extern char D_80014AFC[]; /* "select item" */
extern char D_80014B08[]; /* " (%d/%d)" */
extern char D_80097E9C[]; /* "%s" */
extern char D_80097EA0[]; /* "\n\n" */
extern char D_80097EA4[]; /* "->" */
extern char D_80097EA8[]; /* "  " */
extern char D_80097EAC[]; /* "\n" */

s32 AdtSelect(char *title, debug_menu_choice *menu, s32 selection)
{
    u8 buf[0x8090];
    s32 last;
    u32 trg;
    debug_menu_choice *p;
    s32 count;
    s32 pages;
    s32 page;
    s32 first;
    u32 pad;
    short i;
    char *fmt;
    char *name;

    do
    {
    } while (AdtPadRead(0) != 0);

    if (title == 0)
        title = D_80014AFC;

    count = 0;
    name = menu->choice_name;
    if (name != 0)
    {
        p = menu;
        do
        {
            p++;
            count++;
        } while (p->choice_name != 0);
    }

    pages = count / 0x12 + 1;
    AdtGetDisp(buf);

    for (;;)
    {
        DrawPrim(buf + 0x8078);
        trg = pad;
        pad = AdtPadRead(0);
        trg = ~trg & pad;
        page = selection / 0x12;
        first = page * 0x12;
        last = first + 0x12;
        if (count < last)
            last = count;
        FntPrint(D_80097E9C, title);
        if (pages > 1)
            FntPrint(D_80014B08, page + 1, pages);
        FntPrint(D_80097EA0);
        i = first;
        do
        {
            /* do{}while(0) wrappers: flow.c loop-depth ref weighting;
               see the matching notes above and docs/matching-cookbook.md */
            for (; i < last; i++)
            {
                if (selection == i)
                    fmt = D_80097EA4;
                else
                    fmt = D_80097EA8;
                FntPrint(fmt);
                FntPrint(menu[i].choice_name);
                FntPrint(D_80097EAC);
            }
        } while (0);
        FntFlush(-1);
        VSync(3);
        if (pad & 0x820)
            break;
        if (pad & 0x40)
        {
            selection = count - 1;
            break;
        }
        i = -1;
        if (!(trg & 0x1000))
        {
            i = 1;
            if (!(trg & 0x4000))
            {
                i = -0x12;
                if (!(trg & 0x8000))
                {
                    i = 0;
                    do
                    {
                        if (trg & 0x2000)
                            i = 0x12;
                    } while (0);
                }
            }
        }
        selection += i;
        if (selection < 0)
            selection = 0;
        else if (count <= selection)
            selection = count - 1;
    }
    AdtReleaseDisp(buf);
    return menu[selection].choice_number;
}

#endif /* NON_MATCHING */
