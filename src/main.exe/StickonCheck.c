#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MapVector * StickonCheck(void);
 *     MOTION.C:346, 17 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $v1       short rv
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern short RefrectVector[16];
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 90 of 256 bytes differ, all downstream of ONE
 * reorg delay-slot STEAL at guard 3 (mechanism established from the
 * cc1-2.8.1 reorg.c source, see below); every field offset/type this function
 * establishes is still correct (proven independently of the residual).
 *
 * StickonCheck (0x8001d374, 0x100 bytes) — "can the character stick to the
 * wall/ceiling here" probe: only for character types 0/1 with the area
 * attribute's top 2 bits clear (item.h's new `attrib` @0x28), it queries
 * GetAreaMapVector for the surface ahead of `dtL`, checks the SAME top-2-
 * bits guard on the query's own result, then (unless status 0xc/"already
 * sticking", or the surface's reflect-vector flags a no-stick edge/slope,
 * or its reflect value is the -1 sentinel) switches into the stick motion
 * (unless already status 0xc) and returns the query result; otherwise NULL.
 * Same original TU as dispose_weapon_data_of_char_.c/NowReturnNormal.c/
 * ReturnNormal.c/FUN_80027304.c (Me_MOTION_C, dtL, motID, motMODE all
 * gp-relative here, matching those files' convention).
 *
 * `map` (Ghidra's own data symbol table names it "map" — the
 * GetAreaMapVector out-parameter) and `RefrectVector` (Ghidra: "RefrectVector",
 * a table of reflect-vector flag words) are declared under their raw
 * auto-generated names because that's what the assembled `.s` actually
 * references (splat only pulls FUNCTION names from the Ghidra export, not
 * arbitrary data symbols) — an extern spelled `RefrectVector` would not
 * link. `map`'s type here is a local, offsets-only truncated view
 * (only +0x8/s16 and +0xC/u8 are touched by this function; GetAreaMapVector
 * itself writes a much larger 0x20-byte record, per its own Ghidra
 * decompilation, that this function never reads).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `(u16)Me_MOTION_C->type < 2`: item.h keeps `type` s16 (proven signed by
 *    GetHumanoid's `lh`); this TU reads it `lhu` — cast at the use site,
 *    same per-TU load-width disagreement as `lifemax`/`attrib`.
 *  - The whole function is nested guard clauses exactly as Ghidra renders
 *    them (each early exit falls straight to the shared epilogue with
 *    nothing else pending after it, so a plain `return 0;` per guard
 *    reproduces the asm's repeated `v0=0` delay-slot pattern with no
 *    special-casing needed) — proven for 4 of the 5 guards.
 *  - `rv = RefrectVector[map.vector];` is a real STATEMENT before
 *    the `||`, not a comma-expression inside it: the raw asm loads it
 *    unconditionally (no side effects to gate), matching Ghidra's own
 *    comma-rendering of an eagerly-evaluated subexpression. Named `rv`
 *    (not `idxval`) per PSX.SYM's own `reg $v1 short rv` — the only local
 *    this function has; the rename is a pure no-op (measured 90 → 90).
 *  - `(rv & 0x200)` and `(s16)rv != -1` share the SAME cached
 *    load (one `lhu`, reused, with an explicit re-sign-extend for the
 *    second test) — the established u16-field/signed-compare-cast pattern.
 *
 * THE RESIDUAL — mechanism established from the cc1-2.8.1 SOURCE (reorg.c)
 * plus .jump/.sched2 dumps, correcting this park's earlier "reorg branch
 * PREDICTION" claim, which is WRONG (see "disproved" below).
 *
 * All five `return 0` guards enter reorg with IDENTICAL RTL — .jump proves
 * cross-jumping never merged them, so each is inline:
 *     if (!cond) goto L_skip;  v0=0;  goto L_epilogue;  BARRIER;  L_skip:
 * `fill_simple_delay_slots` packs the `v0=0` into the `goto L_epilogue`'s
 * own slot. That makes every guard's FALLTHROUGH a SEQUENCE, and
 * `stop_search_p` returns 1 on a SEQUENCE — so the fallthrough attempt can
 * NEVER fill a guard's slot, whatever the prediction. Each guard therefore
 * depends on the TARGET-thread (skip-label) attempt finding NOTHING: only
 * then does `relax_delay_slots` collapse `cond-jump-around-uncond-jump`
 * into ONE inverted branch that INHERITS the `goto`'s `v0=0` slot — the
 * target's 2-instruction shape (`bnez $v0,.L8001D464` + `addu $v0,$zero,$zero`).
 *
 * The steal is blocked at four of the five guards, for two distinct reasons:
 *   - guards 1/2/5: their skip label leads with a LOAD (`lh`/`lw`). MIPS
 *     `(define_delay (eq_attr "type" "branch") [(and (eq_attr "dslot" "no")
 *     (eq_attr "length" "1")) ...])` — a -mips1 load has dslot=yes, so it is
 *     INELIGIBLE. The scan then can't take the next insn either (it reads the
 *     load's dest, which is now in `set`) → nothing → collapse.
 *   - guard 4: its skip label .L8001D420 leads with an ELIGIBLE `sll`, but the
 *     label has TWO uses (0x8001d410 and 0x8001d418 both branch to it), so
 *     `own_thread_p` → own_target=0 → steal blocked → collapse. This is the
 *     control case proving eligibility alone is not sufficient.
 * Guard 3 alone leads with an eligible, OWNED `lui $v0,%hi(RefrectVector)`
 * (arith, length 1, dslot=no; label has one use and a BARRIER before it), so
 * reorg MOVES it into the slot and redirects the label past it. Its slot is
 * now non-empty, `relax_delay_slots` can no longer collapse the branch-around,
 * and the guard stays in the 4-instruction form: +1 insn. That +1 is exactly
 * repaid later (the final `status != 0xc` guard's slot takes `li $v0,0xc00`,
 * eliding a `nop` the target keeps), so the length still matches (256 both
 * sides) while ~20 instructions in between sit 4 bytes off.
 *
 * To match, reorg must find nothing eligible at guard 3's skip label. Both
 * blockers are unreachable from C here: EXPAND_SUM fixes address-before-index
 * (so the `lui` leads the block no matter how the statement is split — the
 * `lbu` cannot be made to lead), and a second reference to that label cannot
 * be created without emitting an extra branch.
 *
 * RE-VERIFIED independently this round against the RAW target `.s` (per this
 * session's standing warning that park prose here has been wrong before) and
 * against the pinned gcc-2.8.1 SOURCE directly via `tools/ccsrc.py`, not just
 * this file's prior citations — the mechanism above holds and is now sharper:
 *  - `tools/rtlguide.py` labels the owning pass "jump/cross-jump" (an
 *    unconditional `j` survives only in OUR asm: `unconditional_jumps=0/1
 *    target/candidate`). That label names the SYMPTOM, not the cause — the
 *    stray `j` is reorg's UNCOLLAPSED branch-around-branch, not a jump.c/
 *    cross-jump miss; `.jump`/`.jump2` dumps (below) confirm cross-jumping
 *    never even runs on this — the divergence is entirely inside reorg/dbr.
 *  - The `.jump` dump (pre-reorg) shows ALL FIVE guards with byte-identical
 *    RTL shape (`jump_insn cond, label_ref SKIP; insn v0=0; jump_insn
 *    label178; barrier; code_label SKIP:`) — confirms structure is not the
 *    problem; the divergence is entirely a reorg/dbr decision.
 *  - Read `fill_simple_delay_slots` (reorg.c:2975) directly: its own
 *    backward-scan can't fill a conditional guard's slot (the branch's own
 *    condition producer is immediately before it, so `insn_sets_resource_p`
 *    blocks it); `optimize_skip` (reorg.c:1159, the "jump around a single
 *    insn" case) is DEAD CODE for this target — it requires
 *    `ANNUL_IFTRUE_SLOTS`/`ANNUL_IFFALSE_SLOTS`, which only exist if the .md
 *    declares an annulled delay-slot variant, and MIPS/R3000 (no branch-
 *    likely) declares none (confirmed: `tools/ccsrc.py --grep ANNUL_IF` shows
 *    only the reorg.c:145-150 `#ifndef` fallback stubs, no mips.md hit). What
 *    DOES run is `fill_simple_delay_slots`'s trivial same-guard backward scan
 *    on the UNCONDITIONAL `goto epilogue`: it vacuums the immediately-
 *    preceding `v0=0` into ITS OWN slot for all five guards uniformly (this
 *    is why the "collapsed" 2-insn guards all show `v0=0` in the branch's
 *    delay slot, not the goto's). THEN `fill_eager_delay_slots` (reorg.c:3850)
 *    processes each still-unfilled conditional guard: `mostly_true_jump`
 *    gives 0 (EQ), so it tries `fill_slots_from_thread` on the FALLTHROUGH
 *    first — but that thread's leading insn (`v0=0`) was already consumed
 *    into a SEQUENCE by the step above, and `stop_search_p` refuses to walk
 *    into a SEQUENCE, so the fallthrough attempt finds nothing FOR EVERY
 *    GUARD UNIFORMLY (confirmed: this is not guard-3-specific). It then tries
 *    the TARGET thread (`own_target = own_thread_p(label, label, 0)`, i.e.
 *    LABEL_NUSES-gated single ownership) — this is where guards 1/2/5 (load
 *    leader, ineligible) and guard 4 (two label uses, ownership fails) all
 *    fail and guard 3 (owned, eligible `lui`) alone succeeds. Confirmed by
 *    directly reading `.dbr`'s RTL: `(insn 212 (sequence[ (jump_insn 82 ...
 *    label_ref 91) (insn/s 93 (set v0 (high RefrectVector))) ]))` — cc1's own
 *    SEQUENCE literally packages the branch with the stolen `lui`, and this
 *    SEQUENCE does not exist yet in the immediately-prior `.jump2` dump.
 *  - `tools/regalloc.py --local`: self-validated 0 divergences between its
 *    simulation and cc1's own printed homes for THIS draft. Read this as "the
 *    tool models cc1's local allocator correctly", NOT as "our allocation
 *    matches the target's" (the target has no printable allocation trace to
 *    compare against) — allocation pressure is NOT independently ruled out
 *    as a lever by this (see ENTANGLEMENT below); it just isn't a REGISTER
 *    tie in the ordinary sense (no wrong-register-same-shape hunk exists).
 *
 * ENTANGLEMENT (new this round, unresolved — a lever for a future attempt,
 * not exploited here because the only way found to pull it is unsound):
 * forcing `rv`'s live range to extend to the two later `return` sites (by
 * writing `return rv;` instead of `return 0;` at guards 4/5) measurably
 * fixes the ENTIRE guard-3 delay-slot region as a side effect (bytes
 * 0x8001d3e4..0x8001d408 become exact) — proving the guard-3 steal and
 * whatever global register pressure `rv`'s liveness creates are JOINTLY
 * determined, not independent (cookbook §2 rule 7). This is NOT adoptable:
 * `return rv;` is a real semantic bug (a `u16` flag word zero-extended into
 * the `void *` return in place of `NULL` — concretely, `rv == 0x200` makes
 * `(rv & 0x200) != rv` FALSE where the original's `(rv & 0x200) != 0` is
 * TRUE, so this candidate skips a real early-return; `rv == 0xffff` at the
 * `-1` guard returns pointer-value 0xFFFF instead of `NULL`), and even so it
 * does not reach an exact match: `rv` lands in `$a1` there, not the target's
 * `$v1` (48 whole-image bytes still differ). Measured via a fresh permuter
 * run this round (see DISPROVED); the SOUND half of that candidate's delta
 * (a zero-valued `rv = 0;` sentinel standing in for the literal `0` in
 * guards 2/3's compare-and-return, providably still 0 there) is a complete
 * NO-OP alone (90 → 90, cc1 constant-propagates it away) — the entire
 * byte movement comes from the unsound half. A future attempt could look for
 * a SOUND way to extend some pseudo's live range/register class similarly
 * (global_alloc runs once over the whole function, so a hard-register
 * conflict anywhere can change what's live/eligible at guard 3's reorg
 * decision) without changing return semantics.
 *
 * DISPROVED here, measured, do not retry:
 *  - "reorg predicted the branch differently" — no. For guard 3 the condition
 *    is EQ, so `mostly_true_jump` already returns 0 and reorg ALREADY tries the
 *    fallthrough FIRST. It fails on the SEQUENCE, then the target-thread
 *    attempt succeeds. Prediction cannot be the discriminator.
 *  - The `do {} while (0)` at the END of the guard body (the LOOP_BEG
 *    prediction-flip that matched LoadTIMpack): measured 90 → 90, NO effect,
 *    and it is the WRONG DIRECTION — LOOP_BEG makes `mostly_true_jump` return
 *    2, which makes reorg try the TARGET thread first, i.e. exactly the steal
 *    we are trying to prevent.
 *  - A named `u8 vec` temp for `map.vector` (90 → 90; EXPAND_SUM, as above).
 *  - Ghidra's literal nested polarity: 67 differing bytes but a real LENGTH
 *    mismatch (it also changes how `Me_MOTION_C->status` CSEs across the two
 *    later reads).
 *  - `tools/autorules.py` (re-run fresh, still current): 6 candidates, none
 *    improving (u8/u32/s16 retypes of `rv`, `&&`-split, extern-array all
 *    >= baseline).
 *  - `tools/autorules.py --guided` (fresh): finds ONE mechanical win, a
 *    `paired-loop-fence` wrapping the call and the next 3 statements each in
 *    a bare `do{}while(0)` with no macro precedent (90 → 88). REJECTED per
 *    this round's owner steer (humanise, don't add byte-chasing fences) and
 *    per the cookbook's fence taxonomy — this is a bare fence propping up 2
 *    bytes with no macro and no clearer human structure behind it, case (c)
 *    "keep only if load-bearing AND nothing better", not licence to ADD one.
 *  - `tools/permute.py -- --stop-on-zero -j4`, ~34,000 iterations, flat at
 *    460: SUPERSEDED (that run predates today's `-fno-builtin` flag-list fix,
 *    which the orchestrator flagged as voiding every prior permuter
 *    negative). RE-RUN fresh this round (`timeout 240 ... -j4`, 27,968
 *    iterations before the bound): found ONE real improvement at the
 *    authoritative full-link rescore, `output-425-1` = 48/48/256 whole-image/
 *    window/length. REJECTED — see ENTANGLEMENT above: the candidate's win is
 *    entirely an unsound `return rv;` substitution (a real behavioural bug,
 *    not just inhuman style) that additionally still leaves `rv` in the wrong
 *    register (`$a1` vs target `$v1`). Do not re-adopt literally; the
 *    ENTANGLEMENT note above is what's worth re-trying, not this candidate.
 */
typedef struct
{
    u8 pad0[0x8];
    s16 attrib; /* 0x8 */
    u8 pad1[0x2];
    u8 vector; /* 0xC */
} AreaMapVectorResult;

extern Humanoid *Me_MOTION_C;
extern VECTOR *dtL;
extern void *GlobalAreaMap;
extern u16 motID;
extern u16 motMODE;
extern u16 RefrectVector[];
extern AreaMapVectorResult map;
extern s32 GetAreaMapVector(void *mvp, void *pos, s32 wide, s32 width, s32 flag);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/StickonCheck", StickonCheck);
#else
void *StickonCheck(void)
{
    u16 rv;

    if ((u16)Me_MOTION_C->type >= 2)
    {
        return 0;
    }
    if ((Me_MOTION_C->attrib & 0xC000) != 0)
    {
        return 0;
    }
    GetAreaMapVector(GlobalAreaMap, &map, (s32)dtL, Me_MOTION_C->width + 0x64, 5);
    if ((map.attrib & 0xC000) != 0)
    {
        return 0;
    }
    rv = RefrectVector[map.vector];
    if (Me_MOTION_C->status != 0xc && (rv & 0x200) != 0)
    {
        return 0;
    }
    if ((s16)rv == -1)
    {
        return 0;
    }
    if (Me_MOTION_C->status != 0xc)
    {
        motID = 0xc00;
        motMODE = 1;
    }
    return &map;
}
#endif
