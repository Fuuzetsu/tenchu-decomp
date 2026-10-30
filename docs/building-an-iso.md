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
$ ./Build iso            # matching main.exe -> .shake/build/tenchu/tenchu.{bin,cue}
$ ./Build iso-mod        # modded main.exe   -> .shake/build/tenchu/tenchu-mod.{bin,cue}
$ ./Build iso-relink     # normal relink     -> .shake/build/tenchu/tenchu-relink.{bin,cue}
$ ./Build run            # FAST: -loadexe main.exe over the original disc (no repack)
$ ./Build run-iso        # FAITHFUL: repack the disc with main.exe and boot it
$ ./Build run-mod        # fast, -loadexe the modded main.exe
$ ./Build run-iso-mod    # FAITHFUL: repack the disc with the modded main.exe and boot it
$ ./Build run-relink     # fast, -loadexe the normal-link main_relink.exe
$ ./Build run-iso-relink # full boot from the auto-packed relink disc
```

**Mods are patched in place, so the disc stays faithful.** `./Build mod`
(tools/mkmod.py) compiles each `src/mod/main.exe/<fn>.c`, links it at the function's
*original* address, and overwrites that function's bytes — the function has to fit in
its original slot (up to the next symbol), so `main_mod.exe` is **exactly the same
size** as `main.exe`. That lets mkiso retain every dumped `offs=` placement: the
rebuilt data track is byte-identical to the original except `MAIN.EXE`'s sectors.

A larger relink cannot fit the original 271-sector `MAIN.EXE` slot.
`iso-relink` uses `tools/mkiso.py`, which handles that mechanically by removing
forced offsets and letting mkpsxiso auto-pack the data track.

The static executable/media audit supports this design more strongly than a
generic “LBAs might be embedded” warning:

- RUN reads the fixed `0x80100000` handoff record and `Exec` starts the loaded
  executable at the PC from its PS-X header; neither calls a fixed retail MAIN
  entry address;
- MAIN's AFS/XA paths use filenames and `CdSearchFile`/`CdlFILE` positions; and
- MENU/SLPS/ENDING STR paths likewise search by filename and consume returned
  file positions.

That static model now has a bounded emulator result behind it: the current
`+0x10004` growth artifact was auto-packed and reached the moved MAIN entry,
current ELF-derived `main`, moved `PadProc`, and post-loop VSyncs through the
real `SLPS→MENU→MAIN` chain without a first-chance exception. A later probe
also reached relocated `OPEN06.STR` decode and `STAGES.XA` command/callback
checkpoints described below.

If a modded function outgrows its slot, `./Build mod` aborts and tells you by how many
bytes — trim it (drop debug logging, simplify) until it fits. See
[modding-and-nonmatching.md](modding-and-nonmatching.md).

The matching and relink lanes each have a fast and a full-boot launch:

- **`./Build run`** (fast) mounts the original disc and `-loadexe`s our `main.exe`
  over it — no ISO repack, so the edit→test loop is just `./Build run`. Because
  Tenchu boots `SLPS_019.01 → … → TENCHU/MAIN.EXE`, this jumps straight to
  `MAIN.EXE` and skips that launcher; great for iterating on `main.exe`, but if the
  game needs the full boot chain use `run-iso`.
- **`./Build run-iso`** (faithful) boots the repacked disc (our `MAIN.EXE` swapped
  in), running the real `SLPS_019.01 → … → MAIN.EXE`. You can also load `tenchu.cue`
  by hand (drag it into pcsx-redux, or *File → Open Disk Image*).
- **`./Build run-relink`** is the same fast direct-load path for
  `main_relink.exe`. The normal-link finalizer has already regenerated its
  header PC/load/size fields.
- **`./Build run-iso-relink`** boots `tenchu-relink.cue` through the real
  launcher chain. When `main_relink.exe` outgrows retail, this is the
  auto-packed path. The controlled `+0x10004` fixture has passed its launcher,
  menu, MAIN entry, and main-loop smoke; use it plus representative media tests
  for a particular mod.

`tenchu.{bin,cue}`, `tenchu-mod.{bin,cue}`, and
`tenchu-relink.{bin,cue}` are real Shake targets, so the ISO launchers **repack
only when their executable changed**—repeated launches do not rebuild the
~750 MB image. The one thing not tracked is a swap of the *original* disc
(`mkiso.py` discovers it dynamically): `./Build clean` to force a repack then.

`run` finds the emulator via `$PCSX_REDUX`, then `pcsx-redux` on `PATH`, then
`~/programming/pcsx-redux/pcsx-redux`; extra emulator flags go in `$PCSX_REDUX_ARGS`.
Both modes need the original disc (the fast path mounts it for assets; the faithful
path repacks from it), found via `$TENCHU_CUE` or under `disks/` / `~/tenchu-iso/`.

**CPU/BIOS:** no BIOS needed — pcsx-redux falls back to OpenBIOS. But its x64
**dynarec crashes with OpenBIOS** ("Unrecoverable error while running recompiler"
during Tenchu's intro) — a regression of
[grumpycoders/pcsx-redux#695](https://github.com/grumpycoders/pcsx-redux/issues/695)
(that combo was fixed in 2022, then re-broke in the emulator's memory-system
rework). So `run` forces `-interpreter` by default. To use the faster dynarec,
supply a real BIOS and opt in:
`PCSX_REDUX_ARGS='-bios /path/scph.bin -dynarec' ./Build run` (any of `-dynarec`,
`-bios`, `-interpreter` in `$PCSX_REDUX_ARGS` disables the forced default).

## Automated grown-image smoke

`tools/pcsx_smoke.py` provides the repeatable bounded proof. It discovers the
original cue through `--cue`, `$TENCHU_CUE`, `disks/`, or `~/tenchu-iso/`, and
PCSX-Redux through `--pcsx`, `$PCSX_REDUX`, `PATH`, or the usual
`~/programming/pcsx-redux/pcsx-redux` checkout. First generate the controlled
growth artifact:

```console
$ ./Build check-relink-growth
```

The direct `-loadexe` result was produced with:

```console
$ python3 tools/pcsx_smoke.py \
    --exe .shake/build/reloc-growth-probe/main_growth.exe \
    --elf .shake/build/reloc-growth-probe/main_growth.exe.elf \
    --frames 5 --loop-hits 2 --timeout 30
TENCHU_SMOKE armed entry=0x80070260 main=0x800162a4 loop=0x8002adac frames=5 loops=2
TENCHU_SMOKE PASS entry=1 main=1 frames=5 loops=2 cycles=220072300ULL
```

The full auto-LBA result was produced with:

```console
$ nix develop -c python3 tools/pcsx_smoke.py \
    --exe .shake/build/reloc-growth-probe/main_growth.exe \
    --elf .shake/build/reloc-growth-probe/main_growth.exe.elf \
    --repack --frames 5 --loop-hits 2 --timeout 150 --repack-timeout 120
mkiso: main.exe grew (620544 > 555008) — using auto-LBA layout
TENCHU_SMOKE armed entry=0x80070260 main=0x800162a4 loop=0x8002adac frames=5 loops=2
TENCHU_SMOKE PASS entry=1 main=1 frames=5 loops=2 cycles=1528346866ULL
```

That run logged `execute \TENCHU\MENU.EXE;1` followed by
`execute \TENCHU\MAIN.EXE;1`. The harness reads the executable entry from the
grown PS-X header and resolves `main` and `PadProc` from its ELF. Since the
fixture is inserted after `START.c.o`, `main` intentionally remains
`0x800162a4`; the entry moved to `0x80070260` and `PadProc` to `0x8002adac`.

The probe forces a debugger-enabled interpreter and software GPU, uses isolated
temporary memory cards, cleans the temporary ~748 MB repack, and terminates the
whole emulator process group on timeout. In `--repack` mode it supplies bounded
START/CIRCLE/CROSS pulses to cross SLPS/MENU, fails on a first-chance CPU
exception or unexpected pause, and counts VSyncs only after `PadProc` has run.
It proves the boot/main-loop path.

A subsequent representative-media run on the same auto-LBA `+0x10004` disc
observed both files at their repacked positions:

- `OPEN06.STR` was found at runtime LBA 46,593 rather than retail LBA 46,561
  (`+32`), completed stream setup, called `StGetNext`/`get_stream`, and made 21
  combined `DecDCTin`/`DecDCTout` calls by frame 1,381.
- `STAGES.XA` was found at runtime LBA 119,130 rather than retail LBA 119,098
  (`+32`), completed XA setup and `CdControl(0x1b)`, and made four `cbCheckCD`
  callbacks by frame 3,557.

There was no CPU exception. The run stopped at those checkpoints rather than
EOF, so it does not prove complete playback, physical audio output, or broad
gameplay.

## TRIAL editor runtime check

`tools/pcsx_trial.py` boots the built TRIAL executable directly with the
original Gaisen disc available for assets. It chooses Create, accepts the
default settings, passes the card check, and moves the terrain cursor right
and left. Run it after the exact-image gate:

```sh
nix develop --command ./Build check-trial.exe
nix develop --command python3 tools/pcsx_trial.py --pcsx /path/to/pcsx-redux
nix develop --command python3 tools/pcsx_trial.py --pcsx /path/to/pcsx-redux --save
nix develop --command python3 tools/pcsx_trial.py --pcsx /path/to/pcsx-redux --text-entry --save
```

The wrapper accepts `--exe`, `--elf`, `--cue`, `--bios`, and `--timeout`; its
defaults select `.shake/build/tenchu/trial.exe` and the corresponding ELF.
Disc and emulator discovery use the existing smoke tool's conventions.
The emulator must have its shared-library runtime available; `--pcsx` may
point to a local wrapper that supplies that environment.

The probe uses ordinary controller overrides. Function addresses come from
the selected ELF and each execution breakpoint checks the selected EXE's
instruction word. The terrain function's inventory span restricts which
`GetRealPad` calls count as editor input. The EXE snapshot, emulator settings,
and formatted memory cards live in a temporary directory. It rejects CPU
exceptions, unexpected pauses, callback failures, and timeouts.

Verified 2026-10-30: Create → settings → card check → terrain editing, followed
by 90 frames and 46 input polls. Cursor sprite x moved 0 → 25 → 0; the mission
retained theme 0 and the player at grid (16, 18). No game-memory patch or menu
bypass is used. The default run stops at this cursor check. Playtest and the
complete boot/menu-to-TRIAL handoff remain untested by these scenarios.

The optional `--save` scenario continues after the cursor check: Select opens
the command menu, three Down presses choose Save, and Circle confirms saving
to the temporary card. It requires the successful-write page and return to
terrain editing. After the emulator exits, Python validates the card directory
entry, its checksum, the file's icon header, and all 7,484 serialized mission
bytes against a read-only snapshot of game memory. The verified new file was
`BISLPS-01901M0544180`; the probe returned to the map with 62 pad polls at VSync
1,758. This covers creating a user-mission save, but not overwriting, deleting,
formatting a card, or saving preset records. Only disposable cards are written.

The optional `--text-entry` scenario opens the title keyboard from settings.
R2 and L2 switch Latin → Katakana → Latin; Circle enters full-width A (Shift-JIS
0x8260), and Start returns to settings. The probe checks the selected category,
inserted code, advanced cursor, exported title in the settings header, and the
title copied into the mission. It accounts for trailing-space trimming and
the settings wrapper clearing the shared text buffer. Combined with `--save`,
this passed at VSync 2,008 with 62 terrain pad polls and verified file
`BISLPS-01901M00f1980`. Kanji paging, deletion, and every text-input mode have
not been exercised by this bounded scenario.

## What you get

- **`./Build iso`** points the disc's `TENCHU/MAIN.EXE` at the matching build and
  rebuilds with forced LBAs, so the data track is **byte-identical to the
  original disc except `main.exe`'s sectors** (verified). A same-size mod (edit
  `src/main.exe/…` keeping instruction count) rides the same path — only your
  changed bytes differ, so it boots exactly like the original.

- **`./Build iso-mod`** puts the modded `main_mod.exe` (functions patched in place
  by `./Build mod`, see [modding-and-nonmatching.md](modding-and-nonmatching.md)) on
  the disc. Because it's the **same size** as the original `main.exe`, the build keeps
  forced LBAs — the data track is byte-identical to the original except `main.exe`'s
  sectors, exactly like `./Build iso`. So the full boot chain and all streamed
  movies/XA work; `run-iso-mod` is the faithful way to play a mod.

- **`./Build iso-relink`** puts `main_relink.exe` on the disc. A retail-sized
  output retains forced placement; a larger output is auto-packed. Packaging is
  implemented, static code inspection supports the relocated file positions,
  and the controlled `+0x10004` image has passed the full launcher/menu/main-loop
  smoke plus representative STR decode and XA setup/callback checkpoints.
  Complete playback, physical XA audio output, and broad gameplay remain runtime
  validation gates.

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
