#!/usr/bin/env python3
"""Build a *modded* (non-matching) main.exe by overwriting hooked functions IN PLACE.

This is a partial decomp with a fixed memory layout, so anything that changes the
image size shifts every later address and breaks the binary — and there's no free
RAM/disc space to relocate grown code into without disturbing the game's bss/heap or
the tightly-packed streaming assets on the disc (see docs/modding-and-nonmatching.md).

So mods must fit where they already live. For each C file in
`src/mod/main.exe/<name>.c` (filename == function name) mkmod:

  * compiles it with the normal pipeline (cpp | cc1 -G8 | maspsx | as),
  * links it at the function's ORIGINAL address, resolving the game's other symbols
    from main.exe.elf,
  * overwrites that function's bytes in main.exe with the result.

The function must fit in its original slot (its address .. the next symbol). Because
nothing moves, `main.exe` stays exactly the same size: the disc rebuild keeps forced
LBAs (byte-faithful except your function; streamed cutscenes intact) and it runs on
any emulator / real hardware — no trampoline, no 8MB, no runtime injector.

If a function outgrows its slot, mkmod aborts and tells you by how much — trim it
(drop debug logging, simplify) until it fits. A same-size *substitution* is the unit
of change a fixed-layout partial decomp can absorb; *insertions* that grow the image
are not (yet — that needs more of the surrounding data symbolised so a uniform shift
stays self-consistent).

Run inside the nix devShell (needs the cross toolchain, cc1-281 (on PATH), maspsx).
"""
import os, re, subprocess, sys, glob

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
FILE_TEXT_OFF = 0x800

CROSS = "mipsel-unknown-linux-gnu-"
CPP = CROSS + "cpp"
AS = CROSS + "as"
LD = CROSS + "ld"
NM = CROSS + "nm"
OBJCOPY = CROSS + "objcopy"
CC1 = "cc1-281"   # PS1 GCC 2.8.1 cc1, on PATH via the nix devShell

CPP_FLAGS = ("-Iinclude -undef -Wall -lang-c -fno-builtin -gstabs -Dmips "
             "-D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx -D__psx__ "
             "-D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C "
             "-DLANGUAGE_C -DHACKS").split()
CC_FLAGS = ("-mcpu=3000 -quiet -G8 -w -O2 -funsigned-char -fpeephole "
            "-ffunction-cse -fpcc-struct-return -fcommon -fverbose-asm "
            "-fgnu-linker -mgas -msoft-float").split()
MASPSX_FLAGS = ["--aspsx-version=2.77", "-G8"]
AS_FLAGS = ("-EL -Iinclude -march=r3000 -mtune=r3000 -no-pad-sections -O1 "
            "-G0").split()

MOD_SRC_DIR = "src/mod/main.exe"
HDR_DIR = "src/main.exe"
BUILD = ".shake/build/tenchu"
MAIN = f"{BUILD}/main.exe"
MAIN_ELF = f"{BUILD}/main.exe.elf"
WORK = ".shake/build/mod"


def run(cmd, **kw):
    return subprocess.run(cmd, check=True, **kw)


def nm_symbols(elf):
    """{name: vaddr} for defined symbols in elf (addresses masked to 32 bits)."""
    out = subprocess.run([NM, elf], check=True, capture_output=True, text=True).stdout
    syms = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] not in "Uu":
            syms[parts[2]] = int(parts[0], 16) & 0xFFFFFFFF
    return syms


def compile_c(cfile, ofile):
    pp = ofile + ".i"
    s_cc = ofile + ".cc.s"
    s_m = ofile + ".s"
    run([CPP, *CPP_FLAGS, "-I", HDR_DIR, cfile, pp])
    with open(s_cc, "w") as f:
        run([CC1, *CC_FLAGS], stdin=open(pp), stdout=f)
    with open(s_cc) as fin, open(s_m, "w") as fout:
        run(["maspsx", *MASPSX_FLAGS], stdin=fin, stdout=fout)
    run([AS, *AS_FLAGS, "-o", ofile, s_m])


def write_sym_ld(game_syms, exclude, tag):
    """Linker symbol script: every game symbol except `exclude` (defined by the mod)."""
    p = os.path.join(WORK, f"syms_{tag}.ld")
    with open(p, "w") as f:
        for name, addr in game_syms.items():
            if name in exclude:
                continue
            if re.match(r"^[A-Za-z_.$][\w.$]*$", name):
                f.write(f"{name} = 0x{addr:08x};\n")
    return p


def link_at(obj, addr, sym_ld, tag):
    """Link `obj` so its code sits at `addr`; return the raw section bytes."""
    mod_ld = os.path.join(WORK, f"{tag}.ld")
    with open(mod_ld, "w") as f:
        f.write("SECTIONS {\n"
                f"  .mod 0x{addr:08x} : {{\n"
                "    *(.text) *(.text.*) *(.rodata*) *(.data*) *(.sdata*)\n  }\n"
                "  /DISCARD/ : { *(.reginfo) *(.pdr) *(.mdebug*) *(.comment)\n"
                "               *(.bss) *(.sbss) *(.gnu.attributes) }\n}\n")
    elf = os.path.join(WORK, f"{tag}.elf")
    run([LD, "-EL", "-o", elf, "-T", mod_ld, "-T", sym_ld,
         "--no-check-sections", "-nostdlib", obj])
    binf = os.path.join(WORK, f"{tag}.bin")
    run([OBJCOPY, "-O", "binary", "--only-section=.mod", elf, binf])
    return elf, open(binf, "rb").read()


def main():
    if not (os.path.exists(MAIN) and os.path.exists(MAIN_ELF)):
        sys.exit(f"mkmod: {MAIN}(.elf) missing — run ./Build first")
    cfiles = sorted(glob.glob(f"{MOD_SRC_DIR}/*.c"))
    if not cfiles:
        sys.exit(f"mkmod: no mods in {MOD_SRC_DIR}/ — nothing to do")

    os.makedirs(WORK, exist_ok=True)
    hooks = [os.path.splitext(os.path.basename(c))[0] for c in cfiles]
    game_syms = nm_symbols(MAIN_ELF)
    missing = [h for h in hooks if h not in game_syms]
    if missing:
        sys.exit(f"mkmod: hooked function(s) not found in main.exe.elf: {missing}")
    print(f"mkmod: hooking {', '.join(hooks)}")

    base_size = os.path.getsize(MAIN)
    addrs = sorted(set(game_syms.values()))
    data = bytearray(open(MAIN, "rb").read())

    for c, h in zip(cfiles, hooks):
        obj = os.path.join(WORK, os.path.basename(c) + ".o")
        compile_c(c, obj)
        slot = game_syms[h]
        higher = [a for a in addrs if a > slot]
        if not higher:
            sys.exit(f"mkmod: no symbol after {h} @ 0x{slot:08x}; can't size its slot")
        slot_size = min(higher) - slot

        sym_ld = write_sym_ld(game_syms, {h}, h)
        elf, code = link_at(obj, slot, sym_ld, h)
        if nm_symbols(elf).get(h) != slot:
            sys.exit(f"mkmod: {h} did not link at its slot 0x{slot:08x}")
        if len(code) > slot_size:
            sys.exit(
                f"mkmod: {h} compiled to {len(code)} bytes but its slot (0x{slot:08x}"
                f"..0x{slot + slot_size:08x}) is only {slot_size} — over by "
                f"{len(code) - slot_size}. Trim it (drop debug logging / simplify) so "
                f"it fits in place; the fixed layout can't absorb a bigger function.")

        off = FILE_TEXT_OFF + (slot - TEXT_START)
        data[off:off + len(code)] = code
        print(f"  {h}: {len(code)}/{slot_size} bytes -> patched in place @ 0x{slot:08x}")

    assert len(data) == base_size, "in-place patch must not change the file size"
    out = f"{BUILD}/main_mod.exe"
    open(out, "wb").write(data)
    print(f"mkmod: wrote {out}  ({len(data)} bytes, same size as main.exe — the disc "
          f"rebuild stays byte-faithful, so it runs anywhere)")


if __name__ == "__main__":
    main()
