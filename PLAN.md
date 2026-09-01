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
  of 2026-07-19. `briefing_screen_`, `AdtSelect`, and `mission_score_screen` are
  exact pure C; `eval_spline_gte_` is classified as a canonical handwritten GTE
  helper. Remaining guarded drafts are stock SDK or canonical-assembly work.
  Older headers often describe residuals as proven sub-C floors,
  but repeated exact matches have falsified that conclusion. Those diagnostics
  describe one tested pseudo graph, not everything human C can express. Common
  residuals still include
  the named **`la` address-materialization tie** (`%hi` in a temp vs the target
  reg — `PrepareAccess`, `cd_open`, `PlayMusicFromID`, `spare_item_slot_`),
  goto-merge copy-chains (`GotoPosition`, `Think3chase`), and the
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
  fast_tnf3_/fast_tng3_ AND the 1260-pair fast_tng4_/fast_tnf4_ all -> **0**.
  adiv_tng4_/adiv_tnf4_ subsequently went **25 -> 0**. The single-statement
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

A 13-lane sweep across the whole value spectrum (DrawImpact 4, subdivide_quad_ 8,
AdtSelect 9, SetupTelop 9, SetLightningI 15, CameraDirection 7, adiv_tng4_/9008
26, game_over_screen_ 39, PutItemList 27, PadProc 28, draw_fade_ 34, briefing_screen_ 87,
StageEndScreen 202) re-tested every park with the repaired tooling (regalloc
--local, the `-fno-builtin`-fixed permuter). Its immediate result was 0 full
matches and well-characterised ties in the structures tested. The follow-up
human-source pass then falsified most of the closest "floors": SetupTelop,
DrawImpact, CameraDirection, subdivide_quad_, adiv_tng4_, adiv_tnf4_,
SetLightningI, SetWire, DrawBleed, and ControlTraceLine are now exact. These
outcomes are the durable result: compiler diagnostics prove properties of a
pseudo graph, not that the original human decomposition cannot create another.
The observed tie taxonomy is documented in the cookbook — local-alloc
(conflict-free-window, containment, interference-wall), sched1 LUID wall, sched2
emission-order (prologue parm-copy, biv-init), dbr delay-slot, reload round-robin,
hard-conflict register renames, register-coloring cascade. Residual SIZE does not
indicate structural-vs-tie: even 87-byte briefing_screen_ is identical-CFG register
renames.

**What still moves, and what doesn't.** The `-fno-builtin` permuter fix is the ONE
lever that produced progress: it found StageEndScreen 202->199 (a human-plausible
named coordinate variable earlier rounds' buggy permuter missed). But on most ties
its wins are non-human seed-temp/no-op scaffolds (SetLightningI 12, draw_fade_
16/12, adiv_tng4_ 22) — all correctly REJECTED per the human-source directive; the
clean park is the honest state. The human-source discipline held in every lane.

The batch's original conclusion that the frontier had moved off "match more
parks" is withdrawn. The productive frontier was changing source identity at a
larger scale than the permuter searched: same-TU inline helpers, ordered local
copies of formals, direct control-flow tails, and purpose-specific reused locals.
DrawBleed and ControlTraceLine both matched after their bounded searches had
reported floors; the DecodeTMD twins matched after their one-statement proof;
subdivide_quad_ matched after a quantified signature proof. Continue to rank by
value and evidence, but treat every park as a falsifiable claim about one graph.

## Humanising pass — the active loop (2026-08-27)

All game code is matched; the active work is rewriting the matched C into
what the original developers plausibly wrote — while every function stays
byte-identical (`./Build check` green gates every commit).

Workflow (resumable — a fresh session continues from here):

1. `python3 tools/humanscan.py` ranks sources by machine-decomp artifacts
   in CODE (comments stripped): Ghidra-style locals, generic temps,
   `FUN_`/`D_` symbols, goto labels, raw offset casts. `--dumps` lists
   matched files still carrying stale Ghidra/m2c/triage reference dumps
   (matched files drop them — the style of ReqItemFire.c et al.).
2. Per file, top of the ranking first: restore official recovered names
   (PSX.SYM blocks in the file, `reference/psxsym-*`), convert offset-cast
   arithmetic to real struct fields, replace magic numbers with named
   constants, keep the PSX.SYM fact block + matching-notes prose header.
   Renames are byte-neutral; struct-field/control-flow changes must be
   re-verified (`tools/matchdiff.py <Name>` → MATCH, then `./Build check`).
   The matching notes in each file record which shapes are load-bearing
   (fences, split locals, statement order) — do not "clean" those away;
   if a note explains a construct, it stays until a byte-identical
   human alternative is proven.
3. Commit each file (or small family) separately; then re-run the scan.

State (2026-08-27): stale dumps stripped from all 23 matched carriers;
`DamageControl` humanised (labels/locals/format); the whole TMD renderer
family humanised — the fast cluster fully struct-typed (`TMD_FAST_WORK`,
`src/main.exe/tmdfast.h`), the subdivision cluster's `subdivide_quad_`
rewritten on `ADIV_VERT`/`ADIV_FRAME`/`ADIV_WORK`, and the entry
renderers annotated (their index spelling is byte-required — see the
struct-store scheduling rule added to cookbook 3.13).
MILESTONE (2026-08-27): shipped code now contains ZERO Ghidra-style
locals, ZERO param_N parameters, and ZERO D_ data placeholders. Every
data global referenced from matched C is named — by recovered demo
name where one exists (Packet), by content for strings/vector
constants (msg_*/fmt_*/str_*/path_*/vec_*/svec_*, literal quoted at
each extern), by uniform scheme for the 108 dmyGs stub pairs
(str_dmy*/warn_dmy*), and by role read from the code for state/tables
(McardState cluster, ThinkBudget cluster, CamPos*, ADIV/TMD_FAST
workspaces, ...). All of it byte-identical, ~35 commits.

COMPLETED after the milestone (2026-08-27, ~50 commits total):
- The motion/status/type vocabulary: the Act* handler table at
  0x80086b24 proved motID's high byte AND Humanoid.status index the
  same 18 official handlers — MOT_*/STAT_* overlays adopted, the old
  guessed status names retired; Humanoid.type literals use the
  character_kind roster names plus the new character_page (type&0xf0)
  overlay (PAGE_CIVILIAN drives FriendHits, NINKEN kills don't score).
- Every matched TU now opens with honest prose; no headerless files.
- The rename wave is propagated through every name-matching consumer:
  Build.hs maspsxGpExterns, permute.py, reloc-data.main.exe.json, the
  reloc tools and tests. Gates: ./Build check byte-identical,
  ./Build check-relink exit 0, python suite 644 tests OK.
- INCIDENT (resolved, lessons in docs/toolchain.md): three commits
  briefly landed with a pipe-masked failing check — symbol-name
  clobbers, stale gp-extern lists, and an IsVisible parameter-shadow
  bug. All repaired in 2d957d7c; check gating now runs unpiped.

STRUCTURAL CAMPAIGN (2026-08-27, second wave, ~12 commits): the
decompiler-shaped control flow is now either restored or byte-proven
authentic. Restored as the switches they were: ActSTICKON (three
dispatches), ActSQUAT, DrawGore, plus CameraType1's sixteen
break-in-disguise gotos, AttackGeneral's range chain, DrawGore's
level/bounce ladders, GetTargetDistance's angle wrap, and every
OT-priority clamp in the draw family (ten instances, nine files).
Byte-proven authentic (restructure changes bytes — reverted, do not
re-attempt): AttackShort/AttackLong's status-7 result carrier,
DamageControl's damage-scaling ladder (retail contains the redundant
re-test), ActNORMAL's ==0-first command ifs, ActENGAGE's join ladder,
and EVERY do{}while(0) fence (bulk audit, cookbook 3.10). The
fold-identical spelling sweep (x + -1, reversed compares, hex char
escapes) and the scanner's extended machine-local pattern closed the
class out: zero machine locals, temps, or placeholder names remain in
shipped code, and cookbook 3.1 records the ladder-is-a-switch rule.

FINAL WAVE (2026-08-27, user-directed "keep going until done"):
1. DONE — all 74 FUN_ functions carry descriptive names in the
   invented-name convention (snake_case + trailing underscore; the
   address in each file header and reference/psxsym-unnamed.tsv keep
   the placeholder lineage for future official recovery). 163 files
   updated across src/configs/tools/docs; full-rebuild byte-identical.
2. DONE — MODEL_ATTR_CONFLICT (ModelType.attribute bit 15) named from
   bulletproof evidence, and the runtime lane RAN: tools/
   pcsx_attrbits.py (new; rebuilt the stale pcsx-redux checkout to do
   it) observed 332 attribute/status transitions in a live retail
   mission and settled ATTR_WEAPON_DRAWN (0x40, raised at the alarm on
   fighters and civilians, clears with EmergencyNotice — confirming
   the stealth-kill scoring gate) and ATTR_SUSPEND (0x80, the
   ActivateHumans think-budget suspension flag). humanoid.h's bit map
   records the full observed evidence; the remaining player-side
   transients (0x100/0x400/0x800/0x1000/0x2000/0x4000) are logged but
   not yet nameable — rerun the observer with scenario-specific
   steering if they ever matter.
3. DONE — the whole tree is normalized to the humanised style
   (pinned in src/main.exe/.clang-format; ColumnLimit 0 keeps re-runs
   token-preserving; ram_layout.h is machine-parsed and excluded via
   .clang-format-ignore).
4. The remaining scanner hits are byte-required spellings documented
   in place (the ADIV entry renderers' index stores, the subdivider's
   leaf-emit casts, AttackBowControl's byte-addressed table) and
   guarded drafts, which keep their reference dumps by design.

The humanising effort is COMPLETE: shipped code contains no machine
names, no placeholder symbols, no undocumented compiler-shaped
structure, one formatting style, prose on every TU, and every
attribute bit either named from runtime observation or logged with
its observed behavior — byte-identical throughout, ~70 commits.

GOTO-TAIL GRIND (2026-08-27, user-confirmed continuation): the
remaining goto/hex classes triaged to the floor, ~8 more commits.
Structured away (all byte-identical): the break-in-disguise sweep
(10 files); mission_score_screen fully de-gotoed (21 gotos — nine
copy-pasted sign-test shapes with matcher constant-ifs, one label
loop); StageEndScreen's DRAW_SCORE_NUMBER/DRAW_LAST_SCORE_NUMBER
macros now goto-free (clear_first_ flag replaces jump_/label_, 14
call sites); Think1random's reset condition; cbCheckCD's shared
tail (duplicated, cross-jump re-merges); DrawSplash's clamp;
ActJUMP half_count; ActACTION cleanup guard; ActKAGI rope diamond;
Think3attack's add_attack + briefing_screen_'s no-op goto; raw
ALERT/SUSPEND bits named at nine sites (~ATTR_x folds identically).
Proven authentic and documented in cookbook §3 (do not re-attempt):
hand-labelled loops (StageEndScreen layout scan measured +7 insns
structured — same shape in CVAupdate/AfsGetEntry/PutStrain/
RestoreItemLayout/SetBleeds*/DrawShadow/vmemoryGC), CVA-scan
double-breaks, the ReqItem* search-with-fallback family (35 files),
cross-case shared tails (ActATTACK/ActACTION/SearchTarget/
check_cheat_command_), ActDEAD's range dispatch
(constant changes), briefing_screen_'s brightness ladder (island
layout unreachable by structured chains). The residual ~650 gotos
across 170 files are these authentic classes. Remaining hex
attribute bits (0x10, 0x100..0x8000) stay until runtime evidence
names them (rerun tools/pcsx_attrbits.py with targeted steering).

VOCABULARY HUNT (2026-08-28, "would a human write this?" loop, ~20
commits): pad buttons carry the official PsyQ libetc names tree-wide
(ComPad.c's byte order proves the PadRead layout; include/psxsdk/
libetc.h); the AI's canned Command[] tags are CMD_DASH_*/CMD_ROLL_*/
CMD_LUNGE (decoded from the retail table); the cheat inputs are
CHEAT_* (item cap/refill/unlock, revive, debug menu); wpatk decodes
via the HumanData roster's own name strings (WEP_MEIOU, the HANBE/TUZI
twin-katana bosses, WEP_ONININ, WEP_BEAST; high nibble = the Attack*
range class); Humanoid attribute bits ATTR_FALL/WALL/HIT/PUSH named
from the collision resolver; terrain bits MAP_WATER/DEATH/SLOPE_X/Z;
conflict-slot classes CONFLICT_*; MAX_ITEMS/MAX_ENEMIES;
SYSFLAG_DEBUG_SELECT (dead-in-retail latch, documented). Constructs:
memset sizes spell sizeof, x++/comparison/null-cast/paren sweeps,
seven //-form Ghidra dumps stripped, redundant double casts dropped,
ten biased switches unbiased (two byte-required, documented), the
function-cast mystery re-investigated against retail AND demo bytes
(cross-jump blocker, not jalr — cookbook corrected), and
SnapCameraTargetVector's scratch casts demo-verified as the original
author's own idiom. Every remaining humanscan row is a documented
byte-required spelling.

HUMANISING LOOP 2 (2026-08-28, ~35 commits): names decoded from the
game's OWN data wherever a table carries the truth — the stage uids
from StageConfig's title strings (STAGE_TRAINING..STAGE_FREE_PRINCESS),
the HumanData roster names behind wpatk (WEP_MEIOU/WEP_TWIN_KATANA/
WEP_ONININ/WEP_BEAST + the >>4 range class), the Command[] input
streams (CMD_DASH_*/CMD_ROLL_*/CMD_LUNGE), the cheat inputs (CHEAT_*),
official libetc pad names tree-wide (ComPad.c's byte order proves the
PadRead layout), MISC_/PROCESS_/ARC_/IMG_/THINK_MIX_/MUSIC_/CMODE_AIM
vocabularies, MAX_ITEMS/MAX_ENEMIES, ATTR_/MAP_/CONFLICT_ bit families,
STAT_/PAGE_/character_kind coverage completed, item[ITEM_N] flag.
Behavior-renamed inventions: launch_lightning_bolt_,
spread_blood_pool_, draw_visible_characters_, load/swap_balma_area_map_,
think_alarm_reaction_, proc_misc_bonfire_, BLOOD_POOL_MODEL_,
TANKA_SPRITES_. Construct work: DamageControl's damage cascade is case
fallthrough; ten biased switches unbiased (Act handlers dispatch on
real motion ids; two byte-required, mechanism-documented); range
tricks unfolded where operands allow; redundant casts dropped; the
function-cast and twin-arm constructs pinned against gcc 2.8.1's own
jump.c/loop.c; the scratchpad casts and label loops proven original
(lui+ori vs displacement; delay-slot fill prediction). Officials from
the demo's PSX.SYM were never renamed — confusing ones carry
clarifying comments at their definitions instead.

HUMANISING LOOP 3 (2026-08-28, the continuous "& X | X and casts"
audit): every remaining semantic constant is now named or documented —
the AI awareness phases (ATTR_PHASE + PHASE_CALM/SUSPICIOUS/ALERT/
INVESTIGATE, written from SearchTarget's SR), ATTR_SEARCH/FLOAT/
NOFLOOR/LEDGE/WALLANGLE from setter/reader hunts (no runtime needed),
the MODEL_ATTR_ draw/cull family + COLLIDE, MAP_WOOD identified from
the stage ACM data itself (tools/voldump.py, committed — parses
AFS_VOL_200/IX and STAGE.ACM), official PsyQ CdlStat/Cdl-command/
CdlMode/SS_SERIAL constants, SPR_TRANS_ADD/SUB blends, TMD_BANK_
PLAIN/FOG renderer banks, pad.data holds, think mixes, CMODE_AIM.
Measured-and-documented: Humanoid.attribute's s16-with-u16-views mix
is retail's own per-site choice (the u16 flip changes plain lh sites).
Deliberately left: SE ids (no
evidence), per-proc mode counters, ACM base-material
bits 1/2 and the reader-less 0x200/0x2000. (Motion ids were later
named on owner request — see MOTION-ID NAMING below; tuning numbers
became tuning.h knobs in loop 10.)

HUMANISING LOOP 4 (2026-08-28, self-directed): the CVA script grammar
(CVA_CMD_SEQUENCE header rows carrying the CD track, CVA_CMD_WAIT
frame markers), full official-vocabulary harvests from the demo debug
symbols' anonymous enums (MUSIC_ decoded retail's MusicIDTable —
start_demo_ was really the game-over screen and is now
game_over_screen_; MaxImpacts; ITEM_SYSFLAG divergence note) and a
struct-field audit that verified every repo/demo field diff is a
documented retail redesign. The game's OWN debug data named more:
ThinkDB labels every think program (TRACE/WATCH/RANDOM/NINJA/SLEEP/
CHASE..., verified against Think*Func addresses; 0x2222 is
THINK_MIX_PAD2 "PAD 2"), the debug language menu yields LANG_ENGLISH..
LANG_JAPANESE, the camera editor's own r1/r2/p1/p2 strings prove
TCameraPos's field names, and the item picker contributes flavor
labels ("the world", "rikimarukochan"). Ghidra-era screaming
placeholders renamed (EFFECT_CURSOR_, CamPos, DEBUG_CAMERA_*/
DEBUG_PAD_*, CHEAT_COMMANDS_/PAD_HISTORY_/check_cheat_command_,
ARMOUR_EQUIPPED_, is_humanoid_on_stage_) — lesson relearned: renames
must also update the NAME-KEYED maspsxGpExterns lists in
shake/src/Build.hs. BattleType/WeaponType/AIDHumanType annotated from
retail data (parry-stun's conflict-slot cross-index quirk documented).

HUMANISING LOOP 5 (2026-08-28, continued self-iteration): the AI layer
returns synthesized PAD words, so its literals are buttons — spelled
across 15 files (scan turn PADLright, approach PADLup, Square attacks,
the 0xA000 turn-only filter; byte-required negative spellings kept and
cross-noted; cookbook rule added). ATTR_TRACE (bit 8, the patrol-route
flag found via TracePoint's per-waypoint button overlay) COMPLETES the
Humanoid.attribute map. Input archaeology finished: both hidden debug
sequences and all seven pause/briefing combo streams decoded
(newest-first storage; header = result code), CHEAT_QUIT/CHEAT_ARMOUR
and CMD_LUNGE_BACK/CMD_FLIP named from their consumers. The humanoid
skeleton's pinned part indices mapped at ModelArchiveType (head 2,
ONININ pair 8/0xb, hands 0xd/0xe) and MotionManager.mask decoded as
the per-bone animation mask. More vocabularies closed: WEP_KATAOKA
(the boss's matchlock), PAGE_BOSS/CIVILIAN census, RANK_GRAND_MASTER,
ITEM_LOCKED/ITEM_INFINITE stock markers both sides, MOT_ITEM sub-ids,
LANG_ + item flavor labels from the debug menus, official MODEL_/ICON_
arc slots (the archive kept demo order), STAGE_CURE_PRINCESS in the
score formula, and the consolation award's 0xFE-to-1 wraparound
unlock. config/functions.main.exe.tsv resynced (203 stale names).

DONE 2026-08-28: the Ghidra program was resynced (sync_to_ghidra.py
--commit — 687 signatures, 55 globals; the flattener now strips
game_types.h's #include lines so SDK types keep passing through as
identifiers).

HUMANISING LOOP 6 (2026-08-28, the construct scan-and-dive): every
scaffold construct class in the matched sources was censused and
experimentally adjudicated with matchdiff-gated rewrites — goto
return ladders (AttackShort: structured spelling 12 bytes short, ~29
scheduling diffs), one-shot fence towers (DefaultActionHumanoid's
14-deep tower is a 2^depth loop-note weight amplifier; flat and
depth-5 fail identically), while(1)-with-break loops (for-form changes
length), twin identical arms (10 sites: only SaveSI's icon3 twin was
removable — collapsed; the rest documented), and every
previously-undocumented plain fence across ten files (all
load-bearing, all now noted). The broadened census (self-assignments,
empty arms, no-effect statements) found zero true hits. Result: each
remaining un-human construct carries its measured justification, and
the cookbook's scaffold entry records the campaign so nothing gets
re-tried blind.

HUMANISING LOOP 7 (2026-08-28 evening, casts + the tower endgame): the
weird-cast census removed 29 pure-noise casts (nine (Humanoid *)
Me_THINK_C, twenty (AfterimageType *) on the void* illusion slots) and
respelled eight (GsCOORDINATE2 *)base loader casts as &base->locate —
all byte-free — while classifying the rest (scratchpad/persistent
literals, width puns, TItem.model dual-use, MATRIX.t-as-VECTOR, era
valloc casts) as documented or human. DefaultActionHumanoid's goto
reflection spaghetti proved to be a plain structured (vx || vz) choice
(byte-identical), and the 14-deep fence tower was solved to the unit
with regalloc.py + gcc 2.8.1's own global.c: it adds +1 weighted ref
per level to zz (18 -> 32) to cross allocno_compare's floor_log2
priority cliff and win $s0 from the STAT_DEAD constant. OPEN LEAD (the
DAH endgame): the demo's locals inventory is only i/xx/yy/zz/ry — our
reconstruction's invented block locals (top, object_y, size_y,
direction_abs, ...) starve zz of the 14 refs the original factoring
gave it naturally; refactoring regions onto the demo inventory could
retire the tower for real. The permuter independently confirmed the
knife-edge (a single dead i-ref rebalances 96% of the cascade).

HUMANISING LOOP 8 (2026-08-29, adversarial subagent round): three
independent Explore lenses (expression / control-flow / naming) swept
all matched sources; every finding triaged and either fixed or
annotated across five commits. Highlights: ActATTACK's comma chains
are now structured nested ifs and its shifted_mid pun collapsed
entirely; CVAupdate's three goto scan loops are while(1) loops (the
guarded increment was the delay slot; do-while measured +52); the
LoadConstruction offset-walk was a plain indexed loop; five copy-paste
families became textual macros (GET_THROW_ROTATION x14,
SWAP_TWIN_BLADE x4, EMIT_SUBDIV_GT3 x3, DISPOSE_ORNAMENT_ARCHIVE x2,
SET_NOW_MOTION_UNLESS_CVA x3); naming batch fixed wrong addresses,
sizes, format-string comments, KORO_OUT, and proved remap_buttons_'s
s16 ControlScheme extern byte-required (u16 flips lh->lhu). New
reusable rules folded into the cookbook (goto-scan/delay-slot,
offset-walk, comma-chain, pun-collapse). NEXT: another adversarial
round with fresh lenses (data tables / dead code / doc accuracy), and
the DAH demo-locals-inventory endgame remains open.

SKIPPED deliberately: the data-lens suggestion to CamelCase ALL_CAPS_/
trailing-underscore globals -- that casing is the repo's marker for invented
(non-PSX.SYM) names, and erasing it would lose provenance.

DAH ENDGAME MEASUREMENTS (2026-08-29): the in-place-abs variant
(`zz = GetDirection(...); zz = zz >= 0 ? zz : -zz;` flat, no tower)
measures zz at 22 refs / 161 live -> 5465 priority, landing $s2 while
the STAT_DEAD pseudo (39/201 -> 9701) keeps $s0; the full diff is 116
instructions. The cliff arithmetic: zz wins $s0 at
floor_log2(r)*r/live > 0.9701, i.e. 34 refs at <=167 live (10180).
In-place reuse is cheap (+4 refs / +2 live for the abs), and
rotate_y->zz in the loop's turn block projects ~28/169 -- still ~6
refs short with no natural donor left (direction can't merge: live
ranges overlap; ry-as-abs was refuted earlier; twin SetNowMotion calls
are +4 bytes). A decomp-permuter run over the flat variant is searching
the remaining space in the census worktree (scratchpad/permute-dah.log).

DAH ENDGAME CLOSED (2026-08-29): the score-10 permuter candidate from
the earlier run repaired the flat variant's allocation by adding a dead
`i = (i > 0) ? (i - 0x1000) : (i + 0x1000);` -- proof that only extra
refs on the starved pseudo fix the assignment, and that any CODE
carrier leaves its own bytes behind (the residual 10 = the dead
statement itself). The unique zero-code carrier in cc1 2.8.1 is
note-based loop ref-weighting, i.e. the do-while(0) tower family. The
original source necessarily contained a construct that reduces to it
(nested statement macros were the period idiom). The tower stays, fully
documented in the file; no further endgame experiments planned.
DAH PROVENANCE CLOSED (2026-08-30, owner-directed max-effort): the
83-config flag/compiler sweep and the demo-binary witness ended the
question — no flag set or cc1 version (2.6/2.7.2/2.8.0/2.8.1/gs107)
brings flat DAH under 118 differing bytes; the demo compiles the same
statements fence-free because ITS i never crosses a call ($a2), and
retail's added damage arm is what promoted i into the callee-saved
file, creating the i-vs-zz rivalry the four small fences (2/3/3/3)
resolve. Debug-macro fossils refuted at all four sites (two fenced
statements postdate the demo). Full story in the file header.

SPLIT-FENCE GENERALIZATION (2026-08-30): the per-occurrence weighting
law dissolved the other deep towers too — ActivateHumans 3->2,
StageEndScreen 4->2, mission_score 4->2, DrawTargetS cages all <=2,
subdivide_quad_ 3->2, AfsGetEntry 3->2; DrawConstruction's 3 proven
irreducible. Deepest nest repo-wide is now 3 (one site, measured).
RE-VERIFIED AND RESPELLED (2026-08-30, owner request): a fresh removal
campaign measured the remaining unlisted levers — register keyword
inert; reuse + 5-level tower repairs the allocation but the projected
donors (rotate_y, in-place abs) sit in caller-saved registers in the
retail bytes, so only ry is a byte-legal donor (23/174 — nowhere near
the bar; family minimum = ry-merge + depth 11, at the cost of a
PSX.SYM-attested local). Elimination arithmetically impossible. The
tower is now SPELLED as the file-local ONCE/ONCE2/ONCE4/ONCE8
statement-macro family (token-identical expansion, depth arithmetic
visible: 4 + 8 + 2 = 14); cookbook §3.10 records the presentation
rule.

DAH TOWER RETIRED (2026-08-30, macros banned by owner): the macro
respelling was reverted and the 14-level single-site tower replaced by
the same +14 dial DISTRIBUTED over four fence-safe zz statements at
depth <= 3 (zz = map->level @2, zz >>= 1 @3 [encloses 2 refs/level],
the conflict locate->vz store @3, the abs @3; 2+6+3+3 = +14, zz 18 ->
32/159 -> 10062, byte-identical, ./Build check green). New measured
weighting laws while calibrating (now in cookbook §3.10): defs count
+1/level exactly like uses; scaling is linear in depth; rival-mention
regions are self-defeating (GetDirection's args feed i +2/level);
xx-mention sites are banned by xx's own floor_log2 cliff at 16 refs
(one ref reorders s2-s4); `zz = locate->vz` is fence-toxic in the
SCHED dimension (the target hides that load in ConflictDistance.vx's
load-delay shadow; a barrier there costs a +4-byte nop). Also
re-refuted with measurements: `zz = zz;` and dead-boundary `ry = zz;`
add 0 refs (deleted before .lreg counts). The DAH endgame stays
closed; only the spelling changed.

DAH HUMANISED (2026-08-30, owner-directed max-effort round 2 — five
commits, all byte-gated): the four-label goto probe block dissolved
into one if/else with inline ternary-abs conditions (cross-jumping
builds the shared GetAreaMapVector tail; call_map/probe staging and
the duplicated `probe = locate` were chase artifacts); the -1-arm
width pun became `(short)(human->width >> 2)` (combine canonicalizes
the narrow into the sll16/sra18 extract); the conflict scan became
`while ((i = GetConflictResult(object, -1)) >= 0)`; the rcos line is
now symmetric with rsin (no yy staging). The four weight fences are
ONE seven-level tower on `zz >>= 1` — the recorded "site choice is
forced / four fences irreplaceable" conclusion was label-renumbering
noise in raw .s diffs (any +14 distribution over the four zz-only
statements is byte-identical; canonicalize labels before scoring —
cookbook §3.10). The identical-arms `if (object_id != 0)` and the
two-level conflict/object_id nest are gone: THREE EMPTY do{}while(0)
statements bound the same sched1 regions at zero weight, each
measured individually load-bearing, with plain assignments between.
What remains machine-shaped, both proven irreplaceable-in-function
under the pinned toolchain and documented in the header from cc1's
own source and dumps: the tower (zz 18->32 weighted refs, priority
10000 vs i's 9948 across the floor_log2 step at 32) and the three
empty barriers (the backward scheduler's potential_hazard groups
ready memory ops, so retail's load order is unreachable from any
flat statement order). Era question re-closed with a sharper
instrument: 2.8.0-psx / 2.8.1-psx / gs107 emit identical asm for
this function flat AND fenced, 2.7.2-class cannot reproduce even its
instruction count, every plausible flag is inert on the allocation
fingerprint, and the demo (Oct 1997) predates GCC 2.8.0 entirely —
its caller-saved i needs no rivalry story beyond its missing damage
arm. Split-i (mode/no) re-refuted by measurement: the probe/reflect
piece colors caller-saved ($a3) because the loop tail is the only
call-crossing range. Cookbook gained the empty-fence substitution
rule, the label trap, the declaration-order tie lever (correcting
the older blanket no-op claim), and the load-grouping anti-rule.

DAH BARRIER PROVENANCE + KEYWORD SWEEP (2026-08-30, owner challenge
"there must be some other explanation -- volatile etc?"): keywords
measured out -- `volatile` on a short field is byte-visible (blocks
the lh fusion: lhu+sll+sra), so retail's fused lh EXCLUDES volatile
from the original outright; a volatile s32 read fences only memory
ops, not the ALU address chain (measures at the flat baseline);
const/register inert. The region edge itself is unavoidable in flat
C: yy holds both the position load and the turn-arm value (both $a0;
one PSX.SYM local), two sets deny its load the birthing bump, so
only an edge pins it. But the "crazy talk" reading of the empty
do{}while(0)s is wrong in the other direction: an empty one-shot is
exactly what the standard 90s debug macro (#define DBG(x)
do { } while (0)) leaves in a release build; the debug-side
do { FntPrint x; } while (0) wrapper is measured byte-invisible
around a live call; the demo build has bare FntPrint dumps in this
TU; and the three sites are natural collision-record dump spots.
Three deleted debug prints explain all three barriers as ordinary
human source. The seven-level weight tower remains the one construct
with no such reading (it nests around a live statement). File header
and cookbook 3.10 updated.

DAH DEBUG-PRINT FORENSICS (2026-08-30, owner asked for demo hints +
presentation ruling): the demo's actual DAH prints RECOVERED — an
if/else pair right after the map probe, format strings at
0x800106d4/0x800106f4 ("l(ia) h%d v%x ah%x al%x %04x" for the
LEVEL_NONE arm, "l%d h%d ..." otherwise), dumping
level/height/vector/angleH/angleL/attrib; 31 FntPrint sites across
the demo build, unconditional (Fnt buffer only draws when flushed).
Retail kept the machinery: FntPrint still linked AND called (ADT
debug menu), debug_printf_/debug_msg_open_ are retail-era additions.
Presentation ruling: no macro — the barriers stay bare empty
do{}while(0) with "deleted debug print" comments inside the braces
and the evidence in the header; a DBG() spelling would need invented
format strings (fiction) and re-tread the banned-macro costume; the
tower stays a bare honest compiler dial (no debug reading exists for
a 7-deep nest around a live statement).

DAH TOWER ELIMINATED (2026-08-31, Claude+Codex collaboration, owner-
directed): the seven-level do{}while(0) nest is GONE — replaced by ry
live-range fission: `ry = zz` on the three reflect paths (the turn-
block copy above `if (i > 0)` and before MoveHumanoid), `ry &= ~1;
ry >>= 1;` on the turn path, tail subtracts ry. Codex proposed the
fission + mask-carrier mechanism (round 1); measurement found the
missing requirement — the $s0 winner must CONFLICT with i, not just
outrank it (a non-overlapping winner SHARES $s0 with i and the file
cascades) — and the corridor placement + attested-ry reuse closed it.
Oracle along the way: register zz asm("$16") leaves only 28 diff
lines (cse constant-carrier effects), proving the coloring race is
~99% of the tower's job. Load-bearing details in the file header;
reusable pattern in cookbook 3.10. The function now contains no
multi-level fence nests at all: five DBG sites (two recovered demo
prints, three lost-text placeholders) and plain C.

FENCE SWEEP CAMPAIGN (2026-08-31, Claude+Codex, owner-directed): the
DAH lessons applied tree-wide with two instruments (label-canonical
per-layer removal audit + the barrier probe: statements out, bare
empty one-shot in each position) iterated to fixpoint, because
REMOVALS CASCADE - each landed fence change shifts the races other
fences were balancing, unlocking further removals (ActivateHumans'
razor 471-vs-481 race closed itself two rounds after its sibling
fences fell). Landed so far: ActivateHumans FENCE-FREE (all 4);
PlayMusicFormID fence-free after restoring its direct sentinel loop;
SetFlyWire fence-free after restoring its indexed pool scan and ordinary
signed divisions; leLayoutEnemy fence removed; statement fences converted to
bare empty barriers in ProcItemNingyo, ActATTACK(x3), Briefing(x4 incl. its
182-diff region fence), mission_score(x2 + 3 stale layers),
StageEndScreen(x2 + a halved nest), ProcItemJirai, PAD_init, InitPAD,
valloc, draw_time_, draw_digits_, PutNumber, ProcItemSmoke, LoadSI,
InitEffect, DrawPause, DrawImpact, ActSTICKON, ActACTION. All gates
green including shiftability. REMAINING true weight fences (barrier
probe fails both ways), queued for DAH-style race analysis with
Codex: DrawTargetS (round 3 in flight), DrawConstruction,
ProcItemNingyo(2 big), game_over_screen_ (state +4), StageEndScreen
best_x pair, subdivide_quad_ (mega pointer pair s0/s1),
SetCameraMode, ProcItemDokudango, LoadOrnamentArchive, CreateStage,
AttackGeneral(197), StateTransition, SetupSpline, SetBlood,
PutStrain, ProcMiscDoor, GetAreaMapLevel(108, region), AttackLong
(155), AttackIndirect(2, cse constant-carrier), update_card(2+12),
vmemoryGC, AfsGetEntry, AddMisc, RestoreItemLayout, decode_tmd_adiv_,
mission_score(rest). Load-bearing EMPTY barriers
(honest, DBG-readable) stay: LoadTIMpack, set_boot_exec_,
SetupImageToPolyGT4/FT4, SearchTarget(85!), PlayVoice, ComPad,
AttackShort(262!).

CODEX ROUNDS 3-4 LANDED (2026-08-31): DrawTargetS zero-fence (sign-
staged edges; ten cages gone; corner-locals structure proven
impossible - Y 10/26 strictly outranks X 10/27, no tie). game_over_
zero-cage (one calibrated pre-loop identity `state = ((state+state)-
state) & state` supplies exactly +4; the natural `state++` spelling
fixes the whole permutation but bytes demand literal li loads -
recorded as the model's cleanest oracle). update_card_: weight fence
-> one u16-exact consumer identity; its retry wrapper proven a
BLOCK-BOUNDARY class (labels/blocks all fail) and stays documented.
DrawConstruction: three of seven layers gone via one plimit consumer
identity ((plimit+plimit)-plimit ranks it 3333, between slot 3255
and model); residual partitioned into the cell/j visibility corridor
(j needs $s6 reuse across three disjoint roles; fission cannot
co-color) and the slot store corridor. Identical-arms tree scan:
9 sites, 2 dissolved (Think3firstattack, calculate_score), 7 are
genuine multi-role levers. StageEndScreen: layer halved + a false
comment caught by the apply-guard and corrected same hour. The
mutual-race permutation playbook (7 levers) is cookbook doctrine.
Round 5 in flight: subdivide_quad_ mega-pair, GetAreaMapLevel region
fence, and a 15-file mid pack.

ROUND 5 LANDED + TASTE BAR SET (2026-08-31): eight files shed eleven
layers for one-line unsigned carriers (PutStrain, RestoreItemLayout
x3, CreateStage, SetFlyWire, SetupSpline, ProcMiscDoor, AfsGetEntry,
ProcItemDokudango); decode_tmd_adiv_ separately went fence-free via a
NAMED SHARED LOAD (count = *prim) - the best kind of fix, better C
than the original. Project taste policy now: a carrier lands only at
<= one clean commented line per removed layer, <= 3 mentions of one
value; under that bar four ASM-IDENTICAL results were REJECTED and
their fences kept as least-bad (subdivide_quad_'s +23-ref/25-read
blob, AddMisc's 5-read bound, SetCameraMode's and StateTransition's
cast triples). Whole-file keeps with mechanism recorded:
GetAreaMapLevel (region corridor incl. transient quartet),
AttackLong (return-staging region), SetBlood (broad init), vmemoryGC
(inline-helper shape), AttackIndirect (cse constant-carrier),
update_card retry wrapper (block boundary). Round 6 in flight:
ProcItemNingyo's two 174s, mission_score remnants, Briefing l320,
LoadOrnamentArchive, DrawConstruction corridors, AttackIndirect.

FENCE CAMPAIGN CONVERGED (2026-08-31, round 6): every do{}while(0)
in the tree is now processed. Round 6 landed nine more layers
(ProcItemNingyo all three via two item carriers - the two big races
were ONE item/param race; mission_score rank-base + persistent-state
pointer; Briefing's armour pair cancels CARRIER-FREE - either alone
is 12 off, both gone is exact; LoadOrnamentArchive both via one
three-read consumer). The remaining population is all principled,
documented keeps: region/boundary fences with probe matrices
(GetAreaMapLevel, AttackLong, SetBlood, vmemoryGC, SetCameraMode's
scheduler region, AddMisc body, DrawConstruction's two corridors,
AttackIndirect's inheritance boundary, update_card's retry wrapper,
StageEndScreen's +1/+2 pair, mission_score's row-sort empty), four
taste-rejects where the fence beats any known cure (subdivide_quad_,
AddMisc bottom, SetCameraMode first, StateTransition), honest empty
barriers with the DBG reading, and seven multi-role identical-arms.
HEADER-CONTRACT CAMPAIGN (2026-08-31, Codex rounds 12-13): fifteen
session-diary headers rewritten as constraint contracts - 1,320
preamble lines removed with ZERO code tokens changed (gated by
matchdiff + a comments-stripped token comparison + symnote --check).
Targets picked by diary density, not length: BreedLife, EndDrawing,
fast_tng3_/fast_tnf3_ and DrawAfterimage were deliberately KEPT because
their length is distinct facts, not repeated chronology. Two results
worth remembering: cd_read_sectors_'s header contained a live
CONTRADICTION (a proof that CdGetSector failure does a full retry, plus
the superseded claim that it only re-primes mode 6) that only
compression exposed; and AdtSelect finally dropped the 40-line
superseded rulebook its own header existed to refute. The editorial
contract is cookbook doctrine, including the provenance boundary:
behaviour prose is owner-authored (never added, never rewritten),
status and matching constraints are maintainership metadata.

COMMENT-MASS CAMPAIGN (2026-08-31): the tree carried 30,676 comment
lines against 42,048 code lines (0.73). Three byte-neutral moves took
it to ~25,900 (0.62): (1) six self-labelled superseded investigation
logs (AdtSelect 345, briefing_screen_ 196, DrawImpact 187, SetupTelop
261, adiv_tng4_ 147, DrawBleed 95) moved VERBATIM to
docs/matching-archive.md, indexed in docs/README.md, with two-line
pointers left behind; (2) tools/symnote.py stopped repeating its
ten-line demo-locals caveat in every stamped block (5,261 duplicated
lines) - it now lives once in docs/psx-sym.md "Reading a stamped
block" and the generator emits a pointer (390 files regenerated,
symnote --check clean); (3) DamageControl's 88-line session diary
became a 53-line constraints list (every fact kept, only the
derivation story dropped). SCOPE LIMIT confirmed from commit
1612d927: the owner's behavior-gloss ban is on ADDING them - that
commit RESTORED pre-existing summaries - so the ~520 long-standing
`Name (0xADDR) — prose` headers stay untouched.

CONTROL-FLOW CAMPAIGN (2026-08-31, rounds 7-10 with Codex + parallel
solo work): ~215 gotos and ~90 labels removed across ~35 files, every
one byte-identical. Tree gotos 800 -> 587. The productive routes, all
now cookbook doctrine: ACYCLIC guard inversion (a goto into the
statement after an else, a two-arm diamond with the condition negated
to preserve physical arm order, an enclosing if over a bypassed
region, an inverse OUTER guard instead of an inline early return, and
one-line tail duplication that cross-jump re-merges), and THE SWITCH
LEVER — a goto ladder testing one value against constants is usually
an ordinary switch. The switch lever converted twelve ladders between
us and FALSIFIED SIX in-file notes asserting a ladder was required
(spare_item_slot_, ProcItemManebue — which also deleted a helper
local — StartStageSequence, AVCameraSetup, ProcMiscDoor, plus
cd_seek/DrawFrame/MoveFly/camera_terrain_pitch_ which had no note).
Refinements: `default`'s lexical position decides whether a
fallthrough needs an extra jump; if/else-if is NOT equivalent because
expand_case emits both tests before either body; and case order can be
written to preserve the target's physical body order (though
ActNORMAL's 1,2,3,0,4 ladder still resists at 67 lines, and
Think4abandon's at 18 — the lever is not automatic).

CONTROL-FLOW FINDING (2026-08-31, corrected 2026-09-02): goto-shaped loops
in this codebase are often byte-load-bearing. Converting them to real loop
syntax adds NOTE_INSN_LOOP_BEG/END, which multiplies flow.c's loop_depth ref
weighting for the whole body - the same mechanism the fence campaign
exploited. But test the WHOLE dataflow graph: PlayMusicFormID's natural loops
looked 43 lines wrong only while invented table aliases, a pointer-sum helper,
and an empty fence remained; removing all of them makes the direct sentinel
`while` exact. GetAreaMapLevel's inner and outer hand-rotated loops still fail
at 216/196. Corollary worth
remembering: a goto->loop conversion can REPLACE a weight fence,
since the loop notes ARE the fence.

Next humanising dimension queue (recon 2026-08-31): goto-density
leaders for structured-control-flow attempts (measure per DAH's probe
lesson - many gotos will be byte-forced or authentic; only land
reading improvements): AttackShort(31), AttackGeneral(27),
AttackLong(20), DrawConstruction(18), DrawModelArchive(16),
ActivateHumans(16), GetAreaMapLevel(15, the labyrinth search),
DrawSprite(15), DamageControl(15), ActATTACK(15); label-heavy Act*
family (5-10 labels each - possibly authentic dispatch style, check
the demo's Act* shapes first). All load-bearing empty barriers now
carry the standard comment. The Codex session remains resumable for
the next campaign brief.

DAH KNOWN DEBUG PRINTS RESTORED (2026-08-30, owner-directed —
supersedes the no-macro ruling above for RECOVERED content only):
the file now carries a release-emptied DBG(x) macro (#ifdef DEBUG:
do { FntPrint x; } while (0), else do { } while (0); name is a
stand-in) and the demo-recovered map-probe dump pair spelled
verbatim under it — both arms, real strings, args matched
field-for-field; measured byte-inert in retail, and a -DDEBUG
compile emits exactly the two FntPrint calls the demo build had.
The three conflict-arm barriers stay BARE empties with
"text lost" comments: their prints postdate the demo, so any
DBG(("...")) there would be invented text — known content gets the
macro, unknown content stays an honest residue. matchdiff MATCH +
./Build check green.

HUMANISING LOOP 9 (2026-08-29, family lenses + decimal campaign):
adversarial rounds continued -- the Act*/think header re-audit fixed
five wrong glosses (and caught commit 1b20da30 claiming fixes its
died script never applied; edit-verification discipline now in
docs/orchestration.md); the item-family lens harmonized dispose
sentinels/pool bounds/names across 19 files with two measured
byte-required divergences annotated. The decimal campaign converted
hexified decimal constants repo-wide: card-save state machine
(tens-with-substeps scheme), damage/frame/effect quantities, angle
thresholds (rand()%360, <1100), screen coordinates (-160/-120
corners), doll HP 99. Extern cross-check unified six signatures and
adjudicated the set_impact_ex_ s32 tails (negative-constant lever).
Redundant (int) promotion casts trimmed (move-speed pair,
subdivide_quad_ midpoints). Effect-family lens round done: include placement, decimal
stragglers, SetImpact pz, DrawSnow ef/param + otz split (flag reuse
measured required), DrawGore enum R, FlyWire glosses.

TUNING KNOBS (owner directive 2026-08-29): recurring quantities live
in src/main.exe/tuning.h (via game_globals.h) as named defaults --
SCREEN_W/H, DEPTH_LIMIT, EYE_HEIGHT, THROW_HEIGHT, CAMERA_EYE_HEIGHT,
N_EFFECT_SLOTS, CHASE_WALK_SPEED, SWIM_SPEED, GAME_OVER_TIMEOUT. Add
new knobs there when a quantity recurs or is a gameplay lever; keep
one-off frame counts inline. Changing a value deliberately fails
./Build check (mod flow: ./Build mod).

PARKED: shared names for the memory-card status codes (0/2/3/4/6/7
recur across SaveSI/LoadSI/SaveCard/update_card_*) -- needs a careful
semantic derivation of each code (libmcrd result space vs the game's
own returns) before inventing enumerators in memcard.h; the two
preserved retail prototype drifts (update_card_message_ /
update_card_screen_ externs) show this family shipped without shared
headers, so per-TU views may deliberately disagree.

HUMANISING LOOP 10 (2026-08-29, tuning knobs + drift audit): owner
directive implemented -- src/main.exe/tuning.h (via game_globals.h)
holds named handles for recurring/tunable quantities: screen size, OT
depth limit, eye/throw/camera heights, effect-pool size, walk/swim
speeds, game-over timeout, per-item weapon damage, item effect
durations, bait radius, alert durations, card retry cap. Family
lenses: effect family (DrawSnow renames + otz split, DrawGore enum R,
FlyWire glosses, include placement) and card family (decimal
stragglers, pad names, DeleteCard types, prefix drift renamed via full
protocol) fully triaged. The scripted extern-vs-definition audit
measured ~20 drifts: 8 free artifacts unified, 11 RETAIL prototype
bugs preserved+annotated (return narrowings, param views, cd_open's
dead extra argument) -- cookbook 3.17b records the rule. Combat-family
lens done (constants named, div-mul roll respelled, gloss added,
cooldown/SR-range knobs).

HUMANISING CAMPAIGN STATUS (2026-08-29): converged. Lenses run:
expression, control-flow, naming/API, data-tables, dead-code,
doc-accuracy, Act*/think headers, item family, effect family, card
family, combat family. Systematic sweeps: hex/decimal (constants now
read as quantities), extern cross-check, extern-vs-definition drift
audit (retail's no-shared-headers bugs preserved + cookbook 3.17b),
weird casts, goto shapes, pad packs, tuning.h knob extraction. Each
class is either fixed repo-wide or adjudicated in place with a
measurement. Future passes should target NEW code as it gets matched,
not re-sweep the existing corpus.

READER-QUEUE (2026-08-29): COMPLETE. All flags from the three
whole-function readers are landed or adjudicated in place; the
CLAMP_SORT_DEPTH macro (main.exe.h) folds the OT-clamp copy-paste
across ten renderers, and every effect-pool bound reads
N_EFFECT_SLOTS.

ROUND-2 READERS (2026-08-29): COMPLETE. All three partitions re-read
whole-function; ~60 tail findings landed across five textual waves
plus the macro program: DISPOSE_ITEM (35 sites), TAKE_ITEM_SLOT (17),
SET_ITEM_COLLISION (12), RESET_ALERT_DURATION (4),
SET_NOW_MOTION_UNLESS_CVA hoisted (+3 new sites), WORLD_CELL (5),
DELETE_WEAPON_CONFLICTS_AND_AFTERIMAGES (3), CLAMP_SORT_DEPTH (13).
Parked with reasons: mission_score_screen's ten digit blocks (internal
variance needs per-block normalization first), game_over init trio
(statement-order variance is a measured-risk class). The corpus has
now been read whole twice with every flag landed or adjudicated.

GLOSS-COVERAGE PASS (STOPPED 2026-08-29 by owner: "I don't need you
to add comments about what functions do, stop that"). Five batches had
landed before the correction and were then removed on request
(commit 1612d927); NO glosses are to be written — matching notes (measured adjudications)
remain wanted, narrative headers do not. Originally parked as: ~40 large files carry matching notes but
no behavior gloss (what the function does) -- e.g. StageEndScreen,
ControlHumanoid, SetBleeds/SetBleedsDir, ProcItemNingyo/Nemuri/Jirai,
SetupTelop, briefing_screen_, mission_score_screen, ActJUMP,
AttackShort, think_alarm_reaction_. DrawConstruction and
DefaultActionHumanoid got theirs 2026-08-29. Write the rest via
careful per-file reads (agent-assisted, then doc-accuracy-verify;
wrong glosses are worse than none -- always cross-check addresses
against config/symbols.main.exe.txt).

MOTION-ID NAMING (2026-08-29, owner request "sounds like we should
have names for all these motiod IDs"): COMPLETE. Census found 119
distinct ids; three evidence agents produced per-id naming tables
(setter + handler arm for every entry); ~100 invented names landed in
game_types.h's specific-motion enum (marked invented — none are in the
demo symbols), and a context-filtered sweep converted every literal
site across 56 files (assignments, comparisons, case labels,
SetNowMotion/UpdateMotion args), all matchdiff-gated. Family-root
dispatch labels use the existing MOT_* family names; DamageControl's
one-past-end `< 0x71a` respelled as `<= MOT_ATTACK_STEALTH_SIDE_AYAME`
(slti-identical); AttackControl's myid/emid stealth pairing named and
its shared `+= 3` documented the Ayame dead variants 0x110C-0x110E.
Unidentified ids stay hex on purpose: 0x103, 0x502, 0x802 (case labels
whose arms gave no semantic evidence). Commits ab6da043 + 6fe505a0.

SPOT-CHECK ROUNDS (2026-08-29/30, converged-phase loop): random
16-file whole-read reader pairs, three rounds (48 files audited), each
finding landed or measured. Round 1: PROJECTION_DISTANCE 300 + score
table + WALL_AVOID_PUSH/LEDGE_PROBE_RISE/wire knobs to tuning.h,
ITEM_NONE (#define — a negative enumerator signs the enum, cookbook
rule), SetWire probes (ONE, CLAMP_SORT_DEPTH, inline GetWireRotation;
big/alias fills byte-required). Round 2: status7_*->attack_* in four
Attack controllers, vmemoryGC draft suffixes, BreedLife wrong-comment
fixes, Briefing pad names + MAX_SELECTED_ITEMS. Round 3: (s16)-on-s16
cast sweep (36 dropped, 2 required — writer-width rule in cookbook),
residual motion ids under unrelated variable names (y/pd/value/id),
MOT_NORMAL_TURN_R/L, ProcMisc/CameraType1/vrealloc comment corrections
(vrealloc's bit31 compare + words-vs-bytes memcpy quirk now stated
straight), ~10 new measured adjudications. Probe economy holds: ~60%
of flagged constructs are free respells, the rest get notes. Rounds
4-8 (128 files audited total): the readers now also verify comments
against code — a steady stream of factually wrong or stale notes got
corrected (BreedLife's "leash flag", vrealloc's dead-compare claim,
AfsInit's "one 0x3C block" = five handles, tile_sprite_'s wrong
caller/bits, cd_seek's unsigned clamp, trace_ground_'s fixed-point
return, Think3hitaway/Think1sleep glosses, the dmy stub family's
copy-pasted "primitive-sort" wording across 69 TMD files). New free
classes landed: no-op width casts (writer-width rule), cast
sandwiches (subdivide_quad_ ×25), dead-t comma chains and the
comma-assignment condition in ActATTACK, laundered-zero arguments,
constant-on-the-left flips, paren-member (p->f).g flattening,
fifteen-name OR chains -> masked constants. Recurring byte-required
levers keep confirming: mid-sequence pointer aliases, staged
temporaries, twin cross-jump bodies, entry guards before whiles,
register-held constants — each now carries a measured note where
found. Retail's own latent bugs documented, never fixed: AddEnemy's
ItemName[70]/[71] tail writes, AdtSelect's uninitialized first pad
read, cd_seek's dead zero-clamp arm, ActSWIM's dead item arms.

CLOSURE (2026-08-27, loop stopped): a final scanner pass surfaced and
fixed the true last stragglers — leFindEnemy's `local_40` (now epos),
humanscan's in_/unaff_ pattern over-matching semantic labels, ActSTATE's
three guard gotos, and the library FUN_ audit: FUN_8006ebe4 identified
as SsQuit (demo table confirms, 32 bytes, SsEnd/SsQuit shutdown pair in
LoadExecEx); the pad ISR pair (FUN_80083538/FUN_800835b0) is unnamed
even in the demo's own debug symbols (exact 120/56-byte correspondence
to demo FUN_80087138/FUN_800871b0) and the 2D_BG22 sort worker
FUN_80063b94 takes four args where official GsSortFixBg16 takes three —
both stay honestly unidentified rather than guessed. Every remaining
humanscan row is a documented byte-required spelling. Reopen conditions:
new PSX.SYM-grade evidence for the library internals, or evidence for the few remaining
data-dependent bits (the challenge "why runtime?" was right: setter/
reader hunts later named ATTR_SEARCH/FLOAT/NOFLOOR/LEDGE statically;
what genuinely needs data or runtime is only map bit 8's floor-material
identity, and 0x200/0x2000 are reader-less set-only bits).

## Numeric state machines — CLEARED (2026-09-01)

Every `switch` in a matched file whose cases were all numeric literals now
names its states. The detector that found the queue -- 3+ cases, all
numeric -- reports zero remaining.

What the campaign established, all of it in the cookbook:

- Classify PHASE or TAG first. Phases transition into each other, tags are
  parallel alternatives. Non-contiguous values prove a tag
  (LoadConstruction's 0/5/2 are stage-data record kinds); a contiguous
  range proves nothing (CameraPanMode runs 0..8 and is nine parallel
  behaviours). The tell is whether any case moves to another.
- Name states from what the case bodies DO, and prefer shipped evidence
  over reading: the card screen's states came from its text assets, and
  the debug menu's rows from DEBUG_MENU_STAGE_OPTIONS, now readable as
  `tools/gamedata.py DebugStageMenu`. Any AdtSelect menu can be decoded
  the same way -- the rows are {char *name; u_long value}.
- Leave what you cannot prove. The card library's result codes stay
  numeric because no cross-API status enum exists anywhere; a wrong name
  is worse than a digit.
- Put the enum where the thing lives, not where the switch is: in
  effect.h beside the struct, so the Set* and Draw* halves share it, or
  next to the extern for a global. That is what catches the writers in
  other files.
- Then sweep the function so nothing tests by name and assigns by number,
  including ordering comparisons, and re-read the comments, which go
  stale the moment a value is named.

Naming the STATES is worth it; naming the TRANSITIONS is not -- replacing
`mode++` with explicit targets costs lines because the increment is what
the compiler was given (measured twice).

## Round 2 of humanising — real source shape, not names (2026-08-31)

The 2026-08-27 loop closed on NAMES and artifacts. This round is about
SHAPE, driven by two new sources of evidence, and it is where the active
work is. `./Build check` still gates every commit.

**`tools/symtypes.py --locals`** diffs each function's declaration block
against the locals PSX.SYM recorded for the original; ~280 functions
differ. A local we invented is the usual reason a natural spelling will
not compile to the right bytes, so this is the queue. Worked examples:
`SetBlood`'s old `(TEffectSlot *)(idx * sizeof(...) + (int)base)` existed
only because we transcribed loop.c's generated scan pointer into the C;
the indexed loop is exact and retains only the result `slot`. A second
whole-graph pass went further: 20 of the 22 scans now spell the source array
directly as `EffectSlot[idx]`, with no invented base alias either;
`SetupImageToPoly{FT4,GT4}` recovered the original's `tx`/`ty`/`th`, one
of which is a single variable advanced in place where we had two.
Bare, the tool audits GLOBAL declarations the same way. Method that
works: try the WHOLE plain graph first, then bisect — partial edits
misclassify an invented local as required, which is how several of the
"byte-required" notes we have since disproved were created.

**`tools/gamedata.py`** reads the retail data tables that carry a
`char *name` beside an id, so constants can take the game's own name:
`HumanData` (characters), `WeaponModel` (weapons — retail runs
0x04..0x37 and our `weapon_kind` enum already matches it exactly, which
made the parallel `WEP_*` defines the guess and they are gone),
`ThinkDB` (AI think types, whose names encode their own table and
index). `--whatis <value>` searches every table at once.

A sub-seam with a lot left in it: **shadowed scopes**. Where PSX.SYM
records N copies of a name and we have fewer, the original declared it
per block and we either renamed it (`scan_i`, `frame_model`,
`status_pad`, `j`) or flattened it. Six were pure renames and one
(`RestoreItemLayout`) needed the block that justifies the name; all
exact, because a loop counter or pointer lives in a register and nesting
it costs no frame. **107 such name/count gaps remain** — not all are
recoverable (some are demo-only logic, and concurrently-live counters
like DrawConstruction's j/k/l are genuinely distinct), but it is the
largest measured queue we have.

Landed so far: Codex rounds 20 and 21 (58 invented locals across
eighteen files, plus PutLifeBar's recovered nested x/y/n),
ActATTACK's attack switch named as the weapon dispatch it is, 65
pointer-punned reads that were doing nothing, the adiv scratch offsets
spelled in one unit.

Two compiler facts found this round, both in the cookbook:
  - cc1 2.8.1 gives every sibling block's stack object its OWN slot and
    never overlaps by lifetime, so a nested block scope is free for a
    register value and costs full frame space for a stack object. Frame
    size (`subu $sp,$sp,N`) is the cheap discriminator.
  - `(x + x) - x` is the ONLY C-level way to spend one extra reference
    for flow.c to count; `x|x`, `x&x`, `x^0`, `x*1` and `x+0` all fold
    before the count.

Settle an addu-order or spelling question in a six-line scratch file with
the build's cc1 flags, not inside the function — two runs of that
replaced a long-standing "irreplaceable" note with the real rule.

## Current resume point (2026-07-20)

The game-code matching queue is empty. Live output is 537/555 game functions in
exact C plus 18/555 canonical handwritten-assembly originals, or 555/555 and
100% of game-code bytes done. `tools/findsimilar.py --targets --by-value`
returns zero game candidates. `briefing_screen_`, `AdtSelect`, and
`mission_score_screen` are exact C; `eval_spline_gte_` is the eighteenth canonical
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
        3796  36.4%  subdivide_quad_          (residual   8)
        2188  41.8%  game_over_screen_           (residual  75 — 96.6% exact)
        1448  49.2%  briefing_screen_          (residual  87 — 94.0% exact)
        ...
          48 100.0%  get_pad_active_

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
    is not collectable at any price; the prize ranking moves to `subdivide_quad_`
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
    members now MATCH: fast_tng4_/fast_tnf4_ (1260 each),
    fast_tnf3_/fast_tng3_ (984 each), and adiv_tng4_/adiv_tnf4_
    (920 each). The final pair closed by replacing decompiler carrier reuse with
    coherent colour/counter/initializer identities; their former v0/v1 floor was
    decomposition-relative.
  * Also present but hidden by `--max-size 2048`: **`game_over_screen_`** (2188) and
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
6. **Persistent-blob option bytes: adopted.** 0x58 `gNannido` (difficulty,
   `game_difficulty` enum), 0x59 `gSound` (stereo/mono — `InitSoundEffect`
   branches to `SsSetStereo`/`SsSetMono`), 0x5A `gSoundLevel` (CD volume),
   0x5B `gSELevel` (SE volume), 0x5C `gfMemory` (post-mission save-UI
   gate), 0x5D `Anakon` (rumble/analog gate). The run mirrors the demo's
   standalone globals `gNannido..gfMemory` (0x80098090+) minus `gZangyaku`,
   which retail dropped from this run. Evidence per byte in
   `reference/data-symbols-applied.tsv`; field carve in `game_types.h`'s
   `TLinkInfo` (the official demo typedef, adopted for the struct; the
   0x80010000 instance keeps splat's descriptive symbol name
   `PersistentState` since the demo names only the type). Remaining here:
   the still-unnamed retail additions 0x5F/0x60/0x61
   (`InitPersistentState` defaults 0/1/1).

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
