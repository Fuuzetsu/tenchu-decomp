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
