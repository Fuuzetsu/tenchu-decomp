# The shape zoo — true but low-yield shapes, one entry each

Not assigned reading. Grep this file when a residual matches a trigger below;
each entry is a shape that fixed exactly one function (or a small family) and
did not generalize enough for the cookbook's families. Every entry names its
worked example — read that function's `.c` header for the full story.

## Dispatch / control flow

- **Raw value + adjusted index as separate roles**: one remapped char drives a
  table index and a later range test — keep `ch` intact, copy to `index`, adjust
  `index` only (FUN_800570b8; also: source-ordered local aliases for directly
  used params, and right-associated `x0 = x2 = cursor` pair stores).
- **Guard-constant label past the main tail**: `goto ret_min;` to a label at the
  function's very end, or the constant's `lui` floats into the guard's delay slot
  (the `return 0x80000000;` shape).
- **Guard returning its own test's constant**: `if (cond) goto end; … end:
  return;` — the test register already holds the value; a literal `return X;`
  costs a `li` (ItemUse).
- **For-loop with folded entry guard**: `i = 0; if (i >= N) goto ret_k;` before
  `for (; i < N; i++)` — the guard must compare the INDEX (records on its
  quantity); the loop must stay a real `for` (GetConflictResult).
- **Success `return X` keeping its own `jr`**: nest the entry guard so the final
  `return -1` is the else; the surviving bare CODE_LABEL blocks jump.c and reorg
  converts the success return (GetConflictResult, DeleteConflict).
- **Default-then-override temp keeps an address alive**: draft N insns SHORT with
  a missing address recompute — write the two arms as independent stores; cse
  declines to merge the second occurrence. Not root-caused; a shape to try
  (ReqLifeBar, 36 bytes).
- **Two-arm vs pre-zero sign-flag macro variants**: physical jump inventory
  (`flag = 0; if (v < 0) {v = -v; flag = 1;}` vs two-arm form) — count the
  target's unconditional jumps and route per expansion (mission_score_screen 8/8).

## Expressions / width

- **Narrowing store via widened temp**: `dp = sk - (u16)DrawingPage;
  DrawingPage = dp;` — the direct form reads as a narrowing use and reloads
  wrongly-unsigned (EndDrawing).
- **`(short)` cast on an int `|` call argument is a scheduler lever**: uncast,
  extend-then-or shortens the chain and a sibling argument's load floats up,
  evicting a base from `$a0` (Sound).
- **Comparison literal named before an earlier guard** so its `li` fills that
  guard's delay slot; the local's mode matters (ProcItemArrow's `s16` aim bound).
- **Read a stored narrow parameter back** (`bg->w = w; … bg->w >> 1`) to force an
  independent narrowing identity — cc1 store-forwards but re-expands the
  extension (SetupBG).
- **Constant-folded struct-pointer cast is NOT the flat scratchpad idiom**:
  `((MATRIX *)0x1F800000)->t[0]` CSEs the base into a register across stores;
  per-statement flat casts keep the per-store `lui $at` (DrawBleed 496 vs 532).
- **Inline a CSE'd pointer-array element** into its first consuming expression so
  it loads late and reuses a dying pointer's register; a named temp schedules too
  early (ChasetoTarget's `chase[1]`).
- **Absorb an abs cascade with a scratch reuse**: extend an existing local's live
  range as a throwaway scratch BEFORE its real unconditional assignment —
  semantically risky; only a variable whose real role is unconditionally
  reassigned qualifies (Think1random).

## Stores / pointers

- **Conditional store via pre-branch address copy**: `xyz[j] = cond ? (t += K) :
  (t -= K);` — one assignment, conditional RHS; the ternary's condition may need
  to re-read the target memory (UpdateMotion).
- **Two branch-arm stores through one global pointer**: capture the pointer once
  before the branch (`human = Me_MOTION_C;`), non-null arm first — one `%gp_rel`
  load, one address pseudo (AttackControl). Inverse of deferred-global-capture.
- **Adjacent global banks need distinct canonical symbols**: if the candidate
  derives bank 2 as `bank1 + C`, pin a second symbol and unify all consumers on
  it (FUN_8003562c's `sprBlood2`; DrawBlood integration).
- **Assignment-expression publication**: `(Current = (State *)spawned)->attribute
  |= FLAG;` encodes a pointer publication inside the consuming lvalue
  (Think3callaid).
- **Same-address store through the RIGHT pointer variable**: `it->param` vs a
  `pp` alias decides a delay-slot tie at identical value (Launch, 9 bytes).
- **Cross-predecessor indirect cleanup pointer**: assign `call_item = item->proc`
  on every predecessor, null-check/call at the join (ProcItemJirai) — the
  complement of duplicated per-arm cleanups.
- **Block copy through `*p` coalesces the copy cursor** across a multi-pred join;
  `rparam = param;` re-materialises the source address (ProcItemLaunch).
- **Pool-search cursor + post-`found:` continuation as separate locals**
  (`cur`/`it`, `it` assigned exactly twice): invisible under low pressure, a real
  `move` on every edge under high pressure — try it when a pool search is exactly
  8 bytes short (Makibishi/LightningBolt; harmless in Ningyo).
- **Initialize a pointer alias at its first path-specific use**, not at entry
  (SwimCheck); a call can be the boundary between two aliases of one address
  (FUN_80037e0c's `position_base`/`position`).
- **Separate the scan cursor from the loop-exit capture** (`found_slot = slot`
  before the shared label) so the cursor dies (FUN_80037e0c).
- **Struct-typed local cached at a post-branch join**, not at entry — a named
  local whose liveness crosses the label beats hoisting (ItemUse's `me`).
- **A parameter that walks stays in its incoming register; the fixed base gets
  copied** — pick which name walks from the target's `$a0` role (LoadTIMpack).
- **Which zero-initialised local is the "master"** (the literal assignment, rest
  copied) is read off the first `move`/`li` of the chain; wrong master collides
  with a live parameter register (GetCenterAndSize).

- **Two Ghidra-invented adjacent `short[2]` out-params of one call are ONE
  8-byte stack aggregate** (`struct { u16 a, pad0, b, pad1; }`, pass
  `&s.a`/`&s.b`, read back `lhu`) — two tight scalars pack 2 bytes short,
  padding overshoots (ReqItemArrow). Two u16 out-locals with a 4-byte gap are
  one SVECTOR's `.vx`/`.vz` (LightningBolt).
- **Full-width producer → narrow publish → plain copy**: `pad->held = raw;
  do {} while (0);` + two identical branch-local `int` copies of `raw` — the
  fence anchors the store, only the later bit test narrows (ComPad).
- **A second pointer feeds a guard-branch delay slot**: declare `q = p;` right
  after computing `p`, read `p` in one arm and `q` in the other — forces the
  eager address combine AND supplies the stolen move (PadShock).
- **Per-axis mask assignments after each division ladder** (`x &= mask;` …)
  rather than masking only in the final subscript — recovers delay-slot masks
  and saved-register reuse (LoadConstruction's coordinate bucketing).
- **A sentinel-record scan with a special first row**: test row zero once, spell
  the repeated scan with a label/goto, keep `i++;` natural at the backedge
  (ActDEAD — a structured loop peels the known row).
- **Destructive narrow working copy after saving the next value**:
  `value = quotient; quotient <<= 16; } while (quotient != 0);` — the
  target's `move; sll; bnez` chain when quotient is dead after the test
  (StageEndScreen digit loops).
- **Shared `f(0, x)` tail vs two explicit `f(0, lit)` calls**: the shared form
  hoists one `$a0 = 0` into the merged call's delay slot; per-branch literal
  calls keep it in each predecessor (CheckCheatCodes 17→2).
- **A branch-proven scalar distinguishes cross-jumpable zero-store tails**:
  assigning the tested `int shock` (provably 0 in that arm) to both bytes
  keeps the tails distinct while combine still emits `sb zero` — requires an
  immediately dominating equality test (PadProc).
- **A per-axis raw computation needs a temp distinct from its final assigned
  value** (`dx*dx` fed onward) or the whole chain goes callee-saved
  (FUN_80039ddc); preserve per-axis definition order — coefficient-X/input-X/
  output-X then Z (DefaultActionHumanoid). The INVERSE — direct
  `x = A - B;` when the difference itself survives as a call argument — is
  mechanised (`difference-role-fuse`, Think1random).

## Regalloc micro-levers

- **Named `zero` local** (`int zero = 0; while (n > zero)`) flips a pure
  register-swap tie via the pseudo-number tiebreak (GetArcData).
- **Spilled u16 locals loaded early went through int temps** — the u16→int reads
  are real zero_extend insns reload turns into `lhu` in place (BIS).
- **Which operand goes through the named int temp picks the reload register**
  (BIS digit entry).
- **A surviving `andi rN,reg,0xff` across a call is `int c = (u8)var;` with var
  MULTI-DEF** — combine folds the zext only over a single-lbu def (BIS).
- **Byte-neutral cse re-read in a COMPARE donates different preferences**
  (`mx < cq->stock[n]` for `mx < c`) — set_preference takes the first operand
  (BIS).
- **In-place short-lived carrier adds a missing sched1 dependency**:
  `cursor = CVAnow; sound = cursor->param; cursor++; CVAnow = cursor;` — the add
  redefines the carrier so it cannot cross the field read (CVAsequence).
- **Direct reads per switch arm + separate post-update read at the tail**
  (`cursor = CVAnow + 1; CVAnow = cursor;` preserves the move that `CVAnow++;
  cursor = CVAnow;` loses) (CVAupdate).
- **`*p = x = expr;` vs `x = expr; *p = x;`** flips a scheduling tie between
  expr's load and p's address load (PlayerOption).
- **Assignment position around a branch is a double lever**: before the guard =
  delay-slot fill + longer live range + demoted priority (ReqItemDrop).
- **ReqItemUse multi-anchor**: promote block locals to function scope and give
  each a byte-neutral reuse in a distant block whose register is known — three
  anchors fixed a 15-instruction permutation (ReqItemUse, 5272 bytes).
- **Hoist-invariant vs widen-parameter 2-insn allocator tie**: target dedicates a
  register to the raw param and re-derives the widened form per iteration,
  freeing one for the invariant (SetSmoke; RTL-escalation territory).
- **Huge-frame (±32767) a3/t0 rotation** is reload round-robin structure, not
  regalloc — steer by changing which earlier insns in the block need reloads
  (AdtSelect).
- **Folded pointer-address constant vs symbol form is a coupled trade**: the
  constant form can be the local optimum when the symbol's `%hi` range defeats a
  downstream coalesce (StageEndScreen's terminal `&D_8008EA78`).

## Loops

- **Search-flag initialiser placement**: `found = 0;` at loop top is hoisted →
  callee-saved; inside the exhaust/break arm → caller-saved + delay-slot `li`
  (ProcItemLaunch/Smoke).
- **Scratch jitter values as one array** (`long p[3];`) force stack residency
  when the target block-copies them (SetBleedsDir).
- **Loop-invariant store value pre-assigned to a named local** before the loop to
  emit its `li` before the counter's (leResetEnemyLayout).
- **A wrap-around search loop with increment in the backjump's delay slot and
  inverse compensation on the fallthrough exit** is a plain do-while with the
  increment FIRST in the body (DoInfoViewProc).
- **Two-break `while (1)`**: bound as a top `if (...) break`, data terminator as
  a bottom `if (...) break` — combining them changes which edge owns the
  increment (SetupTelop's glyph scan).

## Method / accounting

- **Byte-account the census before inferring an object count**: a lone odd
  `addiu sp,K` in a block-move frame is usually the copy's limit cursor
  (`&src + 16*chunks`), not another local (FUN_80018f00).
- **Screen REGISTER questions in a 25-line testbed; verify SCHEDULING and COPY
  SURVIVAL in the real function** — both false-positive under low pressure
  (division delay slot; ActSTICKON's self-XOR).
- **Hand-written range-splits that model caller-save are LOAD-BEARING** —
  caller-save.c engages only when find_reg already failed; deleting the model
  hands the value a callee-saved register instead (AddEnemy's
  `blood.call_spill[]`, −4 insns when deleted — check whether the allocator has
  any reason to spill first).
