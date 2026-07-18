# Matching cookbook — cc1 2.8.1 source idioms that byte-match

How the original source must be *shaped* for our exact compiler (the nix-pinned
canonical decompals `gcc-2.8.1-psx` cc1, verified equivalent to Sony's
`CC1PSX.EXE` — [toolchain.md](toolchain.md)) to reproduce the original bytes.

**How to read this file.** Do not read it linearly. Read §0–§2 (~300 lines);
then read ONLY the §3 families that §2's router names for your current
residual. The companion files:

- [compiler-facts.md](compiler-facts.md) — verified cc1 mechanics with gcc
  file:line cites; the substrate for auditing a tool's verdict. Grep it.
- [shape-zoo.md](shape-zoo.md) — true-but-narrow shapes, one entry each. Grep it.
- [autorules-index.md](autorules-index.md) — GENERATED index of every mechanised
  rule and every rtlguide residual signature. The cookbook never re-explains a
  mechanised rule; if a rule key is named here, its procedure lives in the tool.
- Worked examples in increasing difficulty: `Think1sleep.c`, `GetRealPad.c`,
  `ProcItemManebue.c`, `ProcItemKusuri.c` (read its header). Function-specific
  history — parks, refuted experiments, round-by-round sagas — lives in that
  function's `src/main.exe/<Name>.c` header, and nowhere else.

**Every rule here is a hypothesis that has been paid for at least once.** Rules
carry their evidence function in parentheses. When a rule and a measurement
disagree, the measurement wins — and report it so the rule gets fixed.

## §0 Contract, workflow, iteration ladder

The lane contract (worktree discipline, tool budgets, permuter operating rules,
polling, commit gates) is `.claude/agents/matcher.md` — it is injected into
every lane and is not duplicated here. Orchestration (targeting, bundling,
harvest, model routing) is [orchestration.md](orchestration.md).

### Workflow

```console
$ tools/reverse.py <Name> --ghidra-export .shake/ghidra-export
      # splits the function, seeds src/main.exe/<Name>.c with Ghidra's C +
      # (appended post-build) m2c's output; verifies ./Build check stays green.
$ tools/maspsxflags.py <Name> --write     # gp externs + --expand-div, synced
$ tools/xref.py <Name>                    # callers pin the prototype; callees
$ # draft from BOTH references + §1 evidence, then loop:
$ tools/matchdiff.py <Name>               # build + byte-compare; iterate to MATCH
$ ./Build check                           # whole-image gate
```

`tools/gpsyms.py` is the gp-only primitive. For big or length-shifted functions
iterate with `tools/asmdiff.py <Name>` (true size from the Ghidra export,
difflib-aligned, `--structural` shows length-changing blocks first); it is a
*view* — only `matchdiff` is the byte gate, and an empty `--structural` view is
not a match claim.

### The NON_MATCHING convention

A close draft that resists source-shaping is preserved as a compile-able XOR,
never `#if 0`:

```c
#ifndef NON_MATCHING
INCLUDE_ASM(...);              /* + any jtbl array for split functions */
#else
... best draft ...
#endif
```

Default `./Build` keeps the stub (image stays byte-identical; the file still
counts unmatched). `matchdiff`/`asmdiff`/`permute` set `NON_MATCHING=<Name>`
automatically. Head the file `STATUS: NON_MATCHING — N of M bytes differ` with
the root cause; on a later match delete the guards. matchdiff refuses to bless a
byte-exact result whose source still contains a guard, `INCLUDE_ASM`, or inline
`__asm__`, and both matchdiff and asmdiff reject a stale `<Name>.NON_MATCHING`
link-map artifact.

**Inline extended asm is an anti-rule**: it encodes no recovered logic and is an
opaque escape hatch — matchdiff blocks it (the sanctioned exception is the
`gte.h` layer, §3.18). If only an asm-bearing experiment matches, the function
stays NON_MATCHING (StateTransition is the worked correction).

### The iteration ladder — fix categories in THIS order

1. **Length first.** A different instruction count means the STRUCTURE is wrong
   (dispatch shape, loop shape, temps, tail merging). Operand diffs are
   meaningless until length is right. Exact length is necessary, not
   sufficient — compare `rtlguide`'s physical control-flow inventory too
   (AddEnemy's exact-length fence candidate hid the real problem behind an
   invented branch). Two standing length causes to rule out first:
   `maspsxflags` (variable division / gp lists) and a bad carve (§3.17).
2. **Wrong/missing/reordered instructions next.** Map each to a family below;
   `tools/findsimilar.py <Name>` names the nearest matched functions — read
   their source and headers first, and act on a printed `TRANSCRIBE` verdict.
3. **Register-only differences last** — they usually fall out of 1–2. Levers in
   order: statement/assignment position, temps and their declarations, then the
   permuter for pure allocation ties.
4. **Machine-apply the mechanical rules first.** The moment the draft compiles:
   `tools/autorules.py <Name>`; for a surviving correct-length residual,
   `tools/rtlguide.py <Name>` then `tools/autorules.py <Name> --guided`. If the
   sweep reports no win, the residual is NOT one of the swept rules — move on
   rather than hand-applying variants. Semantic vetoes stand above any score:
   reject a "win" that changes a PROVEN struct field's access width
   (Think3chase), contradicts a PSX.SYM-recorded type (SetupBG's `short sy`), or
   changes/deletes a target-present sign-extension. Heed `LOCAL-SHAPE
   REGRESSION` and `LENGTH-WIN SHAPE REGRESSION` banners; on a length-mismatched
   draft the score is the length penalty alone — review every adoption
   (ChasetoTarget's `deg` retype). autorules stages candidates under `.shake`
   and publishes atomically; poll a yielded command to completion.
5. **Permuter discipline** (operating detail in matcher.md): never as an early
   decompiler — recover CFG/loops/stack/length deterministically first
   (permute.py preflights and refuses broad residuals). One bounded run for a
   sub-C-looking residual; linked rescore is authoritative (`--rescore-only`
   after interrupts; ControlHumanoid's real winner was proxy output-525).
   Bisect winners — they carry dead statements (bow_shoot_logic). Read a
   plateaued run's best `output-*` dirs for the single load-bearing edit
   (PlayerOption, SetupAppearance). It throws `KeyError: 'rand'` on
   rand()-calling functions (typemap gap) — recognise and move on.
   **TOOL TICKET (permute-rand-message)**: permute.py should print this reason
   itself, as it already does for gte.h functions.
6. **Sub-C early-stop.** A ≤ ~10-byte same-length register swap / adjacent
   reorder that survives one bounded permuter run is below the obvious C level:
   route it via §2's residual table, escalate to the RTL method (§2), and park
   evidence-complete if the named levers are exhausted — do NOT open more
   surgical sessions (~1.4M tokens once produced zero matches this way). But do
   not park same-length residuals unpermuted: the permuter has cracked 5-byte
   register ties (FUN_800568b8, ~400 iters), a 61-byte same-length miss
   (AfsGetHeader), and pure statement-order fixes (FUN_80038c0c) — "autorules
   reports no win" plus right length is the signal to permute, not to park.
   decomp.me (psyq4.3 preset — the real Sony compiler) arbitrates "is this
   expressible at all": if it can't, restructure; if it can, keep hunting.
7. **SCORE INTERACTING FACTORS JOINTLY.** autorules/greedy search can never find
   an n-factor interaction whose every 1-subset scores zero (CreateHumanoid
   matched on three edits, each inert alone — and each separately recorded as a
   measured dead end). A residual that survives a clean sweep plus several
   independent "dead end" notes is itself evidence for a joint lever: retry the
   dead ends in combination (`--beam`/`--depth`) before parking.
8. **Mid-batch sanity**: a whole-image `./Build check` failure while a function
   is mid-match is EXPECTED (wrong length shifts every later object). In a
   multi-function session an EARLIER wrong-length function makes a LATER one's
   window compare the WRONG bytes (spurious diffs; gp immediates off by one
   constant) — sort by address, fix upstream first. A byte count from a
   wrong-length draft is not comparable to an exact-length checkpoint
   (matchdiff rejects it as `LENGTH MISMATCH`; PutItemList).
9. **On MATCH**: challenge every fence (§3.10 debt rule), `./Build check`, write
   the header notes, promote any NEW reusable rule into this file's families —
   or, if it is a decision procedure over a dump, file it as a tool ticket
   instead — commit function + splat.yaml together.

## §1 Evidence first — the original source's own facts

Look up what the authors wrote before drafting anything.

- **A `%hi(SYMBOL)` / `%lo(SYMBOL)` label in the TARGET's disassembly is NOT evidence
  the original named that symbol.** Those labels are splat matching the final computed
  ADDRESS against its own symbol table — they are our annotation, not the compiler's
  instruction selection. `lui s6,%hi(CHOSEN_STAGE)` does not mean the source said
  `CHOSEN_STAGE`; a `PSTATE->stage` field access through a base pointer lands on the
  same address and assembles to the same bytes. **Check whether your current spelling
  ALREADY produces those exact bytes before rewriting toward the label.** FUN_800519bc
  spent a full build-and-revert cycle rewriting proven-identical code into named
  externs because the label read like a demand (and the rewrite regressed 1448 → 1488).
  The same caution applies to any name in a disassembly comment: `tools/tdis.py` and
  objdump print OUR symbol table, not the original's.

- **PSX.SYM** (docs/psx-sym.md): for 442 functions, the prototype with parameter
  names, source file:line, and every local (name, type, register/stack home).
  `tools/matcher-prompt.py <Name>` prints it; each `.c` carries it as a
  `BEGIN PSX.SYM` block (`tools/symnote.py --write --all`). The local count and
  types are often the single biggest codegen lever — use the demo list, in
  order, as the first draft, not as a retail spec (retail sometimes changed the
  API and the aggregate type: AntiWall). Retail access widths and callee ABI
  override. Caveats: layouts are an earlier build (verify offsets with
  `tools/access.py`); a repeated local name is a nested-block scope — and
  therefore a lifetime-boundary hint (PlayVoice's two `min`/`sec` pairs;
  AttackControl's two `enemy`s); `.NNNfake` struct tags were per-TU.
  **PSX.SYM lists a block's locals in REVERSE declaration order** — a listed
  `dz, dy, dx` means the source declared `long dx, dy, dz;`. Block scalars get
  pseudos AT DECLARATION, so transcribing the listed order verbatim swaps register
  roles (measured s0/s1 flip on SetWire). Reverse each block's list before writing
  it.
- **The human structure can be the RIGHT base even when it scores WORSE**, because
  a byte-chased draft is a local optimum away from the source and can be a dead
  end. SetWire from EFFECT.C:1428's own 23 declarations measured 85 vs a banked
  70 — but the 70's fake-fence 10 bytes were unreachable-from-C (the target
  refutes the fence), while the human 80 reproduced the entire t/Q/R interpolation
  island byte-for-byte from plain `t = one - i*0x1000/lcount; Q = t*2; R =
  t*t/0x1000; ...` where the byte-chased draft needed 8 temps and dead-carrier
  reuse. When PSX.SYM gives you the declarations, WRITE THEM, measure, and prefer
  the human base for the next round even at a small byte cost. `siblingdiff --demo`
  can even show the author editing the source between builds (SetWire's `abs`
  changed from builtin to a called flag-setting form demo→retail).
- **NOT every fence is scaffolding — there are THREE cases and conflating them
  damages both directions.** Before deleting a `do{}while(0)`, classify it:
  (a) **a MATCHED SIBLING'S MACRO** — grep the TU's `.h` and `.c` `#define`s for
  the fence body; if it is `DRAW_SCORE_NUMBER` or similar, it is the author's own
  code and STAYS (mission_score_screen: ~20 of 24 fences). (b) **A REPLACEABLE
  byte-chase** — the fence props up bytes AND a genuinely better, more complete
  human structure exists to replace it (SetWire's identical-arm fence: the target
  refutes it, and the human spelling reproduces the island). Adopt the human base
  even at a worse byte count. (c) **A BARE load-bearing fence** — no macro backs
  it, no better structure is on offer, and unwrapping it (test with `autorules`'
  `fence-unwrap`, which sweeps each singly) measurably regresses. **KEEP it**:
  measured necessity, not macro provenance, decides (StageEndScreen's 4-nest at
  L423 scores 202→227 unwrapped; FUN_800519bc's 5 fences, no macro, cost 87→91..1012).
  **The "adopt the worse byte count" move is ONLY for case (b)** — a bare fence
  with nothing better behind it is not a reason to regress. And test fence
  clusters as SUBSETS, not singletons: FUN_800519bc's three position-carriers act
  as one dial (removing any one flips the same s1/s2 tie), which a one-at-a-time
  sweep misreads. And a fence you KEEP can still be tuned via its CONDITION:
  reading a different pseudo in an identical-arm fence's discriminator reorders
  `find_reg` hundreds of insns away (SetLightningI moved an `end`-vs-`scrp` race
  for `s7` that way).
- **Prototypes/globals**: `reference/psxsym-protos.h` beats Ghidra/m2c
  inference; `reference/psxsym-globals.h` beats inventing layouts.
  **TU-mates are contiguous** (`reference/psxsym-tu-map.tsv`) — pins rodata
  pooling and the gp-vs-absolute choice (§3.15).
- **The demo binary**: `tools/siblingdiff.py <Name> --demo` before drafting when
  the name exists there — a structural oracle (SetFlyWire), a schedule-shape
  oracle (CreateHumanoid: two builds with different strides agreeing on chain
  ORDER proves the order is source structure), and the first move on a
  same-length copy residual (SetupSpline). **Check the size ratio first**: a
  ~1:1 demo twin is an oracle; DrawImpact's 0.05-ratio demo is a different
  function wearing the same name — and its PSX.SYM locals describe the DEMO
  body. A demo call that disappears in retail may be an INLINE EXPANSION —
  reconstruct a local `static __inline__` helper and keep the call (AfsFindFile;
  siblingdiff reports the bigram tell). `--compose` maps target ranges across
  demo + matched siblings at once (ActMOVE).
- **Names**: a `FUN_…` block carries the candidate name from
  `reference/psxsym-candidates.tsv` — adopt only via `tools/callmatch.py
  --verify` (it blocks a conservative subset as AMBIGUOUS; don't override with
  positional evidence). Function names are call-site *intent*, not control-flow
  descriptions (`dispose_weapon_data_of_char_` has zero branches).
- **Ghidra vs m2c vs the raw `.s`**: trust the asm over Ghidra's statement order
  (its early temp assignments are SSA artifacts — PlayerOption); its store order
  and branch polarity are normalized, the asm's are the source's. Ghidra's
  SCALAR types are gold; its struct NAMES can be invented (ReqItemJirai's
  shifted `PARAM_ITEM_DROP`) — trust m2c's raw offsets + proven shared headers
  (item.h) + `reference/ghidra_types.h` where they agree. A struct-field pointer
  type nobody dereferences is a guess — grep for other users before trusting.
  Retail load/store width and raw offset outrank a demo symbol's reused union
  member (ProcItemDokudango's `param_dokudango`). m2c and Ghidra both mis-count
  call args in both directions — count `a0–a3` + pre-call `sw N(sp)` in the raw
  `.s` (Makibishi, DrawBG's leading-arg drop); a pre-call `sll/sra` on a live
  arg register proves a signed-short formal (SetupTelop). Two tells that a
  Ghidra-narrow parameter is really WIDE: a full `lw` + one-time `sll/sra`
  re-widen (a genuinely-int param narrowed once folds to a single `lh` —
  SetLightning's 5th), and Ghidra's own `CONCAT22(...)` confession
  (PutItemCursor) — m2c's per-register width is the tiebreaker. `tools/access.py
  <Name>` prints every pointer offset's access width/direction — only the `.s`
  mnemonic knows a union store's width. Per-address PSX.SYM LINE events can
  distinguish a compound expression from adjacent statements when the bytes
  cannot (PadProc's `ct = -PadArrange.time++;`) — demo line tables, so require
  cross-build agreement on the boundary.
- **The matched corpus is an oracle**: grep the whole game for the target's
  SHAPE and check whether a function with that shape is already matched — its
  source IS the answer to "what C produces this?" (FUN_80057b80's prologue
  question: one grep). **Match on the CAUSE, not the shape** — the same oracle
  false-positived when the mechanism (parameter USE KIND) did not transfer.

## §2 THE ROUTER — residual signature → tool → family

First: `tools/matchdiff.py <Name>` (add `--clusters` for insn+byte accounting
per cluster — units in the header; clusters are hypotheses about causes, not a
partition). Then route on what you see. Never hand-derive a number a dump
prints — that habit has produced confidently wrong rounds (see cc1says below).

| Residual signature | Run | Then |
|---|---|---|
| Wrong LENGTH | `maspsxflags --check`, `coverage.py` (carve!), `rtlguide` | §3.1–§3.3, §3.7 structure families; §3.17 carve integrity |
| High-similarity sibling exists | `findsimilar <Name>` | On `TRANSCRIBE` verdict (score ≥0.35 AND same original TU): copy the sibling's C; change only unaligned spans |
| Big function, first pass | `reghist <Name>` | Mega-pseudo split (§3.8); heed the OPCODES-DIFFER banner; zero-sum delta = decomposition already matches — stop splitting |
| Same length, registers only, ONE basic block | `regalloc <Name> --local` (the quantity walk, self-validated) then `--order` for globals | LOCAL tie → `copy-seed`/`binop-operand-seed` (index); local-alloc facts §3.9 |
| Same length, registers, across blocks | `regalloc --order`, `rtlguide` | Identity canon §3.8 first (two-regs-for-one-value?), then allocation steering §3.9 |
| Target holds one value in TWO regs | (read the target) | Two source variables — §3.8. Cheapest question on the board |
| Rotation of many registers | `regalloc --order` | Diff cc1's printed order vs the target-implied order (§3.9). If orders MATCH, the lever is find_reg exclusions / pre-RA insn order, not weighting (FUN_80036284) |
| INSEPARABLE pair (regalloc prints it) | — | No spelling can split them — restructure or park (§3.8). Passing this gate proves nothing (SetupTelop d487683) |
| Delay slot / `nop` diffs | `cc1says`, `rtldump --pass dbr,sched2` | §3.13 delay-slot family: read the block LEADER and the SKIP LABEL, not the branch. **TOOL TICKET (dslot)** |
| Load position / batching | `.sched` LOG_LINKS: classify the ADDRESS | FIXED symbol_ref floats; CHAINED lo_sum is scheduler-chained (extern-array decl!); VARYING reg+K (incl. sp!) pins below stores → §3.13. **TOOL TICKET (loadclass)** |
| Store displaced below loads | LOG_LINKS `REG_DEP_ANTI` lines | A width-cast load cleared `/s` (§3.13); TU-local view struct fixes it. **TOOL TICKET (struct-view autorule)** |
| Preheader contents/order | `rtldump --loop-log` | loop.c economy §3.14 — price the gate, never argue with it |
| Equal-priority sched tie | `sched-deps <Name>`, `schedtrace` | §3.13: what made the priorities EQUAL? sched-deps prints the group + demotion candidates; LAUNCH_PRIORITY bumps; ready-list lines are the truth |
| Empty/odd delay slot, block-leader question | `sched-deps <Name>` (flow-vs-sched leader report), `cc1says` | §3.13 delay-slot family |
| Prologue / parm-copy order | `schedtrace --pass sched2`, `sched-deps --pass sched2` | sched1 never sees the prologue (compiler-facts) |
| Target repeats a block; one of your sites matches | `asmdiff`, then diff the TARGET's sites against each other | Spelling asymmetry — stop looking at the compiler (LoadTIMpack). **TOOL TICKET (selfsim)** |
| Frame size / slot layout | `stackplan [--emit-overlay]` | §3.7: unions, three-phase rule, slot arithmetic, spill-count residue. **TOOL TICKET (slotcalc)** |
| Guard polarity / return island | `rtlguide` (`guard-return-island-layout`) | `terminal-guard-flip` / `if-else-invert` (index), §3.1–§3.2 |
| A store both sides have is absent from the diff | count STORES per destination | Ternary-vs-if/else join (§3.4): two stores vs one is read off the target |
| `move rX,zero` where you emit `move rX,rY` | `rtldump --pass all --trace <uid>` | reload_cse_regs — a CODE_LABEL intervenes; guard-clause inversion (§3.11). **TOOL TICKET (guard-clause-invert autorule)** |
| Fence edit under test | `matchdiff --clusters` before/after | Judge per-cluster, never the total (+7 once hid −27/+34). **TOOL TICKET (matchdiff --baseline)** |
| Sub-C ≤10B named classes | see table below | — |

**The named sub-C classes** (recognise, apply the one lever, else park):
`la`/%hi reload tie → duplicated per-arm call, §3.9 (FileRead; NOT
permuter-immune — cd_open was parked under this diagnosis for 353k tokens and
later matched by moving its narrow counter increments after the safe
early-exit tests); guard delay-slot fill tie → §3.13 skip-label rule
(StickonCheck); adjacent loads in opposite order → prove independence through
`.combine→.sched→.sched2` + LOOP_END fence check first (DefaultActionHumanoid);
`commutative-plus-destination` → `initialized-global-compound` (AttackLong);
narrow round-trip zero fence → the four-part AttackShort shape;
dead constant scratch feeding one narrow store → park after literal/direct
spellings (ProcItemDokudango); `commutative-equality-register-order` →
`eq-literal-swap` once, flat = stop (ActJUMP); pool-scan `addu` operand order
inside an outer loop → while/break form, then accept the 2-byte tie (SetBlood).

**The tools** (details in each `--help`; orchestration.md has the full map):
`rtldump` (any pass dump, `--draft`, `--trace UID` across passes, `--loop-log`,
`--src` for CONTROL variants), `rtlguide` (aligns residual hunks to owning pass
+ C lines, names known signatures, plans the guided sweep), `cc1says` (cc1's
own `;;` narration across all 15 dumps — START HERE on any sub-C residual; it
was built because every hand-derivation of something cc1 prints has cost a
round, twice producing confident wrong conclusions), `schedtrace` (priority +
ready-list tables, LAUNCH_PRIORITY flags), `sched-deps` (the scheduler's
decisions decoded: equal-priority groups, flow-vs-sched block-leader report,
argmove census), `ccsrc` (surfaces the pinned gcc tree — cite file:line, never
recall), `regalloc` (dispositions, priorities,
`--order` self-validated allocation order, `--compare/--between/--enclosed-refs`
exact reweighting arithmetic, `--prefer`, `--notes` REG_EQUIV owner+liveness),
`reghist` (register-mention histogram + opcode banner), `stackplan` (frame
window, union scaffold), `siblingdiff`/`findsimilar` (§1), `nullcheck` (is this
edit even reaching the bytes?), `cc1` standalone testbeds (25-line `.c`, ~1s —
screens register questions; scheduling/copy-survival answers do NOT transfer).

### What each pass tells you

| dump | pass | read it for |
|---|---|---|
| `.greg` | global-alloc | `N in H` dispositions; `;; N regs to allocate:` = the real order; conflicts; preferences; `Need N regs of class …` spill demand |
| `.lreg` | local-alloc | per-block allocation; weighted refs/live length (what global_alloc consumes); copy chains |
| `.loop` | loop opt | the movable decision log — `--loop-log` |
| `.combine` | combine | operand order / reassociation fixes; `N attempts, N successes` |
| `.cse`/`.cse2` | CSE | `Processing block from A to B` = the real block extents; fold/forwarding decisions |
| `.jump`/`.jump2` | jump opt | label survival; cross-jump merges; `make_return_insns` |
| `.flow` | dataflow | pre-sched RTL order; `Register N used M times` ref shifts |
| `.sched`/`.sched2` | schedulers | LOG_LINKS deps; priorities + ready lists; hazard swaps; `register N no longer crosses calls` |
| `.dbr` | reorg | delay-slot fills + `N insns needing delay slots / M got …` counts |

Hard regs: 0 zero, 1 at, 2 v0, 3 v1, 4–7 a0–a3, 8–15 t0–t7, 16–23 s0–s7,
24–25 t8/t9, 28 gp, 29 sp, 30 fp/s8, 31 ra.

### The escalation procedure (correct length, small residual, respelling failed)

1. Locate the diverging instruction and what differs (register, offset,
   missing/extra insn).
2. Run `cc1says`, then dump the owning pass (table above).
3. Read what the pass DECIDED and work back to the source construct, using
   compiler-facts.md as the substrate. Form ONE hypothesis.
4. Test it with the fast loop (`NON_MATCHING=<Name> ./Build` + `asmdiff -n`,
   ~5 s). Dump ONCE to hypothesise; iterate with the loop, not by re-dumping.
5. **Dump a CONTROL variant** (`rtldump --src <variant>`) to falsify a suspected
   cause by removing it — identical deps with and without kill a theory in one
   shot (FUN_8003944c).
6. Testbed: reproduce the shape in a standalone 25-line `.c` (~1 s/variant) for
   register/forwarding questions; do not trust it for scheduling or copy
   survival (§ zoo).
7. The nine escalations before this method existed all turned out to be
   reachable source structure the dumps name outright. There is no target RTL —
   target ASM is the spec; our RTL explains our wrong bytes; we mutate C only.

Worked examples: vrealloc, vfree, valloc, Sound, turn_towards_player_,
UpdateMotion, HangCheck, GetConflictResult, SetSmoke — headers carry the
dump-to-source reasoning.

## §3 The families

### 3.1 Dispatch & body layout

**The layout canon: cc1 compiles `if (cond) A; else B;` as `if (!cond) goto B;`
with A falling through** — the THEN body is always physically first, `cond` is
negated. Everything else here is corollaries:

- **De Morgan is the body-placement lever.** `||` vs `&&` place the bodies
  differently: `bnez a,BIG; beqz b,SMALL` = the `||` form (BIG first);
  `bnez a,BIG; bnez b,BIG` = the `&&` form (SMALL first) (MoveHumanoid). An
  `&&`-chain's body is always the fallthrough after the last test — the single
  lever is inverting into an early-return `||` guard (CGetLevel).
- **When the target branches to the TRIVIAL body and the complex body falls
  through**: `if (cond) goto trivial; <complex>; goto join; trivial: …; join:`
  (FUN_8003a2a8's clamp — also buys the eager-assign-in-delay-slot + override
  shape).
- **Two independent `if (c) goto L;` tests with the short do-nothing case
  falling through** — both real bodies forward-jump targets — is its own shape,
  distinct from if/else-if (ProcMiscSprite/ProcMiscFire; FUN_8004c59c/
  FUN_8004d6d4; also the dual-role variant where both bodies are substantial).
  An explicit goto ladder **decouples test ORDER from body LAYOUT** (cd_seek
  tests 1,0,2, bodies 0,2,1; ProcItemArrow/ProcSightShot forward skips).
- **A guard clause with no second `return` is a plain `if`** — Ghidra renders it
  backwards when there is no return to anchor it (CreateHumanoid's
  `if (mad == 0 || Humans >= 0x28) { SystemOut(…); }`).
- **Ghidra's branch polarity is usually inverted** for a single if/else over a
  comparison — EXCEPT the null-guard-with-two-returns shape, which keeps literal
  polarity (cd_close/Afs*), which itself flips back when: the rest-arm carries
  the loop's back-edge (GetItemType); the success arm ends in a tail-call return
  (leRemoveEnemy); or the success arm is the LONG one (LoadFromCDROM). One-line
  test: **the arm you'd rather have fall into the epilogue is the short one.**
- **cc1 collapses logically-redundant `&&` terms** (`x != 0 && x == 1` → one
  test) — NEST to keep both branches (ProcItemLightningBolt).
- **A search loop's entry-duplicated test is the only guard needed** — cc1 does
  not dedupe an outer `if (0 < N)` against the for-loop's own hoisted entry
  test: drop the outer if (KillHumanoid/GetHumanoid).
  **TOOL TICKET (redundant-entry-guard autorule)**: drop/add the outer if, both
  directions, byte-scored.
- **A sentinel-terminated search that ALSO seeks a value: the SENTINEL test is
  the `while`, the VALUE test is the inner `if(...)break;` — never the reverse.**
  `while (T[i].sentinel != -1) { if (T[i].key == wid) break; i++; }`. Swapping the
  two roles keeps the SAME instruction count but reorders which value is
  materialized before the loop's first test — a same-length residual confined to
  the loop's opening insns (GetWeaponData 32→36 when reversed). This is a
  read-the-target-as-source fix: the entry-duplicated test names which comparison
  the human put in the `while`.
- **Cross-jump threading of a loop's entry test past a textually-identical
  POST-loop check needs both tests in the SAME function's RTL** (`thread_jumps`,
  jump.c:4360). When a search's entry-duplicated `T[0].sentinel != -1` is
  RTL-identical to a post-loop `if (T[i].sentinel != -1)` found-check on the same
  counter, cc1 threads the entry branch straight past both to their shared
  failure label. Factoring the loop into a `static inline` helper that RETURNS the
  index — caller does the found-check — BLOCKS this even under full inlining: the
  call/return-value boundary keeps the two tests out of one RTL stream, so the
  entry branch lands in the loop-exit merge and recomputes the indexed address
  (extra wrong insns). Fix: write the loop AND its post-loop check in one flat
  function scope. GetWeaponData matched at 0 this way, and PSX.SYM confirmed it —
  ONE `reg $a0 short i`, not one per search. (A search with NO post-loop check
  matches fine as a helper; it is specifically the redundant-test threading that
  needs flat visibility.)
- **switch vs ladder**: a dispatcher that RELOADS its variable (two `lbu`) and
  compares signed (`slti`) is a real `switch` (fresh index load, balanced tree);
  a ladder CSEs one load and compares `sltiu` (ProcItemDokudango). Case bodies
  lay out in SOURCE order while tests sort by value — read body ADDRESSES
  (LayoutEnemyOption wrote 1,2,0; Makibishi needed `case 4:` before `case 1:`;
  LoadConstruction's island order). Put the case whose body precedes the
  epilogue LAST (PlayerOption). An explicit `default:` that only returns is CFG
  data — it removes a false predecessor that blocks CSE (AttackGeneral). A
  proven-nonzero case may fall THROUGH a later case's zero-fill guard
  (DamageControl). A shared continuation can live INSIDE a later case body —
  follow the jump-table body addresses (ActSQUAT's `goto move_if_stationary`).
  Identical-valued case bodies can still be distinct islands — direct stores in
  body order, no shared temp (ActCHASE's two `0xf02`s). A jump-table index
  opened by `lhu/addiu -BASE/sll 16/sra 16/sltiu` is a cast-narrowed switch:
  `switch ((short)(ptr->mid - 0xA00))` (ActHANG).
- **The switch index register doubling as its matched constant** is
  `record_jump_equiv` on the taken edge — write the literal in the case body;
  the callee-saved dispatch index is normal (ProcItemGun).
- **Don't reuse the switch variable across calls inside case bodies** — it
  extends one pseudo across calls and adds a prologue save; use a case-local
  (DoInfoViewProc). A tests-then-bodies ladder for two values is a nested
  `switch` — the taken-edge bodies get fresh `li`s because cse doesn't follow
  the taken edge.
- **A callee-saved constant on one path, fresh `li` on another** = a local
  (`u8 ff = 0xff;`) vs a literal; caller-saved when no call intervenes
  (`if (item->proc)` inline is what frees `$v0`) — and the two levers can be
  ATOMIC (`literal-indirect-inline` submits the pair; ProcItemDokudango's
  valley).

### 3.2 Guards, return islands, shared tails

- **Guard `return 0;`s cross-jump into the LAST plain island in emission
  order** — which mid-function label the target's guards branch to tells you
  whether the shared return was an end-of-function `if (cond){body} return 0;`
  or an inline early guard. Same-block `v0=0` gets sched-hoisted into a
  load-delay hole; separate-block survives for cross-jump — both shapes coexist
  (HangCheck). rtlguide names the layout mismatch `guard-return-island-layout`
  — try `if-else-invert`/one shared label once, then park rather than reacting
  to address-drift byte counts (SearchTarget).
- **Two-guard-then-fall-through dispatch converging on ONE shared return tail**:
  `if (A) goto a; if (B) goto b; goto tail; a: …; goto tail; b: …; tail:
  return v;` — three literal `return EXPR;`s each compile their own tail.
  Conversely **independently-written identical returns are what cross-jump
  wants**: an explicit `goto` to one return pins the code at the label's
  position (Think4abandon) — EXCEPT when two byte-identical `return expr;`
  tails fail to merge in this cc1: then funnel through one `ret` + `goto done;`
  (CGetLevel). Route by what the target's tail addresses show.
- **A named `goto` label pins cross-jump's primary copy** — place `reject:`
  exactly where the target's surviving copy lives, and that means **INSIDE the
  guard whose body it is**, not parked at the bottom next to `ret:` (DrawModel;
  DrawClip matched on the first build once the label moved — its park's A/B had
  both arms wrong and reported "unreachable"; the matched sibling DrawSprite had
  the answer in plain sight). N direct rejects with sentinels in delay slots are
  the POST-REORG view of N `goto reject;` sites — after reorg, a branch target
  is not the source-level goto target; the site still aiming at the real block
  had a full slot (compiler-facts: reorg).
- **Assign a sentinel ONLY on each exit edge** (`result = -1; goto ret;` per
  reject) to keep it rematerialised caller-saved; hoisting one assignment to the
  top forces a single materialisation (DrawSprite — whose textually-identical
  reject condition must also be a STANDALONE if, not an else, or cross-jump
  merges it away).
- **A shared flag assignment feeding a real multi-predecessor join is an
  allocation lever; followed immediately by its own local return it
  constant-folds** (`return !flag` after `flag = 1` → literal) — keep the
  success edge into `done:` (IsVisible's three-register rotation: priority 3636
  vs 1500 read straight off the two shapes).
- **Duplicate a complete cleanup/advance tail at each exit** when its physical
  address is inside the state bodies — jump2 merges onto its chosen last copy
  (ProcItemDokudango's cleanup at two origins + advance at three exits). For
  indirect-call cleanups: cache `proc = item->proc` for the null check but call
  `item->proc(item)`, per duplicate — a factored `dispose:` label is a CSE
  boundary that costs a reload + nop (ProcItemNinken/Napalm; LoadSI's error
  arms). Write per-case calls INLINE rather than funnelling arguments through
  shared locals into one post-switch call — cross-jump merges the tails AND each
  address materialises straight into `$a0` (FUN_8004f6c0); when two branches
  build the same call with all-different arguments, write it twice
  (StartDrawing). Keep duplicated branch-local calls when the target preps
  different arguments per predecessor but shows one merged `jal`
  (DefaultActionHumanoid, DamageControl, ActSWIM — where the last residual was
  only WHICH terminal arm is the fallthrough: `terminal-arm-flip`).
- **A referenced `case` label is a hard cross-jump fence** — a boolean `switch`
  keeps an exceptional copy distinct where if/fences/unreferenced labels cannot
  (StateTransition; `case-fence` mechanises the two-way form). Algebraically
  equal affine tails can be kept distinct with different spellings
  (`mul-affine-shape`, FUN_800519bc). Machine-identical calls can be different
  to jump2 (result mode / USAGE fingerprint — ActATTACK's five DeleteConflicts;
  rtlguide prints call fingerprints; changing a shared prototype needs semantic
  review).
- **Direct terminal global tails beat a named value local**: write the complete
  `motID = K; D_80097F0E = 1; return;` at each edge; short-lived HImode
  producers take `$v0` and jump2 merges only the common suffix onto the
  fallthrough anchor — a named local hard-conflicts with `$v0` across the
  comparisons (ActENGAGE; ActACTION; `shared-tail-assign`/`shared-terminal-tail`
  mechanise the adjacent forms).
- **An epilogue `sra` ABOVE the register restores means a SECOND literal
  `return`** (sched2 runs before jump2 — compiler-facts): the extra return's
  CODE_LABEL pins the truncation above the loads, cross-jump erases the
  duplicate body for free. Constraint: the early-return site must not sit
  between two blocks sharing a cse'd value (turn_towards_player_'s unique safe
  site). rtlguide names it `shared-return-extension-schedule`;
  `shared-return-split` / `terminal-call-return` mechanise the goto→return and
  terminal-call edges (Think3attack 10 bytes; Think3hitaway 14 — try the edge
  the target's value flow names, arms are not interchangeable).
- **Two identical tails surviving in the target only because their BASE
  REGISTERS differ** is an allocation outcome — no source donor can prevent the
  merge; chase the register split (§3.8).
- **When both arms end in the same statement on an address-taken variable,
  duplicate it into both arms** — one post-if copy forces a stack reload at the
  merge (AdtMessageBox).
- **A call+return tail shared by several loop exits with the arg `li` in each
  exit's jump delay slot is ONE copy written after the loop** — `break`
  everywhere, write the call once; reorg steals the arg li into each break's
  slot (PauseProc's SsSetMVol).
- **A store shared via the branch delay slot**: load the condition into a temp,
  store between temp and branch (BriefingAndInventorySelectionScreen's entry).
- Guard islands and delay slots interact — the delay-slot side is §3.13.

### 3.3 Loop forms

**Which loop form the original used is read off the target, and the form
decides notes, hoisting, rotation, and delay-slot fills:**

- `for`/`while (cond)`: bottom-test shape via `duplicate_loop_exit_test`; a
  provable initial value folds the entry test away. `while (1) { if (!c) break;
  …; i++; }` keeps a TOP test + unconditional back-jump AND loop notes
  (hoisting) (LoadConstruction's magic-constant diagnostic). A hand `label:`/
  `goto` loop keeps the top test but gets NO notes — no hoisting, no strength
  reduction, no VTOP (GetAreaMapLevel; ClearItemLayout matched first try where
  while(1)+break grew the frame). **No combined address bases + no rotated
  tests ⇒ goto loop.**
- **Fallback placement selects the shape for pool scans**: a give-up
  `return &dmy;` INSIDE the loop body is an invariant `la` that loop.c hoists —
  no spelling suppresses it; hand-roll the loop (SetFrame/SetSplash/SetBleed).
  The same fallback AFTER the loop hoists nothing, and only a genuine bottom
  `do…while` reproduces the `idx++/idx--` steal-and-compensate exit (SetImpact/
  SetExplosion). Do not assume a family idiom carries over.
- **You cannot flip a `break`'s branch polarity by negating its test** —
  byte-identical (verified); move the other case's work inside the `if` ahead of
  the `break` instead.
- **A bottom-tested scan can want its cursor-pointer derivation at the TOP of a
  real `while` body**: `i = 0; while (buf[i]) { p = buf; p += i; …; }` — the
  duplicated entry test consumes `buf[i]` and the back edge forms the bottom
  test's address and the next `p` independently (one fills the delay slot); the
  `if/do` spelling CSEs them into one address, one insn short (AfsFindFile).
- **Counter width**: a `short` counter suppresses strength reduction — the
  recompute-from-base shape (SetupMotionRegist); a `u16` counter that only helps
  in a do-while is a strength-reduction artifact — the counter is `int` and the
  LOOP SHAPE is wrong (FUN_80058c70). Disjoint loops need not share one counter
  (block-scoped `short scan_i` when colouring rotates: ActKAGI, SwimCheck) — and
  conversely repeated hard-register identities recover WHICH named counter drove
  each loop (ArrangeLocalMatrix's `k`/`j` from `$a2`/`$t4`).
- **Index (`T[i].f`) vs walking pointer**: at 2+ touched fields a walker gets
  biased to the LAST field (offset-0 tell); at 3+ it splits into a second
  parallel induction register — index by the loop's own counter
  (FUN_800568b8, FUN_8004a6bc). A literal goto backedge rescues a walker when a
  field store would become a second GIV (DoMiscProc's nonvolatile walker +
  movable `pause = 0`). Indexing can also steer PREHEADER order at identical
  loop bodies (check_for_known_button_combination — require `.loop`'s
  BIV-elimination report). Increment-first beats read-one-ahead
  (SetupTraceLine).
- **Rotated sentinel scans**: the direct `while (table[i] != needle) { if
  (limit <= i) break; i++; }` with a narrow counter emits the pre-load +
  working-register + delay-slot-commit rotation (ActivateHumans, SetupWeapon
  via `subscript-postinc` — treat a postincrement mismatch as a possible
  allocation root cause, and capture the address at the postincrement site when
  the field is read AND written). The dup-while head family: entry check =
  jump.c's duplicated exit test cse-folded at `idx==0`; backedge landing one
  insn past the loop-top load = reorg skipping the redundant target-head load —
  never reconstruct with walker/preload locals (BreedLife).
- **A repeated per-axis normalization is usually ONE shared loop** whose
  branches rotate (DamageControl's passage vector; `__builtin_abs` at entry AND
  backedge to stop a cross-jump). Sibling scans in one function can
  deliberately use DIFFERENT forms — only `for` notes make reorg copy `i+1`
  into two delay slots (StartStageSequence).
- **Two-break scans, wrap-around searches, entry-test cursors**: zoo entries.
- Loop-invariant literals reused across ≥2 calls in a loop need a named local
  assigned once before the loop (RestoreItemLayout, LoadOrnamentArchive);
  in-loop economics beyond this are §3.14.

### 3.4 Expressions, widths, arithmetic

Mechanised spellings live in the index (`type-width`, `param-width`,
`shift-stage`, `shift16-mul`, `rand-mod`, `mod-bias-temp`, `call-arg-pair`,
`split-chain`, `plus-group`, `add-prefix-temp`, `cmp-swap`, `cmp-polarity`,
`eq-literal-swap`, `member-scalar-alias`, `pointee-volatile`, `min-ternary`,
`clamp-shared-return`, `abs-ge`, `builtin-abs`, `difference-role-fuse`,
`call-result-split`, `vector-copy`, `assignment-chain`…). What remains is
judgment:

- **fold reassociation is a spelling contract** (compiler-facts): spell the
  grouping the target's `addiu`+`addu/subu` pair implies — `t + (rand() % 1000
  - 500)`, `dtL->vy - (0x69 - y)` (HangCheck); `A + (B + 25)` to land the
  constant on A (MoveKorogari; rtlguide rejoins the pair as
  `constant-add-reassociation`).
- **Keep calls inline in expressions** (`x = rand() % 200 - 100;` computes on
  `$v0`; a temp inserts a move) — but a two-statement remainder temp is provable
  from the asm: `mult/subu` operating on the MOVED result's `$sN` (the
  `rand-mod` pair). Publishing random results before an independent countdown
  can fill target gaps (ProcItemDokudango). Separate single-definition rand
  temps preserve `$v0` coalescing; a named base temp keeps the bias on the
  coordinate side (ProcItemFire; DrawShadow; `call-result-split`).
- **Width dataflow** — the recurring levers:
  - Loads name the type: `lhu` = u16 field even beside s16 neighbours; original
    TUs disagree about one field — keep the proven header type, cast at the
    divergent use (DoInfoViewProc). A NARROWING use reads even a signed global
    `lhu` while a comparison needs `lh` — two un-CSE'd loads, give the narrowing
    use its own short temp (DeleteConflict). A pure narrowing field-to-field
    copy is `lhu/lbu` regardless of signedness — don't fight it.
  - Store + later `sll/sra` at the consumer = u16 storage + `(s16)` cast AT THE
    CONSUMER, in a wide working local — not an `s16` field or an early cast
    (mission_score_screen; FUN_8002fd9c).
  - `u8` locals re-narrowed after arithmetic get `andi 0xff` + `sltiu`; if the
    target compares `slti`, the local is `s32` (FUN_800576e8, ProcMiscPitfall).
  - A flag's width (`s16` vs `s32`) changes copy chains, case-constant reuse,
    literal rematerialisation, and whole-function length
    (handle_char_state_using_item_, SearchTarget, ActATTACK's HImode
    `cleanup_guard`, ActDAMAGE's `done`) — `type-width` sweeps it; route the
    guided sweep rather than guessing.
  - Store-before-sign-extend on capture-and-increment: the store breaks the
    memory equivalence that would otherwise re-render `(short)v` as a reload
    (InsertConflict).
  - Halfword constants name the element type: `addiu -1` = s16, `ori 0xffff` =
    u16 (initializers included); an `x != 0` test emits `sll+beqz` for s16,
    `andi` for u16 (PauseProc). A u16-init `= -1` that still reloads `lhu` is a
    2-byte union (`pad.s = -1;` + `.u` reads).
  - A narrow increment shared with a call argument goes through a narrow temp
    BEFORE the call (PauseProc); `int io = shortvar;` shares one sign-extended
    pseudo between a zero-test and a later mask (MoveHumanoid); a byte-resign
    writing a DIFFERENT register is a separate local (MoveHumanoid's `o`).
  - Division shortening is blocked by an int dividend temp; write the remainder
    through a temp too, and keep the loop-carried copy int (BIS digit loops —
    the whole cluster).
  - A captured field used in later arithmetic is `s32` even when the field is
    narrow (DrawFrame, DrawSmoke); a full-word global snapshot stays full-width
    (ProcItemArrow); a `u8`-stored countdown tested later wants an `s32` host +
    explicit narrow tests (ProcItemFire); keep a narrow countdown's new value in
    an `s32` temp when the target zero-tests `sll 16` (ProcItemDokudango).
  - An s16→s32 extend the target does IN PLACE is an explicit shift pair
    (`x <<= 16; x >>= 16;`) — otherwise the intermediate routes through `$v0`
    (FUN_800519bc). **TOOL TICKET (signext-inplace-pair autorule)**.
    Two adjacent extensions the target INTERLEAVES are hand-split halves
    (BIS cursor). A narrowed negate needs a separate SImode operand variable
    (compiler-facts; mission_score_screen).
    **TOOL TICKET (narrowed-negate autorule)**.
- **The abs family**: spell the ternary GE (`(raw >= 0) ? raw : -raw`) — LT
  never folds (compiler-facts). Under `-fno-builtin`, plain `abs()` is a real
  call; `__builtin_abs` requests the inline expansion per site (ActKAGI; keep
  the call when retail calls the library: GetVectorLength). The builtin also
  absorbs a sign-fix's allocation shape (ProcMiscDoor) and — being ONE
  multi-insn through reorg — preserves delay-slot `nop`s an open-coded compare
  would let reorg raid (FUN_80056910; DamageControl lesson 1). All three
  ABS_EXPR spellings are identical; do not credit "inline vs named temp"
  (ChasetoTarget, by experiment).
- **A ternary is NOT an if/else variant when both arms assign one destination**:
  it forces a merge pseudo + a SINGLE store at the join. Diagnose by STORE COUNT
  per destination in the target, not delay slots (FUN_8004d6d4: two ternaries,
  55→0; the park had rejected Ghidra's shared trailing assignment as an
  artifact). The inverse select-shape: `bnez/nop/move dst,src/L: sh dst` is
  `if (c) next = saved; X = next;` — NEVER a ternary, which duplicates the store
  (FUN_8005a7a4).
- **Multiplies/divisions**: `%`/`/` by constants are magic-multiply sequences;
  the same magic constant CSEs into one callee-saved register. A negative
  non-power-of-two multiply is spelled as its own strength reduction
  (`((sync - (sync << 4)) << 4) - 0xA` for `* -0xF0`, EndDrawing).
  **TOOL TICKET (neg-mul-strength autorule)**. Two divisions back-to-back
  before either's store block (UpdateSplineControl); divisions by one runtime
  divisor compile eagerly before any consuming call (IsVisible); expanded
  divisions physically before a flag branch belong before the guard in C too
  (PadProc); grouped quotient temps when the target groups the stores
  (FUN_80036284); signed fixed-point bias-and-shift spelled literally
  (SetWire). Variable division needs `--expand-div` (§3.16).
- **Two loads of one short field at one address** = combine's one-use gate —
  the lever is WHICH READ COMES FIRST, not a cast (compiler-facts). An
  intervening store between a load and its first use likewise blocks combine
  from fusing `lhu`+extend into `lh` — write a delay-slot-bound store BEFORE
  the load feeding the next test (AttackContinuousCheck).
- **A conditional negate must re-read its own destination**: `x = y; if (cond)
  x = -x;` picks `negu $rX,$rX`; `x = -y` picks `negu $rX,$rY` — match by
  whether the negate re-reads what it writes (Think1trace).
- **A field compared then re-read inside the hit body**: capture via a comma
  expression in the RHS operand — `if (cond && (z = lv.vz, z < dist))` — the
  only spelling giving short-circuit AND one load (SearchItemTarget2). Keep a
  compared memory read INLINE in both `&&` operands — hoisting it into a temp
  swaps two registers, and neither autorules nor regalloc sees it
  (FUN_800568b8's 5-byte tie).
- **Call-result width is read off the extension position**: extends once at the
  assignment before joins = `int`; re-extends at each compare after joins =
  `short` (PauseProc). A short-returning call result that INDEXES an array
  wants `s32` — s16 lets the extension float to the use and interleave with the
  address calc (Makibishi/LightningBolt). A `(u16)x << 1` widening needs a
  `u32` intermediate or the double-shift collapses (EndDrawing).
- **A narrow field's first use can need a distinct one-use full-width carrier**
  (`s32 initial_h = sp->h;` keeps the `lhu` SI-producing and schedulable —
  FUN_80056910); when an `lbu` field feeds a multiply, a full-width named temp
  schedules the load early without the `andi` a u8 temp inserts
  (ProcItemNemuri).
- **A target's "redundant" extra reload = a fresh field dereference in
  source** — cc1 will not refetch what is live in a register, so provable
  equality is not a licence to reuse the variable (vrealloc's `vh.next =
  vhp->next;`). Ghidra's own SSA shows this: a second separately-named
  dereference instead of temp reuse means the target really refetches
  (CVArun's GsSortSprite re-read; CVAsetup's post-loop nudge).
- **Batching**: N adjacent loads with no use between them are source temps even
  if the stores scatter (ReqItemJirai); an address-expression mult-subterm
  expands FIRST unless spelled as a shift or through a value temp
  (compiler-facts); comparison operand order front-loads one side's evaluation
  (`found > mem` vs `mem < found`, GetConflictResult); a scc+branch
  (`slti/xori/bnez`) is a NAMED flag variable (SetCameraMode).
- **Store order levers**: pointer-field publish FIRST in a store group
  (GetConflictResult); constants stored to narrow+wide fields share an int
  variable only when the constant also feeds arithmetic (ProcItemDrop/Makibishi
  vs LightningBolt); same-byte constants can be distinct identities via a
  byte-equivalent signed value (start_demo_ checkpoint); positive-immediate
  spelling of a byte-store literal needs a named variable (`s32 bias = 0x80;`,
  SelectCameraOwnerOption; same family: compute a fanned-out coordinate once in
  an s32 temp — SetupTelop).
- **Mask spelling picks the instruction**: `& 0xA000` = `andi`; `& ~0x5FFF` =
  `li`+`and` — read the raw immediate (Think2confirm vs Think1sleep).
- **Countdown idioms are per-function**: `count - 1` vs `count + 0xff` — decode
  the immediate, Ghidra's `+0xff` is real (Happou vs LightningBolt).

### 3.5 Calls, prototypes, inline helpers

- **Per-TU prototypes are a lever, not a bug**: a caller-side `extern u32 f()`
  for a u16-returning callee moves the extension after the bit-chain
  (GetRealPad); `extern u16 f()` lets a sign test compile `sll+bltz` with no sra
  (JumpControl). Original TUs routinely disagree with the defining TU —
  respell per-TU, don't fix the shared header. EXCEPTION: SDK ABI facts are
  centralised — `GetTPage` takes four `s32`s (include/psxsdk/libgs.h; tests
  reject TU-local redeclarations).
- **Audit the callee's return width before blaming the allocator** — a
  wrongly-wide prototype creates a full-width call-result pseudo that CSE
  propagates across calls (Think1ninja's `s16 Think1random`). Take prototypes
  from the matched CALLER over Ghidra (FUN_80058a54 → FUN_80058c70).
- **A conditional value used ONLY as a call argument is an inline ternary** in
  the argument position — a preceding if/else blocks sched1 from interleaving
  the flag with the other args (SetupSoundEffect; AVCameraSetup adds: assigning
  it first re-reads the field with the wrong signedness).
- **`GsSortSprite`'s `int pri` argument needs `(u16)` at the call site** (the
  `andi` in the delay slot).
- **Small static-inline helpers are real codegen levers**: pointer formals
  create parameter bindings that act as deliberate CSE barriers
  (FUN_800519bc's TimToDemoSprite); an inlined byte-pack helper may need both
  cursor identities as inputs (AfsGetEntry); keep helpers inside the caller's
  `#if` guard (stub TUs emit unreferenced statics). Demo-call inlining: §1.
- **A guarded indirect call**: null-check through a variable, call through the
  FIELD (`ppu = item->proc; if (!ppu) return; … item->proc(item);`) — cse
  reuses the load and lands `$v0` (the item family).

### 3.6 Volatile discipline

Every `volatile` is a scheduling barrier at each access, and the barrier itself
can BE the residual (start_demo_'s prologue rotation — when a residual block
ROTATES around a volatile, suspect the qualifier). Site-local forms, in
preference order: a pointer-to-volatile view for one access
(`(*(Object *volatile *)&Objects[i])->field`, UpdateEvent; StartStageSequence's
`volatile u16 *stg_think` — `pointee-volatile` sweeps the local-pointer form);
a volatile word view of an explicit spill slot (AddEnemy's post-call reload); a
volatile-qualified parameter OBJECT for a single proven early stack read
(CdaPlayXA). Never make a shared extern declaration volatile. Corroborate an
otherwise-dead target load in another shipped build before encoding it
(ActATTACK's trial-build `Me_MOTION_C` read); do not invent dead reads to fill
bytes.

### 3.7 Stack objects, unions, frames

- **Run `tools/stackplan.py <Name>` first.** It reads the outgoing-arg size, the
  first callee-saved spill, and every `sp+offset` access; the reported window is
  the scratch-union extent, `--emit-overlay` writes a padded C scaffold
  (LoadConstruction's 0x1a0 frame), and it prints `vector-array hint`s
  (FUN_80035f44). Replace neutral names with proven aggregates before treating
  it as source.
- **Overlapping locals are one `u8 buf[N]` + casts** — cc1 2.8 never shares
  sibling-scope slots (`(PARAM_ITEM_USE *)buf` views; LoadCard/SaveCard's single
  0x2000 block buffer). For mutually-exclusive layouts write an explicit union
  with one view per mode (ProcItemFire's 0x60 union; ProcItemJirai, DrawBlood,
  ActDEAD…).
- **Overlapping big buffers whose addresses rematerialise at every call are
  INLINED STATIC HELPERS** (DoInfoViewProc's menus): a nonzero frame-address
  argument is forced into a pseudo and same-valued pseudos CSE across calls
  into one callee-saved register (`addiu $s0,$sp,N` + `move $a1,$s0` twice) —
  a helper's own FIRST local sits at inlined-frame offset 0, so its address is
  the bare register and every call rematerialises `addiu $a1,$sp,N` like the
  target; inline expansion allocates the helper frame as a TEMP SLOT the next
  helper reuses if it fits. Tells: one sp offset written by two unrelated
  struct copies + per-call `addiu $aN,$sp,N` + a frame smaller than the visible
  buffers. **The inverse tell**: args + sum(buffers) + saves = frame exactly ⇒
  PLAIN LOCALS (each copy loop's CODE_LABEL breaks the cse window)
  (LayoutEnemyOption 0xC0). Passing the written object as the helper's pointer
  parameter keeps later argument uses direct; a `p = &spr;` variable is the
  original's shape when the object is the FIRST local.
- **The item param-union family**: a write to OFFSET 0 routes through a fresh
  `((T *)it->param)` recast (folds into `it`'s register: `sw rN,0x20(it)`),
  nonzero offsets through the live `pp` pointer — mechanical, not stylistic
  (ReqItemKaengeki's 6-word tail); `pp` is declared before the null check even
  when none of its named fields are read (ReqItemGoshikimai). A store whose
  WIDTH differs from the proven shared view at that offset is a distinct union
  member — explicit offset cast off the proven pointer, never a new named
  struct (ReqItemDokudango; `access.py` prints the widths).
  Conversely three scalar deltas can be one real VECTOR local (SearchTarget,
  register_character_death), and two xyz triplets 0x10 apart one `VECTOR[2]`
  (FUN_80035f44).
- **Mixed pointer/direct access to one aggregate is a real shape**: keep
  `p = &local` for the base-relative store and the call, spell other fields
  direct (ProcItemFire's launch; DrawTarget's scr/OTZ overlay — the pointer
  dies and members reload from sp). The array variant spells only the
  target-rematerialized stores as typed byte-offset lvalues
  (`array-alias-remat`; mission_score_screen).
- **Frame arithmetic is evidence**: declared locals + saved block vs the
  target's frame — a residue ≡ 0 (mod 4) is the ORIGINAL allocator's
  caller-save/reload slots (AddEnemy: retail spilled exactly two values; beware
  circular evidence from self-sized scratch structs — the non-circular
  signature is `$t1` saved in the jal delay slot + reverse-order restores).
  An unexplained gap with no accesses is a DEAD LOCAL (AttackGunControl's unused
  `PARAM_ITEM_USE`; FUN_8004a6bc's PSX.SYM-recorded `image[25]`; `main`).
  **TOOL TICKET (slotcalc)**: model assign_stack_local from declared sizes +
  target sp+K accesses → the unique declaration order + implied spill count;
  0 or >1 orders fit ⇒ the object set is wrong.
- **Slot order is arithmetic** (compiler-facts): declaration order is the only
  input — compute the order, never permute declarations (FUN_80018f00). When
  declaration order is double-booked (slots AND a CSE merge), break the merge
  with the independent lever — the identical-arm call fence (cbAccess,
  FUN_80018f00). `stack-decl-swap` sweeps the adjacent-pair case
  (leLayoutEnemy).
- **The three-phase rule**: params → declared locals → reload spills (pseudo
  order). A declared local can never sit above a reload spill; target layouts
  with spilled params at the lowest slots are all-reload, unreachable by any
  volatile/declared local (FUN_80032720's header). A pre-loop `sll r,x,16` with
  `sw` of the shifted value + in-loop `lw`/`sra` is ONE sign-extension split by
  a spill — and the live shifted value is itself what forces the spill.
  Rotating t0–t3 reloads at use sites = plain spilled locals in DECLARATION
  order, not an addressable struct (LoadConstruction lesson 1).
- **Block moves**: cast alignment drives copy code (VECTOR word copy vs SVECTOR
  `lwl/lwr`); an opaque `struct { u8 bytes[N]; }` carries alignment 1 and takes
  the runtime-alignment path (LoadCard/SaveCard; SaveSI's explicit chunk loops
  only after the aggregate form proves exact). Four `vx/vy/vz/pad` copies may be
  one aggregate assignment (`vector-copy`); copy-then-adjust when the target
  stores the unmodified component first (`vector-copy-adjust`, ProcItemNinken);
  a pool swap-remove is `pool[i] = pool[last];` (DeleteConflict — access.py
  disproves field-typed renderings); a small table copy is ONE struct assignment
  (load_layout), through a NAMED source pointer when the target materialises the
  member address (InitEffect).
- **Truncated per-TU structs break under array-INDEXING** (stride =
  `i * sizeof(T)`) — reuse the proven full-size element (FUN_80027304).
- **Grouped stores through the same base expression a later copy reads** keep
  the store→copy dependence (the divide-latency interleave).
- **A varargs arg pointer is computed lazily at the call** unless the call is
  near entry (AdtMessageBox vs FUN_8005fe38).
- **cc1 does not fold away a pointer temp to a frame object** — repeated
  `((T *)buf)->field` casts are the source's real shape when the target
  rematerialises `addiu sp,K` per use; a mid-sequence `addiu $a0,$s1,8` is a
  temp assigned between statements. Anti-rules: an element-pointer temp
  materialises the array displacement as a real `addiu` (keep repeated
  `base[n]`), and an address-taken local moves from a spill slot to a frame
  slot, shifting everything.
- **Stack parameters**: used exactly once → load-sunk to the use; a local copy
  defeats the sink and moves the entry `lw` to the copy's DECL position (decl
  order = load order, AddMisc); REG_EQUIV-noted parms carry doubled live length
  and lose races their ref counts predict (compiler-facts).

### 3.8 Variable identity

The identity canon — the cheapest questions on the board, asked in order:

1. **TWO REGISTERS FOR ONE VALUE = TWO VARIABLES.** cc1 never splits a live
   range. If the target holds "one" value in two registers across disjoint
   regions, the original had two variables — full stop, read off the bytes
   (AttackBowControl). It does NOT run backwards: writing two variables does not
   force two registers (no coalescing pass; dying values merge INSEPARABLY —
   regalloc prints the verdict, and a brief once contradicted the tool). And
   one register at both sites proves NOTHING (disjoint halves share free
   registers).
2. **GIVE THE VALUE THE VARIABLE'S IDENTITY.** When the target reuses ONE
   register across load → op → result, the load IS the variable, not a temp:
   split `start = p->field; start = start * inverse;` (DrawImpact 28→8; the
   shift carrier `v = v >> 12;` as its own statement, 8→4). Diagnose by asking
   whether the TARGET's load and product share a register — the symptom's
   register is the LAST thing allocated, not the cause.
3. **Ghidra's reused temps are MEGA-PSEUDOS — split per site/block.**
   `reghist <Name>` first on any big function: a caller-saved register mentioned
   ~70 times is a mega-pseudo (no real source has one); zero-sum deltas confined
   to arg registers = pure renames, decomposition already matches, STOP
   splitting (StageEndScreen, AddEnemy). The target assigning the same
   expression to a1,a1,a1,a2 across blocks is direct proof of separate
   variables (FUN_80057b80 759→619). Delay-slot diffs can be pure CONSEQUENCES
   of allocation — fix allocation, not the schedule.
4. **Two disjoint same-role temps may be ONE reused variable** — merging sums
   refs and vaults priority in `floor_log2` jumps (vfree's `s`; DamageControl's
   id/severity local; DrawImpact's interpolation temps carrying pz/py/px).
   The tell: the target loads two different values into the SAME register in
   disjoint regions. Requires `.lreg`-proven disjoint lives + target
   hard-register homes — names/semantics are irrelevant. Works for long-lived
   roles (AddEnemy's `$s4/$s7/$s5` from demo locals), one local spanning three
   phases in `$s3` (FUN_8005b17c), reuse of a DYING call result for a
   call-crossing value (SoundEx's `dist`/vol coalesce — reach for it when the
   target's frame is LARGER than yours), and callee-saved homes on short-lived
   block locals (BIS's shared `j`). But do NOT fuse two locals just to chase a
   demo register — disjoint ranges land together anyway, and the fused
   multi-def local blocks `num_sign_bit_copies` (AddEnemy's names_offset/type).
   **The INVERSE bites when YOU reuse one name across mutually-exclusive
   branches**: a single `sentinel`/`voice`/`end_marker` local shared between two
   `if`/loop arms can force a REAL conflict with an unrelated pseudo if any one of
   its uses keeps it live past the target's actual lifetime in that arm. `.greg`'s
   `N conflicts:` list shows the false conflict outright. Give each arm its OWN
   local even if PSX.SYM or an earlier round named them the same — splitting
   collapsed 3 clusters in one edit (PlayVoice 9→4). **PSX.SYM naming two things
   the same is a nested-block SCOPE hint, not licence to share one pseudo across
   disjoint branches.**
   **A "materialize a constant into a named local" seed is POSITION-sensitive, not
   just present-or-absent.** The same edit pays off at one site and regresses at
   another: PutItemList's `scale` local worked assigned at its point-of-use, but a
   byte-identical `numY` for `NumberImage.y = 0x64` only paid off declared at the
   FUNCTION TOP — at point-of-use or inside a fence it measured strictly worse.
   When porting this class, sweep the DECLARATION POINT, not only the presence.
   And the reuse must be of an ALREADY-LIVE, now-dead pseudo — a FRESH temp of
   identical shape does NOT reproduce the effect (SetupTelop 11→9 with `north`
   reused, no change with a fresh temp; empirical control).
5. **Split shared pseudos as a PRIORITY lever** — splitting demotes each half
   superlinearly (ControlHumanoid's magnitude 24615 → 10000/14285/14285, last
   7 bytes). A plain COPY cannot split an allocno (cse folds it back — a
   guaranteed no-op, nullcheck); each half must hold a different computed
   value. A half collapsing into one block moves it to local-alloc — fatal or
   required depending on where the TARGET keeps it (ControlHumanoid regressed;
   AttackBowControl required it).
6. **Split state-machine lifetimes Ghidra merged** (scan candidate vs found
   result; pre/post-call pointer; mode-local vs later human —
   ProcItemDokudango), and split input from arithmetic result when the target
   does (`current`/`result`, ActCHASE). Conversely spell the owning path
   (`param->eater->target`) when the target materialises that identity.
   Disjoint semantic phases get block-scoped locals (SetWire, AttackControl's
   two `enemy`s); a variable read/written only within one conditional arm is
   SCOPED to that arm — Ghidra's top-of-function capture is SSA placement, not
   source position (PutStrain's length fix). Disjoint passes need not share a
   stack carrier (LoadConstruction's msize). A shared physical suffix is
   stronger evidence than decompiler scopes (ControlHumanoid's one
   `direction`). Across mutually-exclusive switch cases, reuse the variable by
   ROLE — reusing one tied to another role drags that case's allocation too
   (EquipWeapon).
7. **One value in TWO callee-saved registers simultaneously is an explicit
   source copy** (`trg = cur & (cur ^ opad); opad = trg;` — PauseProc;
   SearchTarget's `base_y` across an arithmetic/stack join). A callee-saved
   value dying at a call whose result lands in the SAME s-register is the
   call-result variable HOSTING the earlier load (`h = …; gy = h; … h =
   GetAreaMapLevel(…, gy, …);` — DebugMenuItemSet). When the target fuses an
   address copy away, make the first access a direct field op and name the
   pointer SECOND (`NumberImage.w = 4; img = &NumberImage;`), keeping later
   reads through the pointer (PutStrain).
8. **Parameters**: a promoted short param is TWO pseudos free — split USES per
   path, not variables (Sound). A reused parameter shows `move $sN,$aN` +
   arithmetic on `$sN` (`x = x / 10;`); pointer params reassigned in place show
   residual per-call offsets (LoadOrnament); a nullable param reassigned to a
   stack fallback keeps addressing the fallback THROUGH the pointer (SetWire).
9. **Pointer chains**: cached pointers living in `$s` across calls are real
   source temps (ProcItemManebue) — but a dead pointer chain may need to stay
   direct (`human->model->rotate.vy += K`, ProcItemKaengeki), and the useful
   cache boundary can be the ENCLOSING object (ProcSightShot's `model` not
   `&model->rotate`). Base + advancing walker want distinct identities
   (BreedLife). Store through the DERIVED pointer when the target does — base+
   offset spelling starves the pointer of refs (FUN_80057b80's `$s6`). When the
   target keeps two same-valued pointers in separate homes, compute both
   independently — a copy carries a preference edge that coalesces them
   (compiler-facts; FUN_80057b80).
10. **Accumulator types carry copies**: `acc = acc | call()`'s surviving
   `or/move` pair is forced by narrowing the ACCUMULATOR to s16 (what PSX.SYM
   recorded) — the copy's source stays live past the copy and
   optimize_reg_copy_1 declines (AddEnemy 125→81). "Only a different data flow
   would fix this" in a park note usually means a TYPE or lifetime change.

### 3.9 Allocation steering

The mechanics are in compiler-facts (priority formula, find_reg order,
preference machinery, REG_N_DEATHS, reload round-robin). The craft:

- **Measure before steering**: `regalloc <Name>` (dispositions, live-across-call
  values, copy chains), `--order` (cc1's own printed order vs ours,
  self-validated; write down the target-implied order and diff), `--compare A
  B` / `--between S L U` / `--enclosed-refs N` (exact weighted-ref deltas and
  fence depths — never hand-arithmetic), `--prefer aN` (who carries a hard
  preference). Confirm a pseudo's IDENTITY by its disposition first — a
  priority row with no `pN->reg` disposition is a SPILLED pseudo, not your
  variable (mission_score_screen's three-round misread). Count the TARGET's
  mentions before any ref-count theory (`grep -c '\$s7'` killed a round's
  story) — but remember refs are loop-depth-weighted, so mention-count bounds
  are UNSOUND in loops.
- **Check the conflict list before any priority move**: a hard-reg conflict
  cannot be outranked (AddEnemy's fence: order flipped exactly as computed,
  zero bytes). rtlguide prints `HARD-CONFLICT`; the fix is lifetime/identity/
  interference, e.g. delay-slot-overwrite chains (`i = MotionUpdateMode;
  if (i != 0) { i = 0; … }`, ActCHASE), reusing a dead named local for the
  operand (think_setting_small_rotation_small_steps_), duplicating a pointer
  assignment into both arms so it can reuse the dead condition register
  (FUN_8001b2f4), or one function-scope local across mutually-exclusive arms to
  move the value from local- to global-alloc (ProcMiscPitfall).
- **Priorities and windows**: ballast the winner-to-be by adding an insn inside
  its live range (hoist a one-arm constant into a pre-branch variable — free
  when the target materialises it anyway, SoundEx); prefer DEMOTING the winner
  (reordering deletes the blocking preference set) over promoting the loser;
  some targets are a WINDOW, not an outrank (ProcItemNingyo's +3-exactly;
  DrawTargetS's four-value ladder) — use `--between` and distribute depth.
- **Preferences**: donate a call-arg preference to a low-priority pseudo to push
  a high-priority one off a register (vrealloc's memcpy `$a2`; SaveSI's donor
  calls); a donated colour can arrive via `REG_DEAD` partners two variables away
  (ControlHumanoid — the `.greg` one-line diff); term count decides whether a
  preference exists at all (compiler-facts).
- **Return funnels**: a shared `ret` copy-preferences its sources together —
  early returns split them (InsertConflict); the flag variant is
  `flag-return-split` (DrawBG, FUN_8005adbc); the INVERSE funnel keeps one
  result pseudo when the target preserves an early narrow result
  (`shared-result-return`, Think3chase).
- **Statement order is the main regalloc lever**: register-held stores first
  frees registers (ProcItemKusuri); one-block ties can hinge on an UNRELATED
  statement's position (PlayerOption); birth order can dissolve a recorded
  "hard-conflict" (mission_score_screen's rowValue — ask which value is created
  first); source liveness across a call beats final scheduled position
  (DrawBlood; SaveCard's inverse); a pre-call narrow capture + post-call
  widening separates a saved copy from its mask (FUN_8005778c).
- **The `%hi` reload tie is `combine_regs` refusing a block-crossing pseudo**
  (compiler-facts): a shared local funnelled from both if/else arms into one
  post-join call can never tie with its `%hi` temp — call the function
  DIRECTLY IN EACH ARM (cross-jump re-merges the tails, length unchanged), and
  the tied qty inherits the `$a0` suggestion. Stay WITHIN the existing if/else
  — converting to early-return can pull the call's address into loop.c's range
  (AfsOpen). Matched by FileRead, PrepareAccess, AfsOpen; found by a Sonnet
  lane from `rtldump --draft` + local-alloc.c. The aggregate-call variant
  duplicates the ARGUMENT setup per arm and writes fields + call once after
  the join (DrawTargetS).
- **Byte-neutral respellings are permuter seed levers** — a cse re-read the
  compiler folds back, or a fence around an UNRELATED block, shifts pseudo
  bookkeeping enough to flip ordering ties; semantic-diff permuter winners
  against the REPRINTED base (the reprint alone skews scores).
- **Reload structure**: a reload-register diff downstream of a reload-COUNT diff
  is one root cause — fix the count, ignore the sites (AdtSelect).
  **TOOL TICKET (regalloc --spill-uses)**: print each use BARE vs IN-MEM — the
  discriminator that retired a wrong park verdict. **TOOL TICKET (regalloc
  --names/--local)**: pseudo→C-name (UNIDENTIFIED when unprovable) and
  local-alloc quantities (invisible for ControlHumanoid).
- **Macros**: expanding a function-like local macro is byte-neutral BY
  CONSTRUCTION (measured: same cpp tokens, same .o sha) — expand FIRST anyway,
  because per-site variation is inexpressible through a shared spelling and the
  residual concentrates exactly where sites must differ (mission_score_screen's
  76-byte first harvest; 0/593 matched drafts contain one, and the three
  parked holdouts hold 24 between them). Expect a measurement, not a windfall
  (AddEnemy).

### 3.10 Fences — what each idiom actually buys

Fences are reconstruction scaffolding, not original idiom — **debt**. At exact
promotion, challenge every one (`fence-unwrap` + `empty-loop-boundary` removal
candidates are in the DEFAULT autorules set precisely because fence effects are
NON-LOCAL: FUN_800519bc's prologue fence closed 25 bytes ~100 lines away). A
fence whose depth sweep is FLAT is not a fence — delete it (AddEnemy's
`weapon++`). The mechanisms:

- **`do{}while(0)` buys exactly three things**: (1) REF WEIGHT — +1 weighted ref
  per enclosed ref per depth, a DIAL (feeds global priority AND local-alloc's
  QTY_CMP_PRI — an unexplained caller-saved shift near fenced code is this);
  (2) a SCHEDULING BARRIER — pins the wrapped statement's emitted order, and the
  first insn after LOOP_END gets a full pending dependency (an EMPTY fence is a
  PURE barrier: zero reweighting — run ref-fence vs empty-fence as an
  ATTRIBUTION TEST: identical counts ⇒ the barrier is doing all the work,
  StageEndScreen); (3) a wall against local-alloc's copy propagation when it
  ENCLOSES the copy. It does supply real CODE_LABELs — blocking combine's
  compare-into-branch merge and reorg's backward scan (FUN_8005a7a4's two tells,
  one fence) — but it CANNOT keep a copy alive past combine (jump1 deletes
  unreferenced labels; only an identical-arm fence can, below) and does NOT
  block cse store-to-load forwarding.
- **The identical-arm fence (`if (c) { A } else { A }`)** buys the one thing the
  loop fence cannot: a real block boundary that survives to flow/cse, erased by
  cross-jump AFTER combine and AFTER allocation. jump1 hoists a common leading
  insn — put a dead store inside one arm to keep the heads different
  (ActivateHumans' last 3 bytes). **Its CONDITION is allocator input**: the
  discriminator's live values participate (`if (human)` matched, `if (target)`
  cost 10 — ActSTATE; `identical-arm-condition` enumerates source-authentic
  probes). Uses: CSE-split of merged frame addresses (cbAccess), stack-address
  hard-conflict breaking (ProcItemNingyo), preserving guarded pointer copies +
  cse-source splits + weight donation — several fences for DIFFERENT pass facts
  in one function (Think3escape's three), scheduling-dependency injection
  (Think3firstattack's atomic pair with a width change), alias/base decoupling
  (SetupFly), raw/cache splits (`alias = cache = raw` + identical arms,
  GetAreaMapVector, PlayVoice).
- **Donor variants** (all erased by jump2): `allocation-donor-fence` (duplicated
  assignment under an initialized/guard-proven discriminator — FUN_80033bc0,
  Think1target, AddEnemy's weapon_entry after removing invented base locals);
  preference donors (function-scope discriminator overwritten before a call,
  ProcItemNemuri); `redundant-field-donor` (repeated pure local-aggregate field
  store, CameraDirection); `disjoint-local-alias` (dead-until-overwrite local
  joins an earlier range, Think1target). After each accepted donor, re-score the
  older fences and nearby initialiser orders as one bounded set — donors
  supersede scaffolding (CameraDirection 47→17).
- **Siting and costing**: a fence weights EVERY allocno the statement mentions —
  source AND dest (AddEnemy's three-round dest blindness); histogram both rivals
  and prefer the opposite if/else branch from the collateral (FUN_80057b80's
  swap came free only in the leaf branch); BLOCK-WRAP a densely interleaved
  region rather than one statement inside it (+19 every time on AddEnemy);
  deepening an existing fence is free, adding a new one is not; pull a value's
  defining statement inside the fence (the mis-bounding +1-nop tax); never fence
  adjacent to a cross-jumped shared tail (−282 bytes, ControlHumanoid); compute
  the depth (`--compare/--enclosed-refs`; floor_log2 is lumpy — aim at the
  step); judge every fence edit on the FULL cluster list, never the total
  (mission_score_screen's +7 = −27 local +34 distant). Fence the SEED, not the
  copy (leaves the copy free to schedule). An identical-arm fence's real job can
  be REFERENCE COUNTING, not the boundary — compare regalloc across the edit
  before crediting the CFG story (PadProc).
- **What fences cannot do**: weight a set that loop.c hoists (it escapes the
  notes); fix preheader ORDER (decided in loop.c); block cse forwarding; supply
  a boundary reorg's simple fill cannot cross when the flip direction is wrong
  (§3.13). The self-XOR (`x ^= x ^ y`) is an INDEPENDENT lever (stops copy
  coalescing) and can be required alongside a fence (ActSTICKON). A producer
  can sit inside a fenced assignment's COMMA RHS to pin a pure
  instruction-order triple with no runtime control flow
  (`do { table[1] = (offset = i * 4, 58); } while (0);` — InitEffect;
  side-effect-free assignments only).
- The one-shot macro wrapper of a private macro is a REAL note range —
  a plain compound block removes accidental weight across every expansion
  (StageEndScreen); loop depth propagates into expanded inline helper bodies
  (vmemoryGC — and re-visit broad call-site fences after a source-identity
  merge; the old weights become the next cycle's cause).

### 3.11 cse, blocks, labels

- **Both spellings of one field can be in the source** — cse hashes by address
  RTX; a through-pointer store cannot forward to an sp-relative load
  (FadeOutDirect: draft one insn SHORT with the store still emitted = cse
  forwarding, not dead-store elimination; a pointer-parameter sibling proves
  nothing about the local-aggregate case).
- **A struct store kills forwarding for every offset off that base** — when
  retail COPIES where you reload and a struct store intervenes, forwarding is
  impossible; use a chained assignment (`a = p->f = call();`) which yields the
  value with NO load (GetAreaMapVector; also recovered `subu` operand order).
- **cse1 vs cse2**: cse1 stops at labels and LOOP_END; cse2 crosses notes with
  a wider block and can fold BACK a copy cse1 kept — a real mechanism, but a
  cautionary one: GetAreaMapVector's 20-byte park was "explained" by it in
  detail and then matched by a dead-store POSITION move + fence removal that a
  fresh permuter run found in minutes (§4's worked example).
- **A local written but never subsequently read is a POSITION lever**: the
  write is dead in value yet gives cc1 a live write to the pseudo at that
  program point, and the site is a FREE VARIABLE — sweep other reachable
  points, including inside a later loop body, before assuming the textually
  natural (symmetric) site (GetAreaMapVector 20→0). Nuance: a dead CONSTANT
  assignment gets folded/propagated away and steers nothing; a dead register
  copy from a live variable survives.
- **Never infer pre-jump2 structure OR allocation from final asm** — cross-jump
  runs after both cse and allocation; impossible-looking co-located store/reload
  pairs were two arms; "register cycles" can be merged-tail artifacts. A store
  that must not be forwarded belongs inside BOTH arms (FUN_80057b80 494→8;
  ActSTICKON's switch-literal tails: plain switch on an UNSIGNED mode with
  inline literals, case order by fallthrough layout).
- **An inline early `return K` mid-function leaves a join CODE_LABEL that ends
  cse's block** — killing an `aN == param` equivalence and re-materialising a
  copy; spell `if (cond) goto retK;` to the single trailing return (the label
  folds away; verify with `rtldump --pass rtl,jump`). The filled delay slot is
  the symptom, not the cause.
- **`move rX,zero` where you emit a register copy proves a CODE_LABEL
  intervenes** (reload_cse_regs, compiler-facts) — an early-return guard clause
  lands the label between the two zeroes (GetNearestHumanoid, first try).
  **TOOL TICKET (guard-clause-invert autorule)**: rewrite a whole-body
  `if (c) {…}` wrapper into `if (!c) return <init>;` + unwrapped body.
- **Offset-0 dereferences**: cse1 canonicalises an offset-0 local-pointer deref
  back to base+const (but not nonzero offsets) (FUN_8004c59c); `find_best_addr`
  folds only where the equivalence is in its table — per-arm duplicated stores
  fold, the join-block store keeps the pointer (compiler-facts).
- **Global re-reads in one extended block fold** (including via narrow
  two-steps); two target loads of one global ⇒ the second sits behind a label or
  is compiler-made — find that structure, don't fight cse with volatile
  (BreedLife lesson 4). The d-global reload pattern across `Act*` follows
  MEM_IN_STRUCT_P (ActHANG vs ActSYURI).
- **A repeated `&local` arg can CSE across an intervening label** — evict via a
  threaded pointer variable reassigned per call + dead `pv = 0;` (SetCameraMode)
  — but note this contradicts the label-breaks-cse expectation in general: the
  LoadSI instance of this note was refuted when LoadSI later matched (its error
  arms duplicated the cleanup instead, §3.2).
- **A `goto`-label's POSITION decides which copy keeps the register**:
  `make_regs_eqv` promotes `flag = x`'s dest only if flag's range ends beyond
  the block AND later than x's — moving the labelled hit-handler block
  earlier/later flips which of two coalesced values owns the register
  (ProcItemSmoke). Same family as statement-order tail-duplication (§3.13);
  the knob is label position.
- **Repeat reads of an ESCAPED struct stop CSEing even across adjacent
  statements** — two stores of one field to different objects need a named
  temp for the shared load (FUN_80039fb0); a clamp's final expression must
  re-read the field fresh when the target shows a reload at an identical value
  (FUN_8003a148). End a loop temp's SCOPE before testing its copied working
  value to steer which equivalent register cse retains — a block-scope-end
  note, zero code (PutLifeBar's digit quotient). A comma-expression
  initializer gives independent loads earlier UIDs without extra code
  (PutLifeBar's `NumberImage.w = (dx = style.dx, dy = style.dy, 4);`); when a
  result must reuse its test register, make the result local the test carrier
  (`color = GameClock & 1; if (color) …` — PutLifeBar's $v0 tail).

### 3.12 Addressing shapes

- **Split vs unsplit is the DECLARATION** (compiler-facts): a two-register
  `lui %hi / addiu %lo` needs a non-small decl (`extern T SYM[];` — the
  `extern-array` rule sweeps it, gp-aware); an interleaved hi/lo pair is the
  tell (anything BETWEEN the halves proves the split, ReqItemUse); a
  same-register pair is an UNSPLIT `la` needing a COMPLETE type ≤ 8 bytes
  (`extern short D_8008E404[4];` — RestoreItemLayout's one-character fix). Two
  sites in one function can legitimately differ (declaration, not allocator).
  8-byte objects are not automatically small (whole-struct assignment wants the
  split — DoBriefingAndInventorySelection's RECT; PutMap's pointer cell as
  `[]`+`[0]` exposing the HIGH to reorg). When probing, grep `\bla\b` too.
- **A bare `lui` with NO `addiu`, reused as a base**, is a LOCAL holding a
  literal pointer cast (`(PersistentState *)0x80010000` — %lo is 0)
  (FUN_8001b2b8); a read-modify-reread absolute across a call wants a raw
  `#define` literal, which cc1 CSEs as `lui+ori` and reorg can steal the store
  (InitGraphicsSystem — named extern and pointer temp both fail).
- **`lui`+`addiu(base)` + scaled index + load at `0(base+index)` is a REAL named
  array** — resolve the address, declare the table with real element
  width/count (DamageControl's `D_80086B6C[deg]`).
- **The addu-order family** (pick per SITE from the target's operand order):
  ARRAY_REF is base-first; pointer index is index-first; `(&p->arr[0])[i]`
  works only through a POINTER (no effect on top-level externs — leSetEnemy);
  the wrapper-struct COMPONENT_REF (`((Wrap *)p)->a[i]`) is the base-first form
  that avoids frame rematerialisation (pointer-to-array casts are DEAD ON
  ARRIVAL — INDIRECT_REF, compiler-facts); a top-level extern needs the named
  byte-offset + integer-sum form (`offset = idx * (s32)sizeof(T); entry =
  (T *)(offset + (s32)arr);` — leSetEnemy 33 bytes; `ptr-index-sum`); multi-dim
  needs the innermost-first grouped integer form (`(z<<2) + ((x<<8) + (y<<5)) +
  (int)WorldMap` — LoadConstruction −21). **TOOL TICKET (arrayref-int-sum
  autorule)**: needs declared dimensions to derive strides. A pointer local vs
  an inlined constant macro flips commutative addu order at index-scaled sites
  only (mission_score_screen). `Array[count]` vs `ptr[count]` helps only where
  the base is freshly materialised (AddEnemy: −4 fresh vs +2 cached). A cached
  `rec = &map[j]` temp flips index-first back to base-first — keep repeated
  `map[j]` when the target is index-first (LoadAreaMap).
- **Offset-0 alias vs enclosing member is a `%hi`-register lever**: the alias
  folds `%hi` into the destination; the member splits the base
  (CheckCheatCodes; `tools/symnear.py` names the enclosing candidates —
  StageSequence's `CamState.mode`; rtlguide flags
  `enclosing-global-field-load`). Choose per use site, not per TU
  (StageEndScreen's per-iteration alias vs aggregate base). Several fixed-page
  globals can be fields of one aggregate — symnear intersection
  (CVAsetup's PSTATE; mission_score_screen's rank+chr).
- **Independent row and element scaling identifies a MULTI-DIMENSIONAL table**
  (`row << a` and `col << b` scaled separately, then added): declare the real
  second dimension (`s16 AIDHumanType[][2]`) — a flattened `t[r*C+c]` folds the
  indices first (think_setting_small_rotation_small_steps_). Build a dynamic
  row base BEFORE a large constant field displacement when retail folds the
  displacement onto a register (`row = (u8 *)state + chr * stride;
  row[field]` — FUN_80052ea8). Name a scaled byte offset in its own `s32`
  statement when the target completes the extension/scale before an
  independent `%hi` base materialisation (UpdateEvent).
- Adjacent tables are separate symbols, not one array with a folded offset
  (Think3firstattack's atkd2); serialized scratchpad access is small-extern
  SYMBOL access, not flat casts (SetCameraMode — MEM_IN_STRUCT_P
  consequences), but same-TU siblings can differ (FUN_80039544's flat `$at`
  casts) — check the raw `.s`; software-pipelined table chains go through OWN
  named pointers per table, last one direct (SetupThinkFunction); signed `slt`
  on pointers is an `(s32)` cast pair.

### 3.13 Scheduling & delay slots

Mechanics in compiler-facts (backward scheduler, priorities, LAUNCH_PRIORITY,
LUID, barriers). The levers:

- **Statement order IS the sched lever inside a block** — sched breaks
  same-priority ties by UID order, which follows source scan order: capture the
  must-come-first value as an explicit statement (SetCommand); two independent
  same-shape chains emit in REVERSE source order — write them opposite
  (DrawSnow 14→0: a draft in the target's machine order is the classic stuck
  state); statement order is INERT when the multiset already matches and only
  interleave differs (nullcheck's byte-identical LHS-first proof) — score both
  orders, it costs one build. In a LOAD-FREE block sched cannot reorder at all
  — a misplaced insn there is pure source order.
- **Address-taken locals' loads never schedule above ANY pointer store** —
  reorder the STATEMENTS (HangCheck's vy-last commit). An independent load CAN
  hoist above textually-earlier unrelated stores — that's the scheduler, don't
  chase it (ReqItemKusuri). A load's freedom is its ADDRESS KIND: FIXED
  symbol_ref floats (and gets pulled into load-delay holes, stealing fillers —
  order the field writes so the intended filler is assigned FIRST,
  FUN_8004c59c 23/29); CHAINED lo_sum is scheduler-chained; VARYING reg+K —
  **including sp-relative** — pins below address-taken stores, and when the
  target's load looks hoisted it's the STORE that sank (FUN_8003944c). A
  width-cast that clears `/s` turns a load into an anti-dep barrier for later
  `%gp_rel` stores — read a displaced store's LOG_LINKS before calling it a
  sched tie (HumanActionControl's typedef fix); equal-priority hazard swaps are
  SYMPTOMS — ask what made the priorities equal; one-point demotions dissolve
  the group (store-fed-by-load N+1 vs store-fed-by-ALU N).
- **LAUNCH_PRIORITY**: single-set producers are bumped at launch; a re-assigned
  local silently loses the bump and its chain emits last (CreateHumanoid —
  split `half`/`nhalf`, zero cost via coalescing; the same knob run the other
  way suppresses the bump, PadProc). SUBREG dests never bump (compound
  assignments to shorts). To move a single-set load earlier, give it a second
  set — but check `--order` first (live-range merge risk). Split-then-coalesce
  through a single-set alias wins when a multi-set destination's load must tie
  promoted producers (ProcItemNingyo's `loaded_model`). Cluster of genuinely
  equal loads: reverse the source assignments as one bounded probe
  (ProcItemNingyo 6→4).
- **Store-to-load source dependencies beat fences**: read back the just-stored
  narrow object (`sb; andi` order with no surviving load, FileOption) — the
  memory-unit tiebreak anti-rule means [store][alu] is unreachable for
  simultaneously-ready independent pairs; make them not simultaneously ready.
- **Delay slots** (reorg mechanics in compiler-facts): read the BLOCK LEADER,
  not the branch — `.flow` vs `.sched` first: if pre-sched order already
  matches, the lever is sched1 priority, not reorg (DrawBleed, where every
  reorg rule was correctly stated and irrelevant; byte-account fills — a nop
  and a lui at the same address shift nothing). For a returning guard, the slot
  is won by SOURCE POSITION: put the producer as the first statement AFTER the
  returning if (death_camera_something_; AddEnemy −6 — free when the variable
  is dead on the else path; inside the arm it loses on priority). A skip
  label's leader + LABEL_NUSES decide a guard's fill (StickonCheck's header:
  the four-guard control). `li` in a branch delay slot = the constant was
  defined in that test's own block — nested ifs with a goto to the shared else,
  not one `&&` (vrealloc's 0x80000000). An "unconditional" post-compare move is
  reorg stealing a fallthrough head whose register is dead on the taken path —
  NOT a comma expression (turn_towards_player_). A delay-slot insn can be
  pulled BACKWARD across calls (ReqItemHappou's `j++`). A dependency-free
  constant as the first statement floats above `sw ra` — write the
  load-bearing statement first, or wrap a no-load body in a fence
  (ReqItemStay; EVENT_OBJ). An empty `do{}while(0)` at the END of an if body
  flips reorg's prediction toward the TARGET thread (+1 COPY) — the flip has a
  DIRECTION; check which outcome you need (LoadTIMpack wanted it; StickonCheck
  measured 90→90 because EQ already predicted 0; a brief once handed this rule
  pointing the wrong way).
- **`tools/sched-deps.py <Name> [--pass sched2]` reads the scheduler's
  decisions instead of hand-deriving them**: equal-priority group membership
  (who ties, which producer cost, which one-point demotion dissolves the
  group), the flow-vs-sched block-leader report ("pre-sched order already
  matches: the lever is priority, not reorg"), and the argmove priority census
  (a FALSIFICATION printer — argmoves are not a priority floor;
  compiler-facts). Use it before any hand-read of `.sched`/`.sched2`.
- **Remat-split orphaned stores**: same UID becomes a `const_int` set, store
  reappears at a fresh UID → not a sched tie, not fence-fixable — the value
  must not be a spilled movable (mission_score_screen).
  **TOOL TICKET (sched-deps remat-split detector)**: deliberately unshipped so
  far — the tree has no case to pin a test against; build it with the next
  live instance.
- **Value computed into a temp before an intervening store** frees the return
  register for the earlier expression (AfsGetHeader's named `tmp` — the tell:
  the second expression's loads schedule before the intervening store while its
  own store stays after). A table-backed field store before a literal field
  store steers the surrounding call schedule (FUN_8004a6bc's bounded two-order
  test; `adjacent-field-store-swap` covers the literal/literal case). When only
  adjacent independent stores remain, score BOTH source orders and trust the
  linked diff — the lexical order that exposes the longer dependency chain
  first often wins, with sched2 emitting the machine order (SetupBG's u/v;
  leSetEnemy's z/r).
- **`.sched2` being exact does not make a delay-slot `nop` safe** — reorg's
  `fill_simple_delay_slots` can still raid the slot; if `.dbr` alone changes
  the sequence, a real dependency/eligibility change is required, not nearby
  permutation (SuccessionAttack's 6-byte checkpoint; its earlier fix — a
  single-set `raw` alias for a signed load — fixed both schedulers when the
  printed priorities suggested a plain tie).
- You cannot park a statement between a compare and its branch
  (compiler-facts); a guard comparison's operand order is a branch-ENCODING
  probe even when the register tie stays (DamageControl −6).
  **TOOL TICKET (cmp-swap guard extension)**: its site selection misses
  regalloc-owned guard hunks.

### 3.14 loop.c economy

The gate, decay, and printed log are in compiler-facts; `rtldump --loop-log`
prints every verdict — **price the gate before theorising**: three orders of
magnitude clear is not a lever (hand-roll the loop to deny the notes); a
two-insn margin is (SetBleedsDir vs SetBleeds — identical movable, opposite
verdicts, the loop's insn count was the whole residual; the fix was making the
loop BIGGER by inlining a precomputed invariant at its use sites, asm
unchanged). **TOOL TICKET (invariant-inline autorule)**: the lane says it would
have matched SetBleedsDir unattended.

- **Guard operand order decides whether a short param's widening hoists**
  (`if (i >= n)` keeps it in-loop with the no-`sra` compare; which operand's
  `sll` comes first in the target reads the source order off, SetSmoke).
- **A user-variable invariant hoists only if it precedes any conditional exit**
  (movable vs `maybe_never`), and the −3 decay is load-bearing: an early hoist
  keeps a LATER invariant in-loop — WHERE you assign `base` changes other
  expressions' fates (SetSmoke's `time<<16`).
- **Identical else-arm constants combine and their savings SUM** — keep one arm
  `-x` and spell the others `zN - x` with pre-loop zero locals to keep solo
  movables (SetBleeds; reload substitutes const-0 into `reg_or_0_operand`,
  zero trace). Read the three move-gate disjuncts before arguing (AddEnemy's
  `menu_base` relocation — a named variable hoists freely when every use is in
  the set's block; the fix for a missing hoist is often to DELETE the variable,
  except frame addresses which hoist ONLY through a named variable — never
  reason from a hoisted insn's final mnemonic, cse2 rewrites hoists).
- **Preheader order is a movable CHAIN**: hoists land after any source
  preheader insn, in group-head scan order; seed a `reg = const` per site to
  merge into a later-scanning group (mission_score_screen 330→254; a bare
  `{ s32 pad; }` scope block buys lifetime free). A giv init is the ONLY thing
  that can follow a hoist — if you hand-spelled one, that's the bug (AddEnemy's
  `names_offset`). Recurring `$t`-register constants = a hoisted-then-SPILLED
  movable group (local_alloc can't produce that class).
- **Unbiased cursor + surviving counter**: goto backedge + explicit preheader
  caches in target order (UpdateItemState 293→0); put a hand-rolled loop's
  invariant computation INSIDE the inner if (outer-body placement lets reorg
  duplicate `i = 0` into the backedge slot).
- A real `while` vs a byte-identical goto shape still differ in flow2 ref
  weighting (loop-body refs count double) — restore the real loop when a
  callee-saved role needs exactly +refs (DamageControl lesson 3; the OPPOSITE
  lever from hoist suppression — pick by what the target needs).

### 3.15 gp vs absolute globals

ASPSX gp-addresses only TU-defined symbols; our per-file `maspsxGpExterns`
encodes the original TU's smalls (toolchain.md has the full story;
`maspsxflags --write` syncs; `symcheck.py` after converting any stub — a
missing entry silently relocates whole regions, 388 symbols once). The
load-bearing fix is the matched function's OWN gp list; symbol pinning and
sibling-stub entries are defensive no-ops (established by independent
removal). Cross-TU aliasing reaches struct FIELD addresses
(`CURRENTLY_SELECTED_CHARACTER_STATE_PTR` = `CamState.Owner`) — check a mystery
symbol's arithmetic against proven layouts first. Split-address consequences
and declaration levers: §3.12 and compiler-facts. gp-output aggregates whose
fields splat auto-named as drifted `D_` symbols: bind ONE fresh correct-address
name and declare the real SVECTOR (GetConflictResult). Stock PsyQ objects may
differ (LIBMCRD's `-mno-split-addresses` exception — encode narrowly in
per-TU `ccExtraFlags`, mirror in permute.py, `Build check-all`).

### 3.16 Dividing by a variable

Variable `/`/`%` needs maspsx `--expand-div` (break 7; signed also break 6 —
compiler-facts). Tells before building: Ghidra `trap(0x1c00)/trap(0x1800)`;
`div` + `break 7`+`break 6` or `divu` + `break 7` in the target.
`maspsxflags --write` syncs Build.hs + permute.py (ProcItemNapalm combined
case; FUN_80036284 unsigned case). Constant division is magic-multiply, no
guards, no flag.

### 3.17 Split functions & carve integrity

- Jump tables: `reverse.py <Name>` then `split-scaffold.py <Name>` (full
  NON_MATCHING stub, all pieces + `_jtbl` arrays + `.rodata` carve; green).
  Delay the `.rodata` carve until the real switch activates; matchdiff's window
  stops at the first piece — iterate with asmdiff, gate with matchdiff's
  whole-image count; permute.py concatenates pieces by first-instruction
  address. reverse.py seeds only the first piece's INCLUDE_ASM — copy the full
  list from splat's generated source.
- **Not every multi-piece report is a jump table**: `__override__prt_` symbols
  are Ghidra call-site prototype overrides that splat treats as boundaries —
  reverse.py detects them and seeds all pieces; a single-`c`-segment carve with
  consecutive calls is ONE function, delete the stubs (Camera,
  SetupStageSequence).
- **An under-sized carve builds GREEN and is silently unmatchable** — the tail
  becomes a `.data` blob defining the labels. Run `tools/coverage.py`; objdump
  the word at `carve_start + size` (a `jr ra` delay slot or `addiu sp,sp,+N`
  teardown = the carve dropped the delay slot — LoadCard, FUN_800593a0,
  valloc, BreedLife 214→59 after the fix). Variants: an orphan anonymous
  `{ start, type: data }` yaml entry (delete it); a phantom function label
  exactly at the boundary — first insn a small POSITIVE `addiu sp,sp,+N`,
  preceding function's `.s` ends in bare `jr ra`; corroborate with zero
  callers + zero address-word grep hits, then move the boundary
  (FUN_8005fe34/FUN_8005fe38). Parked drafts carrying padding hacks to reach
  length: verify the carve FIRST. **TOOL TICKET (reverse.py --size re-carve)**:
  cannot re-carve an existing `c` subsegment; mechanical fix is deleting the
  orphan yaml line.
- **`config/symbols.main.exe.txt` has NO comment syntax.** Before adding
  `D_XXXX = 0x…;`, grep for the ADDRESS — it may exist under its real name
  (splat rejects duplicates; a sibling's rename can break a parked draft's
  auto-label — rename in the draft). Matching a function can DELETE the `D_`
  auto-label its own C needs (last asm reference gone → link failure, or WORSE:
  `undefined_symbols_auto` silently rebinds the name to a drifted address and
  plain assignments in the config are overridden by the later `-T` script —
  bind a FRESH non-colliding name and update EVERY consumer
  (CardVolumeIdPtr/CardPathFormat)). A TU-shared rodata string never referenced
  by carved asm needs the same fresh binding + `extern char[]`
  (ProcMiscSprite's error string, 8+ pages off as a fresh literal).
- `-fdollars-in-identifiers` is required for `reference/ghidra_types.h`.

### 3.18 Toolchain boundaries — park-on-sight classes

- **The PsyQ SDK block (0x80060000+) is a different compiler.** Trivial-frame
  epilogue tell: sp-restore BEFORE `jr ra` with a bare `nop` slot — our cc1
  never emits it (compiler-facts). triage.py/progress.py cut at 0x80060000; the
  LIBCD/LIBPAD/LIBMCRD forwarding wrappers are permanent 6-of-32-byte
  NON_MATCHING (drafts on `codex/sdk-wave2-epilogue-blocked`); libraries differ
  per game (LIBGS's dmyGs* matched). Read `jr ra`'s delay slot before spending
  anything on an SDK cluster. `EVENT_OBJ_80/90/BC` are shared epilogues, not
  functions.
- **The 3-insn sign-extension split (`sll 16 / sra k / sra 16-k`)** has no
  natural-C form (all respellings refold; verified exhaustively) — exactly
  three exist (GetPad, FUN_8001b174, GetPadXY), all parked; triage.py names the
  class. Do NOT confuse with the 2-insn `sll 16 / sra K` (a short index
  sign-extended and scaled — perfectly matchable). PClseek's `break 0x107`
  BIOS trap likewise stays parked (no C representation; gte-policy excludes
  it).
- **GTE**: command opcodes assemble via splat's macros; matching is sanctioned
  ONLY through `src/main.exe/gte.h` for functions in
  `config/gte-allowlist.txt` (docs/gte-policy.md; tests pin containment).
  Key facts: `register long x __asm__("$12");` pinned locals for non-ABI
  entries; file-scope global register variables reserve callee-saved registers
  with NO prologue save; a COMPILED GTE caller needs `nop;nop` before every
  command (`gte_<cmd>()`), a HANDWRITTEN one fills the latency with real work
  (`gte_<cmd>_raw()` — the wrong choice added 6 insns that fuzz-score missed
  and only linked length caught); the permuter CANNOT parse gte.h functions at
  all (it refuses and points at rtlguide — escalate straight to dumps); m2c
  needs `--input-regs` with the full caller-saved list. The
  `addiu r,$zero,0` tell and the drawF3 conventions: compiler-facts and
  docs/gte-policy.md.

### 3.19 Matching `main`

`main` gets the implicit `__main()` call (writing it = 2 bytes over), and its
outsized frame with no matching accesses is a dead aggregate local — size
`u8 dead[0xNN];` to the frame; the demo's own locals were stripped in retail.

## §4 Park discipline

A park is a hypothesis, and the record says parks are wrong often enough to
re-check cheaply before honoring them:

- **Re-verify a park's verdict against the CURRENT rules before trusting it**
  (one grep/matchdiff run): FUN_8004a368's "la-reload tie" park was really the
  offset-0-alias lever and closed in minutes; DrawSnow's park contained its own
  mechanism read as a wall; parks have been dissolved by the ternary join
  (FUN_8004d6d4), the label position (DrawClip), the carve (BreedLife), and a
  stale claim surviving a function's own match (LoadSI).
- **Write negatives falsifiably**: "X failed WITH Z fixed at W" — a negative
  measured while another free variable was held fixed is a statement about the
  PAIR, never about the lever. The worked example: GetAreaMapVector's park
  recorded "dropping the fence: 548 but 45 — the fence is the only reason the
  copy survives"; measured with a dead store pinned at its original site. Move
  the store and dropping the fence is free — it MATCHED (2026-07-17).
  DrawClip's under-powered A/B is the same class; §0's joint-scoring rule is
  the lesson in reverse.
- **A stale permuter negative is not evidence.** GetAreaMapVector's header
  claimed a ~26k-candidate flat run; a FRESH run on the IDENTICAL source found
  the 8-byte win immediately and the fence removal on iteration 1 — tooling
  and RNG both move between rounds (permute now full-link rescores). Re-run
  one bounded permuter pass before any RTL archaeology on a same-length
  residual; the ladder's autorules → permuter → RTL order stands even when a
  checkpoint's confident prose primes you to distrust re-running.
- **A permuter WIN can move a shared statement into ONE arm — that is a silent
  BEHAVIOUR CHANGE, not a valid duplication.** ActITEM's candidate relocated
  `motMODE = 1;` into only the `else` arm, skipping it on the true path. The
  honest form that also matched retail was DUPLICATING it into BOTH arms —
  `if(c){A;shared;} else {B;shared;}` is semantically identical to
  `if(c)A; else B; shared;` and can compile differently, because the target
  computes the shared value independently per arm (once via the if-arm's own
  delay slot). **When a shared post-if/else statement does not match, try
  duplicating into both arms before relocating into one** — and reject any
  candidate that only moves it. (Clean autorules candidate: for a statement
  immediately following a two-armed if/else with no intervening use, try
  shared-into-both-arms and keep if the diff shrinks.)
- **BISECT a permuter winner — it carries dead weight.** CameraDirection's
  candidate wrapped nearly the whole function body in a fence; hand-bisecting it
  down showed only the vDif tail needed wrapping, for the same 7 bytes. And the
  narrower cuts were not merely worse but WRONG-LENGTH (864/868 vs 860), which is
  how you find the load-bearing span rather than guessing it.
- **A seed's lever may be an EXISTING dead local, not a fresh temp.**
  CameraDirection went 17→12 by naming an extern read through an
  already-declared, by-then-dead local (`y = ViewInfo.vrx;` then using `y`) —
  giving the subtraction the right birth point. autorules' `binop-operand-seed`
  tried that exact site and scored nothing, because it synthesises a FRESH temp.
  When a seed rule reports no win, try each already-declared local of matching
  type that is dead at that point before concluding the lever is absent.
- **A permuter WIN can be a STRUCTURAL regression — read the BRANCH DISPLACEMENTS
  in the new diff, not just the byte count.** AddEnemy's fresh run beat its base
  26 vs 27 (killing a "PERMUTER-IMMUNE" claim), and the win was a trap: the whole
  delta was hoisting `weapon_base = WeaponModel` INTO a loop, buying 2 bytes on a
  preheader rotation and paying 1 back as `bne v0,s6,0x8005b5e8` → `0x8005b5e0`.
  **That displacement IS the loop head** — retail materialises the address once
  ABOVE the loop; the 26-state re-executes it every iteration. Rejected. **The
  target's own branch targets are evidence for where a statement belongs**, and a
  score cannot see them.
- **A permuter WIN can be a semantic BUG — disassemble the constants whenever a
  candidate introduces a new local.** A better proxy score is not a match, and
  the failure is not always the known reachability trap (a moved/dropped `goto`
  after an unconditional jump); this one corrupts a VALUE. ControlTraceLine's
  best candidate scored 9/9/504 against a base of 10 by rewriting
  `if (sentinel == -1)` as `if (sentinel == (new_var = -1))` through a fresh
  **`unsigned char`** — silently truncating −1 to 255 (`li v0,255`,
  `0x240200ff`, confirmed by disassembling the build). It compiles cleanly,
  scores better, and would have made a wraparound-sentinel check never fire.
  The permuter mutates C, not semantics — an assignment-expression inside a
  comparison through a narrower fresh type is a whole class it can reach.
  Re-running matchdiff on the score is NOT enough: read the emitted immediate.
  A subtler instance: a candidate can score by reading a fresh pseudo with NO
  reaching definition on any path — real UB that only "works" because gcc-2.8.1
  records no conflict for an undefined pseudo (DrawBleed's rejected 8→4). If a win
  depends on a variable that is never assigned before use, it is not the original.
- **RE-SEED the permuter after ADOPTING a partial win — "one bounded run" means
  once per CHECKPOINT, not once per function.** The search starts from whatever
  source is on disk, so a checkpoint that shrank the residual opens a DIFFERENT
  neighbourhood; a run that plateaued against the OLD baseline says nothing about
  the new one. DrawBleed ran three bounded rounds in sequence, each seeded from
  the previous checkpoint — 47→12 (a second pointer identity beating sched1's
  priority sink), 12→8 (a temp-before-intervening-read capture) — both verified by
  porting the semantic delta, never by score. (This does NOT license
  background-and-wait; each run is still one foreground bounded call.)
- **A permuter candidate that REASSIGNS a value used as a divisor/operand across
  several later insns is disqualified — regardless of score — if the target writes
  that hard register ONCE.** One grep settles it: `grep -c '\$aN' <Name>.s` on the
  target. If the divisor register is written once and read N times with no other
  write, any candidate that reassigns the source variable before the Nth read
  cannot reproduce the target's dependency chain, and its lower whole-image score
  is register-pressure fallout elsewhere, not progress. FUN_80036284 disqualified
  5 of 7 retained candidates this way in one grep (they all reused `duration` as a
  scratch for case 2's quotient; the target's `$a2` is written once at 0x800362f0
  and read 7×). This is the concrete form of "verify a win, never adopt a score"
  for a divide-heavy residual.
- **"An alias copy survives only if it CONFLICTS with its source" is a diagnosis,
  NOT a recipe.** It reads like "make the source dead after the copy and the copy
  coalesces away for free" — but the conflict exists BECAUSE both values are
  genuinely live at once, and routing every later use through the alias so the
  original dies does not remove that need, it relabels which pseudo carries it.
  The pressure resurfaces elsewhere, often as a LENGTH MISMATCH (DrawBleed:
  routing `dx`/`r`/`g`/`b` through the alias cost +4 bytes of length instead of
  coalescing). Build and measure the "eliminate the conflict" edit like any other.
- **A length-neutral residual refuses control-flow fixes by arithmetic**: pure
  register tie + pure reorder with no missing/extra insns means a new branch
  (≥8 bytes on MIPS) converts a small tie into a strictly worse length
  mismatch — the byte budget kills such "untried directions" without a build
  (GetAreaMapVector's own).
- **A "failed" attempt can be the original source**: SetBleedsDir's 13-byte
  state was a local optimum AWAY from the original; the sibling's C — recorded
  as a failed attempt worth 19 bytes — was the answer. Diff parked drafts
  against matched siblings' C before extending them.
- **Length-neutral packages need a payer**: a proven finding costing +1
  rejected on length means go find what pays for it (FUN_80057b80's four-site
  package; mission_score_screen's ±4 complementary pair — "both are the same
  parked pair" is an instruction to try them TOGETHER).
- **Diff the region a "missing piece" lives in against the CURRENT draft before
  hunting it** — the defect may be collateral from the edit that "needs" it
  (mission_score_screen round 7).
- **Park-on-sight classes** (each with its printed/verifiable tell): SDK
  epilogue shape; the 3-insn SIGNEXT split; `addiu r,$zero,0`; the preheader
  priority-1 LUID tie between two hoisted constant loads (StageEndScreen,
  ~140 bytes); INSEPARABLE pairs with no post-copy read.
- **A false PARK rule is the most expensive error shape we have** — a wrong fix
  fails loudly next round; a wrong park tells lanes to abandon functions and
  nobody re-checks. The monument: the "hard-register argmove floor — park on
  sight" rule, REFUTED 2026-07-17 by measurement on its own cited function
  (23 of 30 argmoves above priority 1; `sched-deps` prints the census). It had
  "passed" by reproducing ONE function's bytes — a model that reproduces one
  function is not thereby a general mechanism. Before adding any park-on-sight
  rule, demand a falsification a TOOL prints, not a derivation.
- **Park format**: STATUS line with byte count, the residual's cluster
  addresses, the mechanism WITH its falsification, what was measured (numbers,
  not adjectives), and the named levers already burned. The next reader should
  be able to refute you in one command.
