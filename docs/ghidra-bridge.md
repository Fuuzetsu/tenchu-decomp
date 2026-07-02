# Bridging the Ghidra project into the decomp

You have a separate Ghidra project with symbols/types/decompilation. This brings
its decompiled C into the repo **one function at a time**, so each function gets a
`src/main.exe/<fn>.c` with the ASM split out and Ghidra's C as a reference to turn
into matching C. The build stays byte-identical at every step.

## 1. Export from Ghidra (once, re-run when it changes)

`tools/ghidra/ExportDecomp.java` decompiles every function and writes, to a chosen
output dir:

- `functions.tsv` — one line per function: `<hexaddr>\t<sizeBytes>\t<name>`
- `c/<hexaddr>.c` — that function's decompiled C

Run it on the program that holds `main.exe` (loaded at `0x80011000`):

- **GUI:** Script Manager → add `tools/ghidra` to script dirs → run `ExportDecomp`
  → pick an output dir; or
- **headless:**
  ```
  analyzeHeadless <proj_dir> <proj_name> -process <program> \
    -scriptPath tools/ghidra -postScript ExportDecomp.java <outDir>
  ```

The function **size** in `functions.tsv` is what makes this reliable — splat needs
the exact extent, and guessing from the next symbol is wrong (there's often data
between functions).

## 2. Reverse a function

```console
$ nix develop
$ tools/reverse.py <name> --ghidra-export <outDir>
```

For that one function it:

1. adds `[offset, c, <name>]` to `config/splat.main.exe.yaml` (splitting the
   surrounding `data` block; leading/trailing data is preserved);
2. re-runs splat, so its ASM appears under `.shake/gen/…/nonmatchings/<name>/`;
3. writes `src/main.exe/<name>.c` = the `INCLUDE_ASM` stub **+ Ghidra's C in `//`
   line comments** (line comments because Ghidra's C often contains `/* */`);
4. rebuilds and asserts **`./Build check` is still byte-identical** — the split
   only relabels bytes as code, so the output must not change. If it fails, the
   function extent was wrong (or the region has rodata splat must be told about);
   revert with `git checkout config/splat.main.exe.yaml && rm src/main.exe/<name>.c`.

Then turn the commented C into real matching C, drop the `INCLUDE_ASM`, and iterate
with the permuter / asm-differ as usual. Same pattern as `get_held_buttons`.

Manual mode (no export): `tools/reverse.py <name> --addr 0x… --size 0x…`.

## Symbol names (done)

`tools/ghidra/ExportSymbolsTypes.java` dumps all global symbols → `symbols.tsv`,
and `tools/import_symbols.py` adopts those names across the whole decomp
(`config/symbols`, the splat yaml, `src/*.c` + filenames, `main.exe.h`). Only
names change (addresses are identical) so `./Build check` stays byte-identical.
Re-run it after you rename things in Ghidra:

```console
$ tools/import_symbols.py --ghidra-export .shake/ghidra-export
```

It renames game-RAM symbols only (leaves PSX hardware/BIOS names), never
downgrades a real name to a Ghidra placeholder (`FUN_…`), and skips collisions.

## Types (reference)

`ExportSymbolsTypes.java` also writes `types.h`; a snapshot is committed at
[`reference/ghidra_types.h`](../reference/ghidra_types.h) (157 structs/enums).
It's **reference only, not built** — types affect codegen, so replacing
`main.exe.h` wholesale risks breaking already-matched functions. Adopt types
**per function** as you reverse (copy the struct you need into `main.exe.h`,
adjust, and let `./Build check` catch layout mistakes).

Example: `Think1sleep` is blocked by a wrong
`character_state` layout. Ghidra's decompilation is `Me_THINK_C->motion->mid` /
`->count` — i.e. the character struct has a `motion` pointer at 0x5C (correct),
and `motion` points to a `{ short mid; short count; … }`. Pull that layout from
`reference/ghidra_types.h` to fix it.

## Notes

- Only the program's data-type manager is exported (PSY-Q + applied game types).
  Game structs in the separate "Shared data types" *archive* that aren't applied
  in `MAIN.EXE` won't appear; export the archive separately if needed.
- rodata: if a split function has associated `.rodata` (jump tables, floats)
  inside its region, the byte-check catches the mismatch — handle case-by-case
  (a splat `rodata` subsegment) for now.
