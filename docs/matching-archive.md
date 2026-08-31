# Matching archive — superseded per-function investigation logs


These sections were moved out of `src/main.exe/*.c` verbatim (2026-08-31).
Each was **already self-labelled superseded or historical by its own file**:
they record how a match was reached, not what the source must do. The active
constraints — the facts a future editor must not break — stayed in the files.

Reusable rules belong in [`matching-cookbook.md`](matching-cookbook.md); this
file is the raw log, kept because measurements are expensive and a refuted
hypothesis is worth knowing.


## AdtSelect

```
---- SUPERSEDED HISTORICAL LOG (rounds 1-6; useful measurements only) ----
---- ROUND 6: re-verified against the levers that POSTDATE rounds 1-5
     (fixed-toolchain permuter, cookbook fence-classification) — verdict
     UNCHANGED: CURRENT(9). ----

1. THE PERMUTER -fno-builtin BUG IS FIXED (it lived in CPP not CC_FLAGS, so
   earlier runs searched a different program).  Re-ran the search RE-SEEDED
   from this draft: `timeout 240 tools/permute.py AdtSelect -- --stop-on-zero
   -j4`, 21684 iterations + authoritative full-link rescore -> 9/9/776,
   best candidate is the unmodified base (empty semantic delta).  Several
   iterations hit proxy score 30 (< base's proxy), but the full-link rescore
   rejected every one.  The fix does not affect AdtSelect (it calls no
   builtins), and the residual is genuinely permuter-immune, as rounds 3-5 said.

2. THE TWO do{}while(0) FENCES ARE COOKBOOK CASE (c) — BARE LOAD-BEARING,
   KEEP.  Unwrapping is NOT a byte-chase that hides a better human structure:
   removing BOTH (autorules fence-unwrap L412/L446, each +16) yields 37 bytes
   that are ENTIRELY callee-saved register renames — mode s1<->s2,
   count s3<->s4, trg/last/page shuffled among s1-s4 (the classic "same
   s1/s2 tie" the cookbook §3.10 case-c/cluster note names).  The fences fix
   the GLOBAL find_reg allocation; nothing more complete is behind them, so
   the "adopt the worse byte count" move (case b only) does NOT apply.  They
   do not touch the count-loop RELOAD tie (a3/t0 are allocate_reload_reg's
   round-robin, not find_reg), so no fence tuning can close the 9.

3. The single-use `name` temp was removed (byte-NEUTRAL, 9->9): the site-1
   deref is now the `menu[count].name != 0` count guard directly.  The
   RTL is identical because combine always folded the temp — reg 93 below is
   the loaded name value of that guard, whether or not a C `name`
   named it (measured: candidate with the temp is byte-for-byte the same).
   PSX.SYM's prototype declares no such local, so this is the human spelling.

---- WHY (verified line-by-line against the nix-pinned gcc-2.8.1 sources,
         and against the .greg RTL dump — see ROUND 3 correction below) ----

For `name = menu->name` the insn is `(set (reg 93) (mem (reg 81)))`
with reg 81 = menu spilled (reg_equiv_address = sp+32972; 32972 > 32767 so
the address must be materialised).  The .greg dump confirms the operand is a
DOUBLE MEM — insn 48 carries
  REG_EQUIV (mem/s:SI (mem:SI (plus:SI (reg 29 sp) (const_int 32972))))
and reload emits exactly three insns:
  470/472  a3 = 32972 ; a3 = a3 + sp      <- reload A (materialise the slot)
  475      t0 = mem[a3]                   <- reload B (load menu's value)
  48       v0 = mem[t0]                   <- the movsi ('m' absorbs it)
Site 2 (`p = menu`, insns 479/481/55) has only ONE reload, hence one reg.

  reload.c 2552   a MEM operand goes STRAIGHT to find_reloads_address — it
                  never reaches find_reloads_toplev.
  reload.c 4296   reg_equiv_address branch: recurse on the sp+32972 PLUS
                  (RELOAD_FOR_INPADDR_ADDRESS via ADDR_TYPE), THEN
                  push_reload menu's value (RELOAD_FOR_INPUT_ADDRESS).
                  A is therefore ALWAYS pushed before B.
  reload1.c 4345  reload_reg_class_lower's last tiebreak is `r1 - r2`
                  (push order); A and B tie on every earlier key (both
                  non-optional, both GR_REGS, non-solitary, nregs 1), so A is
                  ALWAYS allocated first and always wins a3.  Not perturbable.

---- ROUND 3 CORRECTION: the "operand must require a register" escape is
     NOT the discriminator.  Rounds 1-2 (and the cookbook) had this wrong. ----

reload1.c's reload_reg_free_p bars B from A's register on BOTH paths, so
defeating reload.c 3812's retype would NOT produce the target:
  - RETYPED (what we get): both become RELOAD_FOR_OPERAND_ADDRESS, whose
    free-check returns 0 for any reg in reload_reg_used_in_op_addr.  A marks
    it, so B is barred.  ==> t0.
  - UN-RETYPED (the proposed escape): A stays RELOAD_FOR_INPADDR_ADDRESS and
    B stays RELOAD_FOR_INPUT_ADDRESS — but the RELOAD_FOR_INPUT_ADDRESS case
    explicitly bars BOTH reload_reg_used_in_input_addr[opnum] AND
    reload_reg_used_in_inpaddr_addr[opnum].  A marks the latter.  ==> t0 again.
So B can NEVER share A's register, retype or no retype.  The escape is dead
not because no C spelling reaches it, but because reaching it changes nothing.

The self-tie that DOES exist is a different reload type.  `if (title == 0)`
emits `lw a3,0(a3)` at 0x8005FF3C — and we already match those bytes.  There
the pseudo's VALUE is a bare operand requiring a register, so it is reloaded
as RELOAD_FOR_INPUT, whose free-check scans input_addr/inpaddr_addr only for
`i > opnum` — an INPUT reload may therefore share its OWN address reload's
register.  Same for `menu[i]` / `menu[mode]`.  That is the real rule.

Two consequences for site 1:
  1. Making the operand require a register would ADD a third reload
     (RELOAD_FOR_INPUT, on top of A and B) — it cannot merge A and B.
  2. No C spelling can make it anyway: every MIPS pattern with a 'd'-only
     constraint carries a register_operand PREDICATE (branch_zero, addsi3,
     mulsi3 …), so combine's recog refuses a MEM; and register_operand's only
     MEM escape (recog.c 883-892, `(subreg (mem))` pre-reload) is unreachable
     for a same-mode SImode pointer deref.  The deref is always a standalone
     movsi whose 'm' absorbs the MEM.

WHAT THIS LEAVES: the target's site 1 emits `ori a3,0x80CC; addu a3,a3,sp;
lw a3,0(a3)` — ONE reload register.  Since two reloads at one opnum can never
share, the target must have only ONE reload there, i.e. menu's load is an
INPUT-type reload (like title's) rather than an INPUT_ADDRESS/INPADDR pair.
That is a sharper open question than round 3's brief posed, and it is NOT
answerable by respelling the deref: it needs a source shape in which menu's
value is a bare register operand whose reload register is then reused as the
deref address — which the copy lever (below) provably cannot supply either.

---- WHAT ROUND 2 DISPROVED (do not re-derive) ----

Round 1's "open question" — *a copy that is single-use yet outlives combine*
— is a DEAD END even if solved.  A surviving copy `X = menu` puts menu's
value in X's ALLOCATED register, and reload's spill registers (a3, t0) are
excluded from allocation: the .greg dispositions here only ever use
$v0/$v1/$a0/$s0-$s7/$fp/HI.  So a copy can only ever emit
`lw v0/v1,0(a3); lw v0,0(v0/v1)` — never the target's `lw a3,0(a3)`.
Measured directly (tools/rtldump.py --src): the multi-use copy still emits
`lw $8,0($7)` at site 1 with p in $3.  Combine also copy-propagates
`p = menu` into the deref even when p is multi-use, so the catch-22 the old
note described is not even reachable.  Chasing a combine fence here is wasted
effort: the copy is the wrong lever, not a blocked one.

Also disproved: adding any bare-operand use of `menu` to force the self-tie
(e.g. `if (menu == 0) return -1;`) raises menu's ref count enough that it is
no longer spilled at all — it lands in $fp and the whole shape changes.

---- WHY IT IS ONE DECISION, NOT TWO ----

reload1.c 5082-5091: allocate_reload_reg starts its scan at
`last_spill_reg + 1` and wraps `% n_spills`, setting last_spill_reg on each
success (5185) — a ROUND ROBIN.  The function's reload regs therefore run
a3, t0, a3, t0, ...  Site 1 consuming ONE reload reg (target) instead of TWO
(ours) shifts every later reload by one position, which is exactly why site 2
flips t0<->a3.  Fixing site 1 would fix site 2 for free; there is no separate
site-2 bug.

---- CLOSED ----

autorules (23 candidates, no improving edit; both do{}while(0) fences are
load-bearing, unwrapping either costs +16); a bounded decomp-permuter run
(flat at 9, base best); reghist (194/194 insns, delta sum 0 — no
decomposition lever).  The demo build's AdtSelect has a DIFFERENT frame
(0x80E0/0x80E4) yet emits the same a3/t0 split, so the shape is a stable
source property, not a frame-size artefact.  All other allocation (9
callee-saved pseudos + 2 spilled params) matches.

---- ROUND 4: the REG_EQUIV hypothesis is REFUTED, and the real
     discriminator is now named (reload.c 3806-3855, the RETYPE) ----

Round 4 was asked to kill the double-MEM REG_EQUIV note by giving its pseudo
a second-basic-block reference.  Two measured findings:

1. THE NOTE IS NOT ON `menu`.  update_equiv_regs uses regno = REGNO(SET_DEST),
   so the note belongs to `name` (reg 93), whose .greg home is a HARD REGISTER
   ($v0).  Its *value* merely mentions menu's slot.  Everything in reload1.c
   keyed on reg_equiv_memory_loc[93] is gated on `reg_renumber[93] < 0`, so
   with reg 93 in $v0 the note is INERT.

2. KILLING IT CHANGES NOTHING AT SITE 1 (measured).  Reusing `name` as the
   count-loop's test variable makes reg_n_sets[93]==2, so update_equiv_regs
   skips it: insn 48's note list becomes (nil) and the double-MEM count in
   .greg drops to 0.  Site 1 still emits `lw t0,0(a3)`, byte-for-byte.  The
   edit costs +4 (13/776: a pure v0<->v1 rename downstream).  REVERTED.
   => The note was never causal.  Do not re-open it.

THE ACTUAL DISCRIMINATOR (read from the pinned gcc-2.8.1 sources, and it
explains the title-vs-menu asymmetry the earlier rounds only described):

  reload.c 2564  find_reloads passes address_type[i] (= RELOAD_FOR_INPUT_
                 ADDRESS for a read operand) into find_reloads_toplev.
  reload.c 4296  so the reg_equiv_address branch gives A = INPADDR_ADDRESS
                 (the recursion, via ADDR_TYPE) and B = INPUT_ADDRESS.
  reload.c 3806  THE RETYPE, gated on
                   `operand_reloadnum[reload_opnum[i]] < 0 || reload_optional[…]`
                 i.e. it fires exactly when THE OPERAND ITSELF IS NOT RELOADED.
                 Both A and B then become RELOAD_FOR_OPERAND_ADDRESS (3855).
  reload1.c 4618 RELOAD_FOR_OPERAND_ADDRESS returns 0 for any reg in
                 reload_reg_used_in_op_addr — A marks it, so B is barred.

Why `if (title == 0)` self-ties and `name = menu->name` cannot:
branch_zero's 'd' constraint RELOADS the operand, so operand_reloadnum >= 0
and THE RETYPE NEVER FIRES; A stays INPUT_ADDRESS and B is the operand's own
RELOAD_FOR_INPUT, which scans input_addr/inpaddr_addr only for `i > opnum`
(reload1.c 4558) and so may take A's register => `lw a3,0(a3)`.  The deref's
movsi 'm' absorbs (mem (reg 81)), so its operand is never reloaded, the retype
always fires, and RELOAD_FOR_INPUT additionally bars reload_reg_used_in_op_addr
(reload1.c 4546) — which poisons the INPUT path too, even if one were reached.

So the target's site 1 is NOT this insn shape at all: it needs menu's VALUE as
a bare register operand.  Round 3 already proved no C spelling reaches that for
a deref (register_operand rejects a MEM), and round 2 proved a copy cannot
supply it (copies get ALLOCATED regs, never a3/t0).  Both re-verified here
against the actual sources — every claim in the ROUND 3 CORRECTION checks out.

The demo build's ORIGINAL also self-ties at site 1 at a different frame
(0x80E0/0x80E4 vs 0x80C8/0x80CC), so it is a robust property of the original
source, not a frame artefact — but nothing reachable from this C reproduces it.

---- STATUS AFTER ROUND 4: EVIDENCE-COMPLETE, PARKED ----

Round 3 was asked to make site 1's operand require a register.  That question
is answered NO twice over (see the ROUND 3 CORRECTION above): no C spelling
can do it, AND doing it would not produce the target anyway, because
reload_reg_free_p bars the second reload from the first's register on both
the retyped and un-retyped paths.  The lever the last two rounds were aimed
at does not exist.  Round 4 closed the REG_EQUIV lever the same way.

Every source-level lever is now closed with measurements: autorules (23
candidates, none improving), a bounded permuter run (flat at 9), reghist
(194/194 insns, delta sum 0), both do{}while(0) fences load-bearing (+16 each
if unwrapped), the copy lever (dead — reload regs are not allocatable), and
every respelling of the deref (identical RTL: the operand is always a
standalone movsi absorbing (mem (reg 81)) via 'm').  The demo build emits the
same split at a different frame (0x80E0/0x80E4), so it is not a frame artefact.

The remaining question is NOT a C-spelling question: it is why the original's
site 1 has ONE reload where gcc-2.8.1 gives this source structure TWO.  Any
future round should start there — from the .greg RTL and reload.c 4296 — and
should NOT re-open the operand-constraint or copy levers.

---- ROUND 5: independently re-verified against the pinned reload.c/reload1.c
     source (not the header's prose) — ROUND 4's VERDICT HOLDS, and the exact
     discriminator the audit wanted a `--spill-uses` flag for is now named ----

This round was briefed with two "tractability" claims: (1) mips.h defines no
REG_ALLOC_ORDER, so find_reg walks hard regs numerically and whichever pseudo
the target put in $a3 was "allocated earlier"; (2) the target's `lw a3,0(a3)`
is a "reuse a dying variable / coalesce upward" shape reachable by giving the
address and the value ONE C variable instead of two.  Neither survives
contact with the actual pinned source (`/nix/store/*-source/reload.c`,
`reload1.c`, `reload.h` — not memory, read fresh this round, every line
number below checked with `sed -n` against that tree):

1. Claim 1 is TRUE but does not apply to the registers in question.
   `tools/regalloc.py AdtSelect --order` (self-validated against cc1's own
   `;; 18 regs to allocate:` line) lists 18 allocnos; NONE of them dispose to
   $a3 or $t0 — p80/p81 (title/menu) are IN the list, at priority 930/819,
   but their disposition is "—" (spilled).  $a3/$t0 here are RELOAD
   registers, chosen by reload1.c's own, separate mechanism
   (`allocate_reload_reg`, reload1.c:5031), not by global.c's `find_reg`.
   That said, the underlying FACT generalises: reload1.c:3918-3936
   (`order_regs_for_reload`, which builds `potential_reload_regs[]`, the
   pool `allocate_reload_reg` round-robins over) is ALSO `#ifdef
   REG_ALLOC_ORDER` / `#else` ascending-by-regno, so absent that macro
   reload's own candidate pool is ALSO built low-to-high.  This is a real,
   previously-undocumented fact about this cc1 — but it decides which
   register a reload tries FIRST among registers that are already free; it
   cannot make a CATEGORICALLY-conflicting register free.  See point 3.

2. Claim 2 is FALSE for this exact operand shape, and the reason is now
   precise instead of prose.  Two DIFFERENT reload functions handle a
   reg_equiv_address pseudo, chosen by whether it is DEREFERENCED (used as
   a MEM's address) or used BARE (used as a value):
     - `menu->name` makes reg81 the address inside `(mem (reg 81))`
       — a MEM operand — so `find_reloads` (reload.c:2554) hands it to
       `find_reloads_address`, whose REG case (reload.c:4296,
       `reg_equiv_address[regno] != 0`) RECURSES: push a reload for the
       spill slot's own address typed `ADDR_TYPE(type)` = INPADDR_ADDRESS
       (reload.h's `ADDR_TYPE` macro, reload.c:302-306), THEN push a
       second reload for menu's value typed INPUT_ADDRESS (reload.c
       4296's `push_reload(tem, ..., type)`, unchanged `type`) — TWO
       reloads, A pushed strictly before B.
     - `if (title == 0)` and `p = menu` use their pseudo BARE (title as
       branch_zero's compared value; menu as the plain SET source with no
       `->`).  A bare REG operand goes to `find_reloads_toplev`
       (reload.c:4066), whose OWN reg_equiv_address branch
       (reload.c:4090) builds `(mem (its own equiv address))` ONE level
       and calls `find_reloads_address` on THAT — an address that is a
       PLUS (sp + const), not a bare REG, so it never enters the
       recursive REG-in-REG branch at all.  ONE reload, type RELOAD_FOR_
       INPUT (reload.c:2520-2521, since `modified[i]==RELOAD_READ`).
   Confirmed directly in the current draft's own `.greg` RTL
   (`tools/regalloc.py AdtSelect --rtl`): site 1 is insns 470/472 (a3 =
   32972; a3 = a3+sp) + 475 (t0 = mem[a3]) + insn 48 (v0 = mem[t0]) — TWO
   reload insns before the original deref, exactly the two-push shape.
   Site 2 is insns 479/481 (a3 = 32972; a3 = a3+sp) + insn 55 itself
   (`v1 = mem[a3]`, the ORIGINAL low-UID insn, not a reload insn) — ONE
   reload, the value lands straight in p's own destination register.
   `tools/cc1says.py AdtSelect --pass greg` prints this demand verbatim:
   `Need 2 regs of class GR_REGS (for insn 38)` (site 1's insn number
   this round) — nothing analogous for site 2.

3. WHY the two reloads can never share a register — verified against
   reload.c 3806-3855 (the retype) and reload1.c 4514-4630
   (`reload_reg_free_p`), not summarised:
     - The retype (reload.c:3806, gated on `operand_reloadnum[opnum] < 0`)
       fires for INPUT_ADDRESS/INPADDR_ADDRESS reloads whose OPERAND
       itself was never separately reloaded — true here, since the movsi's
       'm' alternative absorbs the MEM without a whole-operand reload.  It
       does NOT fire for RELOAD_FOR_INPUT (reload.c:3806's condition list
       has no INPUT case) — so title/p's single reload is never touched by
       this block at all; it was never eligible to begin with.
     - Site 1's A and B both retype to RELOAD_FOR_OPERAND_ADDRESS
       (reload.c:3855).  `reload_reg_free_p`'s OPERAND_ADDRESS case
       (reload1.c:4618) checks a single INSN-WIDE bit,
       `reload_reg_used_in_op_addr` (set by `mark_reload_reg_in_use`,
       reload1.c:4419) — not indexed by opnum, so ANY two OPERAND_ADDRESS
       reloads in the same insn conflict, unconditionally.  This is why
       the ROUND 3 CORRECTION's "un-retyped escape" also fails:
       RELOAD_FOR_INPUT_ADDRESS's own free-check (reload1.c:4567) tests
       `reload_reg_used_in_inpaddr_addr[opnum]` for the SAME opnum, and A
       (INPADDR_ADDRESS) sets exactly that bit (reload1.c:4407) — barred
       either way, verified both paths.
     - RELOAD_FOR_INPUT's free-check (reload1.c:4546) is different in
       kind, not degree: it scans `reload_reg_used_in_input_addr`/
       `_inpaddr_addr` only for LATER opnums (`i = opnum+1 ...`), never the
       reload's OWN opnum.  So a RELOAD_FOR_INPUT reload is free to sit in
       whatever register ITS OWN address reload (same opnum) just used —
       this is the entire mechanism behind title's self-tie, and it is
       categorically unavailable to a MEM operand, because a MEM operand
       never gets a RELOAD_FOR_INPUT of its own (the 'm' alternative
       absorbs it with no separate operand-level reload to retype from).
   None of this is a priority/ordering question — `reload_reg_used_in_op_
   addr` is a hard conflict bit, not a preference.  Per the cookbook's own
   rule ("a flipped allocation order over a hard-reg conflict is a
   guaranteed no-op"), claim 1's true-but-inapplicable REG_ALLOC_ORDER
   fact cannot rescue this: there is no ordering of reload requests that
   turns a categorical bar into a free register.

4. This closes the audit's own open item (docs/cookbook-audit.md T2,
   "`--spill-uses`: each use BARE vs IN-MEM (the AdtSelect discriminator)")
   by hand: BARE-vs-IN-MEM is exactly the find_reloads_toplev-vs-
   find_reloads_address fork above, and it is fully determined by whether
   the C-level use is a dereference (always IN-MEM, always two reloads,
   always barred) or a plain value use (always BARE, always one reload,
   always self-tie-eligible).  `menu->name` is irreducibly a
   dereference — no C spelling changes which reload function handles it —
   so site 1 can never reach the BARE path while site 2 and title's check
   already sit on it.  `tools/regalloc.py` still has no `--spill-uses`
   flag (checked: absent from its argparse definitions on master as of
   this round) — it would have saved the manual reload.c/reload1.c read
   above, and is worth building for the next residual of this shape, but
   this round did NOT hand-simulate its output — it read the compiler's
   actual source and the actual `.greg` RTL, which is the escalation the
   cookbook prescribes for a sub-C tie once the tooling gap is confirmed.

5. Fresh measurements this round, current tooling, current master (c2e25ea
   base): `tools/autorules.py AdtSelect` — 28 rules / 31 candidates (up
   from round 4's 23; the ruleset has grown), still zero improving edits.
   A bounded permuter run (`timeout 240 tools/permute.py AdtSelect --
   --stop-on-zero -j4`, 22459 iterations + the authoritative rescore) —
   still `9 / 9 / 776`, best candidate is the unmodified base.  The
   permuter's own preflight now names this residual class on sight:
   "a <=10-byte register-swap / adjacent-reorder residual is usually
   sub-C-level (reload/sched) and permuter-immune."

STATUS UNCHANGED: CURRENT(9), still parked.  Unlike some other functions'
park prose in this project, round 4's mechanical citations were checked
line-by-line against the real pinned gcc-2.8.1 source this round and are
ALL accurate — this is not a re-assertion, it is an independent
reproduction with exact line numbers.  A future round should only re-open
this if it can show the target's site 1 is NOT `menu->name` at all
(e.g. a different field order, or the counting loop computing `name`
as a byproduct) — anything that keeps it a dereference of a spilled
pointer-to-struct cannot self-tie in this cc1.
```


## DrawBleed

```
HISTORICAL 47-BYTE PARK (superseded) was ONE clean mirror swap: sched1 (the
PRE-RA scheduler) dragged `dy`/`dz`'s loads from the top of the merge block
to the bottom, shortening their live range so global-alloc gave them v0/v1
while `lui %hi(ViewInfo)` took the block-leader slot and a1/a2 instead of
`dy`/`dz`. That park correctly proved `.flow` already matches the target (so no
STATEMENT reorder inside the block can fix it — sched1 overrides source
order) and that the lever must be sched1's PRIORITY model. It did not have
`tools/sched-deps.py`/`tools/rtlguide.py` yet (both landed after that
park) and its one permuter run predated `d02da20` (permute.py's own
CC_FLAGS mirror still had `-fno-builtin` in the wrong flag group, so that
negative was searching a DIFFERENT PROGRAM than the build — void).

The historical 47 -> 8 checkpoint introduced a genuinely SEPARATE pseudo
for the value `param` already holds, introduced right where sched1 was
choosing to sink `dy`/`dz`. A fresh bounded permuter run (post-fno-builtin-
fix) found it census-first as a random-variable mutation; the win was
verified by porting the semantic delta and re-measuring with
`tools/matchdiff.py`, never trusting the permuter's own proxy score:

  1. `param2 = param;` seeded at the tail of BOTH inner branches (the
     `time==0` -> `ef->proc=0` arm and the velocity-update `else` arm),
     then `dy = param2->pos.vy; dz = param2->pos.vz;` after the if. Having
     a second RTL identity for the same pointer value changes sched1's
     priority computation enough that `dy`/`dz` land early again (47->12).
     Per `tools/regalloc.py`'s own diagnosis ("gcc-2.8.1 has NO coalescing
     pass ... An alias copy survives ONLY if the alias CONFLICTS with its
     source"): `param` is still live after the copy (needed for `dx` and
     later `r`/`g`/`b`), so the copy DOES conflict and survives as a real
     `move` — that's the residual's fixed cost from here on.
  2. `savedTime = param->time;` seeded BEFORE the `dy`/`dz` reads, with
     `param->time = savedTime - 1;` at the original position — the
     "compute into a named temp before an intervening read" idiom
     (§3.13, AfsGetHeader family) fixed the remaining `time`-vs-`dy`
     ordering tie (12->8).

Its former residual (8 bytes) was all one issue — `param2` needed its OWN
register because it conflicts with `param` (proven above), and the allocator
picks a2 instead of coalescing with s1:

             TARGET                          OURS
  0x3ac  nop                             move  a2,s1        (param2=param)
  0x400  lw a1,4(s1)  pos.vy             lw a1,4(a2)
  0x404  lw a2,8(s1)  pos.vz             lw a2,8(a2)

`tools/rtlguide.py DrawBleed` named this precisely: owner
cse/coalescing+regalloc, register goal "a2 -> s1 x2".  The previous round
called that unreachable, but only after holding the invented alias structure
fixed.  The exact source above disproves the conclusion; these three tests
remain useful only as a record of that local minimum:
  - `tools/autorules.py DrawBleed --guided` (the rules rtlguide names for
    this exact signature: ptr-base-split, deref-address-split,
    disjoint-local-alias, identical-arm-fence, etc., beam depth 2, 160
    compiled candidates): no improving edit.
  - Covering the THIRD path too (`param2 = param;` also on the implicit
    `mode!=0` fast path, as an `else` on the outer `if`) costs +2 bytes
    (8->10): that path's `bnez` delay slot is already spoken for by the
    ViewInfo `lui` in the target, with provably zero spare room.
  - Making `param` fully dead after the copy (routing `dx` and the
    `r`/`g`/`b` reads through `param2` too, so the alias no longer
    conflicts, satisfying regalloc's own coalescing condition) does not
    coalesce the copy away — it costs a LENGTH MISMATCH (536 vs 532)
    instead. The register pressure moves, it does not disappear.

A REJECTED further permuter find (8 -> 4, found twice independently by
fresh bounded runs seeded from the 12- and 8-byte checkpoints): deleting
the `param2 = param;` assignment ENTIRELY (from both branches, leaving
`param2` read via `dy = param2->pos.vy` with no reaching definition on ANY
path) links to 4 whole-image differing bytes. This is a genuinely
uninitialized local — gcc-2.8.1 has no def for `param2`'s pseudo, so
global-alloc records no conflict against it and happens to hand it s1,
making the read coincidentally correct FOR THIS EXACT COMPILATION. It is
not adopted: every well-defined attempt to reproduce the same "no
conflict" state (see above) either failed to move the bytes or cost length,
and a construct whose correctness depends on an absent conflict edge
rather than a proven value is exactly the "scores better while being
further from the target's shape" trap the permuter contract warns about.
Left here as a lead: if a later round finds a WELL-DEFINED source shape
that reaches 4 (or 0), the candidate is banked at
`.shake/permuter/DrawBleed/output-10-1/source.c` (regenerated per-run, not
committed) — the transformation to reproduce is
`git diff` against this file with both `param2 = param;` lines removed
and `param->vec.vy = param2->vec.vy + 1;` reverted to `param->vec.vy += 1`.

LOAD WIDTH is separately settled and must not be regressed: `pos.vx/vy/vz`
(VECTOR, `long`) must be captured full-width before the narrowing
scratchpad store; the y/z scalar `s32` views are also what prevent sched1
from sinking those loads.  An early narrowing cast instead emits `lhu`.

Also measured and rejected inside the original 47-byte decomposition (not
claims about other source structures): struct-typed scratchpad casts (CSE
folds the base, -> 496);
plain statement reorders inside the merge block (sched1 overrides them,
-> 512); x/y/z holding differences instead of raw positions (-> 512);
dropping the ViewInfo `(short)` casts (-> 512); volatile scratchpad
stores (536 / 524 / 524, non-monotonic, no form found that lands 532).
```


## DrawImpact

```
HISTORICAL PARK RECORD (the evidence below describes the superseded
checkpoint and is retained because it shows why decomposition-specific
"unreachable" conclusions must be challenged):

Before the final source-identity fix, 4 of 772 bytes differed (was 28).
Exact 772-byte /
193-instruction extent, exact physical CFG, and exact instruction sequence;
the residual is 4 register fields (2 clusters x 2 insns, 1 byte each),
mirrored across the green and blue channels: the start-colour load is $v0
where the target uses $a0.

WHAT FIXED 28 -> 4 (both instances of ONE rule; see the cookbook entry
"Give the value the variable's identity"):
  - `start = param->start_color.channel.r; start = start * inverse;`
    instead of the one-expression `start = ...r * inverse;`.  The load then
    IS the `start` pseudo rather than a separate local temp, reproducing the
    target's `lbu a0,18(s0) / mult a0,a2 / mflo a0` (all $a0).  This ALSO
    dissolved the whole "coupled allocation cycle" the previous checkpoint
    described: ratio -> $a3, the end-colour load -> $a1 and the size
    quotient -> $v0 all fell out at once.  28 -> 8.
  - `start2 = start2 >> 12;` as its own statement before the store, so the
    shifted value is `start2` itself: `sra v1,v1,0xc` (was `sra a1,v1,0xc`).
    8 -> 4.

The previous note's diagnosis was inverted.  It read the four wrong
registers as one irreducible cycle needing "new identity/preference
evidence"; in fact ratio's $t0 was a SYMPTOM.  The cause was the shared
`end_raw` mega-pseudo (6 refs) winning $a3 at allocation slot #9 and exiling
ratio (priority 2068, slot #18) to $t0 — confirmed with `regalloc.py
--order`.  Splitting `end_raw` per site does fix ratio -> $a3 on its own,
but costs +4 bytes; the identity rule above fixes it for free and subsumes
it, so `end_raw` stays shared here.

ROUND 2 re-derived the residual from scratch and CORRECTS the round-1 note
above on three points.  The residual itself is unchanged: 2 clusters,
4 insns, 4 bytes, all register-field-only —
    T  lbu a0,17(s0) / mult a0,a2      O  lbu v0,17(s0) / mult v0,a2
    T  lbu a0,16(s0) / mult a0,a2      O  lbu v0,16(s0) / mult v0,a2
The green/blue start-colour load is $v0; the target uses $a0.  Their product
correctly stays $v1 (start2), so the load is a separate pseudo — reg131
(green) and reg138 (blue) in the .lreg RTL, each born at the load and dead
at the mult.  (Round 1 named "reg126" as the start_colour load; in the
current source reg126 is an `ashiftrt` operand.  Its `;; 125 conflicts: 2
29` vs `;; 130 conflicts: 29` grep refers to pseudo numbers that no longer
exist — do not start there.)

WHY $a0 IS EARNED, AND WHY IT IS OUT OF REACH FROM C HERE.  `start` (p85)
gets $a0 because it is live across the size computation, where $v0 (size's
end product) and $v1 (red's `start >> 12`) are both occupied — hence its
`conflicts: v0 v1` in `regalloc.py --order`.  The green/blue loads occupy a
2-insn window in which nothing else is live, so they conflict with nothing
and take the lowest free reg, $v0.  Every lever that grants them those
conflicts also perturbs something that already matches:

  - Routing green+blue through `start` -> 20 bytes.  NOT for round 1's
    stated reason ("extends `start`'s range to $a1").  Measured: p85's refs
    go 12 -> 16, and floor_log2 is a STEP (3 -> 4), so its priority jumps
    18947 -> 27826 and it moves from allocation slot #4 to #0, where it
    takes $a1 and exiles end_raw to $a0.
  - ...but the priority is NOT the lever either: routing GREEN ONLY gives
    p85 refs 14 / priority exactly 20000 / slot #1, identical
    `conflicts: v0 v1` — and it STILL takes $a1 (20 bytes).  p85 takes $a0
    at slot #4 and $a1 at slots #0 and #1 with the same conflict set, so the
    deciding factor is find_reg's register CHOICE, not the order.
    regalloc.py --order self-validates the ORDER only; its "takes the lowest
    free register each time" model does not predict this (the draw_fade_
    caveat).  Any plan of the form "move pseudo X to slot N so it takes the
    lowest free reg" must be MEASURED, not derived.
  - A shared green+blue load carrier (`s32 col; col = ...g; start2 = col *
    inverse;`) is a literal NO-OP — `nullcheck.py` exits 1, identical
    codegen.  It DOES do what it looks like (col is promoted out of
    local_alloc into a global allocno, p87), but with refs 4 / live 4 its
    priority is 20000 -> slot #1, where nothing conflicts and it takes $v0
    anyway.  Recorded because it looks promising and costs a round.
  - The same carrier extended to all THREE channels -> 24 bytes.  The $v0 /
    $v1 values live across red's load are GLOBAL allocnos (p123 = size's end
    product, p124 = red's `start >> 12`), not local qtys, so they inject
    allocno conflicts, not hard-reg conflicts; col at slot #1 is coloured
    BEFORE them, grabs $v0 and exiles them (ratio -> $t0, end_raw -> $a3,
    red's load -> $v0).
  - Hoisting the carrier's load one statement (above the preceding channel's
    store) -> 764 bytes, LENGTH MISMATCH.  The target's two load-delay nops
    (0x80034024, 0x80034064) are load-bearing: the hoist lets the scheduler
    fill BOTH (2 x 4 = the 8 missing bytes).  So the load must be born
    immediately before its mult and NO range-extension lever exists on the
    load side — which is what closes off the conflict-injection route.

Also measured and rejected: operand swap `inverse * ...` (4, but emits
`mult a2,v0`); `end_raw` per-site split (776); full-inline `/0x1000` (784);
hoisting the load before `size` (45); a named `end_q` (776, kills the
delay-slot fill).  The `do{}while(0)` below is LOAD-BEARING (fence-unwrap
costs 5).  autorules: 54 candidates, nothing — and this run DID include the
repaired `binop-operand-seed` (20 real scores, no `invalid`).  permuter:
18715 iterations, plateaued at its base score, no candidate below it.
`siblingdiff --demo` is a dead end (demo body 236 bytes vs retail 772,
6/193 insns); the PSX.SYM local list above describes the DEMO body.

ROUND 3 independently RE-DERIVED the local_alloc claim above from raw RTL
(not just regalloc.py's summary) and it holds; one more lever was measured
and closed.  All facts below cross-check ROUND 2; none of it moves the 4.

  - `tools/regalloc.py DrawImpact --order` confirms p131/p138 are absent
    from the 19-entry GLOBAL allocno table (`;; 19 regs to allocate: 86 127
    134 141 85 123 87 90 92 88 93 81 84 124 82 117 97 89 80` — matches
    `cc1says.py --pass greg` exactly) and appear only in the LOCAL-ONLY
    list, both `->v0`.
  - Reading the raw `.lreg` dump (`tools/rtldump.py DrawImpact --pass
    lreg`) rather than trusting the summary: green's load (reg131) is "in
    block 8" and so is reg130, and NOTHING ELSE is local to block 8.
    reg130 is red's finished `spr->r` sum (born at its `addu`, dead at its
    `sb`); reg131 is the green load (born at `lbu`, dead at `mult`) — the
    two do not overlap (130 dies exactly when 131 is born), so both
    independently take $v0 for free; block 8's "Registers live at start"
    lists only cross-block GLOBAL pseudo NUMBERS (80 81 82 84 89 97 124
    127), which local_alloc cannot yet be avoiding since it runs BEFORE
    global_alloc assigns them a hard reg.  There is no SECOND local
    quantity in this window to reorder against — which is exactly the
    precondition the AttackBowControl "seed the loser" trick
    (docs/matching-cookbook.md) needs and does not have here.
  - Proved that absence matters, not just asserted it: mirrored the RED
    fix onto GREEN ALONE (not a shared carrier — round 2 tested carriers,
    not this) — `start2 = param->start_color.channel.g; start2 = start2 *
    inverse;`.  `nullcheck.py` confirms it is a REAL edit (codegen hash
    changes), but it merges the load into `start2`'s (p86) own eventual
    register, $v1 — not $a0 — emitting `lbu v1,17(s0)` / `mult v1,a2`.
    Still 4 bytes, same two clusters, just a different wrong register.
    Mechanically this is why: RED's identity trick worked because `start`
    (p85) itself already resolves to $a0; GREEN/BLUE's persisting variable
    `start2` (p86) resolves to $v1, so identity-merging is GUARANTEED to
    reproduce $v1, never $a0.  This closes "give it identity" as a lever
    for green/blue specifically (it is not a re-run of round 2's carrier
    experiments, which shared the two channels; this is the per-channel,
    unshared form regalloc.py's own LOCAL-ONLY hint suggests first).
  - Fresh bounded permuter run (240s wall, output redirected to a FILE per
    the pipe-hang contract note, current post-cc38bd2 tooling): 20603
    iterations, base score 20 never beaten.  Authoritative post-SIGTERM
    full-link rescore ties base.c and all 3 retained candidates at exactly
    4/4/772 differing bytes.  Independently reconfirms round 2's 18715-
    iteration plateau under the now-fixed rescore path.
  - `tools/reghist.py`: delta sum 0 (v0 +4 / a0 -4), a pure register-field
    swap, no opcode or count difference anywhere in 193 insns.

ROUND 4 re-ran the residual against the REPAIRED tooling (the permuter/
regalloc `-fno-builtin` bug that once searched a different program is fixed;
it now lives in permute.py CC_FLAGS, verified) and the new `regalloc.py
--local` quantity walk.  All three confirm the park; none opens a lever.
  - `regalloc.py --local` self-validates (reproduced all 46 of cc1's printed
    homes, 0 divergences).  It lands the green load in block 8 as qty #1
    (p131, refs 2, [10,12), pri 10000), coloured AFTER qty #0 (p126,p130,
    the red `spr->r` sum, [4,8)); the two ranges are disjoint so qty #1 has
    an EMPTY conflict set and takes the lowest free reg $v0.  Blue is the
    identical shape in block 12 (p138).  This is the round-3 raw-.lreg
    finding, now one self-validated tool call.
  - Fresh bounded permuter on the FIXED program: 18979 iterations, base
    score 20 never beaten; authoritative post-SIGTERM full-link rescore ties
    base.c and all 5 retained candidates at exactly 4/4/772.  The fno-builtin
    repair did NOT change the floor — reconfirms rounds 2/3 under the correct
    program.  RESULT.md best candidate == base.c, empty semantic diff.
  - NEW variant, not tested by rounds 1-3: `start` (p85) for the green/blue
    LOAD only, `start2` (p86) for the product — the exact split the target's
    `lbu a0 / mflo v1` shows (load and product in DIFFERENT regs).  Measured
    20 bytes: extending p85 across the colour block flips its GLOBAL home
    $a0 -> $a1 and exiles end_raw $a1 -> $a0 (the a0<->a1 swap is visible
    across red+green+blue in the diff).  Round 1's "route through start" and
    this are now both closed with the load/product split made explicit.

THE UNREACHABILITY IS STRUCTURAL, stated mechanically: the green/blue loads
can earn $a0 ONLY as a GLOBAL allocno whose conflict set contains $v0 AND
$v1 (find_reg otherwise hands a conflict-free value the lowest free reg,
$v0).  The ONLY values that conflict with $v0/$v1 are those live during the
size x red INTERLEAVE (0x80033f94-0x80034018, where size's two `>>12` halves
occupy $v1 then $v0) — which is exactly why RED's load earns $a0.  Green/blue
are emitted AFTER that interleave, so any variable that conflicts with
$v0/$v1 must span red — i.e. BE `start` (p85) — and extending p85 flips it to
$a1 (measured above).  So no source structure grants green/blue's loads $a0
without losing red's.  This is stronger than "conflict-free window"; it names
why the ONLY conflict source is unreachable from the loads' position.

Do not re-open this without a genuinely new SOURCE-STRUCTURE theory: two
independent methods (regalloc.py's conflict list; raw .lreg block
liveness) now agree the load's 2-insn window is provably conflict-free on
our side, and the identity-merge experiment shows the "give it identity"
lever is mechanically guaranteed to land on the wrong register here. A
tool that, given a LOCAL-ONLY pseudo, printed its block's full local-only
roster plus pairwise conflict status directly (regalloc.py --order already
does this for GLOBAL allocnos) would have made this cross-check one call
instead of hand-correlating cc1says --pass lreg against the raw dump.
```


## adiv_tng4_

```
=== HISTORICAL INVESTIGATION (all residual/open claims below superseded) ===
the old residual (s4-group [sw s4,48; move s4,a3] emitted before the s0-group
[sw s0,32; lw s0,96; lui/lw HWD0; li v0,4]) is now byte-exact via TWO source
changes, and the residual moved to a NEW, SMALLER, single-decision tie:

1. `cnt = count;` as the LAST statement before the guard (loop counts cnt).
   The old header's "cc1 COALESCES it -> no-op" was a MISDIAGNOSIS: the copy
   SURVIVES (combine folds insn10's p83=a3 into it, making it read HARD a3 and
   deleting the entry copy — .greg proof), but sched1 was hoisting it to the
   TOP of block 1, keeping its sched2 LUID below the body leaders. Mechanism
   (sched.c): the backward list scheduler's adjust_priority() boosts insns
   whose dest is SINGLE-SET ("birthing", REG_N_SETS==1) to LAUNCH_PRIORITY
   0x7f000001 when they become ready; 12/22/25 are all birthing, the cnt-copy
   (2 sets: init+decrement) never is, so it loses every contested tick and the
   LAST backward pick = TOPMOST placement. rank_for_schedule tie-break within
   equal priority is INSN_LUID DESCENDING (confirmed in source).
2. Statement order `sh#1(hwd/2); vwd = VWD0; sh#2` — the VWD0 read moved
   AFTER the HWD0 sh (was before vp). This opens a one-tick BUBBLE at
   sched1 T-20 (the VWD0 chain's launches drain first; .i.sched shows ready
   list = {copy} ALONE) where the cnt-copy is picked mid-block -> chain pos 10
   -> sched2 LUID above all three leaders -> the leader cluster emits
   [12][22][25][sw s4; move s4,a3] = TARGET. The bubble is the whole lever:
   with VWD0 read early (any earlier position — swept), the copy tops out and
   the old 26-rotation returns.

NEW RESIDUAL (25 bytes = ONE local-alloc color bit): the HWD0/VWD0 divide
region has v0<->v1 swapped: target hwd(HWD0)=v1, li-4 temp=v0, HWD0-div
addu/sra accumulate in v1 (addu v1,v1,v0 — local-alloc ties the addu dest to
the DYING dividend'S qty; the tie fires in ours too, whole qty just colored
v0), VWD0=v0 both. Ours: hwd-qty=v0, '4'=v1. The sh#1-vs-lui slide at
0x80058cc8 is a forced anti-dep consequence of the same bit (our VWD0 lw v0
must wait for sh-v0; target's sh reads v1 so its lw hoists). ONE decision.

REFUTED this round (each measured; ~20 builds):
 - de-birthing the leaders to un-boost them (2nd sets via {4,0x96} or
   {HWD0,VWD0} variable fusion, dead exit-block sets, carrying the return
   through the '4' var or work): every variant either (a) makes the fused var
   die TWICE in block 1 -> local-alloc drops its qty (combine_regs requires
   qty; comment "not local to this block or dies more than once") -> the
   div-chain addu loses the dividend tie -> addu v0,... (wrong, 21..64 bytes),
   or (b) makes the var GLOBAL -> global.c colors it v0-first (no
   REG_ALLOC_ORDER on MIPS) -> same color loss; dead sets are flow-deleted
   BEFORE sched1 recounts REG_N_SETS, so they never de-birth anything.
 - guard-variable/counter splits (single-set cnt for a birthing boost, n=cnt
   in preheader): allocation wall — cnt never reaches s4: its a3 copy-pref
   wins while a3 is free (V1 build: beq a3 + preheader move s4,a3), and with
   a3 conflicted it falls to v1/t1 (find_reg numeric order), never s4.
 - statement-permutation sweep under the working core (10 variants: vp
   positions, const-store hoists, cnt positions, hwd/vwd decl swap):
   all 25 (color bit inert) or worse (64-81: moving hwd=HWD0 after *work=4
   flips the leader LUID order; hoisting VWD0 reverts the rotation).
 - narrow count, non-volatile shift/6, HWD0[] array form: as before
   (see git history for the prior header's full text).

The color bit DECODED (QTY_CMP_PRI = floor_log2(refs)*refs*1e4/(death-birth),
qsort desc, then qty# asc): the contested qty is the TIE-CHAIN TRIPLE
{hwd(85), addu-dest, sra-dest} — block_alloc ties each op's dest to its
dying first operand — refs 3+2+2=7, log2=2. Its rival, the li-4 temp qty
(refs 2, span [li..sw]=2 suids), scores 10000. The triple's span runs from
the HWD0 lw to the sh#1 STORE's sched1 slot: in the color-good (pre-bubble,
VWD0-early) chain the VWD0 lw interleaves INTO the div chain, stretching the
span to 16 suids -> 8750 < 10000 -> '4' allocates first, takes v0, and the
overlapped triple falls to v1 = TARGET. In the bubble chain the div chain is
compact (12 suids -> 11667 > 10000) -> triple takes v0 first = our 25.
Span-stretch attempts (split `h = hwd/2; vwd = VWD0; sh#1 = (short)h`
with fresh h / hwd-self-update / cd-reuse) all re-broke the WINDOW
(26/26/43) — the stretch and the bubble compete for the same sched1 slots.

PROVEN IRRECONCILABLE at single-statement reach (round 3 close-out; a
12-variant decoupling sweep under the color-good core all 26+ or LENGTH
MISMATCH, plus the .i.sched gate analysis):
  - The copy can only sink via a tick where it is ALONE-ready. Ticks are
    filled by pri>=2 stores unless GATED: a store is unready until every
    program-LATER may-alias load is placed (anti-dep, backward sched). The
    one protectable tick (T-20) is created by the VWD0-lw's latency-2
    launch, and it is empty ONLY when sh#1 and the *work=4 store are gated
    BEHIND that same VWD0-lw, i.e. `vwd = VWD0` AFTER sh#1 in source.
  - The color bit needs sh#1's sched1 slot at pos>=10 (div-qty span >= 16
    -> 8750 < the '4' qty's 10000). sh#1 reaches pos 10 only UNGATED
    (VWD0-lw program-before it) — and then sh#1 itself fills T-20 (V8
    dump: sh#1 picked exactly T-20; the copy exiled to T-29 = the old
    rotation). One tick, two claimants; every legal gate assignment hands
    it to exactly one.
  - The cascade anchor cannot shift: t0's lw is capped program-BELOW
    every work-store by may-alias ordering (target bytes: lw 4(v0) before
    li 150), so the uVar launch timing that positions the mid-region is
    rigid, and the tick budget is fixed by the insn count.
  - Fallbacks closed: ref-lowering the div qty must keep the addu-dividend
    tie (target addu v1,v1,v0; tie loss measured = 21-byte build) — the
    best legal reduction {hwd,addu} scores EXACTLY 10000 = the '4' qty,
    and qty_compare_1's tie-break (qty number asc = birth order) still
    picks hwd first -> v0. Raising the '4' qty needs a third ref in a
    2-suid span: no existing reader, any new one emits (+4), arithmetic
    double-reads tree-fold, and dead exit reads are flow-deleted before
    the REG_N_REFS recount (measured).
A fix, if one exists, needs a construct OUTSIDE single-statement reorder
(e.g. cc1 instrumentation to enumerate find_free_reg outcomes, or a
different decomposition of the /2 chains that re-times the whole block).
============================================================================

WHAT UNLOCKED THE 620 PARK (each independently measured; the park's
"failed" singles were pair-negatives, cookbook §4):

1. SUPERSEDED: -fno-strength-reduce appeared necessary only while the input
   record was flattened into a u_short pointer. The old argument was:
   r0 lives in $a3 with caller-save spill/reload around the jal
   (sw a3,24(sp) in the delay slot / lw a3,24(sp) after), and
   CALLER_SAVE_PROFITABLE(refs,calls) = 4*calls < refs needs r0's refs
   loop-depth-weighted (flow.c REG_N_REFS += loop_depth) => real LOOP
   NOTES (do-while, not the park's goto hack); with notes, loop.c would
   reduce the 21-giv cursor class (combine_givs sums benefits) into a
   second induction register the target provably lacks — no C spelling
   denies a 21-member DEST_ADDR class, so SR was off. This one flag +
   do-while replaces the park's items 1-2 (the goto loop was compensating
   for the wrong root cause and killed the ref weighting).
2. volatile u_long shift/ot (CdaPlayXA parameter-qualifier
   precedent), read once each (shiftWord / the org load) at their statement
   positions. Plain stack parms' entry copies are pinned at the top and
   local-alloc colours them a1/a0 — p85->a0 is what evicted primtop into
   the park's `move t0,a0`. Volatile keeps the reads at their use sites
   (lw v0,92/lw v1,88 late), a0 stays free, primtop's pseudo takes its
   copy-preference for $a0 and the entry move vanishes.
3. work = wp local copy carrying ALL body uses. update_equiv_regs
   DOUBLES a REG_EQUIV-noted single-set parm's live length
   (local-alloc.c:1153), halving wp's global priority (17880) below
   puVar4's (21120) -> wrong s0/s1. The unnoted local scores ~2x and wins
   s0 first; the copy coalesces away via the hard-reg copy preference.
4. po/vp: BOTH defined above the guard (po = work+0x38;
   vp = po), loop stores via vp (the copy -> s3), call arg
   = po (the original -> spilled 16(sp), reload-inherited move s3,t0 +
   sw t0,16(sp) in the beqz delay slot, lw a0,16(sp) at the call).
   This is the park's two "failed" singles applied JOINTLY.
5. code = 0x3c named once (li v0,60), stored to +0x53 through it, and
   cd = code inside the guard -> the target's `move s5,v0` (a real
   register copy, not a rematerialised li).
6. The gte address-argument scratch reuse: t0/t2/t1 (and their
   disjoint second lives) host the stsxy3/stsz4 address temps as OWN
   STATEMENTS (in-macro-arg assignments get renamed by combine's
   new-pseudo path — measured, +8 bytes): t0 = work+0x24 (a1, also the
   packet-word life), t2 = work+0x23 / work+0x2a (a0 pair), t1 =
   work+0x30 (v1, also the shift read). Multi-set kills both
   move_movables hoisting (the +144/+168 temps hoisted+spilled otherwise;
   loop.c runs before combine so the kill survives the rename) and the
   volatile read's birthing bump.

Final verification: the unguarded C object has 224 target/body instructions
and byte-matches the 0x398-byte target extent exactly.
```


## briefing_screen_

```
HISTORICAL NOTES BELOW ARE SUPERSEDED and retained only as an experiment log.
KEY FINDING: the `do{sequence=0;}while(0)` fence was NOT load-bearing — it walled
off the basic block sched2 needs in order to interleave the callee-saved register
spills with the init writes; removing it (plain `sequence = 0;`) let the saves
interleave as retail does and closed 25 bytes on its own. Moving `fade_step = -8`
to just after the first PathFileRead lets it fill that call's branch delay slot.
Both position narrowings are now explicit in-place shifts (`position <<= 16;
position >>= 16;` and `position = counter << 16; position >>= 16;`) so the
sign-extend is `sll s0,s0,16; sra s0,s0,16` in place of a v0 scratch.
Retained: snapshotting old_pad before its write (delay slot), the distinct
full-word tpage capture (lw/nop/sra), capturing the call result/xbase before
their stores. LOAD-BEARING (confirmed by regression on removal): the outer strip
`do{}while(0)` (length mismatch without it) and the nested position-carrier
fences (regress to 100).
REMAINING (all greg register/schedule ties, no source lever found; rejected:
single-expr scroll=128, tpage/xbase var-merge, `3*x`):
(1) prologue — the prefix/s2 setup schedules after the sequence/fade spills
instead of first, and the two init constants land in v0 where retail reuses t0
(xbase materialized 0xff60/ori vs retail -160/addiu); (2) four caller-saved ties
where retail reuses t0 for load temps (tpage_word->a2, xbase_value->v1,
scroll-adjusted->a2) and v1 for the brightness `0xa0-position` intermediate (->v0)
— rtlguide hard-conflicts a2->v0 (p89/p96/p197), v0->v1 (p82); (3) position=counter's
in-place sign-extend is scheduled two insns before the GetTPage(0,0) arg moves.
Fuzzy: 94.48% (up from 90.88%).
ROUND 2 (fresh, skeptical re-verification of the above — all CONFIRMED, not
refuted): tools/autorules.py unguided (73 candidates) and --guided off a fresh
rtlguide run (160 candidates, budget-capped beam) both report no improving
edit; tools/permute.py (-j4, 16457 iterations, stop-on-zero) plateaus exactly
at 87 with zero candidates beating base.c. Four NEW hypotheses tested and
REJECTED (each rebuilt+remeasured, not just reasoned):
- `xbase` as `s16` (to get retail's `addiu -160` instead of `ori 0xff60` per
  the addiu/ori-names-the-type rule): regresses 87->1464. Root cause read off
  the objdump: cc1's CSE now recognises the signed -160 constant as the SAME
  value as the later `sprite.x = -160;` (also genuinely -160 in target) and
  merges them into ONE persistent value, promoted to a callee-saved register
  (`li s3,-160` + an extra spill) to survive the 6 intervening calls — even
  though TARGET keeps these two materialisations separate (t0 early, v0 late,
  never merged) despite being the identical constant. Whatever makes retail
  avoid that merge is not reproduced by simply signing the constant.
- Same constant via a two-step signed local (`s16 xbase_init = -0xa0; stack.xbase
  = xbase_init;`, field left `u16`): identical regression to 1464, same root
  cause (the local still carries -160 as a signed SImode constant that CSE
  coalesces with sprite.x's).
- Replacing every `PSTATE->StageNo`/`PSTATE->language` in this function with the
  named externs `CHOSEN_STAGE`/`CHOSEN_LANGUAGE` (used elsewhere in this file
  for CHOSEN_CHARACTER/STAGE_LAYOUT_NUMBER, and pervasively in StageEndScreen/
  mission_score_screen): regresses 87->1488. cc1 cannot fold two INDEPENDENT
  extern symbol_refs' `%hi` together, so every use re-materialises its own
  lui+lbu instead of sharing one base register. CAUTION for future readers:
  the target .s file's own `%hi(CHOSEN_STAGE)` labels are NOT evidence of the
  source spelling — they are splat's disassembler matching the final computed
  address against the symbol table, and our baseline's `PSTATE->StageNo` access
  already produces BYTE-IDENTICAL bytes for that instruction (confirmed: it
  was never part of any diffed cluster). Do not re-attempt this lever without
  new evidence.
- Merging `adjusted = stack.scroll; adjusted += StageScrollAdj[lang][stage];` into
  one statement `adjusted = stack.scroll + StageScrollAdj[lang][stage];`: regresses
  87->97, same length, but DOES prove the two-statement split is a live,
  non-neutral lever for that hunk (load order changed) — just backwards from
  what's needed here. The reverse pairing (index loads merged some other way)
  was not tried; a future round could probe adjacent orderings of this hunk
  specifically (target: PSTATE->language/PSTATE->StageNo lbus BEFORE the
  `stack.scroll` reload, i.e. index computed before the reload it's added to).
rtlguide's "register goals" section (re-run fresh) shows the a2<->v0 and
v0<->v1 swaps are genuine HARD-CONFLICTs (cannot be reached by priority/weight
alone), but the four ->t0 goals (v0->t0 x4, a2->t0 x3, v1->t0 x2) carry NO
hard-conflict annotation — t0 is simply never tried by find_reg's numeric
search because a lower-numbered register is always free first. No source lever
was found this round to make t0 the winner; a future attempt should look for
what makes v0/v1/a2 UNAVAILABLE at those points in target (extend some other
value's live range to occupy them), not try to prefer t0 directly.
ROUND 3 (the 5 do{}while(0) fences audited one at a time — which are real,
which are ours): a macro search across every header AND every .c file's own
local #define in src/main.exe found NO function-like macro anywhere whose
body matches any of the 5 — the only do{}while(0)-shaped macros in this TU
family are DRAW_SCORE_NUMBER/DRAW_SCORE_COLON (mission_score_screen.c,
StageEndScreen.c), unrelated in shape and purpose. So none of the 5 are
macro hygiene by the letter of that test. Measured anyway, per-fence,
BOTH by hand (rebuilt+remeasured, one fence unwrapped at a time and in
combination) AND by `tools/autorules.py`'s independent `fence-unwrap` rule
(both agree exactly):
  - outer strip `do{ if(counter>=0){...} }while(0)` (L290): unwrapping it
    alone scores 87->1012 (autorules) — a de facto length mismatch, same
    conclusion as round 1's "length mismatch without it", now with an exact
    number. It wraps a REAL conditional block, not a bare statement — a
    plausible genuine "skip the rest of this case without goto" idiom. KEEP.
  - the three position-carrier wrappers around `position = signed_width -
    position;` (L307/L309/L311, nested 3 deep): unwrapping ANY ONE of them
    — even the innermost alone, even with the other two still wrapping it —
    scores 87->91, and unwrapping any subset (one, two, or all three) never
    scores worse OR better than 91: it is ONE dial, not three. Depth 3
    keeps tpage_value in s2 and signed_width in s1 (matching target);
    dropping below depth 3 flips the pair to s1/s2 swapped — a genuine
    local-alloc QTY_CMP_PRI tie, not a scheduling-order tie (the flat and
    fenced forms emit the IDENTICAL instruction sequence, only two register
    assignments swap).
  - `do{ sprite.tpage = tpage_result; }while(0)` (L318): unwrapped alone,
    87->93 — the single costliest fence, autorules-confirmed. Unwrapped
    TOGETHER with the other three (full flatten to 5 plain statements),
    the total is still only 91, i.e. its individual cost is absorbed once
    the other three are also gone (not additive — same 91 floor either way).
  CORRECTION to round 1: "the nested position-carrier fences (regress to
  100)" was measured against an earlier intermediate state of this file and
  is stale; the correct, freshly-measured number against the CURRENT source
  is 91 (any subset of the three) or 93 (tpage alone), never 100.
  A genuinely different, non-fence, non-flatten restructuring was also
  tried: folding the whole computation into one expression close to
  Ghidra's natural rendering (`xbase_value = stack.xbase; position =
  xbase_value - (signed_width - position) * 8; sprite.tpage =
  tpage_result;`) — this is WORSE (105), refuting the hypothesis that the
  5-statement decomposition through a reused `position` carrier is itself
  the byte-chased part; that decomposition is closer to target than the
  more "natural" single-expression alternative.
  PRECEDENT (same TU family): StageEndScreen.c has the identical pattern —
  `best_x = 0x80010000;` wrapped in FOUR nested do{}while(0) (its L498-510)
  with the comment "Preserve the target allocator weight for this reused
  identity", and its own STATUS header states verbatim: "The four nested
  do{}while(0) ... are LOAD-BEARING, measured: autorules' fence-unwrap
  scores each at 202->227. They are not scaffolding; do not remove them."
  That is this project's own established standard for exactly this
  situation: once a genuine macro search comes up empty, MEASURED
  NECESSITY — not textual macro provenance — decides whether a fence
  stays. This is NOT the SetWire / mission_score_screen situation (a worse
  byte count adopted because a genuinely BETTER, more complete alternative
  structure was found): here every alternative tried (flatten in every
  combination, single-expression fold) was strictly WORSE with no
  compensating gain in plausibility — there is no suppressed better
  structure to prefer, only an unresolved register-coloring tie the fences
  currently paper over. VERDICT: keep all 5 fences.
  Re-confirmed fresh: `tools/autorules.py` unguided (73 candidates,
  including all 5 fence-unwrap candidates individually) finds no improving
  edit; a bounded fresh permuter run (240s search + rescore, base score
  1095) never beats its own base score, consistent with round 2's
  16457-iteration run. No orphaned permuter state left behind.
ROUND 4 (re-verify with the -fno-builtin permute.py CC_FLAGS fix + hunt a
STRUCTURAL lever, per an owner directive that an 87B residual "hides a
missing/mis-shaped block"). RESULT: the structural premise is FALSIFIED and
87 is re-confirmed as the hard floor.
  - CFG IS IDENTICAL: matchdiff/asmdiff show 362 vs 362 insns, no missing or
    extra branch, all 15 diff blocks are register renames + 2 local schedule
    swaps (prologue prefix-order, GetTPage(0,0) arg-move vs position sign-ext).
    There is no missing/mis-shaped block to find — the residual is genuinely
    sub-C register allocation (t0 numeric-order, the a2->v0/v0->v1 hard-conflicts).
  - FRESH permuter WITH the -fno-builtin fix (the reason this round exists):
    19444 iterations, -j4, --stop-on-zero. Authoritative full-link rescore:
    base.c 87 is best; every retained candidate rescores >=87 (output-860 has a
    BETTER proxy score 860 but rescores to 91). The fix changed nothing; the
    round-2/3 plateau claim stands.
  - autorules re-run (73 candidates) — no improving edit (unchanged).
  - NEW structural hypotheses tested & REJECTED (each rebuilt+remeasured):
    * index-first scroll (`adjusted = StageScrollAdj[..]; adjusted += stack.scroll;`
      — round 2's own "untried" suggestion): 87->102. The scroll-first split is
      optimal; the reverse pairing is worse, not better.
    * tpage_value as a direct `stack.tpage_base >> 16` OR a plain two-statement
      capture (dropping the identical-arm `if (strip_width!=0)`): LENGTH
      MISMATCH 1440 (-2 insns). That `if` is LOAD-BEARING: it is a control-flow
      merge that blocks cse's store->load forwarding of the prologue
      `stack.tpage_base = strip_px<<16;`, forcing the target's fresh lw/sra
      reload instead of reusing strip_px. Not scaffolding — it reconstructs a
      real target reload (cookbook "a redundant reload = a fresh dereference").
    * shared brightness intermediate (one `bright_base` var feeding both
      `(position+0xa0)*3` and `(0xa0-position)*3`): LENGTH MISMATCH 1436;
      cc1 merges the arms. Target keeps them separate (branch1 intermediate on
      v1, branch2 on v0 — the residual v0/v1 tie, a HARD-CONFLICT per rtlguide).
    * capturing the first PathFileRead 2nd arg into a local before the inits
      (to occupy v0 across the const materialisations, per round 2's hint):
      87->127; it hoists the whole index calc and reshuffles the prologue.
    * reorder `signed_width` birth before tpage + flatten the position-carriers
      (test whether birth-order gives target's s1/s2 without the fences):
      LENGTH MISMATCH 1452. Birth-order does NOT substitute for the loop_depth
      ref-weighting the nested fences provide.
  - FENCE VERDICT (owner asked to challenge them): the position-carrier nested
    do{}while(0) is the SAME matched technique as the matched sibling
    BriefingAndInventorySelectionScreen (lever #3: "NESTED do{}while(0) (two
    deep)... each level adds +1 loop_depth to flow.c's ref weighting, flipping
    the {v0,v1} pairing"; lever #5: do{}while(0) carrier fences). That sibling
    matched at 0 WITH these. So the fences ARE "the human structure the matched
    sibling shows", not invented scaffolding — KEPT. `siblingdiff --demo` found
    no PSX.EXE sibling, but ROUND 5 found the renderer in demo/GAME.EXE.
ROUND 5 (compiler output first, with the earlier cookbook conclusions treated
as hypotheses): demo/GAME.EXE has a copy of this strip renderer at
0x800431e0..0x80043344. Its data flow is ordinary human C: `u = counter * 4`,
`GetTPage(0, 0, (s16)tpage + counter, 0x100)`, an x position derived from
`(s16)width - counter`, and edge brightness `(160 - x) * 3`. Literal ports of
those expressions do not reproduce this build's allocation: the direct call
expression scores 83 bytes and the single x expression changes the length.
The pinned compiler's sched2 dump explains the four-byte call-order residual:
the sign-extension and a0/a1 argument setup are equal-ready UID choices; the
spelling that fixes their order also shortens the counter lifetime and loses
the target's s0 position carrier after the call. A split right-edge brightness
expression produces retail's v1/v0 order exactly, but jump2 cross-jumps the
now-identical multiply tails (as `rtx_renumbered_equal_p` permits after reload)
and shortens the function to 1436 bytes. A whole padded working struct, moving
the first read, moving xbase across the call, narrow/volatile locals, and
direct human constant spellings were all measured neutral or worse. The one
coherent improvement retained is a named right-edge brightness intermediate;
```
