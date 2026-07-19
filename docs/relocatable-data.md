# Relocatable loaded data

`tools/reloc_audit.py` found a conservative set of 185 aligned literal words
in loaded data that point back into the movable MAIN.EXE image. This is a much
cleaner problem than the raw-code candidates:

- all 185 source words are in trailing owner `72CD0.data.s`;
- all 185 targets are in the leading-data range;
- the targets inspected so far are exact string or string-pool entry starts;
  and
- the generated owner around a target is often broader than the object being
  addressed, so rewriting a pointer as `D_800xxxxx + addend` would invent an
  ownership claim.

The owner distribution is useful for batching the work:

| Generated source owner | Pointers |
|---|---:|
| `D_8008EA90` | 76 |
| `CD_comstr` | 32 |
| `ThinkDB` | 18 |
| `CD_intstr` | 8 |
| Nine coherent four-entry tables | 36 |
| `WeaponModel` | 3 |
| Twelve singletons | 12 |

This inventory is conservative. It says that the word is aligned and falls in
the movable range; table structure, type, and symbol naming still require
review.

## Representation

`tools/reloc_data.py` applies `config/reloc-data.main.exe.json` to copies of
Splat's generated assembly. Each reviewed manifest entry records:

- the pointer's generated file, exact address, and enclosing `dlabel` owner;
- the target's generated file, exact address, and enclosing owner; and
- a human-readable symbol for the exact target.

The transformer inserts a section-relative object label directly at the
target and changes only the pointer operand from a literal to that symbol. For
example, the representation is conceptually:

```asm
/* exact interior target inside the larger generated string-pool owner */
.globl stage_sound_prefix_default
.type stage_sound_prefix_default, @object
stage_sound_prefix_default:
    .asciz "K:\\WORK\\CDIMAGE\\SOUND\\"

STAGE_SOUND_PREFICES:
    .word stage_sound_prefix_english
    .word stage_sound_prefix_french
    .word stage_sound_prefix_italian
    .word stage_sound_prefix_default
```

The enclosing generated owner is only an evidence guard. It is deliberately
not used as the relocation base. This prevents a label at `0x80013524` from
silently becoming `D_80013500 + 0x24`, a top-level relationship not supported
by the data itself.

The tool refuses stale or ambiguous input: wrong owners, missing addresses,
changed pointer literals, duplicate sources, symbol/target conflicts, and
attempts to overwrite the generated input directory all fail before any
output is written.

## First slice

The committed manifest covers two clear, game-owned four-entry tables:

- `STAGE_SOUND_PREFICES` at `0x8008EA58..0x8008EA64`; and
- `STAGE_ANIMATION_PREFICES` at `0x8008EA68..0x8008EA74`.

Their eight targets are the default, Italian, French, and English path strings
inside generated owners `D_80013500` and `D_8001359C`. The names are supported
by the source table names and literal string contents. No wider type or
top-level target symbol is inferred.

Validate the manifest against a generated tree without writing anything:

```console
$ tools/reloc_data.py \
    --manifest config/reloc-data.main.exe.json \
    --input-dir .shake/gen/main.exe/asm/data \
    --check
reloc-data: validated 8 pointer words and 8 exact targets across 2 files
```

Or write separate assembly inputs for a future normal-link lane:

```console
$ tools/reloc_data.py \
    --manifest config/reloc-data.main.exe.json \
    --input-dir .shake/gen/main.exe/asm/data \
    --output-dir .shake/build/reloc-data/asm
```

Only the two touched files are emitted. This prototype is intentionally not
wired into `./Build`; the default matching lane still consumes Splat's files
unchanged.

## Binutils proof

The proof uses GNU MIPS assembler and linker output, not a source-level
assumption:

- unmodified `72CD0.data.s.o` has 343 `.rel.data` records;
- the rewritten object has 351, with the eight additions all `R_MIPS_32`
  against the new exact target symbols;
- each target symbol is section-relative in `207C.data.s.o` at the expected
  interior offset, never `ABS`;
- a retail-address partial link reproduces the eight shipped pointer words;
- moving both loaded-data inputs by `+4` increments every pointer by `+4`; and
- moving them by `+0x10004` increments every pointer by `+0x10004`, including
  the high-word carry.

The focused test performs the same assemble/readelf/link checks with hermetic
fixtures when MIPS binutils are available:

```console
$ python3 -m unittest -v tools.tests.test_reloc_data
```

After this slice, the audit's literal-pointer count falls from 185 to 177 and
the trailing input's symbolic-word count rises from 343 to 351.

## Scaling without inventing source

Continue by coherent owner, not by blindly replacing every aligned word. The
next strong candidates are `CD_comstr`/`CD_intstr`, `ThinkDB`, and the named
four-entry archive/sprite tables. For every batch:

1. confirm the exact target directive and enclosing owner;
2. name the exact interior object, not an enclosing blob plus a guessed addend;
3. require retail bytes plus `R_MIPS_32` in assembler output;
4. require controlled `+4` and `+0x10004` links; and
5. migrate to typed C tables only when PSX.SYM, headers, or uses establish the
   genuine type.

Symbolic assembly remains an honest, editable relocation-bearing source form
when a stronger type is not yet known. This is the loaded-data counterpart to
the symbolic/original-object policy in [relocatable-build.md](relocatable-build.md).
