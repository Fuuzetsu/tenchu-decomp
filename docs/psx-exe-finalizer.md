# PS-X EXE header finalizer and validator

`tools/psxexe.py` is the downstream output-finalization tool used by
`./Build check-reloc-bss`. Running the normal matching build does not invoke it
or change the retail header; only the opt-in normal-link proof uses it.

The tool addresses one narrow part of shiftability: once the linker has chosen
the code/data layout, the PS-X EXE header must describe the result. It does not
make raw instructions, absolute pointers, BSS globals, or the executable-loader
ABI relocatable.

## Finalizing an image

The input is an EXE-shaped linker/`objcopy` output:

- bytes `0x000..0x7ff` are a PS-X EXE header template;
- bytes from `0x800` onward are the file-backed payload.

`finalize` copies the header template and always regenerates these words:

| Offset | Field | Generated value |
|---:|---|---|
| `0x10` | initial PC | explicit address, ELF entry, or named symbol |
| `0x18` | `t_addr` / load address | explicit address, ELF load segment, or named symbol |
| `0x1c` | `t_size` / load size | payload length after zero-padding to `0x800` |

Every other header byte is preserved unless the caller explicitly supplies a
repeatable `--set FIELD=VALUE`. The normal-link build uses
`--set sp=<TENCHU_INITIAL_STACK_ADDRESS>` so the executable header consumes the
same central RAM policy as the linker; PC, `t_addr`, and `t_size` cannot be set
this way because they are always linker/payload-derived. GP, data/BSS fields,
reserved words, and the license/marker area otherwise remain untouched. Input
and output paths must be different, so an experiment cannot silently rewrite
the linker artifact or the matching build.

With explicit addresses:

```console
$ tools/psxexe.py finalize linked.exe -o game.exe \
    --entry 0x80060268 --load-address 0x80011000 \
    --set sp=0x801ffff0 \
    --expect gp=0 --expect sp=0x801ffff0
```

With an ELF, a named symbol can provide either value. Without the corresponding
`--*-symbol`, `e_entry` supplies the PC and the lowest nonempty PSX `PT_LOAD`
segment supplies the load address:

```console
$ tools/psxexe.py finalize linked.exe -o game.exe \
    --elf game.elf \
    --entry-symbol __SN_ENTRY_POINT \
    --load-symbol __load_start
```

This repository's current ELF deliberately has `e_entry == 0`, so its entry
must be named explicitly. Its header is a separate zero-address `PT_LOAD`; the
tool ignores that segment and derives `0x80011000` from the addressed payload.

A GNU ld map works when both layout symbols are named:

```console
$ tools/psxexe.py finalize linked.exe -o game.exe \
    --map game.map \
    --entry-symbol __SN_ENTRY_POINT \
    --load-symbol __load_start
```

The linker script, not this tool, should own those symbols. That keeps header
generation downstream of the same layout decisions that relocate the program.

## Validation

Validation is read-only:

```console
$ tools/psxexe.py validate game.exe \
    --elf game.elf --entry-symbol __SN_ENTRY_POINT \
    --expect gp=0 --expect sp=0x801ffff0
valid game.exe: PC=0x80060268, load=0x80011000, \
    t_size=0x87000, file=0x87800, GP=0x00000000, SP=0x801ffff0
```

It checks:

- the `PS-X EXE` magic and complete `0x800`-byte header;
- `t_size > 0` and `t_size % 0x800 == 0`;
- exactly `filesize == 0x800 + t_size`;
- word alignment of PC, load address, and nonzero GP/data/BSS/SP addresses;
- containment of PC within the loaded payload;
- non-wrapping text/data/BSS address ranges; and
- any ELF/map-derived values or repeatable `--expect FIELD=VALUE` assertions.

Expectation names are the header field names printed by `--help`: `pc`, `gp`,
`t_addr`, `t_size`, `d_addr`, `d_size`, `b_addr`, `b_size`, `sp`, and
`sp_offset`. Readable aliases such as `entry`, `load-address`, `bss-size`, and
`stack` are also accepted.

The finalizer runs this same validator before it atomically writes the output.
Consequently a successful output has a sector-padded payload, a PC inside that
payload, and the exact file/header-size relationship required by the later ISO
packing step.

## Tests

The dedicated tests use synthetic PS-X EXE, ELF32, and GNU-map fixtures; they do
not require the proprietary disc or cross toolchain:

```console
$ python -m unittest tools.tests.test_psxexe
```

The implementation is wired into the normal-link layout proof:

```console
$ ./Build check-reloc-bss
```

That lane obtains both `__SN_ENTRY_POINT` and `__load_start` from its linked
ELF, obtains the configured stack from `src/main.exe/ram_layout.h`, pads the
logical payload by `0x150` bytes at retail size, and requires the finalized
image to be byte-identical to the shipped executable. A grown or reconfigured
image therefore gets PC, load address, load size, and stack policy from current
inputs instead of inheriting stale retail header words.
