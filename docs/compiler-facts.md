# Verified cc1 2.8.1 mechanics — the audit substrate

Facts about the pinned compiler, each verified against the gcc-2.8.1 sources on
disk and/or measured on a named function. This file exists so an agent can
**audit a tool's verdict**: when `regalloc`/`schedtrace`/`rtlguide` prints a
claim, these are the mechanics it rests on. It is NOT assigned reading — grep it.

Rules of evidence: a mechanism claim is worth exactly as much as the source line
it cites. `tools/ccsrc.py` surfaces the pinned gcc tree (an unpacked directory
despite its name: `/nix/store/117i80brbgcdmcl46gmpzwizikbjyx5m-gcc-2.8.1.tar.gz/`).
Two lanes have "remembered" gcc code that does not exist (a cost comparison in
`reload_cse_simplify_set`; `rare_destination` returning 1) — open the file.

## Front end / trees

- **`*(&x) = v` does NOT stack-force `x`** in this cc1 — the `*&` fold beats
  `mark_addressable`, so `x` stays a register candidate (measured: frame shrank 8
  bytes on SetWire when a `*&`-spelled out-param stopped forcing its local). A
  macro out-parameter reconstruction that must genuinely address a local needs a
  REAL pointer local (`T *rxp = &rx;`), not the `*&` spelling.
- **Power-of-2 division emits by DESTINATION**: rvalue `/K` on a fresh temp in an
  expression (`t*t/0x1000`, accumulator sums) emits the IN-PLACE
  `bgez; addiu 0xFFF; sra` shape, while a store-back `/K` on a user variable
  (`dx /= 0x100;`) emits the copy-in-delay-slot shape. Both are reachable from
  plain human `/ONE` spellings — pick the one whose destination matches the target
  (SetWire).

- **Enum struct fields are 4 bytes** (no `-fshort-enums`) — a raw enum-typedef
  field silently shifts every later field; repo convention is
  `u16 name; // enum X` (character_state). An enum-typed dispatch parameter
  with all-non-negative enumerators compares UNSIGNED (`sltiu`), a plain `s32`
  gives `slti` (ProcMiscSprite/ProcMiscFire).
- **`convert_to_integer` narrows `NEGATE_EXPR` through a truncation** (convert.c),
  so `narrow = -narrowVar` can never sign-extend first; a separate SImode operand
  variable blocks the narrowing and keeps `sll/sra` + `negu`-on-extended
  (mission_score_screen).
- **ARRAY_REF gate**: `c-typeck.c:1406` builds an ARRAY_REF only when the operand
  has ARRAY_TYPE **and is not an INDIRECT_REF**. A pointer-to-array cast
  `(*(T (*)[N])p)[i]` is an INDIRECT_REF → index-first arithmetic; a one-field
  wrapper struct `((Wrap *)p)->a[i]` is a COMPONENT_REF → base-first, no frame
  rematerialisation (AddEnemy cluster D).
- **fold: `A op 0 ? A : -A` → ABS_EXPR** (fold-const.c) — but only the GE spelling;
  the LT ternary re-binds `arg1` after swapping arms so the abs check misses and
  expansion takes the branchy path (SoundEx, UpdateMotion). `x >= 0 ? x : -x`,
  `__builtin_abs(x)`, and an assigned builtin are IDENTICAL post-fold
  (ChasetoTarget, by direct experiment). Open-coded `t = x; if (x < 0) t = -x;`
  never reaches ABS_EXPR.
- **fold reassociation**: `split_tree` builds `a - (C - b)` as `(a - C) + b` and
  `a - (b - C)` as `(a + C) - b` (it builds, does not refold); the naive
  `a - C + b` is reassociated to `a + (b - C)` (HangCheck). `A + (B + C)`
  canonicalises to `(A + C) + B` — the constant migrates to the first operand
  (vfree, MoveKorogari).
- **`x & (1 << n)` (variable n) always canonicalises to `(x >> n) & 1`**
  (`srav`+`andi`) — verified standalone; the literal `sllv`+`and` shape needs a
  named `mask = 1 << n` first.
- **`expand_assignment` is LHS-first** for COMPONENT_REF/ARRAY_REF destinations
  (`get_inner_reference` + offset expand precede the RHS expand) — measured: RTL
  moves, bytes identical when the multiset already matched (nullcheck).
- **expand evaluates a binary op's operands left-to-right**, so the LEFT operand's
  pseudo is born first and lives longer (binop-operand-seed's mechanism).
- **A constant-last element address (`addu rX,sp,rIdx; addiu rX,rX,K`) can ONLY be
  born from an INDIRECT_REF deref via EXPAND_SUM.** `expand_expr`'s PLUS
  (expr.c:6216) sends every NON-EXPAND_SUM pointer sum to `binop` — base-first,
  the `addiu` materialised FIRST — and c-typeck folds `&*p` (3038), `&a[i]`
  (3047) and `&p->member0` (3113) back to the plain sum. So **no C pointer
  variable, `&array[i]`, or call argument can be born constant-last**: if the
  target's element pointer is constant-last AND feeds a call arg, that shape is
  UNREACHABLE from C, not a tie to tune (mission_score_screen's bank pairs, 65
  bytes, now closed WITH mechanism; the M-form measured +24, plain-index −16).
- **EXPAND_SUM special-cases a mult-by-constant subterm inside an address** —
  it expands FIRST regardless of source order; a shift spelling or a named value
  temp preserves source order (`arr[a + b*c]`). `form_sum` sorts a CONSTANT term
  last: `(plus reg_index CONST)` emits index-first `addu` (mission_score_screen).
- **`expand_case`** builds a value-sorted compare tree over a FRESH index load but
  lays case bodies in SOURCE order; a case ending in `return` is pushed later
  (LayoutEnemyOption, Makibishi). An if/goto ladder CSEs the load and compares
  small unsigned types `sltiu`.
- **`store_flag`** cannot put the constant in the immediate field for `>`:
  `cond = x > K-1` folds to `lui`/`slt` vs `K<<16`; `cond = x < K` is the natural
  `slti` (FUN_8005a7a4).
- **`main` gets an implicit `__main()` call** inserted by gcc; writing it
  explicitly compiles a second call (main).

## Modes, parameters, frames

- **`promote_mode` is the IDENTITY on this target**: mips.h defines no
  `PROMOTE_MODE` and no `POINTERS_EXTEND_UNSIGNED`. Never reason about which types
  it moves. This constrains direct formal-parameter conversion; it does not
  constrain later ordinary local copies (the distinction that matched
  FUN_80057b80).
- **Narrow locals are genuine QI/HI pseudos** (`reg/v:QI`, `reg/v:HI`); a narrow
  local always needs a real extension and can never produce a bare `move` at a
  join (ActivateHumans). A `short`→`short` copy coalesces to NOTHING; routing
  through an SImode temp restores the truncating move (ControlTraceLine 167→10).
- **`assign_parms` (function.c:4142)** emits arg-register copies in declaration
  order UNLESS `nominal_mode != passed_mode`; then the real assignment is deferred
  into `conversion_insns`, flushed after all parms — so a narrow first parameter's
  copy gets a LUID greater than every other parm copy. Used as a VALUE the
  deferred insn stays a bare `move` (truncation is lazy, at uses: cd_control);
  used as a POINTER BASE the deferred insn IS the truncation (`andi`/`sll;sra`).
  `DECL_ARG_TYPE` overrides the declared type only for float→double and the
  C_PROMOTING_INTEGER_TYPE_P (char/short) list (c-decl.c:5048-5065, 5437-5444).
- **A promoted `short` parameter is TWO pseudos for free**: the raw SImode
  arg-register copy plus the HImode declared variable (Sound).
- **Frame slots are assigned in THREE phases**: `assign_parms` (addressable
  params) → `expand_decl` (declared locals) → reload's `alter_reg` (spilled
  pseudos, in pseudo-number order). A declared local can never sit above a reload
  spill (FUN_80032720's park proof, in its header).
- **`assign_stack_local` assigns slots in declaration order**, ascending from the
  outgoing-arg boundary, each BLKmode local rounded up to 8 bytes — slot order is
  arithmetic, not searchable (FUN_80018f00; AdtMessageBox: each separately
  declared aggregate is individually rounded; LoadConstruction: odd 20-byte
  aggregates spaced 24 apart are N separate locals).
- **`mips_expand_epilogue` ALWAYS lets reorg pull the sp-restore into `jr ra`'s
  delay slot** for a trivial frame — a bare `nop` there with the restore before
  the jump is another compiler (the SDK-boundary tell; verified via `.dbr` +
  sources).
- **A varargs `&fmt + sizeof(fmt)` computed at entry** is held in a callee-saved
  register across the whole function; inline at the call site it uses a
  caller-saved temp (AdtMessageBox vs FUN_8005fe38).

## cse

- **cse hashes memory by ADDRESS RTX**: a store through a pinned register base
  cannot forward to a load spelled sp-relative — two spellings of one field are
  two cse identities (FadeOutDirect).
- **A struct store kills store-to-load forwarding for EVERY offset off that
  base** — `memrefs_conflict_p`'s offset math predicts dismissal that does not
  happen; bisect before believing it (GetAreaMapVector).
- **cse1 ends its extended block at every CODE_LABEL and at
  `NOTE_INSN_LOOP_END`; cse2 runs `after_loop=1` with a WIDER block and crosses
  loop notes.** `make_regs_eqv` promotes a copy's dest to canonical only if it
  outlives the block — so cse2 can fold back a copy cse1 kept
  (GetAreaMapVector). A fence can never permanently split same-value `lui`s.
- **Naming the SENTINEL constant vs naming the FETCHED value are DIFFERENT
  allocation levers** for a `(T*)field != (T*)SENTINEL` compare. Only naming the
  constant (`T *one = (T *)1; if (field != one)`, ProcItemFire.c's idiom) forces a
  separate materialising insn that participates in allocation; naming the fetched
  value (`enemy = field;`, DamageControl.c's idiom) does not reproduce that win.
  Test both independently before concluding a permuter's local-introduction is
  inert (WeaponHitWeapon, verified both ways).
- **`record_jump_equiv`** records a guard's comparison on its FIRST slt operand's
  quantity (GetConflictResult), and on a `beq reg,CONST` taken edge records
  pseudo==constant — surviving calls (ProcItemGun's literal case stores).
- **`find_best_addr`** rewrites `MEM(ptr)` to `MEM(base+K)` only where the
  defining equivalence is in cse1's table — rebuilt at every multi-predecessor
  label; nonzero offsets are never folded (FUN_8004c59c).
- **cse's tables do not follow the taken edge** of a branch — constants proven on
  the fallthrough path are unavailable in branch-taken bodies (the nested-switch
  fresh `li` fact).
- **cse has a (set REG0 REG1) copy-swap special case for ADJACENT pairs**
  (`prev_nonnote_insn` adjacency): `la` + copy back-to-back gets rewritten so the
  la lands in the copy's dest (AddMisc).
- **`MEM_IN_STRUCT_P` alias heuristic (cse.c and sched.c)**: a varying-address
  struct-member store does not invalidate a fixed-address non-struct scalar's
  cached load (ActHANG vs ActSYURI d-global reloads); sched.c:860
  `anti_dependence` dismisses load→store only when the LOAD is `/s` varying and
  the STORE is non-struct fixed. A width-forcing cast `*(u16 *)&p->field` is an
  INDIRECT_REF and CLEARS `/s` (HumanActionControl's 11 bytes).

## combine

- **combine fuses `(set (reg:HI N) (mem:HI))` + `sll` + `sra` into one `lh` only
  when N has ONE use** (`added_sets_2`); with two reads of a short field the
  original `lhu` survives for the narrow use — the lever is which read comes
  first (nullcheck; the (u16) cast is a same-mode no-op there).
- **`s16 local = p->field;` does NOT pin the load** — combine relocates the fused
  sign-extend load to its USE; declaration order and s32 flips are inert.
- **`num_sign_bit_copies` elides a redundant call-arg narrow only for a
  single-assignment `(s16)`-cast local** (AddEnemy).
- **combine merges a compare into its branch and `find_split_point` re-splits AT
  the branch** — you cannot park a statement between a compare and its branch;
  the compare's shape changes too (`sra/slti` → `lui/slt`).
- **A reference combine later folds away still counts in `reg_n_refs`** — a
  provable no-op use (`(X & 0x7fffffff) << 2`) is a free ref across a
  `floor_log2` step (vmemoryGC's 12-byte park; vfree's mask second use).

## loop.c

- **Preheader hoist ORDER is the loop-body SCAN order of the hoisted sets**
  (`move_movables` emits in `scan_loop` discovery order), and hoists land AFTER any
  insn the source already put in the preheader. **So a target preheader with a
  hoisted CONSTANT before pointer assignments proves those pointers were assigned
  IN-LOOP, at their use sites** — a pre-loop assignment is unreachable for that
  particular pseudo. This diagnosed SetLightningI's 65->15 draft and
  start_demo_; SetLightningI ultimately matched by replacing those manual
  pseudos with the same-TU inline projection helpers.
- **A `p = &Global.field` pointer set with a REG_EQUIV const note costs ZERO final
  instructions** — reload deletes it — **yet it shifts local-alloc quantity spans
  while it exists.** That makes it a legal, byte-free register-STEERING lever for a
  local-alloc rotation residual: introduce the `&Global.field` local to move the
  qty walk without adding an instruction. It is a useful diagnostic/steering
  device (SetLightningI's partial draft), but the exact source may instead have
  a helper parameter identity; zero-byte steering is not evidence of authorship.
- **A whole-function `+1` cascade of caller-saved temp choices is ONE defect, not
  N.** reload's spill-register pick is a FUNCTION-WIDE usage census
  (`order_regs_for_reload`), so one divergent cluster shifts every downstream temp
  by one register. Fix the single divergent site and the whole tail snaps into
  place — do not chase the downstream temps individually (SetLightningI, and the
  DecodeTMD family's param_1/$a0 cascade was the same shape).
- **The hoist gate**: `threshold * savings * lifetime >= insn_count`,
  `threshold0 = (loop_has_call ? 1 : 2) * (1 + n_non_fixed_regs)` = 29 here,
  **decaying by 3 per movable already moved** (loop.c:1640, 1728). A
  `high/lo_sum` pair gets savings 2 / lifetime 2 from `force_movables`
  (loop.c:1200) → scores 116 (SetBleedsDir vs SetBleeds: identical movable,
  opposite verdicts, discriminator = the loop's real insn count). A value live
  across a loop scores ~1102 vs insn_count ~37 — three orders clear is not a
  lever (PRICE it: `rtldump --loop-log`).
- **The move gates are TWO stages** (loop.c:691-708): gate 1 is the DISJUNCTION
  (1) used only in the set's own block (`reg_in_basic_block_p` — which also
  requires the LAST USE in that block, loop.c:1066) OR (2) `!REG_USERVAR_P &&
  !REG_LOOP_TEST_P` OR (3) guaranteed executed; gate 2 is
  `invariant_p && (n_times_set == 1 || consec_sets_invariant_p) && !trap`.
  `n_times_set == 1` is a disjunct, not a requirement (AddEnemy rounds 9/10).
- **`maybe_never` is set past ANY `CODE_LABEL` or `JUMP_INSN`**, not merely a
  backward jump.
- **`combine_movables` merges same-value set-once movables** (multi-USE members
  merge fine — `n_times_used` is a copy of `n_times_set`), summing savings and
  lifetimes into the first head; hoists emit in scan order of each group head,
  ALL after any source preheader insn (mission_score_screen's 76-byte drop).
- **BLOCK_BEG/END notes receive luids** (loop.c:404) but are not insns — a bare
  `{ s32 pad; }` scope block stretches a movable's lifetime for free.
- **`reg_single_usage`**: loop.c substitutes-and-deletes a set-once single-use
  register copy inside a call-containing loop when the folded address stays
  legitimate (`CONSTANT_ADDRESS_P` absolute addresses qualify); a `tmp = n;` copy
  cannot block invariance. Dead probes never reach loop.c
  (`delete_trivially_dead_insns` iterates first).
- **giv inits are emitted by `strength_reduce` (loop.c:6405) AFTER
  `move_movables`** — the only thing that can legally sit after a hoist in a
  preheader is a giv init; a source statement can only precede the hoists
  (AddEnemy's `names_offset` non-variable).
- **`move_movables` emits with `emit_insn_before(loop_start)`** — a hoisted set
  escapes any enclosing fence's notes, so ballast cannot weight it (AddEnemy).
- **Identical invariant else-arm constants COMBINE, so their hoist savings SUM**
  (SetBleeds' three per-arm `negu`s; solo `zN - x` spellings keep them separate).
- **A `short` loop counter suppresses strength reduction** (loop.c needs a plain
  SImode giv) — the recompute-from-base shape (SetupMotionRegist).
- **`duplicate_loop_exit_test` (jump.c)** fires only when `NOTE_INSN_LOOP_BEG` is
  immediately followed by a simplejump → the bottom-test do-while shape;
  `while(1){if(!c)break;…}` keeps the top test AND loop notes; a hand `goto` loop
  has no notes at all — no hoisting, no strength reduction, no VTOP.
- **Loop-body refs count DOUBLE in flow2** for a real loop vs a goto shape at
  identical bytes (DamageControl lesson 3).

## Register allocation

- **A pseudo's live range is NEVER split**: one C variable = one hard register
  for its whole life. Two hard regs for one logical value in the target = the
  original had two variables; the converse proves nothing — disjoint halves fall
  back onto the same free register (AttackBowControl).
- **There is no coalescing pass.** Non-conflicting allocnos land on the same
  register and copies vanish as self-moves; `global_conflicts` processes
  `REG_DEAD` before `mark_reg_store`, so a value dying AT an op never conflicts
  with its consumer — INSEPARABLE, no spelling can split them (SetupSpline;
  `regalloc` prints the verdict).
- **Priority** = `floor_log2(n_refs) * n_refs / live_length * 10000 * size`
  (global.c `allocno_compare`); **n_refs is LOOP-DEPTH-WEIGHTED** — each RTL
  mention adds its loop depth (`.lreg`'s printed count is already weighted).
  `floor_log2` steps at powers of two — one ref can swing 15000→21333. Ties break
  by pseudo number. `.greg`'s `;; N regs to allocate:` line is printed AFTER the
  qsort — it IS the allocation order (`regalloc --order` self-validates against
  it; 12 functions / 267 allocnos, zero divergence; `.lreg` live_length scored
  0 violations across 53 adjacent pairs where `.flow`'s scored 15).
- **`find_reg` walks hard registers numerically** (MIPS defines no
  `REG_ALLOC_ORDER`): first-allocated wins the LOWER register; `$s8` = `$30` is
  the LAST callee-saved scanned, so a high-ref value in `$s8` ranked BELOW its
  neighbours (mission_score_screen chased this backwards).
- **`set_preference` strips exactly ONE RTX level** to operand 0 and bails unless
  it finds a REG — `(set X (plus a b))` donates a's colour to X;
  `(set X (plus (plus a b) c))` donates NOTHING. It maps through `reg_renumber`,
  so a local_alloc'd pseudo donates as a HARD register; it fires on uses too.
- **`expand_preferences` unions preferences BOTH WAYS** between a SET_DEST global
  and any non-conflicting global carrying a `REG_DEAD` note on that insn — a
  global can inherit a preference it never touched (ControlHumanoid: the lever
  was two variables away).
- **`regs_someone_prefers[A]`** = union of full-prefs of LOWER-priority
  CONFLICTING allocnos minus A's own; an allocno's OWN preference is the only
  thing that reclaims such a register (find_reg's preference pass re-copies
  `used` from conflicts-only `used1`).
- **A single-use value assigned straight into a call argument cannot donate a
  copy preference** — combine folds the pair and flow deletes the dead def; it
  needs ≥2 uses to survive as a donor.
- **local_alloc colours quantities shortest-lived-first**; the first coloured
  takes `$v0`. `QTY_CMP_PRI` uses the same loop-depth-weighted refs, so fences
  re-rank short-lived constants too. Seeds: `copy-seed` / `binop-operand-seed`
  exploit birth order.
- **`combine_regs` refuses a tie when the source pseudo crosses a block
  boundary** (`reg_qty == -1`) — the `%hi` reload tie; duplicated per-arm calls
  make the argument pseudo block-local (FileRead, PrepareAccess, AfsOpen).
- **`update_equiv_regs`**: REG_EQUIV attaches to the SET_DEST's regno only, iff
  `REG_BASIC_BLOCK >= 0` (one block) and the MEM is unchanged for the register's
  life; it doubles REG_LIVE_LENGTH; `reg_n_sets == 2` skips the pseudo. A note on
  a pseudo that GOT a hard register is INERT (reload1.c keys on
  `reg_renumber[N] < 0`). A stack parameter used exactly once (REG_N_REFS==2) is
  load-sunk to its use (AdtSelect trap; AddMisc; `regalloc --notes` prints owner
  + liveness).
- **`REG_N_DEATHS == 1` gates local tying** (`reg_qty = -2` seed): a value that
  dies on two paths can never be locally tied — duplicate the consuming copy into
  both arms to REFUSE a tie (mission_score_screen's medal temp).
- **`optimize_reg_copy_1` lives in local-alloc.c** (no regmove.c in 2.8.1):
  propagates `dst = src` forward over a block; the scan breaks on CODE_LABEL,
  JUMP_INSN, or NOTE_INSN_LOOP_BEG/END, and the guard is
  `!find_reg_note(insn, REG_DEAD, SET_SRC)`. A fence must ENCLOSE the copy to
  block it (ProcItemSmoke, ActSTICKON).
- **A hard-conflict cannot be outranked**: if the desired hard register is in the
  allocno's `;; conflicts:` list, no priority change works — change lifetime,
  identity, or the interference graph (AddEnemy's `category = 0` fence: order
  flipped exactly as computed, zero bytes moved).

- **`regalloc.py --local` prints local-alloc's quantity walk** — the per-block
  `QTY_CMP_PRI` order, coalesced quantities, hard-reg homes and conflict sets that
  `--order` gives for GLOBAL allocnos but nobody printed for LOCAL ones (four lanes
  hand-traced it). It SIMULATES `block_alloc` and refuses the derived view if its
  assignment diverges from cc1's printed homes (self-validated across ~150
  functions, 0 divergences; fault-injected). Six `block_alloc` rules it models,
  each a correction someone would otherwise get wrong: **`qty_size` is in WORDS**
  (`PSEUDO_REGNO_SIZE`), not `GET_MODE_SIZE` bytes; the `next_qty<=3` sort is an
  INCOMPLETE compare-exchange that can hand a low-priority quantity the low
  register; tying is operand-0 only and a MEM is opaque; a hard operand commits the
  output and blocks a later tie; a re-set of an already-live pseudo cannot tie;
  call-crossers are restricted to `$s0-$s7`.
- **`.greg`'s conflict dump is the STATIC interference graph, printed BEFORE
  coloring** (`dump_conflicts`, global.c:555 — the line before the
  `for (i = 0; i < max_allocno; i++)` allocation loop). So a register absent from
  an allocno's printed conflict list can still be UNAVAILABLE at coloring time,
  via a THIRD allocno that conflicts with both and got there first. **"Not in my
  conflict list" is not "available to me"** — check transitive roommates
  (SetupTelop: `fill_white` conflicts only with `v`, yet lands on `$t0` because of
  who took the rest).
- **When K mutually-conflicting allocnos compete for K same-class registers,
  assignment is a TOURNAMENT, not K independent questions.** The Nth-ranked member
  of the clique takes the Nth register, so demoting the current #1 to free its
  register for a desired #4 does not work in isolation — the freed slot goes to
  whoever is next-highest. Simulate or `--compare` the FULL resulting order before
  spending a round on a weighting edit (SetupTelop's scaffolded draft had a
  col/font/bits/v 4-clique; dropping `col` alone handed `$a1` to `font`). This
  predicts one interference graph only: SetupTelop matched after deleting the
  fences that created that graph and restoring the original local identities.
- **`find_reg`'s preference machinery can only promote a pseudo to a register that
  is ALREADY outside its excluded set** (global.c:900-1037) — it can never override
  a real conflict. A value crossing zero calls carries no preferences and gets
  nothing from this lever.

## reload

- **WHICH reload entry point handles a spilled pseudo is decided by how the C
  USES it — dereference vs bare use — and that is the whole BARE-vs-IN-MEM
  fork.** A struct/pointer DEREFERENCE of a spilled parameter (`p->field`)
  routes through `find_reloads_address`'s `reg_equiv_address` recursion
  (reload.c:2554, 4296) and ALWAYS pushes two reloads that retype into the
  mutually-barring `RELOAD_FOR_OPERAND_ADDRESS` class (3806-3855) — **no C
  spelling of the dereference escapes it**. A BARE use of the same spilled
  pseudo (a comparison `if (title == 0)`, or a plain copy `p = menu;`) routes
  through `find_reloads_toplev` and pushes ONE `RELOAD_FOR_INPUT` reload whose
  free-check never bars its own address register. **That is the only shape that
  can self-tie a materialize+load pair onto one hard register** (AdtSelect
  0x8005FF3C; every cited line re-verified by hand against the pinned tree).
  This is the distinction `regalloc --spill-uses` was specced for — the flag
  does not exist, so this fork currently costs a reload.c read.
- **`order_regs_for_reload` is ALSO `#ifdef REG_ALLOC_ORDER`/else-ascending**
  (reload1.c:3918-3936), so "MIPS defines no REG_ALLOC_ORDER ⇒ numeric walk"
  extends to reload's own spill-register pool, not just `global.c`'s `find_reg`
  and `local-alloc.c`'s `find_free_reg`. But for reload registers it is usually
  preempted by the categorical `reload_reg_free_p` conflict check, so it is a
  much WEAKER lever there than for ordinary allocno ties — do not reason about
  reload registers as if they were allocnos (AdtSelect).
- **The self-tie retype gate** (reload.c:3806-3855) fires iff the operand is NOT
  itself reloaded (`operand_reloadnum < 0`), converting INPADDR/INPUT_ADDRESS
  reloads to `RELOAD_FOR_OPERAND_ADDRESS`, which bars sharing. `branch_zero`'s
  `'d'` constraint reloads the operand, so `if (x == 0)` self-ties; `p->field`
  through movsi's `'m'` never does (AdtSelect's header carries the full proof).
- **`allocate_reload_reg` round-robins from `last_spill_reg`**
  (reload1.c:5082-5091): one extra reload anywhere shifts every later reload
  register — a reload-register diff downstream of a reload-COUNT diff is ONE
  root cause.
- **Reload rewrites a spilled movable's insn IN PLACE into the remat and emits
  the real store at a NEW uid** — dependents keep pointing at the remat, the
  store inherits priority 1 and sinks (mission_score_screen; `.sched`-vs-`.dbr`
  UID check).
- **A spilled hoisted pseudo is rematerialised from its REG_EQUAL note after
  sched1** — `lui/addiu` glued to the `sw` that no scheduler can separate
  (SetBleedsDir).
- **`reload_cse_regs` (toplev.c:3501)** runs between `.greg` and `.sched2` with
  NO dump of its own; it UNCONDITIONALLY rewrites a constant SET_SRC into a copy
  from the first hard register (scanning regno 0 up) already holding the value;
  `$zero` can never win (only observed sets populate `reg_values`); it forgets
  everything at a CODE_LABEL. So target `move rX,zero` PROVES a label intervenes
  between the two zeroes (GetNearestHumanoid; diagnose with
  `rtldump --pass all --trace <uid>`).
- **Huge-frame TUs (offsets past ±32767)** spill params to arg-home slots once
  the callee-saved file is full; every use rematerialises li/addu/lw through
  reload's spill registers — an a3/t0 rotation there is reload structure, not
  regalloc (AdtSelect).

## Scheduling (sched.c) and reorg

- **A QImode (byte) store PINS every later fixed-address load below it.**
  `true_dependence`'s struct/scalar escape — the clause that lets a varying struct
  ref not conflict with a fixed non-struct ref — is guarded by
  `GET_MODE (mem) != QImode` (**sched.c:846-857**). So a byte store to a struct
  takes no escape and conflicts with everything after it, INCLUDING call-argument
  loads emitted by `load_register_parameters`. **Read it backwards to recover the
  source**: if retail shows such a load ABOVE a byte store, the original emitted
  the load first — spell `ot = GLOBAL;` before the store and pass `ot`. combine
  cannot re-sink it (the may-alias store blocks the merge) and the arg-reg copy
  self-deletes via local_alloc's ascending scan. Worth ~47 bytes in two windows on
  mission_score_screen.
- **cse's `insert_regs` coalesces a copy into an older equivalent register only
  when the MODES MATCH** (`GET_MODE (classp->exp) == GET_MODE (x)`). So an `s16`
  local assigned a constant that an SImode register already holds keeps its own
  `li` — HImode cannot join the SImode class. `reload_cse_regs` later rewrites
  `li`→`move` across modes for CONST_INTs. **Use a narrower local to keep a wanted
  copy/`li` alive** (mission_score_screen).
- **A `do{}while(0)` fence is TWO barriers at once, and the second one is the
  expensive surprise.** Its loop notes bound sched (below) AND bound **cse1's
  extended block** (which ends at every CODE_LABEL and at `NOTE_INSN_LOOP_END` —
  see the cse section). Both facts were recorded here separately for a long time
  and nobody joined them. So removing a fence does not just re-schedule: it lets
  cse propagate values across the span, which can reallocate the whole function.
  Measured: start_demo_'s fence removal cost **679 bytes** against a 39-byte
  residual — a 17x overshoot that reads as "the fence was load-bearing" without
  saying WHY. Price both effects before touching one (and note cse2 runs
  `after_loop=1` and crosses loop notes anyway, so the cse half is a cse1-only
  lever).
- **A `do{}while(0)` fence is a TOTAL, BIDIRECTIONAL scheduling barrier — never a
  priority hint.** An insn carrying loop notes (`sched.c:2091-2114`) gets
  `add_dependence` on **every** `reg_last_uses[i]` (anti) and **every**
  `reg_last_sets[i]` (true), then sets `reg_pending_sets_all = 1` and calls
  `flush_pending_lists`. So it depends on everything before it and everything
  after depends on it: **nothing crosses at any priority.** gcc says why in
  place — *"we must be sure that no instructions are scheduled across it,
  otherwise the reg_n_refs info (which depends on loop_depth) would become
  incorrect"*. Three consequences:
    1. **Never model a fence as a ranking/priority lever** — it does not compete,
       it forbids.
    2. **The TARGET can REFUTE a fence.** If retail shows any insn crossing a
       span, the original had NO fence in that span, full stop —
       mission_score_screen's `move a0,s2` reaches its `jal` across 20 insns,
       which `reg_pending_sets_all` makes impossible under a loop note. This is
       rare, cheap, *negative* evidence: read it off the asm before theorising.
    3. **Only LOOP/EH/SETJMP notes qualify** (`sched.c:2293`). A BLOCK note — a
       bare `{ s32 pad; }` scope — is **not** a barrier (though loop.c:404 gives
       both a LUID), so a scope pad cannot substitute for a fence.
- **Splitting a mega-pseudo changes WHICH ALLOCATOR owns each fragment.** A
  variable live in >=2 blocks is a global allocno; split per-site and each
  fragment becomes block-local, handing it to `local_alloc` — whose
  `combine_regs`/`REG_N_DEATHS` interlock may itself be load-bearing. So a split
  is not a uniform "reduce pressure" move: it re-homes the decision. Re-check
  LENGTH after every split (mission_score_screen: `brightness` split cleanly at
  the row, but not at the medal or number-init — both measured 4632).

- **sched1 runs BEFORE local-alloc** (toplev.c:579-580: *"flag_schedule_insns means
  schedule insns within basic blocks, before local_alloc"*; sched2 is after global).
  So source ORDER among independent loads in a straight-line block is fixed by sched1
  and never reaches local-alloc's priority computation — statement/declaration reorders
  there are codegen no-ops (DrawHinoko: three respellings byte-identical). Generalises
  the "sched1 runs before reload" note.
- **Both schedulers are BACKWARD list schedulers**: T-1 is the block's LAST slot,
  filled first — an insn picked earlier lands LATER. "Emit first" = "lose the
  ranking". **T is NOT an address index**: `clock += stalls` (3747) skips T
  values, so "each T decrement = +4 bytes" is false (monotonic, not evenly
  spaced). Use `sched-deps`' INDEX column.
- **`rank_for_schedule` (2415) only SORTS — it does NOT decide the pick.** Its
  ties break priority DESC → class DESC → LUID DESC, but `schedule_select`
  (**2689-2703**) then walks each EQUAL-PRIORITY group and moves the insn with
  the largest **`potential_hazard`** to the front, and that is what is scheduled.
  **LUID decides only among insns equal in priority AND equal in hazard.** cc1
  announces each override (`;; insn N has a greater potential hazard`);
  `sched-deps` marks it `<- HAZARD SWAP: beat insn M`. Any note reasoning "the
  priority tie falls through to LUID" has skipped the hazard scan — that gap held
  up AddEnemy's cluster-E "unreachability proof" for twelve rounds.
- **A ready-list line prints `, now` TWICE on a hazard cycle, and the FIRST
  list's head is the LOSER.** `schedule_select` (2713) prints before its swap and
  only when it swaps, then names the winner; `schedule_block` (3793) prints after
  the pick. **The pick is the head of the LAST `, now`.** ("The pick is the first
  insn of the `now` list" was "verified across 11 consecutive insns" — against
  FUN_80057b80 block 0's ELEVEN HAZARD SWAPS, the only cycles where it breaks.)
- **`priority()` (sched.c:1452) is NOT depth**: it accumulates
  `priority(x) + insn_cost(x, prev, insn) - 1` over LOG_LINKS (producers), and
  gcc's own comment on that `- 1` says it exists so *"when all instructions have a
  latency of 1 ... all instructions will end up with a priority of one, and hence
  no scheduling will be done."* **So priority 1 means UNIT LATENCY, not "no
  producers"** — an insn printing 1 while DEPENDING on another priority-1 insn is
  correct, not a bug. `ref_count` counts CONSUMERS and does not feed priority —
  confusing the two inverted a correct park once.
- **`adjust_priority` (sched.c:2535) does exactly ONE thing**: its `n_deaths`
  switch is DEAD (gcc's own *"??? This code has no effect, because REG_DEAD notes
  are removed before we ever get here"*), so only `case 0` runs — if
  `birthing_insn_p`, set `INSN_PRIORITY(prev) = max_priority` (2605:
  `MAX(INSN_PRIORITY(ready[0]), INSN_PRIORITY(insn))`, and `insn` holds
  LAUNCH_PRIORITY from 3923 at that moment, which is why a bumped insn surfaces as
  `(7f000001)`). **It is SCHED1-ONLY** — both it (2540) and `birthing_insn_p`
  (2504) are gated on `reload_completed == 0`. The macro `ADJUST_PRIORITY` is
  undefined on MIPS and that is IRRELEVANT: the bump is in the FUNCTION.
- **The birthing bump can only reach an insn that is somebody's PRODUCER inside
  the block.** `adjust_priority(prev)` is called from exactly one place —
  `schedule_insn`'s loop over `LOG_LINKS(insn)` (**sched.c:2631**) — so an insn
  that nothing in the block depends on is never passed to it and is **never
  bumped, whatever its REG_N_SETS**. The tell is printed: **`ref_count = 0`** in
  the `;; insn[N]` table means no `INSN_DEPEND`, i.e. live-out with no consumer
  here. Worked example: StageEndScreen's `(set (reg/v:SI 95) (const_int 82))` —
  `priority = 1, ref_count = 0`, bumped 0 times across 650 bumps elsewhere in the
  same function. **So "can this constant be lifted off the floor?" is answered by
  ref_count BEFORE REG_N_SETS**; a live-out constant with no in-block use is
  unreachable by this lever, and giving it a consumer means changing what the C
  does with it.
- **Read the birthing gate BACKWARDS to recover the original's variable
  structure.** Since only a `REG_N_SETS == 1` def can be bumped, and a bumped def
  is picked first and therefore lands LAST (adjacent to its uses), **a target that
  shows a const/copy def sitting next to its use cluster is telling you the
  original set that variable EXACTLY ONCE** — so split your shared variable into
  per-site fragments. A shared def is picked last, lands at the block top,
  stretches its live range and gets exiled from the ascending-scan register. That
  is what a 4-set `brightness` was doing to mission_score_screen (233 -> 145 once
  split, with the cse mode-guard above needed to stop cse coalescing the split
  copy back).
- **`birthing_insn_p` (2499) fires iff `(set (REG) …)`, dest live,
  `REG_N_SETS(dest) == 1`.** A `(set (SUBREG …) …)` dest is never birthing (every
  compound assignment to a short local). A re-assigned local (`REG_N_SETS != 1`)
  silently loses the bump (CreateHumanoid). **A hard-register ARGUMENT move can
  never be birthing** — its dest is `a0..a3`, and `REG_N_SETS` counts every call
  site in the function (AddEnemy). *(Distinct from the REFUTED "argmoves are a
  priority floor" rule — argmoves routinely sit above priority 1 via their
  producers; this closes only the birthing escape.)*
- **`rank_for_schedule`'s class is a CEILING, not a lever.** An insn independent of
  `last_scheduled_insn` (`link == 0`) **or** reachable at `insn_cost == 1` gets
  class 3 — the maximum — unconditionally; only a dependence with cost ≠ 1 drops
  to class 1 (data) or 2 (anti/output), and the sort is DESCENDING. So perturbing
  a dependence's cost can only sort the DEPENDENT insn **later**. "Make the dep
  cost ≠ 1 to flip the tie" was proposed once as an open lever and is exactly
  backwards. FUN_80057b80 confirmed the scheduler fact, then bypassed the tie by
  defining ordered local pointer copies instead of using formal pseudos directly.
- **All-latency-1 blocks cannot be reordered by sched** (sched.c:1521's own
  comment) — only a LOAD (cost 2) differentiates; a misplaced insn in a load-free
  block is SOURCE ORDER.
- **Equal-priority `potential_hazard` swaps happen only within an equal-priority
  group** — demoting one contender by a point dissolves the group; a store fed by
  a LOAD holds priority N+1 via load-use cost 2, a store fed by an ALU op sits
  one lower (HumanActionControl).
- **sched's equal-priority tiebreak prefers the memory-unit insn**: [alu][store]
  from a simultaneously-ready pair; the fix is a real source dependency
  (store-forwarded re-read), not permutation (FileOption).
- **Loop notes are sched barriers**: the first real insn after
  `NOTE_INSN_LOOP_END` receives a full pending-register/memory dependency
  (`reg_pending_sets_all`); mid-block loop notes are full barriers in
  `sched_analyze_insn`.
- **sched1 runs before reload; the prologue does not exist yet** — parm-copy and
  prologue ordering questions live ONLY in sched2 (`schedtrace --pass sched2`).
  sched2 runs BEFORE jump2 (the `.jump2` dump already carries sched2's lists) —
  a second `return` pins an epilogue `sra` above the restores
  (turn_towards_player_).
- **An argument-register move is LUID-last** (calls.c:1632 precomputes all
  register parameters before filling any hard reg) — but **argmoves are NOT
  pinned to priority 1** (the old "permanent scheduling floor — park on sight"
  rule is REFUTED, 2026-07-17): `move aN,rX` depends on rX's producer, and on
  the rule's own cited function 23 of 30 argmoves sit above priority 1 (one at
  53). Run `tools/sched-deps.py <Name> --pass sched2` — it prints that census
  as a falsification, never a park verdict. Priority 1 means UNIT LATENCY, not
  "no producers".
- **reorg (`.dbr`)**: `fill_simple_delay_slots` scans backward and stops at
  labels/jumps/SEQUENCEs; loads are ineligible for MIPS delay slots
  (`dslot=yes`); a 2+-use label (`own_target=0`) blocks the steal; the
  fallthrough can never fill an inline `if (c) return 0;` guard's slot (it is
  already a SEQUENCE) — what leads the SKIP LABEL decides (StickonCheck's header
  carries the four-guard control experiment).
- **`fill_simple_delay_slots`' backward scan (reorg.c:3064-3110) rejects only
  insns touching the BRANCH'S OWN OPERANDS, and takes the first survivor.** So
  if exactly ONE independent instruction is reachable in that block, it is stolen
  **wherever you put it textually** — moving it is not a lever, and a
  `do{}while(0)` fence does not help because NOTEs return 0 from `stop_search_p`
  (the scan walks straight through). **A rejected candidate is NOT a hard stop**
  (reorg.c:3095-3129): only a CODE_LABEL / JUMP_INSN / BARRIER / already-filled
  SEQUENCE / asm insn stops the scan; a candidate that merely conflicts with the
  branch's operands has its OWN read/write set folded into the accumulator and the
  scan CONTINUES backward — so an independent insn two or more steps back is still
  reachable and stolen. That is why "reorder the two entry insns" is never a lever
  when exactly one truly-independent candidate exists anywhere in the
  backward-reachable prefix (ChasetoTarget, traced to reorg.c by UID). ActITEM proved this across 9 source-shape
  variants (4 statement positions, a fence, an if/else duplication, a De Morgan
  merge): every alternative regressed or mismatched length. **If your draft fills
  a slot the target leaves as a bare `nop`, count the independent candidates in
  the block before trying to reposition anything** — `reghist` shows this as a
  `+1 move / −1 nop` structural signature, not a register tie.
- **A value-redundant reassignment hoisted out of a conditional forces two
  coalescing pseudos apart — and then becomes reorg's favourite delay-slot bait.**
  gcc 2.8.1 has NO coalescing pass; `global_conflicts` computes conflicts from
  real simultaneous liveness, not source spelling. So if the target keeps in two
  registers what your draft coalesces onto one, and the variables don't naturally
  overlap, writing an otherwise-redundant `x = 0;` *outside* the conditional at a
  point where the other pseudo is still live creates a genuine conflict and
  separates them (ActITEM: 26 → 10 bytes). The caveat is the rule above — that
  newly freestanding insn may now be the only independent candidate near a branch,
  and will be stolen into its slot.
- **`mostly_true_jump` scans BACKWARD from the branch target over NOTES ONLY**;
  `NOTE_INSN_LOOP_BEG` → returns 2 (mostly-taken) → reorg raids the TARGET
  thread first (+1 COPY of the merge block's leader, label redirected); predicted
  not-taken raids the fallthrough it owns (−1 MOVE). The flip has a DIRECTION
  (LoadTIMpack wanted the copy; StickonCheck's EQ guard already predicted 0).
- **reorg deletes a same-address reload only if the register is untouched
  between** store and reload (redundant-insn elimination; absent from
  cse/combine dumps) — to keep a "redundant" reload, redefine the register in
  the interval (ReqItemUse).
- **reorg steals a shared block's first insn and retargets THROUGH it** — N
  rejects with sentinels in delay slots are the post-reorg view of N
  `goto reject;` sites; the site still pointing at the real block had a full
  slot (DrawModelArchive, DrawClip).
- **An `abssi2` is ONE `multi`-type insn through reorg** (template carries its
  own branch) — invisible to delay-slot filling; the open-coded form exposes a
  real `bgez` that steals (DamageControl lesson 1, FUN_80056910).
- **cc1 emits `.set reorder`** — a `bgez` delay slot can be filled by the
  ASSEMBLER; cc1's own dbr only considers the single preceding insn.
- **`x = 0` compiles to `move r,$0`, never `addiu r,$zero,0`** — the target byte
  form `addiu r,$zero,0` is unreachable from plain C (the draw*/DrawTMD
  handwritten-family tell; docs/gte-policy.md).

## jump / cross-jump

- **`find_cross_jump` scans backward and gives up when stream 1 reaches a
  CODE_LABEL** (it skips labels only in stream 2) — a referenced `case` label is
  a hard cross-jump fence (StateTransition).
- **Cross-jump compares whole insns**: a `CALL_INSN`'s result mode and
  `CALL_INSN_FUNCTION_USAGE` distinguish machine-identical `jal`s (ActATTACK);
  algebraically equal `x*3+480` vs `(x+160)*3` do not present the same suffix
  (FUN_800519bc).
- **`jump_optimize(insns,1,1,0)` — the only cross_jump=1 call (toplev.c:3548) —
  runs AFTER combine and AFTER allocation**: never infer pre-jump2 block
  structure or allocation constraints from final asm (FUN_80057b80 494→8→0;
  ActSTICKON's "register cycle" was a jump2 artifact). jump1 hoists a common
  leading insn out of identical arms — a dead store in one arm keeps the heads
  different.
- **Guard `return 0;` islands all cross-jump into the LAST plain island in
  emission order** (HangCheck); `make_return_insns` converts a jump-to-next
  return into its own `jr` when a bare CODE_LABEL survives (GetConflictResult).
- **A lone-jump body is foldable**: `if (cond) goto retK;` lets jump.c delete the
  join label that an inline `if (cond) return K;` would leave — cse's block then
  spans the call (the redundant `move aN,sM` fix).

## Assembler / maspsx layer

- **ASPSX gp-addresses only symbols defined in the TU being assembled**; externs
  are absolute. Per-file `maspsxGpExterns` in Build.hs encodes the original TU's
  smalls (toolchain.md). A missing entry silently relocates whole data regions —
  `symcheck.py` catches it (`D_<HEX>` must equal `0x<HEX>`).
- **-msplit-addresses is effectively ON**: non-small externs compile to
  HIGH/LO_SUM through allocated registers; ≤ -G8 smalls keep the one-insn macro
  expanded via `$at` (stores) or the dest reg (loads). `ENCODE_SECTION_INFO`
  (mips.h) sets `SYMBOL_REF_FLAG` iff the decl's type is COMPLETE and
  `0 < sizeof <= 8` — `extern T x[];` is incomplete and always splits; a complete
  type of size ≤ 8 gives the same-register unsplit `la` (RestoreItemLayout's
  one-character fix). Offset-0 accesses fold `%lo` into the displacement; a
  nonzero constant offset forces full materialisation of the DECLARED symbol's
  own address (never partially absorbing the offset).
- **Variable division needs maspsx `--expand-div`** to reproduce ASPSX's
  `break 7` (÷0) and, for signed div, `break 6` guards; `maspsxflags.py --write`
  syncs it (with the gp lists) from the target asm. **Those `break` guards are a
  post-cc1 TEXT expansion of a single `div` RTL insn — cc1's allocator and
  scheduler see ONE instruction, not basic-block boundaries.** So when reading
  `.greg`/`.sched` liveness around a variable division, do not treat the guard
  branches as CFG edges; the `mflo`/operands live-range is computed as if the div
  were one insn (PadProc).
- **maspsx `nop # DEBUG:` comment lines QUOTE instructions** — strip `#` to EOL
  before counting anything; in a gte.h function `swc2 $17,…` is COP2 reg 17, not
  `$s1`.
- **maspsx `break` takes the single-value form** (`break 0x107`), not objdump's
  two-operand rendering.
- The canonical cc1 reads the **low** halfword for `(short)(int)` truncation; a
  high-half `lhu` (offset+2) means the old non-canonical binary crept back in.
