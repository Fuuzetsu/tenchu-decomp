# PSX.SYM — the original source's names and types

The Japanese demo disc (`Rittai Ninja Katsugeki Tenchu (Japan) (Demo)`, PAPX-90029,
1997-10-26) shipped `PSX.SYM` inside its AFS archive, alongside leftover build-pipeline
junk (`VOLMAKE.BAT`, `AFSMAKE.EXE`). It is the Psy-Q linker's debug output — the
authors' own source file names, function prototypes with parameter names, struct /
union / enum layouts with field names, line numbers, and the `static`-vs-`extern`
storage class of every symbol.

boricj found it and [wrote it up](https://boricj.net/tenchu1/2024/04/01/part-8.html);
our Ghidra symbols descend from his version-tracking of it. But that propagation was
lossy, and it only ever carried *names*. The types, prototypes and translation-unit
structure never reached us. They have now.

## Getting it

```console
$ tools/extract-demo.py ~/Downloads/PAPX90029.BIN -o disks/demo
```

Walks the MODE2/2352 ISO9660 tree, pulls `TENCHU.VOL`, walks its AFS directory
(36-byte big-endian records, `IX` signature) and carves `PSX.SYM` plus the demo
executables. `disks/` is gitignored; the extractor asserts
`sha1(PSX.SYM) = ffc15ac96e595a2c073529b368be8bcc53a52bcd`.

## Resuming from cold

Everything derived is committed under `reference/`. Only the inputs are not, because
they are copyrighted game data (`disks/` is gitignored). To rebuild the world:

```console
$ tools/extract-demo.py ~/Downloads/PAPX90029.BIN -o disks/demo   # PSX.SYM + PSX.EXE
$ tools/symdump.py                                                # -> reference/psxsym-*
```

`reference/demo-psxexe.functions.tsv` and `.labels.tsv` are the demo `PSX.EXE`'s
Ghidra names (boricj's PSX.SYM propagation), which the three matchers match *against*
and then re-filter through `PSX.SYM` for provenance. Regenerate them with:

```console
$ ghidra-analyzeHeadless <proj_dir> \
    'tenchu-decompile/Rittai Ninja Katsugeki - Tenchu (Japan) (Demo)/TENCHU.VOL/CDIMAGE' \
    -process 'PSX.EXE' -noanalysis -readOnly -scriptPath tools/ghidra \
    -postScript ExportFunctionsLabels.java \
    reference/demo-psxexe.functions.tsv reference/demo-psxexe.labels.tsv
```

The matchers also read `.shake/ghidra-export/functions.tsv` (see
[ghidra-bridge.md](ghidra-bridge.md)) and `disks/tenchu/main.exe`. The Ghidra
TSV remains authoritative for function boundaries and sizes, but its names are
a snapshot. `tools/function_inventory.py` overlays the current named `c`
subsegments from `config/splat.main.exe.yaml` by address. Without that overlay,
an already-adopted name still looks like `FUN_…`, named callees disappear from
call signatures, and the generated unnamed/unplaced inventories become stale.

Then, to look for more names:

```console
$ tools/symmatch.py --candidates reference/psxsym-candidates.tsv \
                    --unplaced   reference/psxsym-unplaced.tsv \
                    --left       reference/psxsym-unnamed.tsv
$ tools/xbuildnames.py --apply /tmp/x.tsv
$ tools/callmatch.py   --apply /tmp/c.tsv
$ tools/callmatch.py   --verify /tmp/x.tsv          # ALWAYS, before adopting
$ tools/import_symbols.py --renames /tmp/x.tsv      # rebuilds + gates on ./Build check
$ tools/datamatch.py --apply /tmp/d.tsv             # re-run after ANY function renames
```

`import_symbols.py --renames` both renames existing symbols and **defines new ones**
(rewriting splat's `D_800BC108` auto-label across `src/`, the gp-extern lists in
`Build.hs`/`permute.py`, and the yaml), then gates on a byte-identical `./Build check`.

## Reading it

`tools/psxsym.py` parses the `MND` format: an 8-byte header then a stream of
`u32 offset ; u8 tag ; payload`. The tag table is in the module docstring. It parses
the whole 1.48 MB to EOF with **zero unknown tags** and reproduces the 1097 functions
Ghidra shows in `PSX.SYM.elf` — that agreement is the correctness check.

Contents: **1696 plain symbols**, **442 functions with full debug info** across
**31 source files**, **60894 type definitions**.

```console
$ tools/symdump.py            # regenerates everything under reference/
```

| file | what |
|---|---|
| `reference/psxsym-types.h` | 415 aggregates + 289 typedefs, original field names |
| `reference/psxsym-protos.h` | 442 prototypes, 308 with the authors' parameter names |
| `reference/psxsym-tu-map.tsv` | function → source file, line, frame size, saved-reg mask |
| `reference/psxsym-data.tsv` | 567 data symbols with their original types |
| `reference/psxsym-locals.tsv` | **2516 parameters and locals** across 394 functions — name, type, and where the demo build put each one |
| `reference/psxsym-statics.tsv` | 304 `static` declarations (73 functions, 231 objects) |

### Locals

The richest seam, and the last one opened. `C_REG` records a local's **register
number** (`16` = `$s0`), `C_AUTO` its **frame-pointer-relative offset** — and `$fp`
is `$sp`, so the real slot is `sp + fsize + offset`. `valloc`'s 1 KB debug buffer
lands at `sp+40`, just under the saved registers at `fsize-8`. Checks out.

```
DrawSprite  3DCTRL.C  param  $s1      struct Sprite3D *sprt
DrawSprite  3DCTRL.C  stack  sp+16    struct MATRIX mat
DrawSprite  3DCTRL.C  reg    $s1      struct ModelType *objp
DrawSprite  3DCTRL.C  stack  sp+48    short rxy[2]
```

The demo build's *register allocation* need not survive into retail, but the
**number of locals and their types** are exactly what drive cc1's codegen, and those
do. A name repeated inside one function is a distinct nested-block scope, not a
duplicate. `tools/matcher-prompt.py` prints them for the target function.

### Source-line spans

`psxsym-tu-map.tsv` carries each function's start line, and its end line **when that
can be trusted**. The `0x8E` end-of-function record is not reliable everywhere: in
`THINK_2.C` and `THINK_4.C` every function claims to end at the file's last line,
overlapping its successors. So a span is only reported when it is consistent with
where the next function in the same file starts, and — for the last function in a
file, which nothing bounds — when the implied code density is at least 2 bytes per
line (verified spans sit at a median of 13.3). 431 of 442 survive; median 24 source
lines, p90 83. Useful as a sanity prior on how much C a body should be.

## Putting the facts in front of the code

`tools/matcher-prompt.py` injects the prototype, TU, storage class, locals and
touched globals into every matcher-agent launch prompt. But the facts belong next to
the code too — a human reading a parked `NON_MATCHING` file, or an agent that opens a
`.c` without a prompt, should see what the authors wrote.

```console
$ tools/symnote.py --write --all     # stamp/refresh every src/main.exe/*.c
$ tools/symnote.py --check --all     # non-zero if any block is stale (CI)
```

It inserts one `BEGIN PSX.SYM … END PSX.SYM` comment after the last `#include`,
regenerated in place and idempotent. **434 of 557 files carry one**; the other 123 are
functions PSX.SYM never described. Comments change no bytes, so `./Build check` is the
gate.

Global ownership is extent-based, not "nearest typed name". The generated globals
header records each original object's byte size (with a bare pointer forced to the
stored four-byte pointer size, not COFF's referent size). `symnote.py` attributes an
interior reference only when it falls inside that extent, and a meaningful retail
symbol boundary still splits it. This matters in both directions: a generated
`D_<addr>` can legitimately be an interior byte of `PadPort`, while the Adt state at
`0x8008f1b8` must not be claimed by the preceding 40-byte `StageAppearance` table.
Earlier-build extents are deliberately conservative; if retail enlarged an object,
the note may omit a true edge rather than invent a false one.

```c
/* BEGIN PSX.SYM — the original source's own facts …
 * short DrawSprite(struct Sprite3D *sprt);
 *     3DCTRL.C:593, 14 src lines, frame 72 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals …
 *     param $s1       struct Sprite3D * sprt
 *     stack sp+16     struct MATRIX mat
 *     reg   $s1       struct ModelType * objp
 *     stack sp+48     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */
```

A `FUN_…` file gets its recorded candidate name instead, with the warning not to adopt
it without `callmatch.py --verify`.

### How much to trust it

Measured across the first agent batches that had it. **Struct layouts are reliable;
local lists are a strong prior that retail sometimes overwrote.**

| what | verdict |
|---|---|
| Struct/union layouts and field names | Reliable. `POLY_GT4` gave `SetupImageToPolyGT4` every offset in the `.s`, first try. The `TAFS`/`TAFSElement`/`TAFSFileHandle` layouts carried two AFS functions with no debug info of their own. |
| Prototypes and parameter names | Reliable. |
| **Local count and types** | The single biggest lever *when right* — `FileRead`'s exact 2-local list is what matched it. But retail can replace them when an API changes; verify access widths and callees. |
| **Array bounds in globals** | Suspect. PSX.SYM says `LifeBar[4]`; retail's asm plainly walks `LifeBar[5]`. |

Measured failures share one cause — the demo is an *earlier build*:

* `SetupImageToPolyGT4`: PSX.SYM lists 3 locals (`tx`, `ty`, `th`). Retail rewrote the
  function into `SetupImageToPolyFT4`'s shape and needs 10.
* `LoadFromDEVPC`: PSX.SYM records 1 local. Retail's asm plainly needs `fd`, `size`,
  `buff`.
* `AntiWall`: the demo records `SVECTOR` outputs for `ApplyMatrixSV`; retail calls
  `ApplyRotMatrix`, whose 32-bit stores prove three `VECTOR` locals instead.

So: transcribe the local list first, then **check it against the `.s` before you trust
the count**. `tools/access.py` settles any field offset or width. When they disagree,
the asm wins — every time.

### Adopting the original parameter names

```console
$ tools/symnote.py --params           # worklist: our names vs the originals
$ tools/symnote.py --rename-params    # adopt them (code only), then ./Build check
```

A parameter rename cannot change codegen, so `./Build check` is a hard gate. The guard
is *capture*: the tool refuses when the original name already means something else in
that file. 8 of 11 were adopted (`GetVectorRotation(from,to)` → `(start,end)`,
`vinit(addr)` → `(adr)`, `AttackPQD(arg0,arg1)` → `(sfrm,efrm)`); 3 are blocked —
`AddMisc` wants `a,b,c`, `GetAreaMapLevel` wants `area`, `PathFileRead` wants `path`,
each already taken by a local. Renaming the local first would unblock them.

It rewrites **code only**. If a file's prose comment used the old name, fix it by hand
— `--params` shows you exactly which functions changed.

**PSX.SYM addresses are not retail addresses.** It describes an *earlier, lost
build* — its `.text` starts at `0x8001387c`, the demo's own `PSX.EXE` at
`0x8001389c`, and a constant `+0x20` shift only reconciles 85% of the functions.
Names and types transfer to retail; addresses do not. There is one narrower and
useful relationship: PSX.SYM's **data** addresses map to the demo `PSX.EXE` data
addresses by a rigid relocation which `datamatch.py` infers and calibrates below.
That still does not map either earlier build directly to retail.

## What survives the build gap

Two structural facts do:

* the linker emits each translation unit **contiguously**, and
* within a TU the compiler emits functions in **source order**.

Retail `main.exe` reproduces the demo's 31 TUs as 36 contiguous blocks. That is what
makes name recovery possible at all.

## The three matchers

Each is independent, each measures its own precision against a control set of
functions already named on both sides. Never trust one alone — see the worked failure
below.

| tool | signal | control precision | good for |
|---|---|---|---|
| `tools/symmatch.py` | TU + source order, Needleman-Wunsch, scored on frame size / saved-reg mask / code size | frame+mask agree on only **63%** of anchors | game code inside the 31 debug TUs |
| `tools/xbuildnames.py` | normalized instruction identity vs demo `PSX.EXE` | **96.6%** (357 control pairs) | library / support code, unedited bodies |
| `tools/callmatch.py` | multiset of named `jal` targets | **98.0%** (99 control pairs) | anything whose body was rewritten |

`xbuildnames` masks link-time immediates (`%hi`/`%lo`, gp and stack offsets, `j`/`jal`
targets) and keeps PC-relative branch offsets, which survive relinking. Its known
failure mode is functions differing *only* in a masked immediate: it happily equates
`SpuRead` and `SpuWrite`. Uniqueness on both sides plus the PSX.SYM provenance filter
keeps that out of the proposals, but do not lower those guards.

### Naming the other executables (no decompilation required)

`tools/xexe.py --target menu.exe --merged-tsv reference/xexe-menu.exe.tsv`
composes two sources for a symbol-less executable, with strict precedence:
every main.exe function found by normalized identity is copied under **our
current adopted name** (splat C names and `config/symbols` overlaid onto the
Ghidra snapshot), and demo `PSX.EXE` names only fill target ranges no main.exe
function claims. Same-span name disagreements are printed for review, never
resolved silently — the residual list is exactly the `SpuRead`/`SpuWrite`
class plus boricj-propagation variants. Demo rows carry a
`psxsym`/`ghidra` provenance column; `DEAD_*`/`FUN_*`/`LAB_*` demo rows are
discarded as information-free.

The committed tables (`reference/xexe-menu.exe.tsv`, 636 rows;
`reference/xexe-ending.exe.tsv`, 654 rows) name the shared engine/SDK core
plus demo-only helpers such as the `St*` CD-stream ring API and the
`GsTMDfast*` renderers.

Evolved bodies that resist identity go through `--place-by-calls`, a
`callmatch`-style pass run after composition: candidate function starts come
from the target's own `jal` graph (extents capped at the next known start;
data-word false starts self-filter below the three-distinct-named-callees
threshold), demo callee multisets are restricted to names the target side
already knows, and the search iterates to a fixpoint so each accepted
placement widens the vocabulary — the OPMOVIE.C family names its own
siblings. Two tiers: `calls-exact` is callmatch's unique-multiset rule;
`calls-contain` additionally bounds extra calls and size gap, rejects
Pareto-dominated fits, and rejects candidates that a second demo signature
also fits (the historical `SetCDVolume`/`InitSoundEffect` coin flip is
dropped as contested, not guessed). This placed `open_stream`,
`close_stream`, and `strInit` in both MENU.EXE and ENDING.EXE — the
OPMOVIE.C movie-player entry points the runtime movie gate needs — plus
`ComPad`/`SetupSoundEffect` in ENDING.EXE and a solid `InitGraphicsSystem`
containment in both. `Opening`/`PutMoji`/`strNext` remain unplaced: their
remaining callees stay below the evidence threshold, honest ground for a
future pass once more anchors exist.

### Verify before you adopt

```console
$ tools/callmatch.py --verify reference/psxsym-applied.tsv
```

For each proposed name it checks that the demo function's named callees are
**contained in** the retail function's (containment, not equality — retail functions
grow). This is not theoretical: `symmatch` proposed `SetPadState` for `0x80032610`
because the frame size *and* the saved-register mask both matched. The callees said
otherwise —

```
retail 0x80032610 calls: GsGetWorkBase, GsSetWorkBase, SetDrawMove, AddPrim
demo   UpdateTexScroll : GsGetWorkBase, GsSetWorkBase, SetDrawMove, AddPrim
demo   SetPadState     : GsSortSprite x5
```

— and the name was wrong. Frame/mask agreement is weak evidence on its own.
Call signature outranks frame shape, but is still not sufficient by itself.
Animation/item callbacks can share the same small callee multiset. A later
machine-code audit showed that the historical `AttackFire` adoption was such a
collision: the demo function launches napalm over a frame range, which matches
retail `0x80027730`, while `0x8001f6b8` launches a lightning bolt on one frame.
The names are now corrected: the former is `AttackFire`, while the latter has
returned to the honest retail-only name `handle_char_state_attacking_SEVEN_`.
Verify signature, parameters, constants, and semantics together before adopting
a name.

A **named callback setter can bridge a callback that was completely rewritten**
between builds. `DrawImpact` is the worked case: the demo's `SetImpact` stores
the relocated address whose low half is the demo `DrawImpact` address (`0x2480`),
while retail's exact `SetImpact` stores `0x80033f10`. The retail callback then
consumes the impact fields that setter initialises, projects the point, sorts the
sprite, advances its lifetime, and clears the slot. `callmatch --verify` reports
0% callee coverage because the old callback used `rand`, `UpdateCoordinate`, and
`DrawSprite` while retail replaced that implementation; here that rejection is
expected, not contrary evidence. Require both the same named setter/parameter
contract and a producer-consumer field/lifecycle fit before using this signal —
a callback-address store by itself is not enough.

The same producer/consumer proof can justify a **retail-only semantic pair**
when no earlier-build name exists. `SetSnow` is called only by the already-named
`ProcMiscSnowfall`, fills one particle's position/velocity/ground/sprite fields,
and installs `DrawSnow`; that callback consumes precisely those fields, wraps
the particle around the camera, projects the snow sprite, and ends it at ground
level. The two names were unused and follow the surrounding `Set*` -> `Draw*`
convention. Require an exclusive named caller, exact field/lifecycle agreement,
and an unused conventional pair—mere thematic similarity is not enough.

`callmatch --verify` now blocks this mechanically as `AMBIGUOUS`: after full
containment it searches for another retail function that contains every demo
callee, has no more extra named calls, and is at least as close in code size.
That conservative Pareto test exposed the historical `AttackFire` row:
`0x80027730` was the strictly better competing fit. Prototype/local evidence
then completed the correction, and the adjacent gun callback at `0x800274e8`
was independently identified as `AttackGunControl` from its two-short contract,
gun item kind, and weapon-object role.

## What we adopted

**48 names**, all rebuilt byte-identical, listed in `reference/psxsym-applied.tsv`.
Ten of them replaced *guessed* names with the originals, and the guesses corroborate
the recovery:

| our guess | original |
|---|---|
| `handle_char_state_using_item_` | `ActITEM` |
| `handle_char_state_posing_` | `ActACTION` |
| `think_setting_weapon_logic1_` | `Think3area` |
| `load_sprite_info_perhaps` | `SetupImageToPolyFT4` |
| `change_clear_pad_init_card_` | `InitCARD` |
| `DebugMenuItemSet` | `AddItem2` |
| `PlayMusicFromID` | `PlayMusicFormID` (original typo) |
| `think_setting_go_towards_player` | `Think2contact` |
| `handle_char_state_dead_` | `ActDEAD` |
| `debug_menu_select_stage` | `SelectStage` |

## What we did NOT get, and where it went

Three lists are regenerated by `tools/symmatch.py --candidates --unplaced --left`:

* **`reference/psxsym-candidates.tsv`** — 5 names the TU alignment can still suggest
  after current repository names are overlaid. Every remaining row is `LOW`:
  positional evidence only. `ViewAdjustBG`, `SetGore`, the old positional
  `GetFreeItemSlot`, `RequestItem`, and `SetPadState` candidates have all produced
  false positives in audits. The placeholder/current descriptive name stays until
  independent signature and semantic evidence identifies the function.

* **`reference/psxsym-unplaced.tsv`** — 44 functions PSX.SYM knows that we could not
  locate. Ten each in `OPENING.C` and `OPMOVIE.C` and six in `MOJI.C` are almost
  certainly *not in `main.exe` at all* (the demo was one monolithic `PSX.EXE`; retail
  split menu/opening/ending into `MENU.EXE` and `ENDING.EXE`). The `strInit`/`strNext`/
  `open_stream` cluster is Psy-Q `libpress` streaming. The rest — `DrawImpact`,
  `PutMoji*` and `QuakeCamera` are real game code and
  should be findable; they are the best remaining targets.

* **`reference/psxsym-unnamed.tsv`** — 73 retail placeholders with no candidate at
  all. Many are in the `0x8005xxxx` library region (no debug info in the SYM),
  the rest are game code added after the demo.

## Data symbols

Ordering does not transfer for data — the segment moved far more than `.text` did.
What transfers is **who touches what**. `tools/datamatch.py` decodes the data
references of every function we have named on *both* sides (`lui`/`%lo` pairs and
`$gp`-relative accesses), aligns the two instruction streams with `difflib` so
inserted/removed instructions don't shift the correspondence, and zips the reference
sequences. Each zip is one vote that retail address X is the demo's symbol Y; a
global touched by several known functions collects several independent votes.

The committed demo Ghidra label export is lossy and used to make this stage look
exhausted. PSX.SYM itself lists many data names that the export omitted, and the old
loader also collapsed multiple labels at one address into one dictionary value.
`datamatch.py` now recovers those mechanically:

1. Build name-to-address sets on both sides, retaining aliases.
2. Use only names unique in both images to infer `demo_addr - psxsym_addr`.
3. Require at least 64 anchors and 99% agreement before doing anything. The current
   inputs give **323/323 (100%) at `+0x358`**.
4. Synthesize the omitted demo label/address pairs from that calibrated relocation.
5. Drop linker-boundary aliases and compiler spellings such as `vector.17`.
   A duplicated meaningful static such as `CID` is usable only when the referencing
   function's original PSX.SYM translation unit selects the same definition.

The relocation supplies a name on the **demo** side only. Retail addresses still
come exclusively from aligned references inside independently named function pairs.
For adoption, unanimity and reverse uniqueness are checked against every raw vote,
not merely the already-unanimous subset. `--apply` refuses to write a table unless
the relocation calibration clears its thresholds, at least 64 existing-name
controls survive, and control precision is exactly 100%.

A name is adopted only when every vote for X agrees, at least two *distinct*
functions witness it, and no other raw-vote address claims the name. Generated
tables record the PSX.SYM address, demo address, and complete witness-function list,
so each decision remains auditable.

Control precision — retail addresses whose current name is itself a PSX.SYM name —
is **100% (172/172)** after the calibrated-relocation batch and the independently
corroborated `StageAppearance` rename. Note that the two
apparent counter-examples in an earlier run
were not errors: our `Me_MOTION_C` is the de-duplicated spelling of the demo's static
`Me`, and `StageBosses`/`StageEnemies` at `0x80097c74`/`0x80097c76` disagree with the
demo's single `StageEnemies` short — the tool correctly refused to propose, because the
name was already taken. **That one has since been chased down: our names are right.**
Retail's `Think3callaid` reads `0x80097c76`/`0x80097c78` together and writes them back
`+1`/`-1`, an adjacent pair used as a pair, exactly like the demo's
`StageEnemies`/`StageCitizens`; retail simply inserted a new short at `…c74`.

That is worth internalising: **a reference-alignment vote is weakest at an insertion
point**, because `difflib` aligns the demo's store to the *first* identical retail
store. When a vote contradicts an existing name, check how the symbols are *used
together* before believing it.

`reference/data-symbols-applied.tsv` now records **229 global/data naming
decisions**:

* **35 from `datamatch`** — `struct ConflictObjectType ConflictObject[64]` (21 votes),
  `struct WeaponType WeaponDB[28]`, `short RefrectVector[16]`, `long AttackActionCount`,
  plus Psy-Q internals like `GsOUT_PACKET_P` (26 votes) and `_svm_*`.
* **71 from calibrated-relocation `datamatch`** — 63 previously unnamed globals and
  eight replacements for descriptive repository names. Highlights include
  `ItemImage` (18 witnesses), `motMODE` (28), `CameraTarget`, `VoiceMode`,
  `SyurikenModel`, `ArrowModel`, `NingyoModel`, `sprNapalm`, `sprNapalm2`, and SDK
  internals such as `GsTON` (33). Every row retains its PSX.SYM address, demo
  address, and witness functions.
* **121 from the Ghidra export** — already named in the Ghidra program for *this*
  binary (`PersistentState`, `MusicTable`, `CdReset`, `GsA4divTNG4`…) but never
  adopted, because `import_symbols.py` could only *rename* existing entries. It can
  now define new ones. This historical batch also included 85 address-keyed SDK/text
  labels below retail's data start, so the table's record count is not itself a
  count of real data-region globals.
* **Two independently corroborated names** — the original `StageAppearance` has
  one aligned witness plus its exact `short *[10]` type and per-stage indexing in
  both `AddEnemy` and `SetupCharacterParameter`; the retail-only
  `AdtMessageBoxCount` has a same-binary Ghidra label and is incremented and printed
  only by `AdtMessageBox`.

Across all sources, the current symbol configuration has **211 retail data-region
names that also occur in PSX.SYM**, up from 139 before relocation synthesis.

`reference/psxsym-globals.h` joins the 113 globals we name with PSX.SYM's original
type, and `tools/matcher-prompt.py` lists the ones a target function touches:

```
- Globals it touches, with the original declaration:
    `extern short ConflictObjects;`                          /* 0x800976ec */
    `extern struct ConflictObjectType ConflictObject[64];`   /* 0x800bc108 */
    `extern struct tag_TItem items[30];`                     /* 0x800bfbb0 */
```

48 single-vote suggestions are parked in `reference/psxsym-data-candidates.tsv`.
One witness can come from a misaligned instruction block; two independent functions
rarely agree by accident.

## Still on the table

* **The rest of the 567 globals.** 210 PSX.SYM names now have retail data-region
  addresses; the remainder are touched only by
  functions we have not yet named on both sides, so naming more *functions* directly
  unlocks more *data*. The two loops feed each other — re-run `datamatch.py` after
  every batch of function renames.
* **Statics.** 73 functions and 231 objects the original build declared `static`.
  A `static` never gets a `%gp` extern and changes how the symbol is emitted; this list
  should feed `tools/gpsyms.py`.
* **The SLD stream.** The parser uses the per-function line records but still
  discards the per-instruction line deltas (`0x80`/`0x82`/`0x84`/`0x86`). The raw
  records were manually decoded for `SetupSpline`: ACTION.C:349-358 exposed the
  statement boundaries and ordering that turned a 12-byte register-only residual
  into an exact 240-byte match. Exposing the address→source-line map in a tool would
  make that evidence routinely available instead of requiring a one-off decoder.
* **Block structure.** `0x90`/`0x92` block start/end records (with line numbers) give
  the nesting depth of every function. Parsed and skipped today.
* **`MENU.EXE` / `ENDING.EXE`.** The unplaced `OPENING.C`/`OPMOVIE.C`/`MOJI.C` functions
  are presumably there — the demo was one monolithic `PSX.EXE`. Same matchers apply.

## What else the disc holds

Nothing else in `PSX.SYM` is unread: it parses to EOF with zero unknown tags, and the
only record classes we discard are the SLD deltas and block markers above.

On the disc itself:

* **`VOLMAKE.BAT`** (embedded in `TENCHU.VOL`) names the stage and enemy assets:
  `ma_akin`→`akindo`, `ma_cat`→`castle`, `ma_jyou1`→`jouka`, `ma_tan`→`tanren`,
  `ma_tem`→`temple`, `ma_yami`→`yamijou`, and enemies `rounin`, `rouban`, `echigoya`,
  `hanbe`, `jochu`, `rat`, `cat`, `dog`. Useful for naming stage/enemy tables.
* **The AFS directory preserves the original asset tree** — `K:\WORK\CDIMAGE\` with
  `ANIM`, `DEMO`, `HUMAN`, `WEAPON`, `IMAGE`, `SOUND`, `STAGE`, and per-stage
  directories `AKINDO CASTLE CAVE CAVE2 JOUKA JOUKA2 TANREN TEMPLE YAMIJOU`. 351 files:
  `.TMD` models, `.MAD`/`.AMD` animation, `.TIM`/`.TPD` textures, `.VAB` sound banks,
  `.CAD`/`.ESD` per-stage data, `.ACM`/`.CON`/`.POS` cutscene data.
* **`AFSMAKE.EXE`** (PE + DOS stub) and `$RES_UP.BAT` are the authors' build tools.
* The original tree lived at `J:\DEVELOP\PS\tenchu\tomo\NINJA_OP\`.

**The retail disc has none of this.** Its `DATA.VOL` holds 1048 files and the only
`.TXT` entries are the memory-card description strings (`CARD_J.TXT` etc.); no `.BAT`,
no `.EXE`, no `.SYM`. The demo is the single source.
