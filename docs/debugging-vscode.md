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

### Why it is built this way, and the one compromise

This project builds with the original 1997 `gcc 2.8.1`, which emits STABS
(or DWARF 1). Modern gdb (17.1, and 12.1 — the bug is old, and a bit-older
gdb does not help; tested) **crashes** — an internal-error in
`create_range_type`, not a graceful failure — reading three of gcc 2.8.1's
otherwise-standard type encodings, each of which comes out zero-length:
self-referential unsigned ranges (`unsigned int:t4=r4;0;-1;`), `size;0`
floats/doubles, and arrays with a type-0 index. This is the fundamental
issue: gdb abandoned its STABS reader, and this compiler predates DWARF 2.

`tools/stabs_lines.py` patches only those three forms into ones the current
reader accepts, keeping everything else — so gdb gets the real types and
**Locals shows variables with values** (ints, pointers, structs, arrays),
not just names. The single casualty: `float`/`double` are rewritten to
int32 ranges, so a float local would show its raw 32-bit value rather than a
decimal. In practice this costs nothing — the PS1 has no FPU and this game
is fixed-point: there are **zero** float/double local variables in the game
source. Exact types (including floats) are always available in the Typed
Debugger import (`tools/redux_typed_debugger.py`).

The fully robust fix — correct float display and no per-encoding patching —
is to emit modern DWARF instead of STABS; gdb reads DWARF flawlessly. That is
a larger tool and unbuilt; the patch approach covers this codebase well.
Stack-resident locals additionally depend on gdb reading a 32-bit frame
offset that it currently prints sign-oddly; register locals (most, under
`-O2`) are exact.

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

## Editing variable values — not currently supported (2026-07-20)

Reading (locals, registers, memory), breakpoints, and stepping work.
*Changing* a value does not, for two separate reasons:

- **"cannot assign to variable expression"** — gdb-side. Under `-O2` many
  locals are coalesced or live only transiently, so gdb has a value but no
  writable location and refuses before sending anything. Not fixable without
  lower optimization (which would break byte-matching).
- **The connection drops** when the target *can* be written — a PCSX-Redux
  gdb-stub bug. Its `P` (write-register) handler in `src/core/gdb-server.cc`
  assumes a two-hex-digit register number and `close()`s the socket
  otherwise; gdb sends register numbers minimally, so any local in registers
  0–15 ($v0, $a0–$a3, $t0–$t9, …) produces a one-digit `P` packet → `E00` +
  socket close → gdb reports "program exited, code 0" while the game keeps
  running (reattach works). Confirmed live: writing `$a1` (reg 5) drops it,
  `$s0` (reg 16) does not.

  A minimal stub fix (parse the register number up to `=`, reply `E00`
  without closing) was tried and stopped the disconnect, **but resuming then
  stopped working** — so the write path needs more than the packet parse, and
  it is parked. That patch also could not be link-tested locally: the
  pcsx-redux tree here has an `fmt` header/lib skew (`fmt::v12::report_error`
  undefined at link) unrelated to the change. Worth an upstream PR + deeper
  look when revisited.

Net: treat the debugger as read/step-only for now. Memory writes (the `M`
packet) are unaffected, so poking RAM directly via the memory view still
works.

## Notes

- Use one attach at a time; the server serves a single client.
- The disc chain (`run-iso*`) works the same — attach once MAIN.EXE is
  running. On the disc path SLPS/MENU occupy the same addresses first, so set
  function breakpoints (which are re-resolved) rather than raw-address ones.
- `set architecture mips:3000` is applied automatically by the config; if you
  drive gdb by hand, set it before `target remote`.
