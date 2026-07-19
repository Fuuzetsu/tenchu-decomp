# Relocatable build and the PsyQ boundary

The long-term goal is an executable whose game code and data can move when the
source changes. That does **not** require translating every statically linked
Sony PsyQ routine into C. It requires every input to the final link to carry
relocation information, or to be source that can be assembled/compiled into a
relocatable object.

This distinction gives the PsyQ block three valid source forms:

1. a target-proven C reconstruction, when we want readable/editable SDK code;
2. canonical assembly with symbolic references, especially for routines that
   were originally handwritten assembly; or
3. the original PsyQ library member, converted to a MIPS ELF relocatable object.

Only a raw byte blob with embedded absolute addresses is incompatible with a
relocatable build. The standalone SDK text carves before file offset `0x72cd0`
have now been converted from that form, but generated `72CD0.data.s` still
begins with raw SDK instructions before continuing into loaded data. An exact C
match is therefore a readability and provenance win, not a prerequisite for
shiftability.

## What other PS1 decompilations do

The projects below use all three forms. There is no scene-wide convention that
stock SDK code must first become C.

- [Metal Gear Solid](https://github.com/FoxdieTeam/mgs_reversing) keeps the SDK
  outside the game repository. Its
  [build accepts `PSYQ_SDK`](https://github.com/FoxdieTeam/mgs_reversing/blob/7a0b95d1f84a3801b06ca39c82983b4cac976ddd/build/build.py#L31-L48),
  and its
  [linker command file](https://github.com/FoxdieTeam/mgs_reversing/blob/7a0b95d1f84a3801b06ca39c82983b4cac976ddd/build/linker_command_file.txt#L392-L403)
  names original members such as `libgpu/P22.OBJ` and `libgte/MTX_10.OBJ`
  directly. The game source is editable even though those stock libraries have
  not all been rewritten as game-owned C. Its README also lists shiftability as
  work beyond reaching 100% C for the main executable.
- [Silent Hill](https://github.com/Vatuu/silent-hill-decomp) sets an external
  object path and has `o` PsyQ
  subsegments in its
  [bodyprog splat configuration](https://github.com/Vatuu/silent-hill-decomp/blob/2715d7e2748d6a3a80122ad4ba6001082550e8ea/configs/USA/bodyprog.yaml#L291-L315)
  and keeps converted MIPS ELF objects such as
  [`lib/libgs/matrix.o`](https://github.com/Vatuu/silent-hill-decomp/blob/2715d7e2748d6a3a80122ad4ba6001082550e8ea/lib/libgs/matrix.o).
  That particular object exports `GsInitCoordinate2`, `GsSetLsMatrix`, the
  `GsSetLightMatrix` family, and `GsMulCoord*`; those are vendor libgs functions,
  not Tenchu functions.
  Its README separately identifies raw-data migration and removal of hardcoded
  addresses as the remaining shiftability work.
- [Castlevania: Symphony of the Night](https://github.com/Xeeynamo/sotn-decomp)
  treats the PSX SDK as a distinct shared-library artifact. Its
  [main-executable configuration](https://github.com/Xeeynamo/sotn-decomp/blob/c1b143fdc7cedf51f9bdd389d39c87737bb2ac3f/config/splat.us.main.yaml#L108-L170)
  mixes reconstructed `psxsdk` C with canonical `asm` entries for low-level
  libgte routines. This is the self-contained reconstruction route, but even it
  does not insist that every SDK routine have a C spelling.

The closest fit for Tenchu is the first two projects: keep a hermetic reference
build, and add an optional external-object lane for stock SDK members. SOTN's
separate shared-library effort remains a useful model if a fully open,
self-contained SDK replacement becomes a goal later.

## Local relocation proof: `GS_107.OBJ`

The claim above was tested against Tenchu, not assumed from file formats.

Externally supplied PsyQ 4.4, 4.5, and 4.6 archives contain the same 1,724-byte
`GS_107.OBJ` member. It was extracted with `PSYLIB.EXE` under `wibo` and
converted to an ELF relocatable object. Its `.text` is `0x570` bytes and it
carries sixteen `.rel.text` entries: local and external `R_MIPS_26` calls plus
`R_MIPS_HI16`/`R_MIPS_LO16` pairs for `GsLIGHTWSMATRIX` and `_LC`. Its `.bss`
contains those two matrices.

Linking the converted object with these retail placements reproduced all
`0x570` Tenchu bytes exactly:

```text
.text             0x800655e4
.bss              0x800c64a0
SquareRoot0       0x800779a4
SetColorMatrix    0x80078524
```

Replacing the complete mixed raw/C `GS_107` range in the Tenchu linker script
with only this object also produced the shipped
555,008-byte executable and its expected SHA-256. Linking the same input at
unrelated text/BSS addresses also succeeded; after masking all sixteen
relocation words, the retail-address and moved-address text was identical.
This proves that the matching C reconstructions already in this repository are
not what makes this SDK object movable; the original object's relocation
records do.

Two implementation details matter:

- Use PCSX-Redux's
  [`psyq-obj-parser`](https://github.com/grumpycoders/pcsx-redux/tree/main/tools/psyq-obj-parser),
  or another pinned converter with a validation/normalisation stage. The tested
  `psyq2elf` revision preserved bytes and relocations but emitted an invalid
  local-symbol/`sh_info` boundary for 90 of 149 libgs members. A no-op GNU
  `objcopy` pass reordered the symbol table correctly; all 149 then passed an
  `ld -r` validation.
- Preserve each original object's section alignment and its placement within
  the library stream. `GS_107.OBJ` begins four bytes past an eight-byte
  boundary in Tenchu; silently rounding it up changes the image even though the
  code and relocation records are otherwise correct.

No PsyQ header, library, object, or tool used for this audit is committed to the
repository.

## Recommended Tenchu architecture

Keep two build lanes with different dependency contracts:

### Implemented first gate: linker-owned game functions

`./Build check-reloc-game` now performs a second, normal GNU `ld` link using
the same relocatable inputs and ordering as the reference build. A small
generator removes every absolute assignment in the retail game-code range
`0x80016134..0x800601d4` and emits `Function = .;` immediately before each of
the 555 game inputs. The current-location anchors preserve original `static`
functions across this decompilation's artificial one-function object split;
they are not fixed-address assignments or trampolines.

At retail sizes the alternate executable is byte-identical to the shipped
image. Its ELF reports game functions as section-relative text symbols rather
than absolute symbols, proving that the linker owns their placement. Generated
scripts and the alternate ELF/executable remain under `.shake/build`.

This gate intentionally stops assigning ownership at the game/SDK boundary.
The next gate now covers the following SDK/CRT text stream through
`0x800834d0`, but raw instructions at that boundary would still move without
relocating, the PS-EXE entry point and load size would remain stale, and the
fixed BSS address range would be consumed. Passing `check-reloc-game` therefore
proves the first prerequisite, not a runnable size-changing build. The
remaining stages below are still required.

### Implemented game-C address input gate

Six otherwise matched C files used numeric address construction solely to
hold retail code generation: `SelectCameraOwnerOption` and `FileOption` built
the `D_80097D70` format-string address from `0x80090000`,
`ProcItemShinsoku` did the same for `CamState.Owner`, `ActivateHumans` did it
for `StageChar`, and both `valloc`'s lazy initializer and `vinit` used literal
`0x800dc000` for `MemoryPool`. The literal instructions had no ELF relocation
records, so moving the pointed-to data or pool would silently leave stale
pointers.

`./Build check-reloc-c-literals` compiles those same sources under one global
`TENCHU_RELOCATABLE` definition. There are no per-function compiler options:
the variant uses the normal object/TU compiler profiles and ordinary symbolic
C expressions. The retail branches remain byte-matching scaffolds; the
conditional is transitional and is not a claim that either spelling is the
original source.

The gate reads the compiler-produced ELF objects and requires these records:

```text
SelectCameraOwnerOption  D_80097D70  HI16=1 LO16=1
FileOption               D_80097D70  HI16=1 LO16=1
ProcItemShinsoku         CamState    HI16=2 LO16=1
ActivateHumans           StageChar   HI16=1 LO16=1
vinit                    MemoryPool  HI16=1 LO16=1
valloc                   MemoryPool  HI16=1 LO16=2
```

It also rejects the corresponding unrelocated literal `LUI` high halves. The
two `CamState` HI16 records are expected compiler output: the high half is
materialised on both sides of a call and shares one LO16 field load. `valloc`'s
second `MemoryPool` LO16 is likewise real compiler output: cc1 naturally
rematerialises the symbol's low half for the following aggregate store.

This is a linked proof, not just an object-table check. The six natural
objects are substituted into the linker-owned game lane. `valloc` grows by
four bytes while `ActivateHumans` and `SelectCameraOwnerOption` are each eight
bytes smaller, so the net text change is a 12-byte shrink. A deliberate
12-byte file-backed pad at the game/SDK boundary restores `Exec` to its retail
address, isolating the later raw SDK while this bounded probe is green. The
verifier reads the final ELF instructions and proves that all six symbolic
references resolve to their targets. It also proves that 55 existing
section-owned pointer words in post-`Exec` data follow the induced game-symbol
movement; every other post-`Exec` file-backed word remains unchanged.

That boundary pad is neither a trampoline nor part of the final normal-link
layout.
The target data symbols and `MemoryPool` still have fixed script assignments;
the achievement here is that these C inputs no longer embed those placements.
Once data/BSS/pool ownership moves to the linker and the remaining raw ranges
are relocation-aware, the pad can disappear and the symbols can move normally.
`check-reloc-game` includes this input/link proof as well as its retail-exact
ownership check.

`./Build relink` consumes the same six compiler objects through a distinct
linker chain with **no boundary pad**. It then applies canonical SDK ownership,
the reviewed loaded-data objects, linker-owned BSS/heap boundaries, and PS-X
EXE header regeneration. `./Build check-relink` measures the compiler-object
delta instead of assuming one, requires 3,529 unique section-owned SDK symbols
to follow it, reapplies every audited HI/LO record in the final ELF, validates
extension/BSS collision bounds, and checks the finalized header. With the
current source the SDK and `_gp` move by 12 bytes and the entry PC becomes
`0x8006025c`.

This is a structural integration milestone, not yet a runtime-ready image.
`D_80097D70`, `CamState`, and `StageChar` are still absolute loaded-data script
assignments, so the compiler relocations resolve to stale retail addresses even
though the underlying bytes moved. The gate prints those three names as known
blockers rather than hiding the discrepancy.

### Implemented second gate: canonical CRT/SDK text

The CRT/PsyQ text stream at virtual addresses
`0x800601d4..0x800834d0` is now relocation-aware without reconstructing Sony
library C. Splat emits 19 raw ranges as canonical MIPS assembly: the two
previously implemented `LIBAPI_4F9D4`/`CRT_SDK_4FA48` carves and all 17
remaining standalone raw `.text` inputs from file offset `0x5492c` through
`0x722e8`. Existing C and canonical-assembly islands keep their own objects and
remain in the original order.

This is a source-form change, not a C decompilation and not a trampoline. The
ordinary reference link still reproduces the shipped executable exactly. The
19 generated objects contain `0x20c44` bytes (134,212 bytes) of `.text` and
6,839 `.rel.text` records reported by GNU `readelf`:

```text
R_MIPS_26     1501
R_MIPS_HI16   2144
R_MIPS_LO16   2144
R_MIPS_PC16   1050
```

`./Build check-reloc-sdk` builds on the game-symbol lane, removes all 1,919
absolute symbol-script aliases in this bounded stream, and performs two normal
links. At retail placement, 1,801 names are emitted by their real sections and
the 118 remaining aliases disappear from both links; those names described
internal C labels which are neither public definitions nor relocation targets.
The first link is byte-identical to retail. The second inserts one file-backed
four-byte word immediately before `Exec`. The verifier checks every emitted
name in the linked ELFs: all 1,801 move by four bytes and remain section-owned.
It also decodes representative early, middle, and late linked `J`, `JAL`, and
`LUI`/`ADDIU` pairs and checks that they retarget to the corresponding moved
symbols. The external `jal main` remains pointed at the unchanged linker-owned
game function.

The source/object audit closes two easy loopholes in that aggregate proof. It
requires the exact 19-object inventory and per-object section/relocation counts,
rejects numeric `J`/`JAL` operands, and rejects new address-looking high-half
literals. The only allowed high values were reviewed as KSEG masks, packed GPU
values, or an SPU bitfield. Six literal `.word` values are also allowlisted as
two PsyQ signatures/return stubs. Removing 31 synthetic
`__override__prt_*` labels was necessary: they were not real function
boundaries, and several split genuine address-materialisation pairs, causing
Splat to preserve numeric high halves instead of emitting `%hi`/`%lo`
relocations.

The shifted artifact is a relocation probe, **not a runnable grown PS-EXE**.
The first untouched input is `72CD0.data.s`; its first symbol, `StartPAD` at
`0x800834d0`, remains absolute and machine-checked. That mixed carve still
begins with SDK instructions and later contains loaded data, so it shifts
physically in the probe without its embedded references or public symbols
following. The PS-X EXE header and BSS layout also remain unchanged. Canonicalise
or replace that remaining code, migrate loaded pointers, and make the later
layout linker-owned before treating arbitrary game growth as runnable.

The generator and ELF verifier accept a `+0x10004` probe so the same checks can
exercise an `R_MIPS_HI16` carry. A whole-executable probe of that size is the
next gate, not a passing claim today: GNU `ld` rejects it because later small
data moves more than 64 KiB away from the still-fixed `_gp`, producing
`R_MIPS_GPREL16` truncations (for example against `D_8009770C`). Moving `_gp`
and its small-data/BSS ownership into the normal linker layout must precede
that large-shift proof.

### Implemented parallel gate: linker-owned BSS boundaries

`./Build check-reloc-bss` builds on the linker-owned game and canonical
SDK-prefix scripts, then proves the next piece of layout ownership. It converts
the zero-filled end of generated
`72CD0.data.s` to a NOBITS input, moves the C `.bss` inputs into a following
NOLOAD output section, removes the corresponding fixed symbol-script
assignments, and represents the virtual-memory pool as an explicit NOLOAD
reservation.

The retail layout established by crt0, the executable bytes, and the alternate
ELF is:

| region or boundary | half-open range / address | evidence in the proof ELF |
|---|---:|---|
| initialized image | `0x80011000..0x80097eb0` | PROGBITS, `0x86eb0` bytes |
| crt0-cleared BSS | `0x80097eb0..0x800cdba8` | NOBITS, `0x35cf8` bytes |
| `HEAP_START` | `0x800cdbac` | section-relative, four bytes after BSS |
| unused BSS-to-pool headroom | `0x800cdba8..0x800dc000` | `0xe458` bytes (58,456) |
| `MemoryPool` reservation | `0x800dc000..0x801fc000` | NOBITS, `0x120000` bytes |
| initial stack | `0x801ffff0` | pool ends `0x3ff0` below it |

All 158 known symbols in crt0's BSS clear range are `B`/`b` symbols at their
retail addresses. `__bss_start`, `__bss_end`, `HEAP_START`, and `MemoryPool`
are likewise section-relative rather than absolute. `_gp` now comes from the
real initialized-data label at `0x80097698` instead of a linker-script absolute
assignment.

The shipped file has one important representational wrinkle. Its last nonzero
initialized byte is at `0x80097eac`; the logical initialized prefix ends at
file size `0x876b0`, but the 555,008-byte PS-X EXE continues with `0x150` zero
bytes to the next sector boundary. Those bytes overlap the beginning of the
address range crt0 clears. The alternate `objcopy` result deliberately stops
at the logical prefix. `tools/psxexe.py` then obtains section-owned
`__SN_ENTRY_POINT` and `__load_start` from the proof ELF, derives the payload
size, appends exactly `0x150` zero bytes at retail size, and regenerates PC,
`t_addr`, and `t_size`. The finalized result is byte-identical to retail; the
padding is no longer modelled as initialized storage.

`check-reloc-bss` is deliberately an **exact-at-retail verification**, while
the generated linker itself now expresses BSS start, crt0 clear end, heap, and
`_gp` with relative layout/collision assertions rather than retail equality
pins. `./Build relink` uses a separate no-pad composition of those same layout
rules with the natural C and canonical SDK inputs. Existing inputs may grow,
and new helper files under `src/main.exe/reloc/` feed
explicit extension text/rodata/data, small-BSS, and BSS inputs instead of being
lost to `/DISCARD/`. The 122 post-padding BSS names are linker-relative aliases,
but most do not yet come from real source storage declarations. Raw SDK/data
carves still embed address constants. Header regeneration is implemented, but
a changed output is not runnable until those references are dealt with.

For a growth-enabled link, BSS must be allowed to follow the grown initialized
image; the implemented crt0 symbols and relative assertions already follow it.
Keeping `MemoryPool` fixed permits at most the `0xe458` bytes of immediate BSS
growth. Both default initializers now reference its symbol, but both also use
the free-word constant `0x47ffe`, which describes exactly the current
`0x120000`-byte reservation ending at `0x801fc000`. Moving only the base would
therefore overrun that end and potentially the stack. A future dynamic-pool
lane must keep the end fixed (or choose a new reviewed end) and derive the free
word count from linker-owned `MemoryPool`/`MemoryPoolEnd` in **both** `vinit`
and `valloc`; merely making the base symbolic is insufficient. The fixed
handoff record at `0x80100000..0x8010005c` intentionally lies *inside* that
pool reservation, so those two ranges must not be modelled as disjoint.

### 1. Reference matching lane

The current default remains hermetic after the user supplies the retail disc.
It uses the clean ABI shims under `include/psxsdk`, matched C where available,
and extracted assembly elsewhere. `./Build check` must stay byte-identical and
must not fetch a proprietary SDK.

### 2. Relocatable lane

The first opt-in lane is implemented for `GS_107.OBJ`; see
[`psyq-object-lane.md`](psyq-object-lane.md). It accepts a user-supplied PsyQ
4.4–4.6 `LIBGS.LIB`, hash-gates the archive and member, validates the converted
ELF, and proves the complete alternate executable byte-identical. Expanding
that manifest-driven lane requires identifying the exact archive
version/member for each Tenchu range, converting each proven member to ELF,
and linking the resulting objects by symbol. Its manifest must record:

- archive version and member name;
- all text/data/BSS sections and original alignment;
- relocation-normalised text identity against the retail range;
- public, local, weak, and common symbols; and
- any Tenchu C or canonical-assembly override that replaces the member.

The unit of compiler provenance remains the original object/source file. A
function-specific `ccExtraFlags` workaround is not acceptable evidence for C
that originally shared an object with other functions.

Converted SDK members should be cached only in ignored build output. The
default build and CI must not acquire them automatically. Where the supplied
archives do not contain Tenchu's exact member/version, keep or improve a
symbolic assembly reconstruction; use C only when compiler/object evidence
supports a plausible source.

## What SDK relocation does not solve

Replacing stock SDK byte carves with relocatable objects is only one part of
shiftability. The current executable still contains raw data and script-assigned
addresses whose pointers do not all follow a moved section. A complete lane
also needs to:

- migrate pointer-bearing raw data to typed/symbolic source or relocation-aware
  assembly;
- remove fixed layout assignments that should be linker-owned;
- regenerate the PS-EXE load size and related header fields;
- preserve overlay and cross-executable contracts; and
- account for disc layout/LBA assumptions when an executable grows.

Therefore “all game functions are source” and “the executable is shiftable” are
separate milestones. PsyQ C coverage should not be used as a proxy for either.

The reviewed loaded-data slices have a strict standalone build gate in
[`relocatable-data.md`](relocatable-data.md), and the same transformed objects
are now part of `./Build relink`. They turn 32 exact interior references into
`R_MIPS_32` relocations and prove retail, `+4`, and `+0x10004` links without
changing the default build. This is a pattern for migrating the remaining 152
pointer-bearing words, not yet a runnable whole-executable growth claim.
