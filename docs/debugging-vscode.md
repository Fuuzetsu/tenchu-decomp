# Debugging Tenchu from VSCode

PCSX-Redux ships a GDB server; a MIPS-aware gdb attaches to it, and VSCode
drives that gdb. This gives a real IDE debug session — named call stacks,
breakpoints by function, registers, memory, watch expressions, disassembly,
and instruction stepping — instead of PCSX-Redux's built-in debugger UI.

## One-time setup

- **VSCode extension:** install Microsoft C/C++ (`ms-vscode.cpptools`); the
  committed `.vscode/launch.json` uses its `cppdbg` adapter.
- **gdb:** none needed on PATH. `tools/mips-gdb` materialises nixpkgs gdb
  (17.1, `mips:3000` target) behind a gcroot on first use and execs it;
  `launch.json` points `miDebuggerPath` at it. Override the nixpkgs checkout
  with `TENCHU_NIXPKGS` if yours is not at `~/programming/nixpkgs`.

## Attaching

1. Launch the game with the GDB server enabled (port 3333):

   ```console
   $ TENCHU_GDB=1 ./Build run-relink      # or run, run-mod, run-iso*, …
   ```

   `TENCHU_GDB=<port>` picks another port. The server coexists with the
   `-dofile` symbol loader and the deferred-launch wrapper, so the build lock
   is released while you debug. Setting `TENCHU_GDB` also builds this layout's
   source-line script (see below) as part of the run.

2. In VSCode, pick the matching **Attach: PCSX-Redux (…)** configuration
   (exact / relink / mod — they differ only in which ELF supplies symbols) and
   start debugging. It connects to `localhost:3333`, stops on connect, and
   you can set breakpoints (`break PadProc`, or click the gutter in the
   disassembly view), inspect `$pc`/registers, read memory, and step.

The ELF's symbol table names everything, so call stacks read as
`PadProc → …` rather than raw addresses — the same names the in-emulator
disassembly gets from `<artifact>.symbols.lua`.

## C source lines

Source-level breakpoints and stepping work, with no extra step: launching
with `TENCHU_GDB=1` builds the matching source-line script for that layout
automatically (exact/mod → `main.exe.debug.gdb`, relink →
`main_relink.exe.debug.gdb`), and prints its path. Then attach with the
matching `launch.json` config — it already `source`s that script — and a
breakpoint in a matched C file (e.g. `ProcItemKusuri`) binds to
`file src/main.exe/ProcItemKusuri.c, line N` and stepping shows C. INCLUDE_ASM
stubs stay instruction-level (no C source exists for them).

The first `TENCHU_GDB=1` run compiles the ~600 debug objects (tens of
seconds); later runs are incremental. `./Build debug-gdb` /
`debug-gdb-relink` build the script on their own if you want it without
launching.

### Why it is built this way

This project builds with the original 1997 `gcc 2.8.1`, which emits STABS
(or DWARF 1). Modern gdb (17.1, and 12.1 — the bug is old) **crashes**
reading gcc 2.8.1's builtin *type* stabs: an internal-error in
`create_range_type` on its self-referential integer ranges
(`unsigned int:t4=r4;0;-1;`) and `size;0` floats. A bit-older gdb does not
help — tested. Source stepping only needs the *line* records, though, so
`tools/stabs_lines.py` filters `-gstabs` output down to the
`N_SO`/`N_FUN`/`N_SLINE` (file/function/line) records and drops every
crash-inducing type descriptor. Types come from the Typed Debugger import
(`tools/redux_typed_debugger.py`) instead.

`-gstabs` was verified not to change `.text`, so each matched C file
recompiles to a byte-identical `-gstabs` debug object under `.shake/main.exe-dbg/`.
A single *merged* debug ELF does **not** work: GNU ld collapses the `N_FUN`
records when it merges hundreds of `.stab` sections (two objects link fine;
~600 leave only a handful of functions with line info). So instead
`tools/gen_debug_gdb.py` emits a gdb script that `add-symbol-file`s each
per-object debug object — which is self-consistent — at the address its
function occupies in the launched layout (read from that layout's ELF). One
debug-object set serves the exact, mod, and relink images; only the resolved
addresses differ, which is why there is a per-layout script.

## Notes

- Use one attach at a time; the server serves a single client.
- The disc chain (`run-iso*`) works the same — attach once MAIN.EXE is
  running. On the disc path SLPS/MENU occupy the same addresses first, so set
  function breakpoints (which are re-resolved) rather than raw-address ones.
- `set architecture mips:3000` is applied automatically by the config; if you
  drive gdb by hand, set it before `target remote`.
