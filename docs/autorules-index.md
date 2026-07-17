# Mechanised-rule index (GENERATED — do not edit)

Regenerate with `tools/cookbooklint.py gen`; CI checks freshness with
`tools/cookbooklint.py gen --check`. Sources of truth: the rule tables
in `tools/autorules.py` and `SIGNATURE_HINTS` in `tools/rtlguide.py`.

The cookbook never re-explains a mechanised rule: if a key below covers
your residual, run the tool. `tools/autorules.py <Name>` sweeps the
default rules; `tools/rtlguide.py <Name>` then
`tools/autorules.py <Name> --guided` routes the guided/advanced ones.

## autorules — default rules

| rule | does |
|---|---|
| `type-width` | flip a local's integer type across width/signedness |
| `param-width` | flip a scalar parameter's integer type across width/signedness |
| `stack-decl-swap` | swap adjacent same-type address-taken object declarations |
| `extern-array` | extern T NAME; -> extern T NAME[]; + NAME->NAME[0] (-G8 split) |
| `and-nest` | split/merge if(a && b) <-> nested ifs (no else) |
| `temp-inline` | inline a single-use local temp into its use |
| `copy-seed` | x = E(y); -> x = y; x = E(x); (local_alloc quantity-order lever) |
| `binop-operand-seed` | x = A op B; -> t = B; x = A op t; (operand-birth order) |
| `call-result-split` | split a reused direct-call result into single-definition locals |
| `late-pointer-direct` | inline a repeated pointer-global assignment in its late region |
| `literal-indirect-inline` | atomically inline a byte literal and indirect-call field temps |
| `call-arg-pair` | inline adjacent same-call temps into one consumer call |
| `abs-ge` | rewrite (x<0)?-x:x -> (x>=0)?x:-x (the abssi2 fold) |
| `builtin-abs` | abs call/sign-fix -> __builtin_abs under cc1 -fno-builtin |
| `or-inplace` | rewrite X = X\|C -> X \|= C (local-alloc lever) |
| `rand-mod` | split/merge x=rand()%K (call-result allocation lever) |
| `mod-bias-temp` | split base+(delta%range-bias) through a typed temp |
| `subscript-postinc` | split/merge a[i];i++ <-> a[i++] (working-copy lever) |
| `ptr-index-sum` | T *p = base+idx -> (T*)(idx*sizeof(T)+(int)base) |
| `vector-copy` | four vx/vy/vz/pad field copies -> one struct assignment |
| `assignment-chain` | merge/split same-literal field stores |
| `vector-copy-adjust` | split/merge a literal-adjusted vx/vy/vz field copy |
| `split-chain` | t = (x-C)>>S -> t = x-C; t = t>>S (split fused chain) |
| `min-ternary` | x=a; if(b<x) x=b; -> x = (b<a)?b:a (min-vs-memory) |
| `clamp-shared-return` | two direct clamp returns -> assignments plus one return |
| `flag-return-split` | local 0/1 default+override -> literal return sites |
| `cmp-swap` | a>mem -> mem<a (comparison operand-order lever) |
| `fence-unwrap` | remove an existing non-empty one-shot fence, keeping its body |

## autorules — guided/advanced rules (opt-in)

| rule | does |
|---|---|
| `allocation-donor-fence` | add a guided initialized-value ref in zero-code identical arms |
| `field-capture-rhs` | fuse/split an adjacent saved field through the overwrite RHS |
| `initialized-global-compound` | split a commutative global sum into accumulator initialization plus += |
| `cmp-polarity` | swap two local comparison operands (regalloc ref-order lever) |
| `eq-literal-swap` | swap ==/!= literal operand order (v0/v1 lever) |
| `shift16-mul` | casted short x<<16 -> x*0x10000 (raw sll lever) |
| `shift-stage` | split x>>N into every two-stage constant shift |
| `dead-host-split` | split signed (A+/-B)/K through a dead signed-s32 local |
| `mul-affine-shape` | toggle x*M+C with (x+C/M)*M |
| `ptr-base-split` | name an extern array base before indexed address/call |
| `deref-address-split` | name a casted dereference address before its scalar load |
| `working-copy-seed-merge` | move a working-copy identity from seed to update/writeback |
| `deferred-global-capture` | test a scalar global directly, then capture once at the join |
| `plus-group` | enumerate 3-term + grouping/constant placement (fold lever) |
| `add-prefix-temp` | name a signed 2-term seam in a narrowed 3-term sum |
| `flag-arm-assign` | move local 0/1 definitions between pre-zero and two-arm forms |
| `default-ladder-hoist` | hoist a three-way literal result default before both comparisons |
| `guard-flag-assign` | move a local flag definition before a guard or onto both goto edges |
| `guard-exit-copy` | move a local value copy across its unique loop-break guard |
| `shared-tail-assign` | duplicate one shared assignment into both preceding arms |
| `shared-writeback-compound` | inline a shared field writeback into compound-update arms |
| `shared-terminal-tail` | duplicate an adjacent assignment+return into both if/else arms |
| `shared-result-return` | funnel two value returns through one result pseudo |
| `terminal-call-return` | return a terminal branch call before the shared result join |
| `difference-role-fuse` | fuse scratch-loaded subtractions into direct difference locals |
| `shared-return-split` | replace a goto to a return label with a second literal return |
| `terminal-arm-flip` | negate and swap adjacent arms ending at one terminal tail |
| `if-else-invert` | invert a compound if/else to swap physical body layout |
| `terminal-guard-flip` | flip an adjacent ==/!= return/goto terminal guard |
| `adjacent-field-store-swap` | swap adjacent literal stores to distinct fields |
| `switch-cse-evict` | dead-overwrite an entry index before a fresh switch load |
| `pointee-volatile` | toggle volatile on a local integer pointer's pointee |
| `pointer-slot-volatile` | force one extern pointer-array slot reload |
| `array-alias-remat` | rebuild selected local-array member lvalues instead of retaining an alias |
| `member-scalar-alias` | toggle long field read through s32 lvalue (MEM_IN_STRUCT lever) |
| `disjoint-local-alias` | join a dead-until-overwrite scalar to an earlier live range |
| `redundant-field-donor` | repeat a pure local-aggregate field assignment |
| `empty-loop-boundary` | insert a weight-free LOOP_END between statements |
| `loop-fence` | wrap an if/loop in a zero-code one-shot do loop |
| `nested-loop-fence` | atomically add two or three loop weights at one site |
| `paired-loop-fence` | wrap adjacent groups in two atomic one-shot loops |
| `loop-range` | wrap 2-4 adjacent statements in one zero-code do loop |
| `loop-boundary-shift` | move the next statement across an existing LOOP_END |
| `identical-arm-condition` | permute the pure discriminator of existing identical arms |
| `identical-arm-fence` | duplicate one statement into zero-code identical arms |
| `case-fence` | if/else -> two-way switch (hard cross-jump CODE_LABEL) |
| `sparse-eq-switch` | three literal equality arms -> sparse switch permutations |

## rtlguide — named residual signatures

A signature names a residual SHAPE and the first levers to try; the
cookbook's router maps each to its family.

| signature | meaning / first move |
|---|---|
| `adjacent-independent-load-order` | first try cmp-swap when both loads feed one relational comparison (equivalent operand order changes evaluation order); otherwise do not assume independence: compare .combine -> .sched -> .sched2, inspect LOG_LINKS and nearby LOOP_END notes, then try boundary-shift or an identical-arm fence before parking |
| `arithmetic-working-copy` | target keeps an arithmetic result in a working register, copies it to the persistent value, then consumes the working value while the candidate folds the result into the persistent register; run guided working-copy-seed-merge to move that identity from the seed site |
| `branch-phi-register-tie` | target and candidate define the same distinct literals on multiple branches and feed one final store, but one carrier register is consistently recoloured; try shared-tail-assign, flag-arm-assign, and guard-flag-assign before type-width. Reject a register improvement that changes the physical branch/jump inventory. If .greg proves the target register is a hard conflict and one late permuter run stays flat, record the tie and park |
| `builtin-abs-inline` | candidate calls abs but target has bgez/negu inline; spell the site __builtin_abs(...) because the matching cc1 receives -fno-builtin |
| `call-result-argument-pipeline` | target transforms the first result across the second call and uses that call's delay slot; inline the adjacent producer pair into their common consumer arguments |
| `commutative-equality-register-order` | target and candidate differ only by load/literal homes and reversed beq/bne operands; try eq-literal-swap once, then treat a flat result as a bounded global-allocation tie |
| `commutative-plus-destination` | (reported by rtlguide; no hint text recorded) |
| `constant-add-reassociation` | (reported by rtlguide; no hint text recorded) |
| `copy-then-inplace-adjust` | target stores a stack field before overwriting it with an adjusted value; try explicit fieldwise copy followed by an in-place +=/-= adjustment |
| `dbr-duplicated-literal-producer` | candidate duplicates one literal producer into at least two branch delay slots where the target keeps nops and one later producer; inspect .sched2 -> .dbr, then run guided loop-range over the complete contiguous producer chain ending at the consumer, not only the literal/store |
| `enclosing-global-field-load` | target keeps distinct base/value registers for a global load while the candidate folds both into one; query one or more scalar names/addresses with tools/symnear.py and try the proven nonzero field of an enclosing global |
| `guard-return-island-layout` | target uses one conditional branch into/out of a shared return island, while the candidate emits the inverse guard plus nop and skip jump; try terminal-guard-flip first for an adjacent equality return/goto, then if-else-invert or one explicit shared return label once; park if RTL-guided body-layout trials are flat |
| `narrow-copy-zero-extension` | target copies a full register where the candidate masks to 16 bits; audit the carrier/copy signedness with type-width and compare jump2. If signed copies remove target blocks while unsigned copies preserve CFG but retain the masks, record the type-mode deadlock and park |
| `path-result-copy-join` | target preserves a path-produced value through two distinct pseudos before testing it, while the candidate coalesces the second copy; split the path result from the final flag at the CFG join, then inspect .lreg/.greg before trying a bounded liveness fence |
| `post-comparison-flag-definition` | target reuses a comparison register for 0/1 branch-delay definitions; assign the flag at the end of each arm, after that arm's comparisons |
| `postincrement-working-copy` | target increments through a distinct working register and copies it back; try merging the adjacent increment into array[index++] |
| `shared-return-extension-schedule` | target sign-extends the return value before restoring saved registers, while the candidate restores first; run guided shared-return-split to replace one goto to the final return label with a second literal return |
| `terminal-arm-layout-flip` | target and candidate have the same instruction multiset and physical control-flow counts, but one aligned conditional has the opposite polarity; try terminal-arm-flip for adjacent compound goto/return arms, then if-else-invert for an explicit else |
