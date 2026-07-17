# RETRO: the cookbook audit

Audited: `docs/matching-cookbook.md` at commit 6cf8d64 — **7,959 lines / 79,629 words /
132 `###` sections + 18 `##` chapter bodies (2,928 lines of un-sectioned bullets)** —
plus `docs/orchestration.md` (1,191 lines), `tools/autorules.py` (28 default + 47
aggressive = 75 mechanised rules), and the seven tools named in the charter.

Line numbers below are against that commit. Bucket totals are measured, not estimated
(script: heading-to-heading extents).

---

## 1. Verdict

**The cookbook is mostly noise for its reader, and the count says so.** Of 132
sections: **43 DELETE (1,943 lines), 22 MECHANISE (526 lines), 37 DEMOTE (1,086
lines), 30 KEEP (1,459 lines, compressible to ~500)**. Of the 2,928 chapter-body
lines, roughly 55–60% should also go (mechanised, duplicated, or tool-doc). Net: a
lane that must match one function needs **~600–800 lines of this 7,959-line file**,
and nothing routes it to the right 600. Two single sections are 826 and 634 lines of
unsorted bullets (§4515, §7326) — they are not sections, they are landfills.

Three specific rots, each measured:

1. **Mechanised rules that kept their prose.** At least **26 sections name their own
   autorules/rtlguide mechanisation in their own text** ("Now mechanized",
   "`autorules` sweeps this", "guided `X` enumerates…") and then keep 10–60 lines of
   prose an agent is still invited to think about. The mechanisation should have been
   the deletion. autorules already holds 75 rules; the cookbook re-explains ~30 of
   them.
2. **The same rule written 3–8 times.** Body-layout/polarity appears at §554, §733,
   §820, §873, §880, §895, §911; the fence topic at §4227, §6598, §6611, §6903,
   §6937, §7268, §7284, §7297, §7316 plus four bullet clusters; variable identity at
   §2777, §3021, §3192, §3268, §3754, plus the mega-pseudo bullets at 7519 and 7736.
   §7284 is §6937's "Mis-siting" bullet nearly verbatim.
3. **Wrong or superseded text still standing**, some marked, some not: §3256
   (superseded by §3245's own words), §3475 (superseded twice), §6532 (kept
   deliberately as a monument to its own four-version error history), §3648 (four
   versions, the charter's exhibit A), the §6135/§6144 corrections **spliced
   mid-sentence into the rule they correct** so the original sentence no longer
   parses.

Deleting a TRUE rule that no longer earns its space is the point: most of the DELETE
bucket is true. It is also *paid for* — mechanised, superseded, or the fourth copy.

---

## 2. Classification — every `###` section

Buckets: **DELETE** = remove; at most one generated index line survives. **MECHANISE**
= the section is a decision procedure over an artifact; build the named tool/rule,
then delete the prose. **KEEP** = judgment content; keep *compressed* into its
consolidated family. **DEMOTE** = true but low-yield; one to three lines in the
"shape zoo" appendix with an example pointer.

### 2.1 DELETE — 43 sections, 1,943 lines

| Line | Section | Why |
|---|---|---|
| 800 | Preserve raw value + adjusted index as separate roles | Instance of the §2777/§3021 identity canon; keep only the FUN_800570b8 example pointer there. |
| 820 | "short do-nothing case falls through" 3-way | Duplicate of the Dispatch bullet at 554–566 (same rule, same functions). |
| 829 | Adjacent equality return/goto guard, two spellings | Mechanised: `terminal-guard-flip` — the section says so itself. |
| 873 | Dual-role dispatch test opposite of its goto-ladder | Third copy of the 554/820 rule. |
| 895 | Guard clause w/o second return is a plain `if` | Instance of §880 + the De Morgan bullet at 733. |
| 911 | `&&`-chain body is always the fallthrough | Fourth restatement of §880's layout fact; fix is `if-else-invert` (mechanised). |
| 926 | Two-guard-then-fall-through shared return tail | Subsumed by the Shared-tails canon + §933. |
| 2612 | The tool (rtldump usage) | Tool doc; `rtldump --help` already carries it nearly verbatim. |
| 2903 | `siblingdiff --demo` is a schedule-shape oracle | Tool doc; orchestration.md already says it twice. |
| 2912 | Operand-BIRTH order decides local_alloc colours | Mechanised: `binop-operand-seed` (in-section). |
| 2939 | Same-length one-block residual → seed the loser | Mechanised: `copy-seed`; the LOCAL-vs-GLOBAL router line moves to the escalation ladder. |
| 2989 | reghist's zero-sum verdict does not prove a rename | The tool now prints this hedge itself; a tool caveat belongs in the tool, once. |
| 3138 | Direct compound arms, jump2 shares the store | Mechanised: `shared-writeback-compound`. |
| 3233 | Two-sided clamp uses assignments | Mechanised: `clamp-shared-return`. |
| 3245 | Min/max vs MEMORY must be the ternary | Mechanised: `min-ternary`. |
| 3256 | Min/clamp reload is a coloring tie — park | Superseded by §3245 ("This SUPERSEDES the older note below"). Dead text standing. |
| 3352 | Shared `f(0,x)` vs two explicit calls | Instance of the duplicated-calls family (Shared-tails canon; `shared-terminal-tail` covers the adjacent shape). |
| 3360 | Abs reaches abssi2 only as the GE ternary | Mechanised: `abs-ge` + `builtin-abs`; keep the `-fno-builtin` fact in toolchain notes. |
| 3446 | Read-only decision tree, post-join capture | Mechanised: `deferred-global-capture` (in-section). |
| 3475 | Abs `negu` source is not a C-level choice | Superseded by §3360 and by the 7636 bullet ("all three spellings identical — do not propagate that framing"). |
| 3565 | Split a computed dereference | Mechanised: `deref-address-split`. |
| 3619 | REG_EQUIV mechanics + the two traps | Mechanised: `regalloc.py --notes` prints owner + liveness; its help text states both traps. |
| 3741 | Callee-saved SWAP = allocno_compare, add ballast | Measurement mechanised: `regalloc --compare`; ballast lives in the fence canon. |
| 3788 | `A + (B + C)` constant placement | Mechanised: `plus-group` ("Now mechanized:"). |
| 3821 | Narrowed three-term sum, named prefix | Mechanised: `add-prefix-temp`. |
| 3867 | Shared-return `sra` above restores = 2nd return | Mechanised: `shared-return-split` + rtlguide `shared-return-extension-schedule`. |
| 3957 | Sink pre-guard flag onto both edges | Mechanised: `guard-flag-assign`. |
| 3995 | `x \|= C` vs `store = x \| C` | Mechanised: `or-inplace`. |
| 4015 | Boolean after each arm's comparisons | Mechanised: `flag-arm-assign` (both directions documented in-rule). |
| 4165 | Adjacent literal field stores | Mechanised: `adjacent-field-store-swap`. |
| 4227 | `optimize_reg_copy_1` + do{}while(0) walls (239 ln) | Mechanism belongs in the fence canon §6937(3); its dozen variants are `loop-fence`/`loop-range`/`empty-loop-boundary`/`paired-`/`nested-loop-fence`. Explode; zero survives as a section. |
| 4515 | reorg deletes a same-address reload… (826 ln) | The heading fact is 6 lines (→ delay-slot canon). The other ~800 lines are an unsorted landfill: `flag-return-split` (mechanised, 4581), regalloc.py's own doc (4995–5027), a dozen fence bullets, permuter operating notes. Explode into index/appendix/park headers. |
| 5599 | False multi-piece split is one function | Mechanised in `reverse.py` (`__override__prt_` detection); one line survives in Split-functions. |
| 5809 | Permuter aborts on rand() (KeyError) | Tool bug note; permute.py must print the reason itself (it already does for gte.h — same treatment). |
| 6486 | Prologue/parm-copy question lives ONLY in sched2 | Mechanised: `schedtrace --pass sched2` now exists; one routing line in the ladder. |
| 6532 | `priority()` is depth-from-top — and cc1 PRINTS it | Mechanised: schedtrace. The four-paragraph error autopsy moves to a changelog; a rule is not a memorial. |
| 6589 | Extern scalar may want to be an ARRAY | Mechanised: `extern-array` ("fully automatic", PadProc). |
| 6611 | Identical-arm fence's real effect = ref counting | One line into §6937; the check is `regalloc --compare` across the edit. |
| 6801 | Dump a CONTROL variant (damaged heading) | One line in the escalation procedure (§2677 step 4); `rtldump --src` is the tool. |
| 7268 | Fence the SEED, not the copy | Fence canon detail → §6937. |
| 7284 | A fence weights EVERY allocno mentioned | Near-verbatim duplicate of §6937's "Mis-siting" bullet. |
| 7297 | BLOCK-WRAP the whole block | Fence canon detail → §6937. |
| 7316 | Two measured facts about the weighting fence | Two lines → §6937. |

### 2.2 MECHANISE — 22 sections, 526 lines (specs in §5)

| Line | Section | Tool / rule that replaces it |
|---|---|---|
| 904 | Search loop's entry-duplicated test is the only guard | autorules `redundant-entry-guard` (drop/add the outer `if`, both directions, byte-scored). |
| 2764 | Target repeats a block byte-identically, one of yours matches | **T8 `selfsim`** — target self-similarity + per-instance residual overlap. |
| 2826 | `*(u16 *)&p->field` cast pins every later gp store | **T4 `loadclass`** (detects the anti-dep) + autorules `struct-view` (the fix; backlog already fully specifies it). |
| 2848 | Equal-priority `potential_hazard` swap is a SYMPTOM | **T1 sched-deps**: equal-priority-group analyzer (who ties, which producer cost, which single-point demotion dissolves the group). |
| 2866 | Two independent chains emit in REVERSE source order | autorules `chain-order-swap` — the section itself says the test "costs ONE BUILD". |
| 2974 | Alias survives only if it CONFLICTS — early-stop oracle | **T2 regalloc verdicts** — regalloc already prints INSEPARABLE; finish the verdict AND its blind-spot line: SetupTelop (d487683) proved passing this gate decides nothing. |
| 3205 | Negative non-pow2 multiply spelled as strength reduction | autorules `neg-mul-strength` (enumerate cc1's own shift/sub spellings for `x * -K`). |
| 3482 | `arr[idx]` on top-level extern expands base-first | autorules `arrayref-int-sum` incl. the multi-dim form — the section marks itself "candidate for a future autorules rule; backlog". |
| 3648 | The self-tie discriminator (four versions) | **T2 `regalloc --spill-uses`** (BARE vs IN-MEM per use — the backlog says this one distinction retired a "do not retry" verdict) + the one-line "reload-count diff upstream = one root cause". The four-version saga → AdtSelect.c's header. |
| 4151 | Statement order steers goto-merge tail-duplication | autorules `merge-def-swap` (swap two adjacent independent defs before a goto-merge). |
| 6363 | Two constraints, one lever; slot order is arithmetic | **T9 `slotcalc`** — assign_stack_local is deterministic; compute the unique declaration order, never search it. |
| 6399 | Reload's remat SPLIT orphans the store | **T1 sched-deps**: detect a UID rewritten in place to a remat + the store at a fresh UID; print the verdict ("not a sched tie, not fence-fixable"). |
| 6417 | A guard's delay slot is decided by its SKIP LABEL | **T5 `dslot`** — the eligibility + LABEL_NUSES table a lane hand-derived and called "the whole answer". |
| 6450 | `move rX,zero` proves a CODE_LABEL intervenes | autorules `guard-clause-invert` — the section says "autorules candidate, fully specified"; plus the `--trace` delta check is already in rtldump. |
| 6559 | Hard-reg argument move is a PERMANENT floor | **T1 sched-deps** park-verdict: argmove + priority-1 + LUID proof printed, "unreachable — park", falsifiable against our own bytes. |
| 6598 | Judge a fence on the FULL cluster list | **T3 `matchdiff --clusters --baseline`** — per-cluster delta across an edit; +7 totals must never again hide −27/+34. |
| 6660 | A load's freedom is its ADDRESS KIND, not `/s` | **T4 `loadclass`** — this section IS the classifier spec (FIXED symbol_ref / CHAINED lo_sum / VARYING reg+K incl. sp). |
| 6745 | code_label blocks cse1's canonicalisation, not cse2's | **T7 `passtrace`** — the .cse-vs-.cse2 first-rewrite sweep two lanes hand-rolled; prose → GetAreaMapVector's park header. |
| 6854 | Inline early return leaves a join CODE_LABEL | autorules `return-to-goto` (`if (c) return K;` ↔ `goto retK`), plus the `rtldump --pass rtl,jump` label check as a T1 verdict. |
| 6872 | Empty delay slot is a SYMPTOM — read the block LEADER | **T1/T5**: `.flow`-vs-`.sched` order comparison + leader eligibility, printed ("pre-sched order already matches target: the lever is sched1 priority, not reorg"). |
| 7167 | Frame arithmetic identifies compiler spill slots | **T9 `slotcalc`**: locals + saves vs target frame → "N compiler spill slots implied"; residue not ≡ 0 (mod 4) fails loudly. |
| 7185 | Loop-invariant symbol_ref remat = hoist-THRESHOLD artifact | Diagnosis already mechanised mid-audit (`rtldump --loop-log`); fix becomes autorules `invariant-inline` — the lane says it would have matched SetBleedsDir unattended. Prose merges into the loop.c-economy canon. |

### 2.3 KEEP — 30 sections, 1,459 lines → target ~500 after consolidation

| Line | Section | Why it stays (and where) |
|---|---|---|
| 880 | `if/else` makes A the fall-through, negates cond | THE body-layout fact; canon home for 820/873/895/911 and the 733 De Morgan bullet. |
| 933 | Guard `return 0;`s cross-jump into the LAST island | Return-island canon (which island survives = source evidence); rtlguide names the signature. |
| 2661 | What each pass tells you (table) | The RTL orientation table — 15 lines, high value. |
| 2677 | The procedure | The escalation loop itself. Absorb 6801's control-variant line. |
| 2777 | TWO REGISTERS FOR ONE VALUE = TWO VARIABLES | The cheapest question on the board; identity canon anchor. |
| 2795 | Splitting a shared pseudo is a PRIORITY lever | Superlinearity + the both-outcomes local_alloc caveat; allocation canon. |
| 2889 | RTL statement order is INERT under sched2 | Paired with `chain-order-swap`: multiset matches ⇒ order is not the lever. |
| 3021 | One C variable = one hard register, never split | The identity canon's mechanism half (merge with 2777). |
| 3192 | A promoted `short` parameter is TWO pseudos free | The named exception to the identity canon. |
| 3268 | Reuse a DYING variable (coalesce upward) | Identity canon: target frame larger ⇒ merge, not split. |
| 3607 | Offset-0 LOCAL-pointer deref cse1-canonicalised | Merge with 6775 into one cse-address note. |
| 3694 | `%hi` reload tie = combine_regs refusing block-crossers | Recognition + fix (duplicate call per arm); matched 4 functions; rtlguide-signature candidate. |
| 3754 | Two disjoint same-role temps may be ONE variable | Identity canon (split ↔ merge, with the .lreg check). |
| 3844 | Donate a call-arg preference | Allocation canon (find_reg avoidance; `--prefer` exists). |
| 4003 | `li` in a delay slot ⇒ constant defined in that test's block | Asm→source reading; delay-slot canon. |
| 4141 | A constant used once across a call is rematerialised | Allocation canon ("second use tips the cost model"). |
| 4216 | goto-label placement decides which copy keeps the register | Label-position family (with 5422 + 6812). |
| 4466 | Inline asm is an anti-rule | Policy; enforcement is mechanised (matchdiff blocks `__asm__`); keep 5 lines. |
| 5906 | Unbiased cursor + surviving counter via goto + explicit hoists | loop.c-economy canon worked shape. |
| 6630 | `addu` swap is a DECLARED-TYPE question; the obvious fix is DOA | Merge with §3482 into ONE addressing-shape section; names the dead end (INDIRECT_REF gate) — the rule style all rules should have. |
| 6703 | Through-pointer vs direct spelling not interchangeable (cse hash) | cse-aliasing canon + its one-instruction-short diagnostic. |
| 6728 | A struct store kills forwarding for EVERY offset | cse-aliasing canon + the chained-assignment escape. |
| 6775 | cse1 `find_best_addr` folds only inside its table | Merge with 3607. |
| 6788 | The gcc source is ON DISK — do not work from recall | Method preamble, 5 lines. |
| 6812 | N direct rejects = POST-REORG view of N `goto reject;` | Label-position canon: post-reorg asm inverts cause/effect + WHERE the label lives (the DrawClip half-rule the charter cites). |
| 6937 | The two fence idioms: what each ACTUALLY does | THE fence section — the sole survivor; absorbs 4227/6598/6611/6903/7268/7284/7297/7316 and the four bullet clusters. |
| 7228 | loop.c movable budget: preheader order is a chain | loop.c-economy canon (merge under §5860 with 7185's residue). |
| 7260 | Recurring t-register constants = hoisted-and-spilled group | loop.c-economy recognition, one paragraph. |
| 7309 | No REG_ALLOC_ORDER: first-allocated wins the LOWER register | Allocation canon, 4 lines. |
| 7326 | Reading global.c's preference machinery (four rules) | The four rules (~80 lines) are the allocation canon core — **including the loop-depth-weighted `n_refs` fact currently buried in a 7452 bullet**. The 550-line bullet tail is a landfill: explode into index / park headers / compiler-facts. |

### 2.4 DEMOTE — 37 sections, 1,086 lines → the "shape zoo" appendix, 1–3 lines each

430 coddog (→ tool doc, orchestration owns targeting) · 844 per-axis raw temp
(inverse is mechanised `difference-role-fuse`) · 920 guard-constant label past the
tail · 956 guard returning its own test's constant, bare goto · 967 for-loop folded
entry guard · 977 success-return jr blocker · 2736 default-then-override temp ("not
root-caused, shape to try") · 3014 demote the WINNER · 3049 hard-conflict ⇒ change
the graph (check itself is mechanised: rtlguide prints HARD-CONFLICT; keep the two
source shapes) · 3212 narrowing store via widened temp · 3220 `(short)` cast as
scheduler lever · 3282 offset-0 alias vs member `%hi` (detection mechanised:
symnear + rtlguide `enclosing-global-field-load`) · 3413 conditional store via
pre-branch address copy · 3424 pre-branch local for two-arm stores (pair with 3446's
mechanised inverse) · 3533 read back a stored narrow parameter · 3553 adjacent banks
need distinct symbols · 3587 assignment-expression publication · 3728 absorb abs
cascade with scratch reuse (semantically risky; keep the caveat) · 3905 cse2 ignores
loop notes, evict via pointer reassign · 3917 fallthrough producer after returning
guard · 3947 delay-slot move is NOT a comma-expression (anti-rule, 2 lines) · 4080
path-result/flag split (rtlguide names `path-result-copy-join`) · 4110 zero-init
"master" · 4122 the parameter that walks · 4132 struct local cached at post-branch
join · 4497 search-flag initialiser placement (fold into loop.c economy) · 4506
block copy through `*p` · 5817 jitter scratch as one array · 5842
hoist-vs-widen 2-insn tie (park note) · 6391 byte-account the census (method
appendix) · 6510 promote_mode is the IDENTITY (compiler-facts; the proof →
FUN_80057b80's header) · 6574 returning guard's slot won by SOURCE POSITION
(delay-slot canon; heading is damaged) · 6622 identical tails survive via base
registers (shared-tails canon) · 6766 `s16 local = p->field` does not pin the load
(compiler-facts) · 6896 constant-folded cast is not the scratchpad idiom · 6903
empty fence flips reorg prediction — WITH ITS DIRECTION (fence canon; the direction
trap is charter evidence) · 7152 hand-written range-split is load-bearing
(scaffolding-audit appendix, with 7167's mechanised check).

### 2.5 The 18 chapter bodies (2,928 lines)

| Chapter (lines) | Verdict |
|---|---|
| Preamble + "Start from the original source's own facts" (1–113) | KEEP, trim ~20%. Evidence-first is the right opening. |
| The workflow (113–129) | KEEP. |
| Iteration protocol (129–372, 243 ln) | KEEP the ladder (~80 ln). DELETE the eight named residual sub-cases into the rtlguide signature index they already have (`la`-tie, guard-slot tie, adjacent-loads, `commutative-plus-destination`, narrow round-trip…); move permuter/autorules operating detail to tool help. Fix the duplicated "4." numbering (224 and 258). |
| Picking targets (372–430) + coddog (430–517) | DELETE from cookbook — orchestration.md owns targeting; coddog is tool doc. |
| Partial matches / NON_MATCHING (517–541) | KEEP. |
| Dispatch (541–800 bullets) | Consolidate to ~½: polarity/layout bullets fold into §880 canon; DELETE mechanised bullets (`sparse-eq-switch` 629, `default-ladder-hoist` 635, `switch-cse-evict` 682, `literal-indirect-inline` 723, `terminal-guard-flip` cross-ref). Case-body-order and jump-table-index rules KEEP. |
| Loops (965–1317, 352 ln) | KEEP ~60%. The loop-FORM selection rules (for vs while(1)+break vs goto-label vs do-while; fallback placement decides the shape) are the highest-value judgment content in the book. DELETE `subscript-postinc` (1073, mechanised); compress worked cases to pointers. |
| Expressions (1317–2183, 866 ln) | The worst offender: ~90 bold mini-rules, ≥15 already mechanised (`type-width` 1437, `param-width` 1754, `rand-mod` 1622, `mod-bias-temp` 1348, `call-arg-pair` 1631, `shift-stage` 1319, `shift16-mul` 1820, `member-scalar-alias` 2010, `pointee-volatile` 2096, `cmp-swap` 2139, `cmp-polarity` 2145, `ptr-index-sum` 1931, `ptr-base-split` 1949, `call-result-split`, `difference-role-fuse`). Cut to ~300 ln of families: narrow-width dataflow, call/prototype evidence (keep the m2c/Ghidra arg-count reconciliation — pure judgment), volatile discipline, store/load batching. |
| Stack objects (2183–2590, 407 ln) | KEEP ~50%: stackplan-first, unions/overlays, block-move family, inlined-helper mechanics. stackplan already mechanises the recognition half. |
| Reading cc1's RTL dumps (2590–2734) | KEEP — the method chapter (see §4). |
| Register allocation steering intro (2734) | Merges into the allocation canon. |
| Shared tails (5341–5599, 258 ln) | Consolidate to ~100: cross-jump mechanics, label pinning, duplicated-calls family. DELETE mechanised paragraphs (`case-fence` 5355, `mul-affine-shape` 5362, `terminal-arm-flip` 5411, `shared-tail-assign`/`shared-terminal-tail` 5509). |
| Split functions (5609) | KEEP short (tooling pointer). |
| gp vs absolute (5649–5809) | KEEP ~60% (recognition: bare-lui, interleave tell, 8-byte counterexample); setup is mechanised (`maspsxflags`); `symcheck` covers the invariant. |
| loop.c hoisting economy (5860–5933) | KEEP — canon home; absorb 7185/7228/7260/5906/4497. |
| --expand-div (5933–5964) | Compress to ~10 ln: the `break 7/6` tell + "run `maspsxflags --write`". |
| Matching main (5964–5977) | KEEP (tiny). |
| Toolchain gotchas (5977–6363, 386 ln) | Split: SDK-boundary, GTE policy, SIGNEXT class = KEEP (park-on-sight recognisers, already partly in triage.py). The FUN_80057b80/AddEnemy investigation bullets (6135–6360) → park headers / compiler-facts. Net ~50% cut. |

---

## 3. The owner's RTL question, answered

> "Do we need rules that can look at RTL and make conclusions? The agents can look
> at dumps but I don't know if they process them except by looking at them."

**Agents do not process dumps. They eyeball them, hand-derive what the dump already
prints, and err — reproducibly.** The record, all from this rollout:

- A lane read the `ref_count` column as *priority* and inverted a correct
  six-round-old park; the orchestrator folded the inversion into the cookbook and
  briefed it onward (§6532 is the fossil).
- A brief hand-counted 16 refs where `.lreg` measures 19; the lever built on it
  could not work, and a lane spent a round proving that.
- A REG_EQUIV note was attributed to the pseudo its *value* mentions, not its
  SET_DEST owner (AdtSelect round 4 — chased the wrong pseudo).
- `.sched2`'s backward T-order was hand-inverted; schedtrace was sched1-only and a
  lane called hand-decoding the raw dump "the single biggest cost of its round".
- UIDs were correlated against target asm by eye (the sched-deps backlog entry);
  maspsx `nop # DEBUG` comment lines were counted as instructions, manufacturing a
  phantom "+3 surplus refs" theory.
- Two lanes "remembered" gcc code that does not exist (a cost comparison in
  `reload_cse_simplify_set`; `rare_destination` returning 1).

**And the counter-evidence is now three-for-three: cc1 PRINTS its own decision, in a
table, and nobody read it until a tool surfaced it.**

1. `.sched`: `;; insn[N]: priority = P, ref_count = R` — 250 lines on AddEnemy;
   the misread above happened while this table existed. → `schedtrace.py`.
2. `.sched2`: `;; ready list at T-N: 6 (1) 4 (1), now 6 4` — FUN_80057b80's entire
   8-byte residual is that one line. → `schedtrace --pass sched2`.
3. `.loop`: `Insn 363: regno 164 (life 2), savings 2 moved to 388` vs the sibling's
   `not desirable` — SetBleedsDir's whole 13-byte residual was that one line, found
   by hand, called "the single highest-value artifact here". → `rtldump --loop-log`
   (added mid-audit).

So the answer is: **yes, we need rules that look at RTL and conclude — and every one
of them must be a program.** A rule of the form "read the dump; if X conclude Y" is
a decision procedure: as prose it is executed by a different fallible reader per
lane per round; as code it is executed once, tested, and pinned. The asymmetry the
charter names is real and measurable: the ~13 tool bugs were each fixed centrally
once and now have regression pins (`test_matching_tools.py`); the ~6 wrong prose
rules each cost one or more lanes to refute, and two were re-propagated by the
orchestrator after being refuted.

**The honest qualification:** tools produced 13 false measurements. The lesson is
not "tools are safe", it is that tools fail *debuggably* — and that agents must be
able to audit a tool's verdict. That is what the KEEP bucket is for: the ~30
compiler invariants (one-var-one-reg, priority formula, cross-jump mechanics, fence
semantics) are the audit substrate. Keep them; delete the procedures.

**The division of labour:**
- **dump → fact**: always a tool. The tool prints the fact, its provenance, and —
  in the same output — what it cannot distinguish (the reghist lesson, now policy in
  orchestration.md).
- **fact → source shape**: the cookbook. Short rules that map a *tool verdict* to a
  C-shape family ("loadclass says VARYING ⇒ reorder statements, not the scheduler").
- **shape → bytes**: the build. matchdiff is the only judge.

**Concrete deliverable this implies (do it before writing any new rule): an
inventory of every self-reporting table in cc1's `-da` output.** 15 passes dump;
we systematically surface perhaps four (greg dispositions/conflicts/preferences +
the allocate-order line, lreg use-counts, sched/sched2 priority+ready lists, loop
movables). Unread, known to exist: `.flow`'s `Register N used M times` (the
promoted-retype ref-shift evidence, §7567), combine's LOG_LINKS provenance,
reload's spill/reload decisions, cse's equivalence-class traces, jump2's cross-jump
decisions. One day of grepping toplev.c/print_rtl callers; each table either maps
to a surfacing tool or becomes a named gap. The pattern has paid ≥3 lane-rounds and
at least two wrong conclusions already; the inventory is cheaper than any one of
them.

---

## 4. The tools — specs, ranked by lane-rounds saved

Evidence-based ranking (rounds already paid ≈ rounds a tool saves going forward).
Every spec includes how it FAILS LOUDLY — that is the non-negotiable part; the
charter's 13 bugs were all silent-success failures.

**The tool contract** (applies to every entry; distilled from the 13 bugs):
1. A guarded source measures the DRAFT by default; measuring the stub requires an
   explicit loud flag. Zero-line/empty input is an ERROR, never "IDENTICAL".
2. Print provenance: file, build hash, dump path — stable across runs (the TMPDIR
   lesson), verified in the agents' env (CLAUDE_SCRATCH set).
3. Every verdict carries its blind spot inline ("cannot distinguish rename from
   copy-structure"; "passing this gate proves nothing — SetupTelop").
4. Truncation states its consequence; totals print before samples.
5. On exit the world is defined: rebuild or atomic publish (the autorules lesson).
6. Every fixed bug gets a pin in `test_matching_tools.py`.
7. Where the compiler prints its own answer, self-validate the model against it and
   abort on divergence (the `.greg` allocate-order oracle: 0 violations in 53 pairs).

### T1. `sched-deps` (schedtrace v2 + the dump-table inventory) — ≥6 rounds paid
Absorbs: §2848, §6399, §6559, §6872; the uid→asm map and LUID-tie asks; the
three-for-three printed tables.
- **Inputs**: `<Name>`, `--pass sched|sched2`, optional `--uid N` / `--block N`.
- **Reads**: `.sched`/`.sched2`/`.dbr`/`.flow` from rtldump (draft-aware).
- **Decides/prints**: per insn: UID → RTL → mapped C line (debug notes) → final asm
  line (alignment); priority + ref_count from cc1's own lines (never derived);
  ready-list decisions re-rendered FORWARD with "picked earlier = lands later"
  applied; equal-priority groups with each member's producer class/cost and which
  one-point demotion dissolves the group (§2848); remat-split detection (same UID
  becomes a `const_int` set while the store reappears at a fresh UID → verdict:
  "reload split — not a sched tie, not fence-fixable", §6399); block-leader report
  (`.flow` order vs `.sched` order → "pre-sched order already matches target: lever
  is priority, not reorg", §6872); the arg-move floor verdict (argmove + priority 1
  + LUID proof → "unreachable, park", §6559) — falsified against our own bytes
  before printing, per the AddEnemy precedent.
- **Fails loudly**: refuses an empty dump; refuses to derive any number cc1 prints
  (asserts the printed line exists, else "dump format changed — do not trust");
  every park verdict prints the falsification it ran.

### T2. `regalloc --order` + identity pack — ≥5 rounds paid; asked by THREE lanes
The lane that hand-reproduced all 25 SetupTelop allocnos called this "the single
highest-value tooling gap". Absorbs §2974, §3648, backlog `--local`/`--names`/
`--spill-uses`/self-validation.
- **Inputs**: `<Name>`, `--order`, `--names`, `--spill-uses P`, existing
  `--compare/--between/--prefer`.
- **Reads**: `.lreg` + `.greg` (+ `-g` stabs for names) and, for target-implied
  constraints, the matchdiff alignment.
- **Decides/prints**: the computed allocation order (priority =
  `floor_log2(n_refs)·n_refs/live_length·10000·size`, **n_refs loop-depth-weighted**
  — each RTL mention adds its loop depth; the canon sections omit this and a lane
  had to establish it) **self-validated against `.greg`'s own `;; N regs to
  allocate:` line — divergence aborts the output**; pseudo→C-name (UNIDENTIFIED
  when stabs/live-range arithmetic cannot pin it — never guess: the spilled
  `insertedRank` misread as `rankSpriteBase` survived three rounds); the
  target-implied order (which pseudo must precede which for the target's registers)
  and per-pseudo "needs n_refs ≥ N / IMPOSSIBLE: hard-conflict with $rX" lines;
  `--spill-uses`: each use BARE vs IN-MEM (the AdtSelect discriminator);
  local-alloc quantities (`--local`), since local colours drive global preferences
  and were invisible for ControlHumanoid.
- **Fails loudly**: the INSEPARABLE/LOCAL-ONLY verdicts carry "passing this gate
  proves nothing (SetupTelop d487683)"; a priority claim about an unidentified
  pseudo is refused, not emitted.

### T3. `matchdiff --clusters` (+ `--baseline`) — ≥4 rounds paid
The insn/byte mix-up survived three briefs; the `--max 40` truncation built a false
byte-account (12/160 vs 23/239); byte-accounting is mandated and hand-rolled every
round; fence edits need per-cluster deltas (+7 hid −27/+34).
- **Inputs**: `<Name>`, `--clusters [gap]`, `--baseline <saved.json>`.
- **Prints**: per cluster: address range, INSNS and BYTES (units in the header —
  the mix-up class), classification via alignment (reg-swap / opcode / moved /
  insert / delete / branch-retarget-hidden count), owning function region; with
  `--baseline`: per-cluster delta table ("L839 fence removal: cluster 3: −27B;
  clusters 7,8 NEW +34B @0x80055938").
- **Fails loudly**: stale/NON_MATCHING artifact refusal (already built); totals
  before samples; a moved insn is never rendered as a bare delete (the asmdiff
  lesson); "clusters are a hypothesis about causes, not a partition" printed in the
  header.

### T4. `loadclass` — ~3 rounds paid
§6660 IS this spec; FUN_8004c59c's 23/29 bytes "would have been named immediately";
HumanActionControl's 11 bytes = one REG_DEP_ANTI line; refuted FUN_8003944c's park.
- **Inputs**: `<Name>` (+ `--block N`).
- **Reads**: `.sched`/`.sched2` LOG_LINKS + the insn RTL.
- **Prints**: per load: address kind — FIXED `(mem (symbol_ref))` "floats
  anywhere" / CHAINED `(mem (lo_sum …))` "scheduler-chained (extern-array
  declaration!)" / VARYING `(plus (reg N) K)` "pinned below stores S1..Sn —
  **includes sp-relative**" — plus `/s` flag and the anti-dep list; per displaced
  store the §2826 verdict ("REG_DEP_ANTI exists only because load U lacks `/s` →
  try struct-view").
- **Fails loudly**: unknown address form prints UNKNOWN + the raw RTX, never a
  bucket guess; empty dump = error.

### T5. `dslot` — ~3 rounds paid
StickonCheck's park IS this table ("that table IS the whole answer"); LoadTIMpack's
direction trap was briefed backwards; ChasetoTarget needed a hand-read of reorg.c.
- **Inputs**: `<Name>`.
- **Reads**: `.dbr` + final asm + `.sched2`.
- **Prints**: per branch: condition class, `mostly_true_jump` result AND why
  (LOOP_BEG scan-back etc., with DIRECTION: "returns 2 ⇒ raids the TARGET thread ⇒
  +1 COPY" vs "0 ⇒ fallthrough MOVE −1"); thread ownership; skip-label leader insn
  + eligibility (`dslot` attr; loads ineligible) + LABEL_NUSES; actual fill + which
  reorg routine took it; for an empty slot, the blocking reason.
- **Fails loudly**: cross-checks its predicted fill against the actual bytes and
  prints MODEL-DIVERGES instead of a wrong explanation.

### T6. `siblingdiff --verdict` ("TRANSCRIBE THIS BODY") — ~3 rounds paid, cheapest build
Three lanes found the answer in a sibling's C by eye; DrawClip's answer key
(DrawSprite, 0.48 + same TU) sat unread through a park.
- **Decides**: top sibling score ≥ threshold AND same original TU
  (psxsym-tu-map) ⇒ print `TRANSCRIBE: <Sibling>.c is the template — matched spans
  X..Y align; change only <unaligned spans>`. Below threshold or cross-TU ⇒ prints
  why it did NOT fire.
- **Fails loudly**: states the score and TU evidence used; never fires from
  similarity alone (the symmatch frame-shape lesson: signature outranks shape).

### T7. `passtrace <Name> <pattern>` — ~2 rounds paid, asked twice
- **Reads**: all pass dumps in pass order.
- **Prints**: the matching insn/operand rendering per pass, first-difference
  marked ("`.cse` keeps reg 91; `.cse2` rewrites to reg 100 ← HERE").
- **Fails loudly**: zero matches across ALL passes is an error (pattern typo ≠
  "nothing changed"); reminds output is post-pass state, and that
  `reload_cse_regs` emits no dump (visible only as a greg→sched2 delta — the
  GetNearestHumanoid lesson baked in).

### T8. `selfsim` — 1–2 rounds paid
LoadTIMpack's park dissolved with one look at the target's own repeated blocks.
- **Decides**: n-gram self-alignment of the TARGET window; if residual bytes fall
  inside instance k of a repeated block whose instance j matches us ⇒ "SPELLING
  ASYMMETRY: target's sites are identical; diff YOUR site k against YOUR site j —
  stop looking at the compiler."
- **Fails loudly**: reports when repeated-block detection is below confidence;
  checks the carve (an under-sized carve fakes asymmetry).

### T9. `slotcalc` (stackplan extension) — 1–2 rounds paid
FUN_80018f00 ("slot order is determined, not searchable") + the AddEnemy circular
spill-slot saga.
- **Decides**: from declared aggregate sizes + target `sp+K` accesses, model
  `assign_stack_local` (declaration order, ascending from the arg boundary, 8-byte
  BLKmode rounding — §7660's fact) → the unique declaration order; frame-residue
  accounting → "N compiler spill slots implied — the ORIGINAL's allocator spilled
  N values" (§7167), flagged as non-circular evidence.
- **Fails loudly**: 0 or >1 orders fit ⇒ "object set is wrong — do not permute
  declarations"; residue not ≡ 0 mod 4 ⇒ "cannot account frame: declaration list
  or carve wrong".

### T10. `findrule` — build LAST, and mostly as data
"Nothing ROUTES a same-length register residual to §3459." True — but after this
restructure the router is a page and the book is 25 families. Implementation:
every rule gets a machine-readable `signature:` line; `findrule` greps signatures +
autorules keys + rtlguide names. If the restructure lands, T10 is an index lookup,
not a tool. Do not build it against the current 7,959-line file.

### Autorules batch (each cites a concrete win; all byte-scored by construction)
`invariant-inline` (SetBleedsDir — the lane says it would have matched unattended);
`guard-clause-invert` (§6450, GetNearestHumanoid first try); `return-to-goto`
(§6854); `chain-order-swap` (§2866, DrawSnow 14→0 in one build); `merge-def-swap`
(§4151, SetBleed); `redundant-entry-guard` (§904); `neg-mul-strength` (§3205);
`arrayref-int-sum` multi-dim (§3482, LoadConstruction −21); `struct-view` (§2826,
HumanActionControl's whole 11B; backlog-specified); `extern-complete-size`
(RestoreItemLayout's one-character fix; backlog-specified); `signext-inplace-pair`
(7747 bullet); `narrowed-negate` (6315 bullet); `join-store` / `guard-expr-inline`
(backlog, both matched a function by hand).

---

## 5. The restructure

### Target shape: ~1,800–2,200 lines (from 7,959), 4 files + 1 generated index

```
docs/matching-cookbook.md        (~1,400 ln)
  0. Contract pointer + workflow + iteration ladder          (~120 ln)
  1. Evidence first: PSX.SYM / siblings / demo / xref        (~90 ln)
  2. THE ROUTER (one page): residual signature → tool →      (~60 ln)
     rule family.  The static findrule.  Examples:
       wrong LENGTH            → structure families 3.1–3.3
       same-length, one block  → regalloc --order → identity/seed families
       same-length, registers  → reghist → rtlguide → regalloc → identity canon
       delay-slot/nop          → dslot → delay-slot family
       load position           → loadclass → statement order / struct-view
       preheader order         → rtldump --loop-log → loop.c economy
       repeated target block   → selfsim → spelling asymmetry
       high-scoring TU sibling → siblingdiff --verdict → transcribe
  3. ~25 consolidated rule families (KEEP + compressed       (~900 ln)
     DEMOTE survivors): dispatch & body layout · loop forms ·
     return islands & shared tails · variable identity ·
     addressing shapes · cse aliasing & block boundaries ·
     THE fence section (§6937 alone) · loop.c economy ·
     allocation priority canon · delay-slot reading · stack
     objects & unions · gp/symbols · div-expand · SDK/GTE
     boundaries · main
  4. Park discipline (a park is a hypothesis; negatives can   (~40 ln)
     be INVERTED — SetBleedsDir's "failed" attempt was the
     original source; write negatives as "with Z fixed at W")

docs/compiler-facts.md           (~350 ln)  verified cc1 mechanics with source
     cites — the audit substrate for checking tool verdicts. Every entry:
     claim + gcc file:line + the function that proved it.
docs/shape-zoo.md                (~250 ln)  the 37 DEMOTEd shapes, 1–3 lines each
     + example pointer. Greppable, never assigned reading.
docs/autorules-index.md          (generated) key → one-liner → trigger signature →
     cookbook family, emitted from autorules.py's own tables + rtlguide's
     signature list. The cookbook NEVER describes a mechanised rule again.
```

Function-specific sagas (AdtSelect's four-version self-tie, StickonCheck's
eligibility autopsy, GetAreaMapVector's cse2 park, FUN_80057b80's promote_mode
proof, the AddEnemy round history) move to the function `.c` headers they describe
— that is where the next lane on THAT function looks, and nowhere else.

### Reading order for a lane (what it must hold in its head)
1. matcher contract (1 page: the loop, the gates, the budgets, "run tools, never
   hand-derive a number a dump prints").
2. Cookbook §0–§2 (~270 lines).
3. ONLY the families §2's router names for its current residual.
Median lane reading: **~600 lines**, down from "8,000 lines, unrouted".

### What belongs where
| Content | Home |
|---|---|
| Loop, gates, budgets, model routing, tool router | `matcher.md` contract + cookbook §2 |
| Source-shape vocabulary, compiler invariants | cookbook §3 / compiler-facts.md |
| How to read a dump/output, column meanings, caveats | the tool's `--help` AND its printed output (a caveat only in a doc is a bug) |
| Function-specific history, refuted experiments | that function's `.c` header |
| Targeting, harvest, bundling, briefs | orchestration.md (already true; delete the cookbook's duplicates) |

### Guards so it never regrows
- CI: the cookbook may not contain "Now mechanized" / an autorules key in prose —
  the index owns those; a PR adding a rule to autorules must delete or index the
  corresponding cookbook text.
- CI lint: duplicated heading text on one line (two exist today), duplicated
  consecutive bold paragraphs (one exists), broken numbered lists (one exists).
- The reflection loop (orchestration.md) gains one line: *"fold a rule" first asks
  "is this a decision procedure over an artifact?" — if yes, it is a tool ticket,
  not a cookbook edit.*

### Mechanical damage to fix regardless of the restructure
1. §6574 and §6801: heading text duplicated on the heading line itself.
2. 6131–6149: two "Correction (AddEnemy round 9/10)" paragraphs spliced
   MID-SENTENCE into the loop.c-disjuncts bullet; the host sentence no longer
   parses.
3. 7649–7652: the "Inline a CSE'd pointer-array element" bullet duplicated, first
   copy truncated mid-sentence.
4. Iteration protocol: two items numbered "4." (lines 224, 258).

---

## 6. One-paragraph summary for the owner

Yes — delete aggressively: 43 of 132 sections are already mechanised, superseded,
or the Nth copy; 37 more are true-but-low-yield; two "sections" are 826- and
634-line landfills. Yes — the sharpest instinct in your question is right: every
rule of the form "read the dump, if X conclude Y" is a program, and the evidence is
now three-for-three that cc1 prints its own decisions and only tools get them read
correctly; the 22 MECHANISE sections above come with implementable specs, ranked,
each required to fail loudly, plus a one-day inventory of every cc1 dump table
nothing currently reads. What remains prose is what should be prose: ~25 families
of source-shape judgment and the compiler invariants an agent needs to audit a
tool's verdict — about a quarter of the current file, of which a lane reads a
tenth per function.
