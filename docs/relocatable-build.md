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
relocatable build. Tenchu's complete CRT/PsyQ text stream is now canonical
relocation-bearing assembly through `UnitVector2` at `0x80086764`; the following
input is loaded data, not another raw-code boundary. An exact C match is
therefore a readability and provenance win, not a prerequisite for
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

## Implemented Tenchu architecture

The project now keeps two complete build contracts.

### Reference matching lane

The default remains hermetic after the user supplies the retail disc. It uses
the clean ABI shims under `include/psxsdk`, matched C where available, and
extracted or canonical assembly elsewhere. `./Build check` must stay
byte-identical and must not fetch a proprietary SDK. Its fixed-at-retail linker
script is an exactness oracle, not the modding layout.

### Normal relink lane

`./Build relink` produces `.shake/build/tenchu/main_relink.exe` by a distinct,
ordinary GNU `ld` link. It does not patch slots, reserve caves, insert
trampolines, or restore changed function sizes at artificial boundaries. The
linker owns the 555 game inputs, the complete CRT/SDK text, initialized data,
`_gp`, BSS/heap boundaries, and the dynamic allocator reservation. The PS-X EXE
finalizer then derives the entry point, load address, loaded size, and sector
padding from the linked ELF.

The historical focused gates remain useful exact oracles:

- `check-reloc-game` proves all game functions section-owned at the retail
  layout;
- `check-reloc-c-literals` proves the compiler records for six matching-only
  numeric-address constructions;
- `check-reloc-sdk` proves the canonical SDK stream at retail and `+4`;
- `check-reloc-data` proves every reviewed pointer at retail, `+4`, and
  `+0x10004`; and
- `check-reloc-bss` proves the linker-owned retail BSS/pool representation and
  a byte-identical finalized PS-X EXE.

`check-relink` is the composed acceptance gate. It checks the compiler objects,
the no-pad link, PS-X header, widened fixed-address audit, and a complete
`+0x10004` GNU-ld growth link.

## Compiler-produced C references

Six otherwise matched C files constructed retail addresses numerically only to
hold cc1's matching schedule. The normal lane compiles those same translation
units with one build-wide `TENCHU_RELOCATABLE` definition and ordinary symbolic
C. There are no function-specific compiler flags. The compiler currently emits
and the gate requires:

```text
SelectCameraOwnerOption  D_80097D70         HI16=1 LO16=1
FileOption               D_80097D70         HI16=1 LO16=1
ProcItemShinsoku         CamState            HI16=2 LO16=1
ActivateHumans           StageChar           HI16=1 LO16=1
vinit                    MemoryPool          HI16=1 LO16=1
vinit                    MemoryPoolCapacity  HI16=1 LO16=1
valloc                   MemoryPool          HI16=1 LO16=2
valloc                   MemoryPoolCapacity  HI16=1 LO16=1
```

`MemoryPoolCapacity` is a linker-defined scalar exposed through the usual
embedded-C symbol-address idiom. The exact branch retains the retail address
and `0x47ffe` word count; neither literal is present in the normal-link
allocator paths. With the current sources these six natural objects shrink the
game stream by 12 bytes, and the no-pad relink moves every later owned input by
that amount. `check-relink` reports 3,739 unique section-owned SDK symbols
following the change and resolves all eight target pairs in the final ELF.

## Ordinary C output-section coverage

Size-changing source support includes globals and statics, not just extra
instructions. The generated retail linker enumerates each existing game
object's base `.text`, `.rodata`, `.data`, and `.bss`, but cc1's pinned `-G8`
pipeline can also emit new small/common sections when an edit adds a global.
The normal-link generator therefore collects the missing sections from both
compiler-produced game-object families: ordinary `*.c.o` inputs and the six
`TENCHU_RELOCATABLE` replacement `*.o` inputs:

- `.sdata`/`.sdata.*` joins the loaded extension near `_gp`;
- `.sbss`/`.sbss.*`/`.scommon` joins the gp-near BSS prefix; and
- residual `.bss`/`.bss.*`/GNU `COMMON` is retained at BSS end.

Files under `src/main.exe/reloc/` have corresponding ordinary
`.text.*`/`.rodata.*`/`.data.*`/small-data/BSS patterns. The focused integration
test uses the actual `cc1-281 → maspsx → GNU as → GNU ld` pipeline on both
object families and covers initialized and tentative definitions above and
below the eight-byte `-G8`
threshold, static objects, const arrays, float/double constants, explicit
`.scommon`, and GNU `COMMON`. It also verifies that every small symbol remains
within signed GPREL16 reach of `_gp`.

Accordingly, “write any C” means ordinary C compiled by this pinned pipeline,
using its normal output sections and fitting the reviewed RAM/relocation
limits. It does not promise arbitrary custom section attributes, unsupported
compiler extensions, or unlimited storage.

The normal lane also has no catch-all section discard. Its generated script
discards only reviewed MIPS/debug metadata, collects the toolchain's expected
empty companion sections behind a zero-size assertion, and invokes GNU `ld`
with `--orphan-handling=error`. An unsupported allocatable section such as
`__attribute__((section(".custom")))` therefore stops `relink` with its section
name instead of producing an executable that silently omits those bytes. The
retail matching lanes retain their generated discard behavior unchanged.

## Mandatory input-object relocation audit

Every `./Build relink` runs `tools/reloc_input_audit.py` immediately after GNU
`ld`, using the generated linker script, linked ELF, and the exact object order
from the link map. `./Build check-relink` reruns the same gate before its growth
proof. The current result is:

```text
map-LOAD objects:                         731
owned nonempty allocatable PROGBITS:      767
reviewed .reginfo/.MIPS.abiflags:        1462
direct J/JAL backed by R_MIPS_26:   6918/6918
symbolic R_MIPS_HI16:                    4788
alloc-data words backed by R_MIPS_32:    1939
findings:                                   0
```

Every allocatable input section must have a normal-link owner. An unknown
section is rejected even when it is empty or has a debug-looking name; the
1,462 unowned metadata sections above pass only because their `.reginfo` or
`.MIPS.abiflags` type, flags, size, alignment, and entry size are structurally
exact. Expected empty compiler companions have an explicit, zero-size-only
allowance.

The relocation checks reject future canonical numeric MAIN references: direct
`J`/`JAL` without `R_MIPS_26`, unrelocated LUI plus canonical low-half address
formation, and aligned allocatable-data pointers without `R_MIPS_32`. This is a
strict regression gate, not a proof over arbitrary register arithmetic or
unaligned opaque packed fields. The reviewed pointer manifest and the shifted
growth proof cover the current exceptional packed fields.

## Complete canonical CRT/SDK text

The entire CRT/PsyQ text stream at `0x800601d4..0x80086764` is now
relocation-aware without reconstructing Sony library C. Splat emits 20 raw
ranges as canonical MIPS assembly, including the former instruction prefix of
the mixed `72CD0` carve; loaded data now begins separately at file offset
`0x75f64` (`UnitVector2`). Existing C and canonical-assembly islands retain
their own objects and original order.

The 20 canonical objects contain `0x23ed8` bytes (147,160 bytes) of `.text` and
7,540 `.rel.text` records reported by GNU `readelf`:

```text
R_MIPS_26     1697
R_MIPS_HI16   2393
R_MIPS_LO16   2393
R_MIPS_PC16   1057
```

`./Build check-reloc-sdk` removes all 2,010 fixed script aliases in that range.
At retail placement, 1,892 names come from their real sections and 118 obsolete
internal aliases are absent consistently. The retail link remains
byte-identical; the `+4` link moves all 1,892 names and machine-checks linked
early, middle, and final `J`/`JAL` and HI16/LO16 examples. The source/object
audit also rejects numeric jump operands and unreviewed address-looking high
halves. This is canonical source, not a trampoline and not a claim that Sony
wrote those routines in C.

## Loaded data and the final-image audit

The reviewed manifest now covers 208 pointer words and 135 targets across ten
generated data files. It includes the 22 name/path pointers in byte-packed
`StageConfig`, all previously reviewed trailing tables, and the cursor stored
in `D_80097E98` that intentionally points to `D_800C2EB0 + 2`. Every entry
produces an exact `R_MIPS_32` and passes retail, `+4`, and `+0x10004` links. See
[`relocatable-data.md`](relocatable-data.md) for the representation and evidence.

`tools/reloc_audit.py` audits the actual objects selected by the composed
linker, not stale Splat inputs. It reconstructs little-endian words from
byte-packed sources and no longer assumes that a pointer target is word
aligned. The current `--fail-on-findings` result is:

```text
final ABS symbols:                 0 in movable MAIN
literal J/JAL candidates:          0
conservative adjacent HI/LO:       0
literal data pointers:             0
findings:                           0
```

The ELF still has absolute constants, but all 472 are outside the movable MAIN
range or are derived scalar values such as capacities. Hardware registers,
BIOS interfaces, masks, and cross-executable ABI addresses should remain fixed;
zero findings does not mean deleting every hexadecimal constant.

## BSS, allocator, and PS-X header

At the retail layout the linked regions are:

| region or boundary | half-open range / address |
|---|---:|
| initialized image | `0x80011000..0x80097eb0` |
| crt0-cleared BSS | `0x80097eb0..0x800cdba8` |
| `HEAP_START` | `0x800cdbac` |
| retail BSS-to-pool headroom | `0x800cdba8..0x800dc000` (`0xe458` bytes) |
| `MemoryPool` | `0x800dc000..0x801fc000` (`0x120000` bytes) |
| initial stack | `0x801ffff0` |

The normal relink aligns BSS after the initialized image and computes
`MemoryPool` as the greater of `0x800dc000` and aligned BSS end. Its upper bound
stays `0x801fc000`; `MemoryPoolCapacity` is derived as
`(MemoryPoolEnd - MemoryPool) / 4 - 2`, and both `vinit` and `valloc` consume
that value. Growth therefore uses the retail headroom first, then advances and
shrinks the pool rather than overrunning its fixed upper bound. Linker
assertions require a valid word-aligned minimum pool and keep BSS below the
fixed `0x80100000..0x8010005c` executable-handoff record. That record
intentionally lies inside the allocator reservation.

The shipped file's final `0x150` zero bytes are sector padding, not initialized
storage. `tools/psxexe.py` takes section-owned `__SN_ENTRY_POINT` and
`__load_start` from the ELF, derives the logical payload size, pads it to a PS-X
sector, and regenerates PC, `t_addr`, and `t_size`. The retail BSS oracle remains
byte-identical; a changed normal link gets a changed header.

## Full-link growth proof

`./Build check-relink-growth` inserts a temporary `0x10004`-byte PROGBITS input
immediately after the real `main.c.o` input and runs the complete linker and
finalizer. The fixture has no assigned address and is neither a patch nor a
trampoline; it occupies the same ordinary input boundary that added code would
occupy. Before linking, the probe mirrors Shake's recursive union of user and
generated `reloc/**/*.c` sources, including nested helpers and one user override
per shared relative path; it neither misses generated inputs nor admits stale
objects. The current proof reports:

```text
inserted text:                  +0x10004
section-owned layout symbols:   7706 checked
compiler target pairs:          8 linked; 4 changed HI16
loaded pointers:                208 resolved at moved sources
BSS delta:                      +0x10000
MemoryPool:                     0x800dc000 -> 0x800ddbb0
MemoryPoolCapacity:             0x47ffe -> 0x47912 words
entry PC:                       0x8006025c -> 0x80070260
PS-X t_size:                    0x87000 -> 0x97000
movable fixed-address findings: 0
```

This proves that GNU `ld` can choose a larger internal layout, including a
64-KiB carry, and that the current static address contracts follow it. Growth
is still bounded by PS1 RAM, the fixed handoff/pool/stack contracts, MIPS
relocation ranges, and whatever new source the user adds.

## Emulator smoke proof

The current `+0x10004` artifact has also passed two bounded, non-interactive
PCSX-Redux tests. Generate it first with `./Build check-relink-growth`, then run
the direct-load probe used for the current result:

```console
$ python3 tools/pcsx_smoke.py \
    --exe .shake/build/reloc-growth-probe/main_growth.exe \
    --elf .shake/build/reloc-growth-probe/main_growth.exe.elf \
    --frames 5 --loop-hits 2 --timeout 30
TENCHU_SMOKE armed entry=0x80070260 main=0x800162a4 loop=0x8002adac frames=5 loops=2
TENCHU_SMOKE PASS entry=1 main=1 frames=5 loops=2 cycles=220072300ULL
```

This completed in about 6.7 seconds in the recorded environment. The probe
reads the entry from the grown PS-X header and `main`/`PadProc` from the grown
ELF; it does not copy retail addresses into Lua. Because the growth fixture is
inserted *after* `main.c.o`, `main` intentionally remains `0x800162a4`, while
the entry moves to `0x80070260` and downstream `PadProc` moves to
`0x8002adac`.

The full-disc result used the same grown EXE with the auto-LBA repack path:

```console
$ nix develop -c python3 tools/pcsx_smoke.py \
    --exe .shake/build/reloc-growth-probe/main_growth.exe \
    --elf .shake/build/reloc-growth-probe/main_growth.exe.elf \
    --repack --frames 5 --loop-hits 2 --timeout 150 --repack-timeout 120
mkiso: main.exe grew (620544 > 555008) — using auto-LBA layout
TENCHU_SMOKE armed entry=0x80070260 main=0x800162a4 loop=0x8002adac frames=5 loops=2
TENCHU_SMOKE PASS entry=1 main=1 frames=5 loops=2 cycles=1528346866ULL
```

The emulator log executed `\TENCHU\MENU.EXE;1` and then
`\TENCHU\MAIN.EXE;1`; the observed cached repack/boot took about 49 seconds.
The Lua probe automatically pulses the launcher/menu controls, rejects any
first-chance CPU exception or unexpected pause, requires two calls to the moved
`PadProc`, and then requires five later VSyncs. It uses the interpreter,
debugger, software GPU, isolated temporary memory cards, and the OpenBIOS
fallback unless `--bios` is supplied.

This proves direct execution and the auto-packed
`SLPS_019.01 → MENU.EXE → MAIN.EXE` handoff through the grown main loop.

The same auto-LBA disc subsequently passed representative STR and XA
checkpoints:

| asset | relocated lookup | observed runtime evidence |
|---|---:|---|
| `OPEN06.STR` | LBA 46,593 versus retail 46,561 (`+32`) | stream setup, `StGetNext`/`get_stream`, 21 combined `DecDCTin`/`DecDCTout` calls, frame 1,381 |
| `STAGES.XA` | LBA 119,130 versus retail 119,098 (`+32`) | XA setup, `CdControl(0x1b)`, four `cbCheckCD` callbacks, frame 3,557 |

No CPU exception occurred. This proves that representative filename-driven
STR decoding and XA command/callback paths follow the relocated disc layout;
the probe did not run either asset to EOF and did not establish physical audio
output or broad gameplay.

## Addresses that intentionally remain fixed

The rule is “no numeric address for something the linker may move,” not “no
numeric address anywhere.” The normal lane deliberately retains:

- the MAIN.EXE load origin at `0x80011000`;
- the pre-load persistent state at `0x80010000..0x80010e70`;
- PS1 BIOS, MMIO, and scratchpad addresses;
- the cross-executable handoff record at `0x80100000..0x8010005c`;
- the allocator upper bound at `0x801fc000`; and
- the initial stack at `0x801ffff0`.

Code/data/BSS definitions inside movable MAIN must instead be section-owned,
and references to them must carry relocations. A relocation against an `ABS`
linker-script alias is still pinned, which is why both the use and the target
definition were migrated.

## Optional original-PsyQ object lane

The independent `GS_107.OBJ` lane remains implemented; see
[`psyq-object-lane.md`](psyq-object-lane.md). It accepts a user-supplied PsyQ
4.4–4.6 `LIBGS.LIB`, hash-gates the archive and member, validates the converted
ELF, and proves a complete byte-identical alternate executable. Expanding that
lane can improve provenance, but it is no longer required to make Tenchu's
current SDK text move: canonical assembly already supplies those relocations.

The unit of compiler provenance remains the original object/source file. A
function-specific `ccExtraFlags` workaround is not acceptable evidence for C
that originally shared an object with other functions. Converted SDK members
should remain ignored build output, and the default build must not fetch them.

## Remaining acceptance boundary

“All game functions are source,” “the static executable relinks after growth,”
and “a grown disc has broad runtime coverage” are separate milestones. The
current `+0x10004` image has passed direct load, the auto-packed full
launcher/menu/main-loop smoke, representative `OPEN06.STR` decode activity,
and representative `STAGES.XA` setup/callback activity. Those results do not
cover every possible edit or subsystem. Running streamed assets to EOF,
confirming physical XA audio output, executable transitions beyond this path,
and broader gameplay remain runtime release gates. Disc packaging and
reproduction steps are documented in
[`building-an-iso.md`](building-an-iso.md).
