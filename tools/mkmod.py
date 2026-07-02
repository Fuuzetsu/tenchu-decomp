#!/usr/bin/env python3
"""Build a *modded* (non-matching) main.exe by trampolining hooked functions.

The problem: this is a partial decomp with a fixed memory layout, so a function
that changes size shifts everything after it and breaks the binary (see
docs/modding-and-nonmatching.md). The fix here keeps every original address
fixed and redirects hooked functions to grown code placed in free RAM after the
image ("the mod region").

For each C file in src/mod/main.exe/<name>.c (filename == function name):
  * compile it with the normal pipeline (cpp | cc1 -G8 | maspsx | as),
  * link all of them at MODBASE, resolving the game's symbols from main.exe.elf,
  * overwrite the first 8 bytes of the original function's slot with
    `j <impl>; nop` (a trampoline),
  * append the mod region to the image and extend the PS-EXE header .text size.

The result: main.exe byte-identical except an 8-byte trampoline per hooked
function + the header size, plus the grown code appended. All callers (symbolic
or raw) hit the slot and jump to your code.

Run inside the nix devShell (needs the cross toolchain, tools/cc1-281, maspsx).
"""
import os, re, struct, subprocess, sys, glob

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# Must stay in sync with shake/src/Build.hs. MODBASE is the first free address
# after the loaded image (main_exe ends at 0x80098000 with zero bss); override
# with $MODBASE if the game's heap turns out to use it (see the docs).
TEXT_START = 0x80011000
FILE_TEXT_OFF = 0x800
MODBASE = int(os.environ.get("MODBASE", "0x80098000"), 0)
ALIGN = 0x800

CROSS = "mipsel-unknown-linux-gnu-"
CPP = CROSS + "cpp"
AS = CROSS + "as"
LD = CROSS + "ld"
NM = CROSS + "nm"
OBJCOPY = CROSS + "objcopy"
CC1 = "tools/cc1-281"

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

    # compile every mod
    objs = []
    for c in cfiles:
        o = os.path.join(WORK, os.path.basename(c) + ".o")
        compile_c(c, o)
        objs.append(o)

    # symbol script = all game symbols except the hooked ones (their defs come
    # from the mod objects). Keep _gp so gp-relative code resolves.
    sym_ld = os.path.join(WORK, "syms.ld")
    with open(sym_ld, "w") as f:
        for name, addr in game_syms.items():
            if name in hooks:
                continue
            if re.match(r"^[A-Za-z_.$][\w.$]*$", name):
                f.write(f"{name} = 0x{addr:08x};\n")

    # link the mod region at MODBASE
    mod_ld = os.path.join(WORK, "mod.ld")
    with open(mod_ld, "w") as f:
        f.write("SECTIONS {\n"
                f"  .modtext 0x{MODBASE:08x} : {{\n"
                "    *(.text) *(.text.*) *(.rodata*) *(.data*) *(.sdata*)\n  }\n"
                "  /DISCARD/ : { *(.reginfo) *(.pdr) *(.mdebug*) *(.comment)\n"
                "               *(.bss) *(.sbss) *(.gnu.attributes) }\n}\n")
    mod_elf = os.path.join(WORK, "mod.elf")
    run([LD, "-EL", "-o", mod_elf, "-T", mod_ld, "-T", sym_ld,
         "--no-check-sections", "-nostdlib", *objs])

    mod_bin = os.path.join(WORK, "mod.bin")
    run([OBJCOPY, "-O", "binary", "--only-section=.modtext", mod_elf, mod_bin])
    impl = nm_symbols(mod_elf)

    # patch: trampoline each hooked slot, append the mod region, fix header
    data = bytearray(open(MAIN, "rb").read())
    base_len = len(data)
    for h in hooks:
        slot = game_syms[h]
        off = FILE_TEXT_OFF + (slot - TEXT_START)
        j = (0x02 << 26) | ((impl[h] >> 2) & 0x03FFFFFF)
        struct.pack_into("<II", data, off, j, 0)  # j impl ; nop
        print(f"  {h}: slot 0x{slot:08x} -> j 0x{impl[h]:08x}")
    mod = bytearray(open(mod_bin, "rb").read())
    mod += b"\x00" * ((-len(mod)) % ALIGN)
    data += mod
    struct.pack_into("<I", data, 0x1C, (base_len - FILE_TEXT_OFF) + len(mod))

    out = f"{BUILD}/main_mod.exe"
    open(out, "wb").write(data)
    print(f"mkmod: wrote {out}  ({len(data)} bytes, mod region {len(mod)} @ "
          f"0x{MODBASE:08x}, header .text size 0x{(base_len-FILE_TEXT_OFF)+len(mod):x})")


if __name__ == "__main__":
    main()
