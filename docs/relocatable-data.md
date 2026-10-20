# Relocatable loaded data

The reviewed manifest covers 190 literal pointer words and 117 targets in the
current loaded image. Use the live widened audit rather than treating either
count as an invariant. The inventory includes more than aligned `.word`
directives: 22 pointers are little-endian fields emitted as four `.byte`
directives in `StageConfig`, and one aligned pointer value targets an unaligned
cursor two bytes into a BSS buffer.

There are four pointer-source files and eleven files once exact target-label owners
are included:

- `149C.data.s` contains 22 `StageConfig` name/path pointers;
- `76394.data.s` contains seven `HumanData` and `WeaponModel` name pointers;
- `797F0.data.s` contains 152 other pointer words;
- `87458.data.s` contains the final 9 pointer words after the recovered
  compiler-generated `.sdata` sections; and
- targets span those files plus `E58`, `207C`, `2EB0`, `33C4`, `37A8`,
  `400C`, and `4900`.

The generated owner around a target is often broader than the object being
addressed, so rewriting an interior pointer as `D_800xxxxx + addend` would
normally invent an ownership claim. The one addend in the manifest is explicit
and independently supported by matching C: `AdtMsgPtr` is the write cursor
`AdtMsgBuf + 2` after the `"%#"` prefix.

The current owner distribution is:

| Generated source owner | Pointers |
|---|---:|
| `BriefingAssets` | 76 |
| `CD_comstr` | 32 |
| `StageConfig` | 22 |
| `CD_intstr` | 8 |
| Nine coherent four-entry tables | 36 |
| `WeaponModel` | 3 |
| Thirteen singletons | 13 |

The audit inventory is conservative: a pointer-shaped value only becomes a
manifest record after its table structure, type/use, owner, and target are
reviewed. Alignment is not an acceptance rule; the BSS cursor is the concrete
counterexample.

## Representation

`tools/reloc_data.py` applies `config/reloc-data.main.exe.json` to copies of
Splat's generated assembly. Each reviewed manifest entry records:

- the pointer's generated file, exact address, and enclosing `dlabel` owner;
- the target's generated file, exact address, and enclosing owner, or one
  reviewed existing section-owned base plus an explicit addend; and
- a human-readable symbol for the exact target.

The transformer normally inserts a section-relative object label directly at
the target and changes only the pointer operand from a literal to that symbol.
For example, the representation is conceptually:

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
by the data itself. The cursor entry is the deliberately narrow exception:
the manifest spells `AdtMsgBuf + 2`, and the transformer and all link probes
check that exact expression.

For byte-packed fields the transformer first proves that four consecutive
`.byte` directives encode the reviewed little-endian pointer, then replaces
the four directives with one `.word symbol`. This preserves size/alignment and
causes GNU `as` to emit the same `R_MIPS_32` record as an originally word-typed
table.

The tool refuses stale or ambiguous input: wrong owners, missing addresses,
changed pointer literals, duplicate sources, symbol/target conflicts, and
attempts to overwrite the generated input directory all fail before any
output is written.

## Reviewed slices

The leading-data slice is the typed `TStageConfig` array recovered from
PSX.SYM. Eleven 0x1c-byte records each contain a `name` and `path` pointer, for
22 byte-packed pointers total. Their target labels are exact starts of the
corresponding stage strings in the same generated file. This slice is important
because an audit restricted to `.word` syntax would miss it completely.

The committed manifest covers eight clear, game-owned language tables plus a
typed demo-screen asset matrix. The first slice was:

- `STAGE_SOUND_PREFICES` at `0x8008EA58..0x8008EA64`; and
- `STAGE_ANIMATION_PREFICES` at `0x8008EA68..0x8008EA74`.

The second slice adds six adjacent, named four-entry tables:

- `ITEM_SELECTION_SCREEN_PATHS`;
- `STAGE_RESULT_BACKGROUND_PATHS`;
- `STAGE_RESULT_RANK_ARCHIVE_PATHS` and `MISSION_SCORE_RANK_ARCHIVE_PATHS`, which deliberately share the
  same four targets;
- `MISSION_SCORE_BACKGROUND_PATHS`; and
- `GAME_OVER_ARCHIVE_PATHS`.

Their strings spell the English, French, Italian, and Japanese filenames in
the same order as the tables.

The next slice covers all 76 remaining literal pointers in `BriefingAssets`.
The C declaration is `DemoScreenAssets BriefingAssets[][11]`, where
each record contains `background`, `foreground`, and `music` fields. The data
is four language blocks of eleven stage records, and each reviewed target is
the exact start of the corresponding `stNNN.tim` or `mojoN.tim` string. Stage
9 has null asset fields. The four `mojo11.tim` fields were already symbolic
references to the exact enclosing `D_800136B0` label, so the manifest leaves
them alone rather than manufacturing redundant aliases.

The later trailing slice covers 59 words:

- four `HumanData[].name` fields and three `WeaponModel[].name` fields, whose
  0x18-byte and 0x0c-byte record types are exercised directly by the matching
  C sources;
- all 32 `CD_comstr` and eight `CD_intstr` entries, including their shared
  `"?"` string;
- four SDK singleton pointers to the BIOS/intr/GPU revision strings and the
  hexadecimal digit alphabet;
- the six XA filenames consumed by the typed `PlayVoice` paths; and
- the `TENCHU_ID` and `CID` card-id string pointers consumed by the card/save
  C sources.

The old manifest also covered 18 literal `ThinkDB[].name` fields. `THINK.C`
now owns the complete typed table: its compiled object carries `R_MIPS_32`
for all 19 non-null names, followed by a null sentinel. Those 18 raw-assembly
records were removed on 2026-10-21 because their source words no longer exist
in generated assembly. The C object's relocations remain covered by the
normal-link input audit and growth proof.

The final cursor word is `AdtMsgPtr` at `0x80097E98`. Matching C writes `"%#"`
to `AdtMsgBuf`, presents the buffer to `AdtMessageBox`, and stores
`&AdtMsgBuf[2]` as the cursor. Its target is
therefore intentionally unaligned and belongs to linker-owned BSS rather than
one of the generated initialized-data files.

Together the slices cover 190 pointer words and 117 targets across eleven
generated files. The names and the sole addend are supported by PSX.SYM types,
matching source table/field uses, or literal contents. No wider top-level
ownership is inferred.

Validate the manifest against a generated tree without writing anything:

```console
$ tools/reloc_data.py \
    --manifest config/reloc-data.main.exe.json \
    --input-dir .shake/gen/main.exe/asm/data \
    --check
reloc-data: validated 190 pointer words and 117 exact targets across 11 files
```

Or write separate assembly inputs directly:

```console
$ tools/reloc_data.py \
    --manifest config/reloc-data.main.exe.json \
    --input-dir .shake/gen/main.exe/asm/data \
    --output-dir .shake/build/reloc-data/asm
```

Only the eleven touched files are emitted. The default matching lane still
consumes Splat's files unchanged.

The standalone build gate applies the transform to temporary copies, assembles
the touched objects, and performs the controlled links described below:

```console
$ ./Build check-reloc-data
reloc-data-lane: verified 190 R_MIPS_32 pointer words, 117 exact targets, and 3 link layouts across 11 files
check-reloc-data: reviewed pointer tables are retail-exact and shift-relocatable
```

`./Build relink` composes all eleven replacements: `E58`, `149C`, `207C`,
`2EB0`, `33C4`, `37A8`, `400C`, `4900`, `76394`, `797F0`, and `87458`. The BSS
transform runs after the `87458` pointer rewrite so both changes survive in one object. The
standalone gate remains the stricter per-record controlled-shift oracle.

## Binutils proof

The proof uses GNU MIPS assembler and linker output, not a source-level
assumption:

- all 190 reviewed sources are individually required to carry `R_MIPS_32` at
  their exact offsets and against the expected target symbol/expression;
- inserted target labels and the existing `AdtMsgBuf` base are
  section-relative, never `ABS`;
- a retail-address partial link reproduces all 190 shipped pointer words;
- moving the relevant source/target sections by `+4` produces the exact moved
  values; and
- the `+0x10004` layout does the same across a high-word carry, including the
  byte-packed `StageConfig` fields and the unaligned BSS cursor.

The focused test performs the same assemble/readelf/link checks with hermetic
fixtures when MIPS binutils are available:

```console
$ python3 -m unittest -v \
    tools.tests.test_reloc_data tools.tests.test_reloc_data_lane
```

Applying the manifest to the current generated inputs reduces the widened live
literal-pointer inventory from 190 to zero. The ordinary generated inputs are
unchanged; only the relocation lane consumes transformed copies.

The default `tools/reloc_audit.py` invocation audits that composed normal
relink, not the untouched matching linker. It maps all eleven substituted objects
to their transformed sources, reconstructs pointer-sized values from remaining
`.byte` directives, and accepts unaligned movable targets as candidates. The
audit rejects a linked replacement without an exact object-to-source mapping,
and it rejects a mapping whose object is absent from the selected linker. This
keeps stale retail source from being counted after its object has been
replaced. Use repeatable `--object-source OBJECT=SOURCE` arguments when auditing
another composed layout; explicit mappings override applicable defaults.

## Scaling without inventing source

If future generated data introduces new candidates, continue by coherent
owner, not by blindly replacing every aligned word. For every batch:

1. confirm the exact source bytes/directive and enclosing owner;
2. name the exact interior object, not an enclosing blob plus a guessed addend;
3. require retail bytes plus `R_MIPS_32` in assembler output;
4. require controlled `+4` and `+0x10004` links; and
5. migrate to typed C tables only when PSX.SYM, headers, or uses establish the
   genuine type.

Symbolic assembly remains an honest, editable relocation-bearing source form
when a stronger type is not yet known. This is the loaded-data counterpart to
the symbolic/original-object policy in [relocatable-build.md](relocatable-build.md).
