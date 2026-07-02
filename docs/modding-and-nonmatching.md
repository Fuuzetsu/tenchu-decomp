# Non-matching builds: modding & debug

You don't always want a byte-match — modifying behaviour, adding debug output,
cheats, etc. This documents what works today and how to make deviations run.

## Building a non-matching binary

`./Build` already builds regardless of matching — only `./Build check` compares
the sha256. So the mod workflow is just:

```console
$ # edit src/main.exe/<fn>.c however you like
$ ./Build            # produces .shake/build/tenchu/main.exe (no hash gate)
```

`check` is only for verifying a decomp still matches; ignore it while modding.

## What works: same-size edits (verified)

An edit that keeps the function the **same size** (same instruction count)
produces a valid, runnable binary. Verified: changing `arg0 >> 4` to `arg0 >> 5`
in `get_held_buttons` yields a binary that is **byte-identical except the one
changed instruction** — same total size, PS-EXE header untouched. Everything
else stays at its original address, so all the game's pointers (symbolic and raw)
are still correct.

Same-size covers a lot: changing constants/immediates, swapping a branch
condition, changing which register/field is used, reordering independent ops,
tweaking arithmetic. Prefer these when you want a drop-in behaviour change.

## What breaks: size-changing edits (and why)

Adding or removing instructions (e.g. a `printf` debug line, an extra call)
**changes the function's size**. Verified: adding a second call to
`get_held_buttons` grew the binary by 8 bytes and made ~68% of the file differ —
because everything laid out **after** the edited function shifts down by 8.

The build still succeeds (it links), but the result is **not runnable as-is**,
because this is a *partial* decomp with a fixed memory layout:

- The linker script places sections **sequentially** from `0x80011000`, so a
  function that grows pushes every later function and data block down.
- The data is a **mix**: splat has resolved ~1300+ pointers in the first data
  block alone to **symbolic** references (jump tables, function pointers) that
  the linker relocates — but thousands of other words are still **raw** bytes,
  including pointers it hasn't symbolised yet, plus a set of **fixed** addresses
  in `config/symbols.main.exe.txt` / `undefined_symbols_auto`.
- When code shifts, the symbolic references move with it but the raw/fixed ones
  don't. The two disagree → the binary is internally inconsistent → it crashes.
- The PS-EXE header's `.text` size (`0x00087000`, hardcoded in the generated
  `header.s`) also no longer matches the grown image.

In short: a fixed-layout partial decomp can absorb *substitutions* but not
*insertions*.

## Making growth work

Three options, cheapest first:

1. **Stay same-size.** For many tweaks you can. MIPS has 32 registers and lots of
   delay slots; you can often express a change without adding instructions.

2. **Trampoline to a mod region (recommended for debug / real growth).** Leave
   the original function's slot the same size, put a `j <mod_addr>` (+ `nop`) in
   it, and compile the grown/debug version into a **new section in free RAM**.
   The exe loads at `0x80011000`–`~0x80098000`; RAM is free from there up to the
   initial `sp` (`0x801FFF F0`), ~1.5 MB. Because every original address stays
   put, all callers (symbolic *and* raw) reach the slot and jump to your code.
   This is the standard ROM-hack hook; it is not wired into the build yet — it
   would need (a) a `mod` output section appended in the linker script and (b) a
   rule that emits the trampoline for a hooked function. Ask and it can be added.

3. **Decompile more.** Once the data around your function is fully symbolic (no
   raw pointers, no fixed addresses into that region), a uniform shift becomes
   self-consistent and functions can grow freely. This is the natural end-state
   of the decomp; it just isn't there yet for most of the binary.

## Running a modified exe

Structural validity: a same-size mod is a valid PS-EXE (header + all other bytes
unchanged), so it boots like the original. To actually run it you need to rebuild
the disc image with the patched `main.exe` (e.g. `mkpsxiso`) and load it in an
emulator (DuckStation / pcsx-redux) or on hardware — that tooling isn't in the
repo yet. The decomp only rebuilds `main.exe`; the rest of the disc
(`disks/tenchu/…`) is the original.
