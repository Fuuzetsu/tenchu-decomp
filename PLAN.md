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
- **Every game function is carved (555/555)**; matched count is live in
  `tools/progress.py` (**534/555 game functions, 95.64% of game-code bytes** as of
  2026-07-19; **551/555 functions and 97.67% of bytes counting the 17
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
  builds with `NON_MATCHING=<Name> ./Build`): only **four game functions remain**
  (2026-07-19): `mission_score_screen`, `FUN_800519bc`, `AdtSelect`, and
  `FUN_8001c730`. Older headers often describe residuals as proven sub-C floors,
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
- **Modding + emulator loop works:** `./Build mod` (grow functions) and
  `./Build iso`/`iso-mod` (bootable disc for pcsx-redux). See docs/.
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
above a byte store = the human wrote the load first = QImode alias pin). SCOPE: main.exe
game code only — the SDK (>=0x80060000, libgte/libgs/libapi) is stock Sony library code
and NOT a target.

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

## Where the work actually is now (2026-07-19)

Fresh `tools/progress.py` and `tools/findsimilar.py --targets --by-value` leave
exactly four game-code functions:

| Function | Size | Authoritative current draft |
|---|---:|---|
| `mission_score_screen` | 4636 | exact length; 32 differing bytes / 14 instructions; ordinary shared score-drawing macro structure is under active reconstruction |
| `FUN_800519bc` | 1448 | exact length; 76 differing bytes / 28 instructions in seven clusters; demo homolog confirms an ordinary renderer loop |
| `AdtSelect` | 776 | exact length; nine differing bytes in one huge-frame reload decision; clean indexed count loop and ordinary D-pad control flow |
| `FUN_8001c730` | 220 | 212-byte draft; 36 aligned lines in ten blocks; compiler evidence identifies ordinary Hermite evaluation plus GTE matrix packing |

All four are active in the current four-slot flywheel. Compiler output and
shipping/demo bytes remain the gate. Cookbook rules and old “unreachable”
claims are hypotheses, especially when they were derived from a scaffolded
local minimum.

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

**THE SDK IS NOT A TARGET (owner decision, 2026-07-17).** Everything at/above
0x80060000 is stock Sony library code — libgte/libgs/libapi/libcard — linked in from
prebuilt `.LIB` files. **Tenchu's original source tree never contained `libgte.c`**, so
for those objects the ASM IS the faithful source form, exactly the argument already
accepted for the `draw*` family. Decompiling PsyQ is a different project and ones
already exist. `progress.py` still counts the SDK separately (116/1068) because the
image contains it; that number is not a work queue.

*Why this needed writing down — the tooling actively pointed the wrong way.*
`--targets` ranks by similarity to already-matched code, which is a proxy for EASY, and
small simple SDK leaves dominate it: the top 50 was **41 SDK / 9 game**, while the five
undrafted game functions sat at **rank 367-382** (similarity 0.05, because they are
large and unique — a statement about resemblance, not value). Reading the top 15 of a
998-row list sent a whole session into libgs. The picker now defaults to game scope and
states every omission (SDK, handwritten-asm, over-size). The SDK functions matched on
that detour (`GsSetLsMatrix`, `_card_clear`, `GsMulCoord0/2/3`, `PAD_init`, `InitPAD`)
are byte-identical and green so they stay, and it did pay for itself twice — the
`-mno-split-addresses` per-TU lever and the `ccExtraFlags` oracle bug (a Build.hs defect
that silently made per-TU flag experiments measure stale objects). But it is not where
effort goes.

**`TransMatrix` and `GsInitCoordinate2`** are parked SDK drafts; leave them. The
"libgte is handwritten asm" idea is a hypothesis with one data point and is moot now
that the SDK is out of scope — if it ever matters, settle it with a survey that applies
the tells mechanically and counts, not with a story.

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
   harvested everything the current 997 shared names reach.
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

0. **The `Act*` character-action family — one coherent batch. TU NOW
   ESTABLISHED.** `ActSYURI` (676 B) and `ActHANG` (700 B, jump table) are
   MATCHED (commit pending) — these proved the shared TU facts, so the remaining
   11 (`ActNORMAL`, `ActMOVE`, `ActSQUAT`, `ActACTION`, `ActITEM`, `ActATTACK`,
   `ActENGAGE`, `ActCHASE`, `ActDAMAGE`, `ActSWIM`, `ActSTICKON`, `ActSTATE`) are
   now much cheaper — Sonnet-viable for the mid-size ones, Fable only for
   `ActATTACK` (6.6 KB, the single biggest unmatched function in the game).
   Established facts:
   - **gp smalls** are MOTION.C's own: `dtM, Me_MOTION_C, motID, D_80097F0E`
     (+ `dtV, dtL, dtPAD` as used per function; `tools/gpsyms.py <Name>` after a
     build). **`StagePlayer` and `GlobalAreaMap` stay ABSOLUTE, not gp.** Add
     every sibling's Build.hs + permute.py entry together and `tools/symcheck.py`
     after each, or a converted stub silently relocates the shared data region.
   - **Structs — item.h is already complete for this family**: `MotionManager`
     (`mid`@0/`count`@2/`loop`@4), `Humanoid` (`attribute`@4, `pad.trig`@0x14,
     `model`@0x58), `ModelArchiveType.object`@0x68. PSX.SYM's `PARAM_ITEM_LAUNCH`
     == item.h's `PARAM_ITEM_USE`.
   - **Idioms** (see cookbook): jump-table dispatch on `mid` is
     `switch ((short)(ptr->mid - 0xA00))`; `D_80097F0E = 1;` is written literally
     per if/else arm (never hoisted); load-width is per-SITE not per-TU; case
     bodies lay in source order (read the order off piece memory order).
   - `ActSTICKON` and `DamageControl` additionally divide by a variable (target
     asm has `div`+`break` guards), so they need `extra "<Name>" =
     ["--expand-div"]` in Build.hs and permute.py — see the cookbook's
     `--expand-div` rule. `HumanActionControl` (the dispatcher) is drafted with
     the full list; use it plus the two matched siblings as templates.

1. **Reverse more functions.** The pipeline is now proven end-to-end on a
   gp-using function (`Think1sleep`). Use `tools/reverse.py <name> --ghidra-export
   .shake/ghidra-export` to split the next one, turn Ghidra's C in the comment into
   matching C (its return type + accumulator *types* matter — `Think1sleep` needed
   `short`/`ushort`, not `s32`), and lean on `tools/permute.py` for residual
   register allocation. `.shake/ghidra-export/c/<addr>.c` is the reference.
2. **decomp-permuter is wired in** (`tools/permute.py`) — see
   [`docs/permuter.md`](docs/permuter.md). (Note: for pure register-allocation
   ties it can stall; the faster path is often reading Ghidra's C for the right
   *variable types/count*, as with `Think1sleep`'s single `ushort` accumulator.)
3. **Commit the disassembly**: move splat's asm to a committed `asm/main.exe/` and
   set splat `base_path: .` so a fresh clone is self-contained.
   (NOTE: the operator prefers keeping `.shake/gen` regenerated-on-demand; only do
   this if that changes — [`docs/project-layout.md`](docs/project-layout.md).)
4. ~~Pin `tools/cc1-281`~~ **DONE** — nix `fetchurl` of decompals/old-gcc 0.17
   `gcc-2.8.1-psx` (sha256-pinned, `cc1-281` on PATH); the committed binary is
   gone (it was a wrong build — see [`docs/toolchain.md`](docs/toolchain.md)).
5. **CI**: a GitHub Actions job running `nix develop --command ./Build check`.
6. **Per-function tooling**: `diff_settings.py` (asm-differ is in the devShell),
   `objdiff.json`, an `m2ctx.py` context generator, a `make <obj>` shim.
7. **Growth-mod streaming safety** (if `iso-mod` movies/XA glitch): keep `main.exe`
   same-size on disc by relocating the mod region into a separate disc file rather
   than appending — see the caveat in [`docs/building-an-iso.md`](docs/building-an-iso.md).

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
done = matched C **plus** the 17 handwritten-assembly originals
(`config/handwritten-asm.txt`, the draw*/DrawTMD renderers) whose asm IS the
faithful source (owner decision 2026-07-16 — see
[`docs/gte-policy.md`](docs/gte-policy.md); `tools/progress.py` reports the
combined `game done (C+asm)` line). For
the SDK the options (in scene-standard order) are:

1. **Leave it as INCLUDE_ASM** — shippable "source + vendored asm" forever.
2. **Link original PSY-Q objects**: convert SDK `.OBJ`/`.LIB` members with
   `psyq-obj-parser` (ships with pcsx-redux, already checked out) and let the
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
     symbols to object definitions — and the built exe comes out byte-for-
     byte THE SAME as today (the bytes are already in via INCLUDE_ASM). It
     buys provenance cleanliness only; a permanent bring-your-own-SDK tax on
     contributors/CI. Defer until "no blobs" matters as a property.
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
   2–4s. SDK lanes are paused while the focus is main.exe game code
   (owner directive 2026-07-16).

Recommended: (1) now, chase the game-code metric, revisit (2) if a "no blobs"
build is ever wanted; (3) whenever a quick win is nice. Progress uploading
(frogress / decomp.dev) can consume `tools/progress.py --json` from CI later.

## Modding / non-matching builds

- **Same-size edits**: `./Build` (without `check`) produces a valid runnable
  binary; only the changed bytes differ.
- **Modified functions**: `./Build mod` — put the function in
  `src/mod/main.exe/<fn>.c` and it's compiled and **patched in place** (it must
  fit its original slot; mkmod aborts with the overage otherwise), so
  `main_mod.exe` stays the same size and the disc rebuild stays byte-faithful.
  See [`docs/modding-and-nonmatching.md`](docs/modding-and-nonmatching.md) and
  `tools/mkmod.py`.

## Running in an emulator (`./Build iso`)

`./Build iso` rebuilds the game's CD image with our `main.exe` (via mkpsxiso,
packaged in `nix/mkpsxiso.nix`) → a `.bin`/`.cue` for pcsx-redux. Matching build →
data track byte-identical to the original except `main.exe`; `./Build iso-mod`
puts the grown `main_mod.exe` on the disc (auto-LBA). Needs the original disc
(`TENCHU_CUE=…` or under `disks/`/`~/tenchu-iso/`). See
[`docs/building-an-iso.md`](docs/building-an-iso.md).

## Notes

- Generator oracle detects staleness by the *set* of generated file paths, not
  contents. Harmless now that the INCLUDE_ASM dep edge exists; do **not** "fix" it
  with hash-retrigger — that would revert hand-edits by re-running splat.
- Reproducibility depended on a warm `~/.cabal` before the nix fix; it no longer does.
