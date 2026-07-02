# Building a bootable ISO (`./Build iso`)

To actually run the decomp/mods, rebuild the game's CD image with our `main.exe`
and load it in an emulator (pcsx-redux) or on hardware.

## Prerequisites: the original disc

You must provide the original disc — it's copyrighted, so it isn't in the repo.
A CD dump as a `.cue` + `.bin` (redump/DIC style; a data track `MODE2/2352` plus
a CD-DA audio track) is what's expected. Point the build at it one of two ways:

- set `TENCHU_CUE=/path/to/game.cue`, or
- drop the `.cue` (+ its `.bin`s) under `disks/` or `~/tenchu-iso/` (auto-found).

On first use the disc is dumped once with `dumpsxiso` into `TENCHU_ISO_WORK`
(default `.shake/iso/`, ~700 MB, cached; override the env var to relocate it).

## Commands

```console
$ nix develop            # provides mkpsxiso + dumpsxiso (packaged in nix/mkpsxiso.nix)
$ ./Build iso            # matching main.exe   -> .shake/build/tenchu/tenchu.{bin,cue}
$ ./Build iso-mod        # grown main_mod.exe  (from src/mod/main.exe/, see modding docs)
```

Load `tenchu.cue` in pcsx-redux (drag it in, or *File → Open Disk Image*).

## What you get

- **`./Build iso`** points the disc's `TENCHU/MAIN.EXE` at the matching build and
  rebuilds with forced LBAs, so the data track is **byte-identical to the
  original disc except `main.exe`'s sectors** (verified). A same-size mod (edit
  `src/main.exe/…` keeping instruction count) rides the same path — only your
  changed bytes differ, so it boots exactly like the original.

- **`./Build iso-mod`** puts the grown `main_mod.exe` (trampolines + mod region,
  see [modding-and-nonmatching.md](modding-and-nonmatching.md)) on the disc.
  Because it's bigger than the original slot, the build drops forced LBAs and
  lets mkpsxiso auto-pack — files after `main.exe` shift. The game finds files by
  name, so the game logic runs; streaming assets that hardcode LBAs (some
  movies/XA) may glitch. (Verified: the ISO's `MAIN.EXE` == our `main_mod.exe`,
  trampoline intact.)

## How it works

`tools/mkiso.py`: dumps the original once (`dumpsxiso -l`, capturing the exact
layout + PSX license), rewrites the `MAIN.EXE` `source` in the generated XML to
our build, then rebuilds with `mkpsxiso`. mkpsxiso reproduces the disc
byte-for-byte from that XML (validated: full round-trip of the untouched disc is
byte-identical on the data track), so swapping only `main.exe` yields a clean,
bootable image. The output is a single `.bin` with two tracks (data + CD-DA
audio) described by the `.cue`.

Note: the disc build is separate from the byte-match `check` — it produces a
*runnable* image, not a byte-match target (except incidentally, for the matching
build).
