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
relocatable build. That is the current problem in ranges such as generated
`5492C.data.s`: calls, `lui`/`addiu` address pairs, and instructions are emitted
as literal `.word` values with no ELF relocations. An exact C match is therefore
a readability and provenance win, not a prerequisite for shiftability.

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
The next gate now covers the first SDK prefix, but later raw SDK instruction
blobs would still move without relocating, the PS-EXE entry point and load size
would remain stale, and the fixed BSS address range would be consumed. Passing
`check-reloc-game` therefore proves the first prerequisite, not a runnable
size-changing build. The remaining stages below are still required.

### Implemented second gate: canonical CRT/SDK prefix

The first contiguous CRT/PsyQ prefix, virtual addresses
`0x800601d4..0x80065100`, is now relocation-aware without reconstructing Sony
library C. Splat emits the two previously raw ranges as canonical MIPS assembly:

- file `0x4f9d4..0x4fa24` (`Exec` through `PCclose`);
- file `0x4fa48..0x54900` (`PCcreat` through `GsInitCoord2param`).

The existing `PClseek` canonical-assembly object lies between those ranges.
`GsSetLsMatrix` is already a relocatable object, but deliberately remains the
fixed boundary of this first proof. The first remaining literal instruction
input follows it: `5492C.data.s`, beginning at `GsSetLightMatrix`.

This is a source-form change, not a C decompilation and not a trampoline. The
ordinary reference link still reproduces the shipped executable exactly. The
larger generated assembly object has `0x4eb8` bytes of `.text` and 1,435
`.rel.text` records reported by GNU `readelf`:

```text
R_MIPS_26      285
R_MIPS_HI16    498
R_MIPS_LO16    498
R_MIPS_PC16    154
```

`./Build check-reloc-sdk` builds on the game-symbol lane, removes 261 absolute
assignments in this bounded prefix, and performs two normal links. The first is
byte-identical to retail. The second inserts one file-backed four-byte word
immediately before `Exec`. The verifier checks the linked ELF, not assembly
text: representative prefix symbols from `Exec` through `GsInitCoord2param`
move by four bytes and remain section-owned, while internal `J`, `JAL`, and
`LUI`/`ADDIU` code-address pairs retarget to the moved symbols. The external
`jal main` remains pointed at the unchanged linker-owned game function.
`GsSetLsMatrix` deliberately remains absolute at `0x80065100`, making the
current stopping point explicit and machine-checked.

The shifted artifact is a relocation probe, **not a runnable grown PS-EXE**.
Everything after `0x80065100` still shifts physically even though later raw
instructions and their public symbols remain fixed; the header and BSS layout
also remain unchanged. Continue carving later raw ranges or replace them with
manifest-verified original PsyQ objects before treating arbitrary game growth
as runnable.

The generator and ELF verifier accept a `+0x10004` probe so the same checks can
exercise an `R_MIPS_HI16` carry. A whole-executable probe of that size is the
next gate, not a passing claim today: GNU `ld` rejects it because later small
data moves more than 64 KiB away from the still-fixed `_gp`, producing
`R_MIPS_GPREL16` truncations (for example against `D_8009770C`). Moving `_gp`
and its small-data/BSS ownership into the normal linker layout must precede
that large-shift proof.

### Implemented second gate: linker-owned BSS boundaries

`./Build check-reloc-bss` composes with `check-reloc-game` and proves the next
piece of layout ownership. It converts the zero-filled end of generated
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

This is deliberately an **exact-at-retail ownership proof**, not permission to
grow code yet. Its linker script still asserts the retail BSS start/end, `_gp`,
heap, and pool addresses. The 122 post-padding BSS names are linker-relative
aliases, but most do not yet come from real source storage declarations. Raw
crt0 and SDK/data carves still embed address constants. Header regeneration is
implemented, but a grown output is not runnable until those references and the
retail boundary assertions are dealt with.

For a growth-enabled link, BSS must be allowed to follow the grown initialized
image, crt0's clear boundaries must be symbolic/relocatable, and the retail
equality assertions must become relative collision assertions. Keeping
`MemoryPool` fixed permits at most the `0xe458` bytes of immediate BSS growth;
larger changes require making its base/size linker-derived or deliberately
shrinking/moving the reservation in the source that initializes it. The fixed
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

The first reviewed loaded-data slice now has a standalone implementation in
[`relocatable-data.md`](relocatable-data.md). It turns eight exact interior
string references into `R_MIPS_32` relocations and proves retail, `+4`, and
`+0x10004` links without changing the default build. It is a pattern for
migrating the remaining pointer-bearing data, not yet a wired build lane.
