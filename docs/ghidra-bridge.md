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

## Notes / next

- The importer currently pulls only the decompiled **C**. Ghidra's **types/structs**
  and **symbol names** are also valuable — importing structs would directly unblock
  `think_setting_sleep` (its blocker is a `character_state` layout mismatch). That's
  a follow-up (a types→`main.exe.h` exporter); ask when you want it.
- `ReduxSymbols.java` (your existing script) already syncs symbol names to
  pcsx-redux; a similar exporter could feed `config/symbols.main.exe.txt`.
- rodata: if a function has associated `.rodata` (jump tables, float constants) that
  lives inside the split region, the byte-check will catch a mismatch — handle those
  case-by-case (splat `rodata` subsegment) for now.
