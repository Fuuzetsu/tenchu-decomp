# Tenchu (PS1) decompilation — build plan

## Approach

Split the original `main.exe` into sections and per-function ASM with `splat`
(`split.py`), then progressively replace `INCLUDE_ASM` stubs with C that
recompiles to the same bytes. The build (Haskell **Shake**, `shake/src/Build.hs`,
which superseded the old `Makefile`) reassembles everything and gates on a
byte-identical `main.exe`.

## Status ✅

- **Round-trip byte-matches.** `./Build clean && ./Build check` reassembles a
  **byte-identical** `main.exe` (sha256 `0690a5c1…3558`) from a clean state.
- **Toolchain is complete for matching.** Pipeline is
  `cpp | cc1-281 -G8 | maspsx --aspsx-version=2.77 | as | ld`; reproducible/offline
  via nix (no cabal/wine). maspsx is integrated (see [`docs/toolchain.md`](docs/toolchain.md)).
- **Every game function is done (555/555)**; matched count is live in
  `tools/progress.py` (**537/555 game functions, 97.90% of game-code bytes** as of
  2026-07-20; **555/555 functions and 100% of bytes counting the 18
  canonical-asm originals**),
  whole-image byte-identical throughout (see `git log` for the
  per-function record; `tools/progress.py` for the live count). The engine is
  the matcher-agent pipeline ([`docs/orchestration.md`](docs/orchestration.md))
  driven **family-first**: map a subsystem's shared struct ONCE and clone/reuse
  across the whole family — `src/main.exe/item.h`
  (`PARAM_ITEM_USE`/`Humanoid`/`MotionManager`/`MotionRegistType`) and
  `src/main.exe/game_types.h` (the byte-proven `character_state`). Highest-yield
  seams landed: the `ProcItem*`/`ReqItem*` item TU, the CD/AFS file-access
  wrappers, the character-AI `Think*`/`think_setting_*`/`character_state`
  family, and small render/util helpers. **The clean seam** (non-jump-table,
  ≤~500-byte, non-dispatch functions) matches **4–5/5 per batch at ~65–250k
  tokens** on Sonnet; pick targets with `tools/triage.py` / `tools/findsimilar.py`.
- **Partial matches** (kept via the
  NON_MATCHING convention — default build stays green byte-identical, draft
  builds with `NON_MATCHING=<Name> ./Build`): **no game functions remain** as
  of 2026-07-19. `FUN_800519bc`, `AdtSelect`, and `mission_score_screen` are
  exact pure C; `FUN_8001c730` is classified as a canonical handwritten GTE
  helper. Remaining guarded drafts are stock SDK or canonical-assembly work.
  Older headers often describe residuals as proven sub-C floors,
  but repeated exact matches have falsified that conclusion. Those diagnostics
  describe one tested pseudo graph, not everything human C can express. Common
  residuals still include
  the named **`la` address-materialization tie** (`%hi` in a temp vs the target
  reg — `PrepareAccess`, `cd_open`, `PlayMusicFromID`, `FUN_8004a368`),
  goto-merge copy-chains (`turn_towards_player_`, `Think3chase`), and the
  big-handler flag/frame ties. The permuter is often immune to these late-pass
  decisions, so fresh compiler dumps, demo homologs, original types/macros, and
  a different human source identity take priority over local byte shaving.
  [`docs/matching-cookbook.md`](docs/matching-cookbook.md)
  records ~60 verified matching idioms. The early sessions pinned down the real
  gp model (ASPSX gp-addresses only TU-local definitions; externs are absolute)
  and produced the reusable infrastructure in
  [`docs/toolchain.md`](docs/toolchain.md): the opt-in `maspsx --gp-extern`
  patch + per-file lists in `Build.hs` (`maspsxGpExterns`), and the `macro.inc`
  `li→addiu` fix. The compiler is the canonical decompals `gcc-2.8.1-psx`,
  nix-pinned and verified equivalent to real Sony `CC1PSX.EXE` (decomp.me
  `psyq4.3`) — see `compiler-fidelity` in the toolchain doc.
- **Same-slot modding + emulator loop works:** `./Build mod` patches functions
  in place and rejects anything larger than the retail slot;
  `./Build iso`/`iso-mod` produce a bootable disc for pcsx-redux. The genuine
  size-changing lane is now `./Build relink`: complete SDK/data/BSS ownership,
  dynamic allocator capacity, regenerated PS-X headers, compiler-produced C
  small/common section retention, and a mandatory post-`ld`
  input audit are composed under `./Build relink`. Its exact 731 map-loaded
  objects contain 767 owned allocatable PROGBITS sections, 1,462 structurally
  reviewed MIPS metadata sections, 6,918/6,918 relocation-backed direct jumps,
  8,148 branches (7,090 same-section plus 1,058 `R_MIPS_PC16`), 2,494/2,494
  relocation-backed `$gp` address uses, 4,788 symbolic HI16 records, and 25,699
  alloc-data four-byte windows with 1,939 `R_MIPS_32`, with zero findings.
  Compiled data is scanned at every byte offset; owned alloc section types are
  checked and special canonical/header allowances are exact-site scoped.
  `./Build check-relink` reruns that gate and the final-image audit, then
  performs the full `+0x10004` GNU-ld growth proof.
  `./Build shiftability-report` composes those proofs into a single human/agent
  dashboard and separates active stale-address blockers from exact-source debt,
  intentional hardware/cross-executable contracts, and configurable RAM
  policy. Fixed contract/policy literals now have one C/Python/linker authority
  in `src/main.exe/ram_layout.h`; active game C and headers contain no duplicate
  fixed-address copies outside that authority.
  The exact grown image passes a bounded PCSX-Redux direct-load smoke and an
  auto-LBA `SLPS_019.01 → MENU.EXE → MAIN.EXE` boot to the moved entry and
  `PadProc`, with later VSyncs and no first-chance exception. The auto-packed
  image also reaches relocated `OPEN06.STR` decode and `STAGES.XA`
  setup/callback checkpoints. See
  [`docs/relocatable-build.md`](docs/relocatable-build.md).
- **decomp.dev progress reporting wired up:** `./Build report` /
  `tools/objdiff-report.py` emit a valid objdiff v2 report (verified against the
  real `report.proto` with protoc); `.github/workflows/report.yml` uploads it as
  the `jp_report` artifact. Build-free (reads the committed
  `config/functions.main.exe.tsv` + config), so CI is Python-only. decomp.dev is
  artifact-driven and still needs a GitHub repo (private OK with a self-hosted
  instance) — full setup in [`docs/decomp-dev.md`](docs/decomp-dev.md).

## Human-source matching — the standing lever (owner directive, 2026-07-18)

**The original was C a human wrote, and a byte-chased park is often a LOCAL OPTIMUM AWAY
FROM THAT SOURCE** — full of fences, split variables, dead-carrier reuse and seed temps
no person types, and those props are frequently exactly what pins the residual above 0.
So when a residual is stuck, WRITE THE FUNCTION THE WAY A HUMAN WOULD and refine from
there, expecting worse bytes at first. `tools/matcher-prompt.py` carries this as standing
guidance now.

Evidence it works, this session:
- **SetWire 70 -> 80 -> 0:** accepting the worse human draft exposed that both
  projection islands were the body of EFFECT.C's earlier GetScreenPosition.
  Reconstructing that debug-proven function as a local inline helper matched all
  1488 bytes and deleted the remaining store-order scramble.
- **mission_score_screen 145 -> 187 (adopted the WORSE number):** ~20 of its 24 fences
  turned out to be the author's own `DRAW_SCORE_NUMBER` macro (defined in matched sibling
  StageEndScreen.c), not our scaffolding. 866 lines vs 2076.
- **ActITEM 2 -> 0:** the two-byte park invented an `s32 item_type` and a redundant
  `flag = 0`, contradicting PSX.SYM's four-local record.  Restoring two ordinary
  valid-mode arms, each with the same `flag`/field writes, lets jump2 merge their
  tails after allocation.  That keeps the target `$a1` colour and removes the
  delay-slot instruction.  This directly falsified the function header's detailed
  “sub-C floor” proof.
- **AddEnemy 26 -> 0:** the 1,600-line park modelled caller saves with a scratch
  struct, volatile reloads, cursor joins, and deep zero-code fences. The demo's
  same-named 1,148-byte body instead exposed two ordinary sentinel scans and the
  author's compact local reuse. Rebuilding the complete 58-line shape made gcc
  emit both caller saves itself and matched all 1,152 retail bytes. It also
  falsified the blanket rule that every PSX.SYM block-local list must be reversed:
  this function matches only in the displayed order.
- **DecodeTMD family ~620 -> 0:** the whole breakthrough was recognising the hand-rolled
  `goto` loop as a WRONG FIX propping up damage it caused (it killed the loop-depth ref
  weighting), and the TU wanting `-fno-strength-reduce`. **FOUR of six now MATCHED:**
  FUN_80059ff4/FUN_8005a3cc AND the 1260-pair FUN_8005961c/FUN_80059b08 all -> **0**.
  FUN_80058c70/FUN_80059008 subsequently went **25 -> 0**. The single-statement
  reach proof was accurate only for that decomposition: one twin uses a dedicated
  `colorWord` instead of reusing a later GTE-address carrier; the other uses the
  real loop counter plus one two-set packet initializer. Both ordinary local
  decompositions reshape the quantity/birthing graph and match all 920 bytes.

The levers: PSX.SYM's `BEGIN PSX.SYM` block gives the authors' own declarations (names,
types and nested-block scopes; declaration order is ambiguous at group boundaries and
must be checked against the recorded hard-register homes); the matched sibling's
MACROS tell you which `do{}while(0)` clusters are legitimate human fences; and the target
is a source oracle (a const/copy def next to its uses = set once = birthing bump; a load
above a byte store = the human wrote the load first = QImode alias pin). SCOPE:
plausible original source first. The SDK (>=0x80060000, libgte/libgs/libapi) is
stock Sony library code, so it is not a bulk C-decomp queue. It is nevertheless
a dependency of the relocatable-build goal; prefer the exact original
relocatable member or canonical assembly when C adds no editing value.

### Batch 2026-07-18: a broad sweep found apparent sub-C floors

A 13-lane sweep across the whole value spectrum (DrawImpact 4, FUN_80057b80 8,
AdtSelect 9, SetupTelop 9, SetLightningI 15, CameraDirection 7, FUN_80058c70/9008
26, start_demo_ 39, PutItemList 27, PadProc 28, FUN_80036284 34, FUN_800519bc 87,
StageEndScreen 202) re-tested every park with the repaired tooling (regalloc
--local, the `-fno-builtin`-fixed permuter). Its immediate result was 0 full
matches and well-characterised ties in the structures tested. The follow-up
human-source pass then falsified most of the closest "floors": SetupTelop,
DrawImpact, CameraDirection, FUN_80057b80, FUN_80058c70, FUN_80059008,
SetLightningI, SetWire, DrawBleed, and ControlTraceLine are now exact. These
outcomes are the durable result: compiler diagnostics prove properties of a
pseudo graph, not that the original human decomposition cannot create another.
The observed tie taxonomy is documented in the cookbook — local-alloc
(conflict-free-window, containment, interference-wall), sched1 LUID wall, sched2
emission-order (prologue parm-copy, biv-init), dbr delay-slot, reload round-robin,
hard-conflict register renames, register-coloring cascade. Residual SIZE does not
indicate structural-vs-tie: even 87-byte FUN_800519bc is identical-CFG register
renames.

**What still moves, and what doesn't.** The `-fno-builtin` permuter fix is the ONE
lever that produced progress: it found StageEndScreen 202->199 (a human-plausible
named coordinate variable earlier rounds' buggy permuter missed). But on most ties
its wins are non-human seed-temp/no-op scaffolds (SetLightningI 12, FUN_80036284
16/12, FUN_80058c70 22) — all correctly REJECTED per the human-source directive; the
clean park is the honest state. The human-source discipline held in every lane.

The batch's original conclusion that the frontier had moved off "match more
parks" is withdrawn. The productive frontier was changing source identity at a
larger scale than the permuter searched: same-TU inline helpers, ordered local
copies of formals, direct control-flow tails, and purpose-specific reused locals.
DrawBleed and ControlTraceLine both matched after their bounded searches had
reported floors; the DecodeTMD twins matched after their one-statement proof;
FUN_80057b80 matched after a quantified signature proof. Continue to rank by
value and evidence, but treat every park as a falsifiable claim about one graph.

## Current resume point (2026-07-20)

The game-code matching queue is empty. Live output is 537/555 game functions in
exact C plus 18/555 canonical handwritten-assembly originals, or 555/555 and
100% of game-code bytes done. `tools/findsimilar.py --targets --by-value`
returns zero game candidates. `FUN_800519bc`, `AdtSelect`, and
`mission_score_screen` are exact C; `FUN_8001c730` is the eighteenth canonical
GTE assembly body. Do not restart the old matching flywheel from the historical
target lists below.

The normal-link shiftability queue is also empty in the current bounded proof:

- `./Build shiftability-report` reports zero active blockers and exact-vs-normal
  source debt `0/6`.
- `ActivateHumans`, `FileOption`, `SelectCameraOwnerOption`, and
  `ProcItemShinsoku` are ordinary exact symbolic C objects. Their reviewed
  globals carry real linker relocations.
- `vinit` and `valloc` use the same human-shaped C in exact and normal builds.
  The normal lane applies one bounded, fail-fast post-compiler rewrite from the
  retail pool and capacity LUI/ORI materialisations in each function to
  standard symbolic LUI/ADDIU pairs. There are no per-source
  `TENCHU_RELOCATABLE` branches,
  per-function compiler flags, or linker encoding aliases.
- `./Build check-relink` passes the complete input/final-image audits and a
  `+0x10004` ordinary GNU-ld growth proof. The proof moves BSS, allocator base
  and capacity, PS-X header entry/size, 208 loaded pointers, and four HI16
  carries with zero movable-address findings.
- `./Build check` remains retail byte-identical; `./Build report` succeeds;
  the full Python suite has 595 passing tests.

Resume shiftability work by running `./Build shiftability-report` first and
working only its `BLOCKERS` section. `DEBT` must remain zero; `CONTRACT` and
`POLICY` are reviewed boundaries, not work items. Normal verification permits
function sizes and offsets to change, but deliberately retains the reviewed
relocation counts and allocator-transform inventory. If a real edit changes
that topology, extend the contract with compiler/linker evidence instead of
weakening or bypassing it. The exact procedure and proof limits are in
`docs/relocatable-build.md`.

**Real-edit end-to-end proof landed (2026-07-20).** A genuine grown-function
edit — `PadProc` +2 instructions calling a brand-new `reloc/` TU with a new
`.sbss` counter and `.sdata` magic — passed `relink`, `check-relink` (with
the growth proof stacked on top), and image-verified PCSX-Redux boots on both
the direct and full `SLPS → MENU → MAIN` repacked-disc paths, with the new
code observed executing every frame (`watchCounter=1..10`,
`watchEquals=0x600df00d`). The run exposed and fixed a real displaced-image
bug (extension section LMA/VMA divergence) that every static gate had missed;
loaded-image congruence is now enforced by the generated script's pinned
extension address and a hard `psxexe.py` program-header check on every
finalized EXE. The smoke probe now image-verifies its breakpoints (immune to
MENU.EXE false positives) and supports `--watch-counter`/`--watch-equals`
memory watches. See `docs/relocatable-build.md` §“Real grown-function edit
proof”.

**Override modding lane landed (2026-07-20).** `src/mod-relink/main.exe/`
overrides any matched TU in the relink lane only: identical compile pipeline,
object substituted at the original link position, `./Build check` stays
byte-identical while the mod is present. The same PadProc+new-TU mod was
re-proven through the override lane (check green with mod present; direct and
full-chain smokes PASS with both watches; `check-relink` green). The
`verify-normal-link` SDK check now measures rigid-block displacement from its
own anchor instead of assuming only the six modeled objects can change size.
The proof is now a committed repeatable gate: `./Build check-relink-realedit`
replays the fixture (`tools/fixtures/relink-realedit/`) through the override
machinery in an isolated composition and verifies growth, the relocated
call, loaded data, a clean strict audit, and the emulator-observed counter
(`TENCHU_REALEDIT_NO_SMOKE=1` for disc-less environments).

**Gameplay runtime gate landed (2026-07-20).** `./Build check-relink-gameplay`
boots the auto-LBA-packed `+0x10004` grown image through the full retail
chain and, with image-word-verified breakpoints and JP-correct pad pulses
(CROSS cancels), requires a constructed stage, `ActivateHumans` dispatching
once per frame sustained (measured 3,600 over two emulated minutes), and
continuous `cbCheckCD` streaming (measured 7,241), with no exception.
PCSX-Redux Lua facts recorded in the probe: breakpoint/listener returns must
stay referenced (GC tears down the native side — this was a hard emulator
segfault) and callbacks must not throw (PCSX deletes the breakpoint).
Remaining runtime gates: opening-movie EOF (needs a MENU.EXE-side anchor —
the FMV plays before MAIN loads), physical audio, save/load, mission
completion, and exe transitions.

### Historical 2026-07-18 frontier

The snapshot below recorded the earlier 31-function frontier and is retained to
explain the source-identity breakthroughs. Its counts and target ranking are no
longer current.

**31 game functions remained after ActITEM.** `tools/findsimilar.py --targets`
printed this set (it defaults to `--scope game`):

    game code            507/555   functions (91.35%)   263272/303244  bytes (86.82%)
    game done (C+asm)    524/555   functions              (the 17 canonical-asm draw*)

  * **THE BYTES ARE IN THE BIG ONES — rank by SIZE, not by residual**
    (`tools/findsimilar.py --targets --by-value`). The counter is ALL-OR-NOTHING per
    function: a park at 97% exact contributes ZERO matched bytes, because the default
    build still links its stub. So the payoff is the function's FULL SIZE.

        6084  15.2%  StageEndScreen        (residual 199 — cluster B confirmed uncollectable)
        4636  26.9%  mission_score_screen  (residual 187 — HUMAN-STRUCTURE rewrite, see below)
        3796  36.4%  FUN_80057b80          (residual   8)
        2188  41.8%  start_demo_           (residual  75 — 96.6% exact)
        1448  49.2%  FUN_800519bc          (residual  87 — 94.0% exact)
        ...
          48 100.0%  FUN_8001b174

    **The top 4 are 42% of everything left; the bottom 10 are ~2%.** Chasing 4- and
    9-byte parks on 500-byte functions ranks by probability and ignores the prize.

    **But `--by-value` is the OPPOSITE error if taken alone: it assumes
    crackability.** The counter being all-or-nothing cuts both ways — a 6084-byte
    function pays 6084 at residual 0 and **NOTHING at residual 1**. So the real
    ranking is `size x P(crack)`, and neither pure ordering is it. Worked example, the
    day it was added: StageEndScreen's cluster 2 turned out to be ONE instruction
    (`addiu s7,zero,0x52`) emitted early, displacing 46 slots by +4 — 168 of its 202
    bytes — and it is unreachable, because the insn has `ref_count = 0` (nothing in
    the block consumes it) so `adjust_priority` is never called on it and the birthing
    bump cannot lift it off priority 1. **CONFIRMED 2026-07-18** (sched-deps: ref=0
    tool-verified; `.loop`: the div-magic is hoisted but current_x is NOT, so its
    low source-LUID makes sched1 emit it first — a genuine un-raisable sched1 LUID
    wall, not a scaffold artifact). So StageEndScreen's cluster B (168 of its bytes)
    is not collectable at any price; the prize ranking moves to `FUN_80057b80`
    (3796, residual 8 — also a proven sched2 wall) and the medium structural
    residuals. **Read the dominant cluster's mechanism before spending a round on
    size alone.**

    **Worked example of the human-source lever paying off (2026-07-18):
    `mission_score_screen` was rebuilt from scratch to the shape of its MATCHED
    sibling `StageEndScreen` — the `DRAW_SCORE_NUMBER` macro family, plain
    `topY`/`resultX` pre-loop variables (StageEnd's `top_y`/`current_x`/`best_x`),
    an s16 `stageItem` lh.** The banked byte-chased draft (169) had propped itself
    up with `drawY` carriers, fence-seeded x-groups, a brightness-alias scaffold,
    and 4 nested fences for the rankSprite flip. The rewrite is +18 bytes (187) but
    the function is guarded (INCLUDE_ASM ships the ORIGINAL bytes, so `./Build check`
    stays byte-identical GREEN — the raw image is unchanged), and it collapses the
    scaffolding to ONE labelled `do{}while(0)`. This is the directive in action:
    prefer the sibling-derived human shape that escapes the carrier/fence local
    minimum over a lower byte count built on invented constructs. **The reverse
    transfer to StageEndScreen was TESTED (2026-07-18) and FAILED**: its cluster-B
    constant `current_x` is a PERSISTENT s7 (loaded once, 5 reads = a sched1 LUID
    wall), NOT a REG_EQUIV rematerialiser like the sibling's `resultX` — the two
    were conflated. The lesson generalised into cookbook §4: verify the target's
    LOAD COUNT before assuming a cluster is the mission_score_screen case. (The
    permuter fix still bought StageEndScreen 202→199 on an UNRELATED cluster.)
  * **33 parked drafts** — each root-caused in its own `.c` header, closest at 1-10
    residual bytes. Residuals are overwhelmingly sub-C (allocation / scheduling /
    reload ties), which is why the tooling investment has overtaken target-picking as
    the lever. **Park prose is a hypothesis** — this week alone, GetAreaMapVector's
    "evidence-complete" park MATCHED once a dead store moved, AddEnemy's five-step
    unreachability proof fell to a tiebreak rule that was simply wrong, and
    SetupSpline's "permuter plateaued" was a crash. Re-check cheaply before honoring
    one (cookbook §4).
  * **The DecodeTMD primitive-renderer family (updated 2026-07-18)** — all six
    members now MATCH: FUN_8005961c/FUN_80059b08 (1260 each),
    FUN_80059ff4/FUN_8005a3cc (984 each), and FUN_80058c70/FUN_80059008
    (920 each). The final pair closed by replacing decompiler carrier reuse with
    coherent colour/counter/initializer identities; their former v0/v1 floor was
    decomposition-relative.
  * Also present but hidden by `--max-size 2048`: **`start_demo_`** (2188) and
    **`mission_score_screen`** (4636). Raise the flag or they are invisible.

**THE SDK IS NOT A BULK C-DECOMP TARGET (revised owner direction,
2026-07-19).** Everything at/above 0x80060000 is stock Sony library code —
libgte/libgs/libapi/libcard — linked in from prebuilt `.LIB` files. Tenchu's
original source tree never contained `libgte.c`, so an exact original `.OBJ`
member or canonical relocatable assembly is an honest source input. A C match
is still valuable when it is natural, complete-object evidence supports it, or
we want to edit that SDK routine; it is not a prerequisite for moving the game.
`progress.py` counts the SDK separately because the image contains it, but that
number is not by itself the work queue. See
[`docs/relocatable-build.md`](docs/relocatable-build.md).

*Why this needed writing down — the tooling actively pointed the wrong way.*
`--targets` ranks by similarity to already-matched code, which is a proxy for EASY, and
small simple SDK leaves dominate it: the top 50 was **41 SDK / 9 game**, while the five
undrafted game functions sat at **rank 367-382** (similarity 0.05, because they are
large and unique — a statement about resemblance, not value). Reading the top 15 of a
998-row list sent a whole session into libgs. The picker now defaults to game scope and
states every omission (SDK, handwritten-asm, over-size). The SDK functions matched on
that detour (`GsSetLsMatrix`, `_card_clear`, `GsMulCoord0/2/3`, `PAD_init`, `InitPAD`)
are byte-identical and green so they stay, and it did pay for itself twice — the
`-mno-split-addresses` original-object profile and the `OriginalObjectCcFlags`
oracle bug (a Build.hs defect that silently made compiler-profile experiments
measure stale objects). But it is not where effort goes.

**`TransMatrix` and `GsInitCoordinate2`** are parked SDK drafts; do not force
them through C. Prefer their exact original object members for the relocatable
lane, or retain symbolic assembly if those members cannot be proven. If their
original source form matters, settle it with complete-object/compiler evidence,
not a generic SDK story.

**The cookbook was restructured** (audit: `docs/cookbook-audit.md`): 8,251 unrouted
lines → `matching-cookbook.md` (1,483: contract, evidence, **router**, 19 families, park
discipline) + `compiler-facts.md` (410, every claim with a gcc `file:line`) +
`shape-zoo.md` (195) + generated `autorules-index.md`. A lane now reads ~600 routed
lines. `tools/cookbooklint.py` guards regrowth. The governing lesson: *every rule of the
form "read the dump, if X conclude Y" is a program* — cc1 prints its own decisions and
only tools get them read correctly. Tool backlog is ranked in the audit's §4.

New session: read this file + [`docs/README.md`](docs/README.md) (the docs index).

## What was actually wrong (fixed 2026-07-02)

The **build environment**, not the decomp logic:

- **Reproducibility hole (fixed).** The nix devShell shipped bare `pkgs.ghc` +
  `cabal-install`, so Shake's deps (shake/aeson/uuid/hashable) weren't in GHC's
  package DB. `./Build` ran `cabal v2-run`, needing a Hackage index + network to
  compile them into `~/.cabal`. A fresh checkout (or reset `~/.cabal`) broke the
  build with `unknown package: uuid`.
  → devShell now uses `haskellPackages.ghcWithPackages [shake aeson uuid hashable]`
  and the `Build` wrapper compiles `Build.hs` with `ghc` directly (no cabal).
  Proven to build **offline with `~/.cabal` absent**.
- **INCLUDE_ASM deps untracked (fixed).** `.c.o`/`.s.o` objects didn't depend on
  the `.include`'d nonmatching `.s` / `include/macro.inc` (only cpp `-MMD` headers
  were tracked), so editing an asm left a stale object → phantom byte-mismatch.
  → objects now assemble with `as --MD` and `need` the parsed+normalised includes.
  Verified: editing a nonmatching `.s` now re-assembles its object.
- **Asset `.bin.o` (fixed, dormant).** Rule did a raw `copyFile'`; the linker
  places assets as ELF `x.bin.o(.data)`, needing a real relocatable object.
  → now `ld -r -b binary` (matches the old Makefile). Fires once assets exist.
- **`check` hardened.** Now asserts the reference `disks/tenchu/main.exe` matches a
  pinned expected sha256, so a swapped/corrupt base image can't pass green.

## Original-source data recovered (2026-07-10)

The Japanese demo disc (PAPX-90029) shipped `PSX.SYM`, the Psy-Q linker's debug
output for an earlier, lost build. It is now parsed, dumped and mined — see
[`docs/psx-sym.md`](docs/psx-sym.md), which is the resume point for this whole thread.

- **Every game function is carved** (555/555), so `permute.py`/`m2c`/the sweep work on
  any of them. `tools/progress.py` is the live count (499/555 game functions when this
  was written).
- **Recovered from `PSX.SYM`**: 442 prototypes with the authors' parameter names, 2516
  locals (name, type, register-or-stack), 415 structs/unions/enums with real field
  names, the 31-file translation-unit map, 304 `static` declarations, 567 typed
  globals. All under `reference/psxsym-*`; `tools/matcher-prompt.py` injects the
  per-function facts into every agent launch.
- **45 function names and 156 data symbols adopted**, all byte-identical. Eleven replaced
  guesses with originals (`handle_char_state_using_item_` → `ActITEM`). 121 of the data
  symbols were already sitting unused in our own Ghidra export — `import_symbols.py`
  could only *rename*, never *define*, until now.
- **Recorded, not dropped**: `reference/psxsym-candidates.tsv` (suggestions too weak to
  adopt), `psxsym-unplaced.tsv` (46 demo functions with no retail home),
  `psxsym-unnamed.tsv` (78 retail placeholders with no candidate),
  `psxsym-data-candidates.tsv`.

Best remaining leads, roughly in value order:

1. **Match more functions.** Every batch of function renames unlocks more data symbols
   (`datamatch.py` can only see a global through a function named on both sides).
   Re-run `tools/datamatch.py` after each; it proposes zero today because it has
   harvested everything the current shared names reach. **Re-verified at 555/555
   (2026-07-20):** the full four-matcher sweep yielded one genuinely new name
   (`SsUtKeyOffV`, adopted green; four other proposals were stale-snapshot
   duplicates of already-adopted names), zero multi-vote data proposals, and 48
   recorded single-vote data candidates. The 73 remaining unnamed retail
   functions are structurally out of the demo's reach (canonical draw*/GTE
   bodies, DecodeTMD-family renderers, SDK leaves absent from the demo). The
   next real unlock is lead 2 below: naming MENU.EXE/ENDING.EXE creates new
   shared edges — the unplaced demo TUs (OPENING.C, MOJI.C, the
   `get_stream`/`strInit` STR family) live there, which is also where the
   opening-movie FMV player was traced during the runtime-gate work.
2. **`MENU.EXE` / `ENDING.EXE`.** The 61 unplaced demo functions — `OPENING.C`,
   `OPMOVIE.C`, `MOJI.C` — are presumably there; the demo was one monolithic
   `PSX.EXE`. The same four matchers apply. The build now carries all six
   executables (`./Build check-all`), each split to a single `data` blob and
   reassembled byte-identically, so a function can be carved out of any of them
   with the usual `reverse.py` workflow — see `docs/build-system.md`. The
   name-recovery matchers (`symmatch`/`xbuildnames`/`callmatch`/`datamatch`) still
   hardcode `main.exe`; they need a `--target` before they can be pointed at the
   others.

   `tools/xexe.py` already locates main.exe's functions inside the other five by
   normalized instruction identity (relocations masked). `trial.exe` -- the Mission
   Editor, a near-complete relink of the engine -- contains 891 of our 1322
   functions unambiguously, 582 of them in end-to-end runs (consecutive functions
   landing exactly back-to-back, which corroborates the boundaries), and 140 of
   them are functions we have already byte-matched. menu/ending/slps_019.01 carry
   ~600 each (engine core + the statically linked Sony SDK); run.exe only 150.

   That is a symbol-recovery path for exes that have *no* symbols at all: mint
   `config/symbols.<target>.txt` from `xexe.py --tsv`. Do it deliberately, not in
   bulk -- and note that a byte-*exact* transfer is rare (92 functions for
   trial.exe, only 4 of them ours), because the two builds place data at different
   addresses, so anything touching a global re-links differently. The C is what
   transfers, not the bytes.

   **Composed naming landed (2026-07-20):** `xexe.py --merged-tsv` copies our
   current adopted names first (splat + config overlays) and lets demo
   PSX.EXE names fill only unclaimed ranges, disagreements reported. The
   committed inventories are `reference/xexe-menu.exe.tsv`,
   `reference/xexe-ending.exe.tsv`, and `reference/xexe-trial.exe.tsv`,
   including the `St*` stream-ring API and `GsTMDfast*` renderers.
   `--place-by-calls` (callmatch-style, iterated to fixpoint with guarded
   containment) additionally placed the evolved OPMOVIE.C entry points —
   `open_stream`/`close_stream`/`strInit` in both MENU and ENDING (the
   movie-EOF gate's anchors) — while `Opening`/`PutMoji`/`strNext` remain
   honestly below the evidence threshold.

   **Decomp scaffolds landed (2026-07-20):** `tools/scaffold_exe.py` split
   `menu.exe`, `ending.exe`, and `trial.exe` into per-function INCLUDE_ASM
   stub TUs under the recovered names (FUN_ placeholders elsewhere), with
   byte-identical `./Build check-all` and per-exe decomp.dev categories from
   `config/functions.<name>.tsv`. See docs/build-system.md §the other five
   executables for the boundary/section/gp lessons baked into the tool.
3. **The `statics` list** (73 functions, 231 objects) should feed `tools/gpsyms.py`: a
   `static` never gets a `%gp` extern.
4. **The SLD stream** (per-instruction line deltas) and the `0x90`/`0x92` block markers
   are parsed and discarded. They would give an address→source-line map for the demo
   build.
5. ~~`StageBosses`/`StageEnemies` shift~~ **RESOLVED, names are correct.** Retail's
   `Think3callaid` loads `0x80097c76`/`0x80097c78` together and writes them back `+1`/`-1`
   — an adjacent pair used as a pair, exactly like the demo's `StageEnemies`/`StageCitizens`.
   Retail *inserted* a new short (`StageBosses`, a Ghidra name absent from PSX.SYM) at
   `…c74`. The datamatch vote that pointed `…c74` at `StageEnemies` was the artefact:
   `difflib` aligns the demo's store to the *first* identical retail store, which is
   exactly what an insertion looks like. **Reference-alignment votes are weakest at an
   insertion point** — corroborate with how the symbols are *used together*.

## Roadmap / TODO (not yet done)

Detailed dev docs live in [`docs/`](docs/). Ranked next steps:

0. **Choose the next product goal explicitly.** There is no active game-match or
   static shiftability blocker. For a real game-code change, build with
   `./Build relink`, gate with `./Build check-relink`, package with
   `./Build iso-relink`, and add any newly observed relocation topology to the
   reviewed contract. Do not create a new exact/normal source spelling.
1. **Broaden grown-image runtime validation.** Direct and full-chain boot, main
   loop, one STR decode, and XA setup/callback have passed. EOF, physical audio,
   broader gameplay, save/load, and later executable transitions remain release
   gates; see `docs/building-an-iso.md`.
2. **Recover/decompile the other executables.** `MENU.EXE`, `ENDING.EXE`, and
   `TRIAL.EXE` are the next genuine source-recovery frontier. Generalize the
   name-recovery tools' hard-coded `main.exe` target before bulk use; the
   PSX.SYM and cross-executable leads are recorded above.
3. **Treat PSY-Q provenance as optional SDK work, not a game-code queue.** Keep
   canonical relocatable assembly in the hermetic lane. Identify/link original
   `.OBJ`/`.LIB` members only when exact provenance or editability justifies the
   external SDK dependency; see `docs/psyq-object-lane.md`.
4. **Commit the disassembly only if the operator preference changes**: move
   splat's asm to a committed `asm/main.exe/` and
   set splat `base_path: .` so a fresh clone is self-contained.
   (NOTE: the operator prefers keeping `.shake/gen` regenerated-on-demand; only do
   this if that changes — [`docs/project-layout.md`](docs/project-layout.md).)
5. **CI**: add a GitHub Actions job running
   `nix develop --command ./Build check`, followed by the relocatable gates when
   their runtime is acceptable.
6. **Adopt the remaining persistent-blob option bytes.** `gNannido`
   (0x80010058, difficulty; demo `TLinkInfo.Nannido`) is adopted with a
   `game_difficulty` enum. The demo TLinkInfo field run implies retail
   0x5A = `SoundLevel` (CD volume, `FUN_8004f68c`/`_PlayMusic`), 0x5B =
   `SELevel` (`PlaySE` scale), 0x5D = `Anakon` (`PadShock` rumble gate),
   0x59/0x5C = `Stereo`/`fMemory` or `ZangyakuOff` (order shifted vs demo —
   verify against the options screen before adopting). Evidence in
   `reference/data-symbols-applied.tsv` and `game_types.h`'s
   `PersistentState`.

## Progress & the SDK endgame

`tools/progress.py` (add `--json` for machine-readable/frogress-shaped output)
reports matched functions/bytes split **game code vs SDK** — the split matters
because ~1000 of the 1623 functions above `0x80061000` are statically-linked
PSY-Q library code (LIBSND/LIBSPU/LIBCD/…), not game logic. The provisional
boundary lives in the tool; refine it as library identification improves.

**Full compilation is already a permanent property**: every unmatched function
is assembled from its extracted asm (INCLUDE_ASM), so the build links a
complete, byte-identical exe at every stage — matching only ever replaces
blobs with C. The realistic goal metric is **100% of game code DONE**, where
done = matched C **plus** the 18 handwritten-assembly originals
(`config/handwritten-asm.txt`, the draw*/DrawTMD renderers) whose asm IS the
faithful source (owner decision 2026-07-16 — see
[`docs/gte-policy.md`](docs/gte-policy.md); `tools/progress.py` reports the
combined `game done (C+asm)` line). For the SDK the options are:

1. **Use symbolic assembly** — a valid relocatable source form when references
   are labels/relocations rather than embedded absolute words, and the right
   representation for proven handwritten routines. **Implemented through the
   complete SDK text stream at `0x800601d4..0x80086764`:** 20 canonical objects
   carry 7,540 relocation records, remain retail-exact, and pass a `+4` linked
   proof. Loaded data now begins at the separate `75F64.data.s` input; there is
   no remaining raw `72CD0` instruction boundary.
2. **Link original PSY-Q objects**: convert SDK `.OBJ`/`.LIB` members with
   a pinned converter such as PCSX-Redux's `psyq-obj-parser` and let the
   linker use them instead of asm blobs. Needs a user-provided PSY-Q SDK
   archive (uncommitted, like the game disc). **Split this in two** — the
   halves have very different cost/benefit:
   - *Identification* (SDK + psyq-obj-parser + `tools/coddog compare-raw`):
     names/boundaries for the ~1021 lib functions and exact per-library
     version knowledge (games shipped MIXED lib versions). High value, zero
     build risk. Do this once SDK archives are on disk.
   - *Link-swap* (actually feeding the objects to ld): requires
     reconstructing the original member link order at our fixed addresses,
     yaml/linker-script restructuring, migrating ~1000 script-assigned
     symbols to object definitions. At retail addresses it comes out byte-for-
     byte the same; at new addresses its relocation records make the SDK code
     move coherently. The default matching lane must remain independent of the
     SDK, while an opt-in relocatable lane may accept `PSYQ_SDK`.
3. **Opportunistic decompilation of trivial clusters** — `tools/coddog
   cluster` found e.g. 108 byte-identical `dmyGsPrstF3NL`-style stubs; one C
   file per cluster is a cheap, large jump in the SDK numbers.
   **DONE for the `dmyGs*` cluster (2026-07-16):** all 108 warn-once stubs
   matched via anchor-then-clone (15 `N` no-light variants are 3-arg/`arg2`
   returns — see the .c headers). **BLOCKED for the CdFlush/CdSetDebug/
   ResetCallback wrapper clusters (24 fns):** their SDK objects came from a
   DIFFERENT compiler that leaves the sp-restore BEFORE `jr ra` with a bare
   nop delay slot — cc1-281 always reorgs it into the delay slot, so the shape
   is unreachable from any source (verified 6/32 bytes on every member).
   Their carves + correct-C NON_MATCHING drafts are preserved on branch
   `codex/sdk-wave2-epilogue-blocked`; do NOT spend agents re-attempting them
   with this toolchain. Per-library compilers differ (LIBGS `dmyGs*` matched
   fine), so check a cluster's epilogue shape FIRST. Unassessed clusters:
   `SetPolyF4` ×5 (looked cc1-compatible per the interrupted agent — true
   pointer-parameter leaves), `EVENT_OBJ_BC` ×8, `funcEvSpIOE` ×8, plus many
   2–4s. The game metric is now complete; pursue SDK C only where natural
   complete-object evidence makes it a clean editable-source win. Canonical
   assembly already handles bulk relocation; the external-object lane is an
   optional provenance/source-form improvement.

Recommended: keep (1) in the hermetic reference and normal-link builds; expand
the now-working `GS_107.OBJ` implementation of (2) only when better original
provenance is worth the external SDK dependency; use (3) for readability or
SDK routines we intend to modify. This follows MGS and Silent Hill precedent
and is backed by an exact-at-retail/links-at-new-address
`GS_107.OBJ` lane documented in `docs/psyq-object-lane.md` and
`docs/relocatable-build.md`. Progress
uploading (frogress / decomp.dev) can consume `tools/progress.py --json` from
CI later.

## Modding / non-matching builds

- **Same-size edits**: `./Build` (without `check`) produces a valid runnable
  binary; only the changed bytes differ.
- **Modified functions**: `./Build mod` — put the function in
  `src/mod/main.exe/<fn>.c` and it's compiled and **patched in place** (it must
  fit its original slot; mkmod aborts with the overage otherwise), so
  `main_mod.exe` stays the same size and the disc rebuild stays byte-faithful.
  See [`docs/modding-and-nonmatching.md`](docs/modding-and-nonmatching.md) and
  `tools/mkmod.py`.
- **Size-changing edits**: `./Build relink` is the normal GNU-ld artifact;
  `relink` itself runs the exact input-relocation/ownership gate immediately
  after `ld`; `check-relink` reruns it plus the zero-finding final-image audit
  and complete `+0x10004` growth proof. That proof mirrors the recursive
  user/generated extension-source inventory, including nested helpers.
  `run-relink` launches it directly, and
  `iso-relink`/`run-iso-relink` package and boot it through the real disc chain.
  `tools/pcsx_smoke.py` has passed both direct and auto-packed full-chain modes
  on that growth artifact, followed by representative STR decode and XA
  setup/callback checkpoints. EOF, physical audio output, and broad gameplay
  remain outstanding.

## Running in an emulator (`./Build iso`)

`./Build iso` rebuilds the game's CD image with our `main.exe` (via mkpsxiso,
packaged in `nix/mkpsxiso.nix`) → a `.bin`/`.cue` for pcsx-redux. Matching build →
data track byte-identical to the original except `main.exe`; `./Build iso-mod`
puts the same-size, in-place-patched `main_mod.exe` on the disc. `mkiso.py`
auto-packs a larger explicitly supplied executable; the controlled growth image
passes the full launcher/menu/main-loop smoke and representative STR/XA
checkpoints, while EOF, physical audio output, and broader gameplay remain
validation gates. Needs the original disc
(`TENCHU_CUE=…` or under `disks/`/`~/tenchu-iso/`). See
[`docs/building-an-iso.md`](docs/building-an-iso.md).

## Notes

- Generator oracle detects staleness by the *set* of generated file paths, not
  contents. Harmless now that the INCLUDE_ASM dep edge exists; do **not** "fix" it
  with hash-retrigger — that would revert hand-edits by re-running splat.
- Reproducibility depended on a warm `~/.cabal` before the nix fix; it no longer does.
