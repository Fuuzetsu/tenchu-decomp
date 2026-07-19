#include "common.h"
#include "main.exe.h"
#include "adt.h"

/*
 * AdtSelect (0x8005fecc, 776 bytes) — modal debug-menu selection widget:
 * waits for pad release, saves the display state into a 0x8090-byte frame
 * buffer, then draws the choice list (18 per page) and moves the cursor on
 * edge-detected pad input until confirm (pad & 0x820 -> current entry) or
 * cancel (pad & 0x40 -> last entry); returns the entry's choice_number.
 *
 * STATUS: MATCHING — ordinary human-shaped C under the reused ADT object's
 * pinned GCC 2.8.0 compiler profile.
 *
 * AUTHORITATIVE UPDATE (2026-07-19): the long round-by-round investigation
 * below is retained as history, not as a rulebook. Its "fences are required",
 * "all C levers are closed", and "parked/impossible" conclusions are
 * superseded by fresh source and compiler evidence:
 *
 * - All eleven contiguous C members of the ADT library object are exact under
 *   GCC 2.8.0. Linking their 2.8.0 objects produces zero differing bytes in
 *   main.exe; both canonical and Sony SN32 GCC 2.8.1 leave this function nine
 *   bytes off. In reload.c, 2.8.0 preserves INPADDR/OUTADDR as
 *   RELOAD_FOR_OPADDR_ADDR, while 2.8.1 retypes them unconditionally to
 *   RELOAD_FOR_OPERAND_ADDRESS. That version change alone decides whether the
 *   huge-frame address and value reload may share a3.
 * - Both synthetic do{}while(0) fences are gone. A normal list-display for
 *   loop plus the human D-pad `if / else if` chain reproduces every target
 *   callee-saved allocation. The earlier 37-byte regression came from spelling
 *   mutually exclusive directions as nested negated tests, not from removing
 *   a compiler fence.
 * - The entry count is the ordinary empty indexed loop
 *   `for (count = 0; menu[count].choice_name != 0; count++) {}`. cc1's loop
 *   strength reduction creates the pointer cursor naturally. This clean source
 *   has the same nine-byte residual as the scaffolded draft.
 * - The demo's same-named body independently emits the TARGET self-tie at its
 *   larger frame offset (`lw a3,0(a3)` followed by the dereference), so the
 *   shape is demonstrably produced by the original human source family. It is
 *   evidence to keep searching upstream source identity, not evidence that the
 *   target is unreachable by this compiler.
 * - Fresh RTL localizes the remaining distinction: initial CSE turns the first
 *   test into a direct MEM through spilled menu, while loop strength reduction
 *   later creates a bare cursor pseudo. A fresh re-run corrected an earlier
 *   claim about volatile/address-taken spellings: NONE reproduced the target
 *   self-tie. A volatile parameter made 784 bytes, a volatile pointer local
 *   made 796 bytes, and loading through a volatile alias kept 776 bytes but
 *   caused a 51-byte allocation cascade. The only exact-length split-copy
 *   diagnostic was an empty asm constraint; it reduced the residual to two
 *   bytes but allocated the temporary/value in v0, not a3. It is lifetime
 *   evidence only, not acceptable source.
 * - The 776-byte body, including the a3 self-tie, is byte-identical in the
 *   shipped ENDING.EXE, MAIN.EXE, MENU.EXE, and TRIAL.EXE. This strongly points
 *   to the one reused ADT library object now represented by the build profile.
 *
 * The clean indexed loop below is exact. Any absolute "no natural C" or "do
 * not retry" language in the historical 2.8.1 log below is superseded.
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
 * ---- SUPERSEDED HISTORICAL LOG (rounds 1-6; useful measurements only) ----
 * ---- ROUND 6: re-verified against the levers that POSTDATE rounds 1-5
 *      (fixed-toolchain permuter, cookbook fence-classification) — verdict
 *      UNCHANGED: CURRENT(9). ----
 *
 * 1. THE PERMUTER -fno-builtin BUG IS FIXED (it lived in CPP not CC_FLAGS, so
 *    earlier runs searched a different program).  Re-ran the search RE-SEEDED
 *    from this draft: `timeout 240 tools/permute.py AdtSelect -- --stop-on-zero
 *    -j4`, 21684 iterations + authoritative full-link rescore -> 9/9/776,
 *    best candidate is the unmodified base (empty semantic delta).  Several
 *    iterations hit proxy score 30 (< base's proxy), but the full-link rescore
 *    rejected every one.  The fix does not affect AdtSelect (it calls no
 *    builtins), and the residual is genuinely permuter-immune, as rounds 3-5 said.
 *
 * 2. THE TWO do{}while(0) FENCES ARE COOKBOOK CASE (c) — BARE LOAD-BEARING,
 *    KEEP.  Unwrapping is NOT a byte-chase that hides a better human structure:
 *    removing BOTH (autorules fence-unwrap L412/L446, each +16) yields 37 bytes
 *    that are ENTIRELY callee-saved register renames — selection s1<->s2,
 *    count s3<->s4, trg/last/page shuffled among s1-s4 (the classic "same
 *    s1/s2 tie" the cookbook §3.10 case-c/cluster note names).  The fences fix
 *    the GLOBAL find_reg allocation; nothing more complete is behind them, so
 *    the "adopt the worse byte count" move (case b only) does NOT apply.  They
 *    do not touch the count-loop RELOAD tie (a3/t0 are allocate_reload_reg's
 *    round-robin, not find_reg), so no fence tuning can close the 9.
 *
 * 3. The single-use `name` temp was removed (byte-NEUTRAL, 9->9): the site-1
 *    deref is now the `if (menu->choice_name != 0)` count guard directly.  The
 *    RTL is identical because combine always folded the temp — reg 93 below is
 *    the loaded choice_name value of that guard, whether or not a C `name`
 *    named it (measured: candidate with the temp is byte-for-byte the same).
 *    PSX.SYM's prototype declares no such local, so this is the human spelling.
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
 *
 * ---- ROUND 5: independently re-verified against the pinned reload.c/reload1.c
 *      source (not the header's prose) — ROUND 4's VERDICT HOLDS, and the exact
 *      discriminator the audit wanted a `--spill-uses` flag for is now named ----
 *
 * This round was briefed with two "tractability" claims: (1) mips.h defines no
 * REG_ALLOC_ORDER, so find_reg walks hard regs numerically and whichever pseudo
 * the target put in $a3 was "allocated earlier"; (2) the target's `lw a3,0(a3)`
 * is a "reuse a dying variable / coalesce upward" shape reachable by giving the
 * address and the value ONE C variable instead of two.  Neither survives
 * contact with the actual pinned source (`/nix/store/*-source/reload.c`,
 * `reload1.c`, `reload.h` — not memory, read fresh this round, every line
 * number below checked with `sed -n` against that tree):
 *
 * 1. Claim 1 is TRUE but does not apply to the registers in question.
 *    `tools/regalloc.py AdtSelect --order` (self-validated against cc1's own
 *    `;; 18 regs to allocate:` line) lists 18 allocnos; NONE of them dispose to
 *    $a3 or $t0 — p80/p81 (title/menu) are IN the list, at priority 930/819,
 *    but their disposition is "—" (spilled).  $a3/$t0 here are RELOAD
 *    registers, chosen by reload1.c's own, separate mechanism
 *    (`allocate_reload_reg`, reload1.c:5031), not by global.c's `find_reg`.
 *    That said, the underlying FACT generalises: reload1.c:3918-3936
 *    (`order_regs_for_reload`, which builds `potential_reload_regs[]`, the
 *    pool `allocate_reload_reg` round-robins over) is ALSO `#ifdef
 *    REG_ALLOC_ORDER` / `#else` ascending-by-regno, so absent that macro
 *    reload's own candidate pool is ALSO built low-to-high.  This is a real,
 *    previously-undocumented fact about this cc1 — but it decides which
 *    register a reload tries FIRST among registers that are already free; it
 *    cannot make a CATEGORICALLY-conflicting register free.  See point 3.
 *
 * 2. Claim 2 is FALSE for this exact operand shape, and the reason is now
 *    precise instead of prose.  Two DIFFERENT reload functions handle a
 *    reg_equiv_address pseudo, chosen by whether it is DEREFERENCED (used as
 *    a MEM's address) or used BARE (used as a value):
 *      - `menu->choice_name` makes reg81 the address inside `(mem (reg 81))`
 *        — a MEM operand — so `find_reloads` (reload.c:2554) hands it to
 *        `find_reloads_address`, whose REG case (reload.c:4296,
 *        `reg_equiv_address[regno] != 0`) RECURSES: push a reload for the
 *        spill slot's own address typed `ADDR_TYPE(type)` = INPADDR_ADDRESS
 *        (reload.h's `ADDR_TYPE` macro, reload.c:302-306), THEN push a
 *        second reload for menu's value typed INPUT_ADDRESS (reload.c
 *        4296's `push_reload(tem, ..., type)`, unchanged `type`) — TWO
 *        reloads, A pushed strictly before B.
 *      - `if (title == 0)` and `p = menu` use their pseudo BARE (title as
 *        branch_zero's compared value; menu as the plain SET source with no
 *        `->`).  A bare REG operand goes to `find_reloads_toplev`
 *        (reload.c:4066), whose OWN reg_equiv_address branch
 *        (reload.c:4090) builds `(mem (its own equiv address))` ONE level
 *        and calls `find_reloads_address` on THAT — an address that is a
 *        PLUS (sp + const), not a bare REG, so it never enters the
 *        recursive REG-in-REG branch at all.  ONE reload, type RELOAD_FOR_
 *        INPUT (reload.c:2520-2521, since `modified[i]==RELOAD_READ`).
 *    Confirmed directly in the current draft's own `.greg` RTL
 *    (`tools/regalloc.py AdtSelect --rtl`): site 1 is insns 470/472 (a3 =
 *    32972; a3 = a3+sp) + 475 (t0 = mem[a3]) + insn 48 (v0 = mem[t0]) — TWO
 *    reload insns before the original deref, exactly the two-push shape.
 *    Site 2 is insns 479/481 (a3 = 32972; a3 = a3+sp) + insn 55 itself
 *    (`v1 = mem[a3]`, the ORIGINAL low-UID insn, not a reload insn) — ONE
 *    reload, the value lands straight in p's own destination register.
 *    `tools/cc1says.py AdtSelect --pass greg` prints this demand verbatim:
 *    `Need 2 regs of class GR_REGS (for insn 38)` (site 1's insn number
 *    this round) — nothing analogous for site 2.
 *
 * 3. WHY the two reloads can never share a register — verified against
 *    reload.c 3806-3855 (the retype) and reload1.c 4514-4630
 *    (`reload_reg_free_p`), not summarised:
 *      - The retype (reload.c:3806, gated on `operand_reloadnum[opnum] < 0`)
 *        fires for INPUT_ADDRESS/INPADDR_ADDRESS reloads whose OPERAND
 *        itself was never separately reloaded — true here, since the movsi's
 *        'm' alternative absorbs the MEM without a whole-operand reload.  It
 *        does NOT fire for RELOAD_FOR_INPUT (reload.c:3806's condition list
 *        has no INPUT case) — so title/p's single reload is never touched by
 *        this block at all; it was never eligible to begin with.
 *      - Site 1's A and B both retype to RELOAD_FOR_OPERAND_ADDRESS
 *        (reload.c:3855).  `reload_reg_free_p`'s OPERAND_ADDRESS case
 *        (reload1.c:4618) checks a single INSN-WIDE bit,
 *        `reload_reg_used_in_op_addr` (set by `mark_reload_reg_in_use`,
 *        reload1.c:4419) — not indexed by opnum, so ANY two OPERAND_ADDRESS
 *        reloads in the same insn conflict, unconditionally.  This is why
 *        the ROUND 3 CORRECTION's "un-retyped escape" also fails:
 *        RELOAD_FOR_INPUT_ADDRESS's own free-check (reload1.c:4567) tests
 *        `reload_reg_used_in_inpaddr_addr[opnum]` for the SAME opnum, and A
 *        (INPADDR_ADDRESS) sets exactly that bit (reload1.c:4407) — barred
 *        either way, verified both paths.
 *      - RELOAD_FOR_INPUT's free-check (reload1.c:4546) is different in
 *        kind, not degree: it scans `reload_reg_used_in_input_addr`/
 *        `_inpaddr_addr` only for LATER opnums (`i = opnum+1 ...`), never the
 *        reload's OWN opnum.  So a RELOAD_FOR_INPUT reload is free to sit in
 *        whatever register ITS OWN address reload (same opnum) just used —
 *        this is the entire mechanism behind title's self-tie, and it is
 *        categorically unavailable to a MEM operand, because a MEM operand
 *        never gets a RELOAD_FOR_INPUT of its own (the 'm' alternative
 *        absorbs it with no separate operand-level reload to retype from).
 *    None of this is a priority/ordering question — `reload_reg_used_in_op_
 *    addr` is a hard conflict bit, not a preference.  Per the cookbook's own
 *    rule ("a flipped allocation order over a hard-reg conflict is a
 *    guaranteed no-op"), claim 1's true-but-inapplicable REG_ALLOC_ORDER
 *    fact cannot rescue this: there is no ordering of reload requests that
 *    turns a categorical bar into a free register.
 *
 * 4. This closes the audit's own open item (docs/cookbook-audit.md T2,
 *    "`--spill-uses`: each use BARE vs IN-MEM (the AdtSelect discriminator)")
 *    by hand: BARE-vs-IN-MEM is exactly the find_reloads_toplev-vs-
 *    find_reloads_address fork above, and it is fully determined by whether
 *    the C-level use is a dereference (always IN-MEM, always two reloads,
 *    always barred) or a plain value use (always BARE, always one reload,
 *    always self-tie-eligible).  `menu->choice_name` is irreducibly a
 *    dereference — no C spelling changes which reload function handles it —
 *    so site 1 can never reach the BARE path while site 2 and title's check
 *    already sit on it.  `tools/regalloc.py` still has no `--spill-uses`
 *    flag (checked: absent from its argparse definitions on master as of
 *    this round) — it would have saved the manual reload.c/reload1.c read
 *    above, and is worth building for the next residual of this shape, but
 *    this round did NOT hand-simulate its output — it read the compiler's
 *    actual source and the actual `.greg` RTL, which is the escalation the
 *    cookbook prescribes for a sub-C tie once the tooling gap is confirmed.
 *
 * 5. Fresh measurements this round, current tooling, current master (c2e25ea
 *    base): `tools/autorules.py AdtSelect` — 28 rules / 31 candidates (up
 *    from round 4's 23; the ruleset has grown), still zero improving edits.
 *    A bounded permuter run (`timeout 240 tools/permute.py AdtSelect --
 *    --stop-on-zero -j4`, 22459 iterations + the authoritative rescore) —
 *    still `9 / 9 / 776`, best candidate is the unmodified base.  The
 *    permuter's own preflight now names this residual class on sight:
 *    "a <=10-byte register-swap / adjacent-reorder residual is usually
 *    sub-C-level (reload/sched) and permuter-immune."
 *
 * STATUS UNCHANGED: CURRENT(9), still parked.  Unlike some other functions'
 * park prose in this project, round 4's mechanical citations were checked
 * line-by-line against the real pinned gcc-2.8.1 source this round and are
 * ALL accurate — this is not a re-assertion, it is an independent
 * reproduction with exact line numbers.  A future round should only re-open
 * this if it can show the target's site 1 is NOT `menu->choice_name` at all
 * (e.g. a different field order, or the counting loop computing `name`
 * as a byproduct) — anything that keeps it a dereference of a spilled
 * pointer-to-struct cannot self-tie in this cc1.
 */

extern s32 (*AdtPadRead)(s32);
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
    TAdtDisp ad;
    s32 last;
    u32 trg;
    s32 count;
    s32 pages;
    s32 page;
    s32 first;
    u32 pad;
    short i;
    char *fmt;

    do
    {
    } while (AdtPadRead(0) != 0);

    if (title == 0)
        title = D_80014AFC;

    for (count = 0; menu[count].choice_name != 0; count++)
    {
    }

    pages = count / 0x12 + 1;
    AdtGetDisp(&ad);

    for (;;)
    {
        DrawPrim(&ad.bg);
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
        FntFlush(-1);
        VSync(3);
        if (pad & 0x820)
            break;
        if (pad & 0x40)
        {
            selection = count - 1;
            break;
        }
        if (trg & 0x1000)
            i = -1;
        else if (trg & 0x4000)
            i = 1;
        else if (trg & 0x8000)
            i = -0x12;
        else if (trg & 0x2000)
            i = 0x12;
        else
            i = 0;
        selection += i;
        if (selection < 0)
            selection = 0;
        else if (count <= selection)
            selection = count - 1;
    }
    AdtReleaseDisp(&ad);
    return menu[selection].choice_number;
}
